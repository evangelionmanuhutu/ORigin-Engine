// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "ScriptEngine.h"

#include "ScriptGlue.h"
#include "Origin\Scene\Component\Component.h"

#include "mono\jit\jit.h"
#include "mono\metadata\assembly.h"
#include "mono\metadata\object.h"
#include "mono\metadata\tabledefs.h"

namespace Origin
{
	static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap =
	{
		{"System.Boolean", ScriptFieldType::Bool},
		{"System.Single", ScriptFieldType::Float},

		{"System.Char", ScriptFieldType::Char},
		{"System.Byte", ScriptFieldType::Byte},
		{"System.Double", ScriptFieldType::Double},

		{"System.Int16", ScriptFieldType::Short},
		{"System.Int32", ScriptFieldType::Int},
		{"System.Int64", ScriptFieldType::Long},

		{"System.UInt16", ScriptFieldType::UShort},
		{"System.UInt32", ScriptFieldType::UInt},
		{"System.UInt64", ScriptFieldType::ULong},
		{"System.UInt", ScriptFieldType::UByte},

		{"ORiginEngine.Vector2", ScriptFieldType::Vector2},
		{"ORiginEngine.Vector3", ScriptFieldType::Vector3},
		{"ORiginEngine.Vector4", ScriptFieldType::Vector4},
		{"ORiginEngine.Entity",	ScriptFieldType::Entity},
	};

	namespace Utils
	{
		static char* ReadBytes(const std::filesystem::path& filepath, uint32_t* outSize)
		{
			std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

			// failed to open file
			if (!stream)
				return nullptr;

			std::streampos end = stream.tellg();
			stream.seekg(0, std::ios::beg);
			uint32_t size = end - stream.tellg();

			// file is empty
			if (size == 0)
				return nullptr;

			auto* buffer = new char[size];
			stream.read((char*)buffer, size);

			*outSize = size;
			return buffer;
		}

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& filepath)
		{
			uint32_t fileSize = 0;
			char* fileData = ReadBytes(filepath, &fileSize);

			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				const char* errorMessage = mono_image_strerror(status);
				return nullptr;
			}
			std::string assemblyPath = filepath.string();
			MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.c_str(), &status, 0);
			mono_image_close(image);

			// don't forget to free the file data
			delete[] fileData;

			return assembly;
		}

		void PrintAssemblyTypes(MonoAssembly* assembly)
		{
			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionTable);

			for (int32_t i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionTable, i, cols, MONO_TYPEDEF_SIZE);

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

				OGN_CORE_INFO("{}.{}", nameSpace, name);
			}
		}

		ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
		{
			std::string typeName = mono_type_get_name(monoType);

			auto it = s_ScriptFieldTypeMap.find(typeName);
			if (it == s_ScriptFieldTypeMap.end())
			{
				OGN_CORE_ERROR("Unkown Field Type : {}", typeName);
				return ScriptFieldType::None;
			}

			return it->second;
		}

		const char* ScriptFieldTypeToString(ScriptFieldType type)
		{
			switch (type)
			{
				case Origin::ScriptFieldType::None:		 return "None";
				case Origin::ScriptFieldType::Float:	 return "Float";
				case Origin::ScriptFieldType::Double:  return "Double";
				case Origin::ScriptFieldType::Bool:		 return "Boolean";
				case Origin::ScriptFieldType::Char:		 return "Char";
				case Origin::ScriptFieldType::Byte:		 return "Byte";
				case Origin::ScriptFieldType::Short:	 return "Short";
				case Origin::ScriptFieldType::Int:		 return "Int";
				case Origin::ScriptFieldType::Long:		 return "Long";
				case Origin::ScriptFieldType::UByte:	 return "UByte";
				case Origin::ScriptFieldType::UShort:  return "UShort";
				case Origin::ScriptFieldType::UInt:		 return "UInt";
				case Origin::ScriptFieldType::ULong:	 return "ULong";
				case Origin::ScriptFieldType::Vector2: return "Vector2";
				case Origin::ScriptFieldType::Vector3: return "Vector3";
				case Origin::ScriptFieldType::Vector4: return "Vector4";
				case Origin::ScriptFieldType::Entity:	 return "Entity";
			}

			OGN_CORE_ERROR("ScriptEngine::ScriptFieldTypeToString: Unkown Field Type");
			return "<Invalid>";
		}
	}

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		ScriptClass EntityClass;

		std::vector<std::string> EntityScriptStorage;

		// Runtime
		Scene* SceneContext = nullptr;
		std::unordered_map<std::string, std::shared_ptr<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, std::shared_ptr<ScriptInstance>> EntityInstances;

		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;
	};

	ScriptEngineData* s_Data = nullptr;

	// Scrip Engine
	void ScriptEngine::InitMono()
	{
		mono_set_assemblies_path("mono/lib");

		MonoDomain* rootDomain = mono_jit_init("ORiginJITRuntime");
		OGN_CORE_ASSERT(rootDomain, "Mono Domain is NULL!");

		s_Data->RootDomain = rootDomain;
	}

	void ScriptEngine::ShutdownMono()
	{
		// cleanup
		mono_assembly_close(s_Data->CoreAssembly);
		s_Data->AppDomain = nullptr;
		s_Data->RootDomain = nullptr;
	}

	void ScriptEngine::Init()
	{
		// ========================
		// Initializing Script Engine
		// ========================

		s_Data = new ScriptEngineData();

		InitMono();
		ScriptGlue::RegisterFunctions();

		bool status = LoadAssembly("resources/scripts/ORigin-ScriptCore.dll");
		if (!status)
		{
			OGN_CORE_ERROR("[ScriptEngine] Could not load ORigin-ScriptCore assembly.");
			return;
		}

		status = LoadAppAssembly("SandboxProject/Binaries/Sandbox.dll");
		if (!status)
		{
			OGN_CORE_ERROR("[ScriptEngine] Could not load Sandbox assembly.");
			return;
		}

		LoadAssemblyClasses();

		// storing classes name into storage
		for (auto it : s_Data->EntityClasses)
			s_Data->EntityScriptStorage.emplace_back(it.first);

		ScriptGlue::RegisterComponents();
		s_Data->EntityClass = ScriptClass("ORiginEngine", "Entity", true);
	}

	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
		s_Data->EntityClasses.clear();
		s_Data->EntityInstances.clear();

		// Cleanup memory before shutdown
		delete s_Data;
		OGN_CORE_WARN("Script Engine Shutdown");
	}

	bool ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		s_Data->AppDomain = mono_domain_create_appdomain("ORiginScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);

		s_Data->CoreAssembly = Utils::LoadMonoAssembly(filepath);
		if (s_Data->CoreAssembly == nullptr)
			return false;

		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);
		return true;
	}

	bool ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
	{
		s_Data->AppAssembly = Utils::LoadMonoAssembly(filepath);
		if (s_Data->AppAssembly == nullptr)
			return false;

		s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);
		

		return true;
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	void ScriptEngine::OnRuntimeStop()
	{
		s_Data->SceneContext = nullptr;
		s_Data->EntityInstances.clear();
	}

	bool ScriptEngine::EntityClassExists(const std::string& fullClassName)
	{
		return s_Data->EntityClasses.find(fullClassName) != s_Data->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		auto& sc = entity.GetComponent<ScriptComponent>();
		if (EntityClassExists(sc.ClassName))
		{
			UUID entityID = entity.GetUUID();

			std::shared_ptr<ScriptInstance> instance = std::make_shared<ScriptInstance>(s_Data->EntityClasses[sc.ClassName], entity);
			s_Data->EntityInstances[entityID] = instance;

			// Copy Fields Value from Editor to Runtime
			if (s_Data->EntityScriptFields.find(entityID) != s_Data->EntityScriptFields.end())
			{
				const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entityID);
				for (const auto& [name, fieldInstance] : fieldMap)
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
			}

			// C# On Create Function
			instance->InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, float time)
	{
		UUID& entityID = entity.GetUUID();

		auto& it = s_Data->EntityInstances.find(entityID);
		if (it == s_Data->EntityInstances.end())
		{
			OGN_CORE_WARN("Entity Script Instance not found!");
			return;
		}

		std::shared_ptr<ScriptInstance> instance = s_Data->EntityInstances.at(entityID);
		instance->InvokeOnUpdate(time);
	}

	std::shared_ptr<ScriptClass> ScriptEngine::GetEntityClass(const std::string& name)
	{
		if (s_Data->EntityClasses.find(name) == s_Data->EntityClasses.end())
			return nullptr;

		return s_Data->EntityClasses.at(name);
	}

	std::unordered_map<std::string, std::shared_ptr<ScriptClass>> ScriptEngine::GetEntityClasses()
	{
		return s_Data->EntityClasses;
	}

	ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		OGN_CORE_ASSERT(entity, "ScriptEngine::GetScriptFieldMap: Failed to get entity");

		UUID entityID = entity.GetUUID();
		return s_Data->EntityScriptFields[entityID];
	}

	std::vector<std::string> ScriptEngine::GetScriptClassStorage()
	{
		return s_Data->EntityScriptStorage;
	}

	std::shared_ptr<ScriptInstance> ScriptEngine::GetEntityScriptInstance(UUID uuid)
	{
		auto& it = s_Data->EntityInstances.find(uuid);
		if (it == s_Data->EntityInstances.end())
		{
			OGN_CORE_ERROR("Script Instnce: Failed to find {} ", uuid);
			return nullptr;
		}

		return it->second;
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data->CoreAssemblyImage;
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	void ScriptEngine::LoadAssemblyClasses()
	{
		s_Data->EntityClasses.clear();

		const MonoTableInfo* typeDefinitionTable = mono_image_get_table_info(s_Data->AppAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionTable);

		MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "ORiginEngine", "Entity");

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* className = mono_metadata_string_heap(s_Data->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);

			std::string fullName;
			if (strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, className);
			else
				fullName = className;

			MonoClass* monoClass = mono_class_from_name(s_Data->AppAssemblyImage, nameSpace, className);

			if(monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			std::shared_ptr<ScriptClass> scriptClass = std::make_shared<ScriptClass>(nameSpace, className);

			s_Data->EntityClasses[fullName] = scriptClass;

			int fieldCount = mono_class_num_fields(monoClass);

			OGN_CORE_WARN("{} has {} fields: ", className, fieldCount);
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);
				uint32_t flags = mono_field_get_flags(field);
				if (flags & FIELD_ATTRIBUTE_PUBLIC)
				{
					MonoType* type = mono_field_get_type(field);
					ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
					OGN_CORE_WARN("{} ({}) is public", fieldName, Utils::ScriptFieldTypeToString(fieldType));

					scriptClass->m_Fields[fieldName] = {fieldType, fieldName, field};
				}
			}
		}

		auto& entityClasses = s_Data->EntityClasses;
	}

	// Script Class
	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool core)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(core ? s_Data->CoreAssemblyImage : s_Data->AppAssemblyImage, classNamespace.c_str(), className.c_str());
	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
	{
		MonoObject* exception = nullptr;
		return mono_runtime_invoke(method, instance, params, &exception);
	}

	ScriptInstance::ScriptInstance(std::shared_ptr<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_OnConstructor = s_Data->EntityClass.GetMethod(".ctor", 1);
		m_OnCreateMethod = scriptClass->GetMethod("OnCreate");
		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);

		// Entity Constructor
		{
			UUID entityID = entity.GetUUID();
			void* param = &entityID;
			m_ScriptClass->InvokeMethod(m_Instance, m_OnConstructor, &param);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if(m_OnCreateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);
	}

	void ScriptInstance::InvokeOnUpdate(float time)
	{
		if (m_OnUpdateMethod)
		{
			void* param = &time;
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
		}
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_get_value(m_Instance, field.ClassField, buffer);

		return true;
	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end())
			return false;

		const ScriptField& field = it->second;
		mono_field_set_value(m_Instance, field.ClassField, (void*)value);

		return true;
	}
}