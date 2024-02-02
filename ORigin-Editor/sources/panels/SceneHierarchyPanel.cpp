// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "SceneHierarchyPanel.h"
#include "../EditorLayer.h"
#include "Origin\Project\Project.h"
#include "Origin\Asset\AssetManager.h"
#include "Origin\Asset\AssetMetaData.h"
#include "Origin\Asset\AssetImporter.h"
#include "Origin\Renderer\Texture.h"
#include "Origin\Renderer\Shader.h"
#include "Origin\Scene\Components.h"
#include "Origin\Audio\AudioSource.h"
#include "Origin\Scripting\ScriptEngine.h"
#include "Origin\Renderer\Renderer.h"
#include "Origin\Scene\Lighting.h"

#include "box2d\b2_revolute_joint.h"
#include "box2d\b2_fixture.h"

#include <glm\gtc\type_ptr.hpp>
#include <misc\cpp\imgui_stdlib.h>

#pragma warning(disable : OGN_DISABLED_WARNINGS)

namespace origin {

	SceneHierarchyPanel* SceneHierarchyPanel::s_Instance = nullptr;

	SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene>& context)
	{
		SetContext(context);
		s_Instance = this;
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	Entity SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		return m_SelectedEntity = entity;
	}

	Entity SceneHierarchyPanel::GetSelectedEntity() const
	{
		return m_SelectedEntity;
	}

	void SceneHierarchyPanel::SetContext(const std::shared_ptr<Scene>& context, bool reset)
	{
		// reset scene
		if (reset)
			m_SelectedEntity = {};

		// set new scene
		m_Context = context;

		// receive selected entity from new scene
		if (!m_SelectedEntity)
			return;

		auto entityID = m_SelectedEntity.GetUUID();
		auto& newEntity = m_Context->GetEntityWithUUID(entityID);

		if (entityID == newEntity.GetUUID())
			m_SelectedEntity = newEntity;
	}

	void SceneHierarchyPanel::DestroyEntity(Entity entity)
	{
		DeleteEntityTree(entity);
		m_Context->DestroyEntity(entity);
		m_SelectedEntity = {};
	}

	void SceneHierarchyPanel::DeleteEntityTree(Entity entity)
	{
		auto& treeComponent = entity.GetComponent<TreeNodeComponent>();
		if (entity.HasParent())
		{
			Entity parent = m_Context->GetEntityWithUUID(entity.GetParentUUID());
			auto& parentTreeComponent = parent.GetComponent<TreeNodeComponent>();
			auto it = parentTreeComponent.Children.find(entity.GetUUID());
			if (it != parentTreeComponent.Children.end())
				parentTreeComponent.Children.erase(entity.GetUUID());
		}
		
		// if this entity has child
		// then erase entity from their parents
		for (auto [childId, child] : treeComponent.Children)
		{
			auto& childTreeComponent = child.GetComponent<TreeNodeComponent>();
			// 1. set the original parent to 0
			// 2. recursively through grand child

			childTreeComponent.Parent = 0;
			for(auto [gchildId, gChild] : childTreeComponent.Children)
				RemoveConnectionsFromChild(gChild, child, entity.GetUUID());
		}
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		EntityHierarchyPanel();
		EntityPropertiesPanel();
	}

	void SceneHierarchyPanel::EntityHierarchyPanel()
	{
		ImGui::Begin("Hierarchy");
		m_HierarchyMenuActive = ImGui::IsWindowFocused();

		if (!m_Context)
		{
			ImGui::End();
			return;
		}

		m_Context->m_Registry.each([&](auto entityID)
			{
				Entity entity{ entityID, m_Context.get() };
				DrawEntityNode(entity);
			});

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

		if (m_HierarchyMenuActive)
		{
			if (ImGui::BeginPopupContextWindow(nullptr, 1, false))
			{
				if (ImGui::BeginMenu("CREATE"))
				{
					if (ImGui::MenuItem("Empty"))
						m_Context->CreateEntity("Empty");

					if (ImGui::MenuItem("MAIN CAMERA"))
					{
						m_Context->CreateCamera("Main Camera");
						m_SelectedEntity.AddComponent<AudioListenerComponent>();
					}

					if (ImGui::MenuItem("CAMERA"))
						m_Context->CreateCamera("Camera");

					if (ImGui::BeginMenu("2D"))
					{
						if (ImGui::MenuItem("Sprite"))
							m_Context->CreateEntity("Sprite");
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Mesh"))
					{
						if (ImGui::MenuItem("Empty Mesh"))
							m_Context->CreateMesh("Empty Mesh");

						ImGui::EndMenu();
					}

					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void SceneHierarchyPanel::EntityPropertiesPanel()
	{
		ImGui::Begin("PROPERTIES");
		if (m_SelectedEntity)
			DrawComponents(m_SelectedEntity);
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity, int index)
	{
		Entity e = m_Context->GetEntityWithUUID(entity.GetUUID());
		if (e.HasParent() && index == 0)
			return;

		ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0)
			| ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

		bool node_open = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entity.GetTag().c_str());

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				m_SelectedEntity = {};
				DestroyEntity(entity);
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("ENTITY_SOURCE_ITEM", &entity, sizeof(Entity));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
			{
				OGN_CORE_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ENTITY ITEM");
				Entity src{ *static_cast<entt::entity*>(payload->Data), m_Context.get() };
				AddNodeChild(entity, src);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemHovered())
		{
			if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				m_SelectedEntity = entity;
		}

		if (node_open)
		{
			auto treeComponent = entity.GetComponent<TreeNodeComponent>();
			for (auto [uuid, child] : treeComponent.Children)
			{
				Entity entt = m_Context->GetEntityWithUUID(uuid);
				DrawEntityNode(entt, index + 1);
			}

			ImGui::TreePop();
		}

		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 1.5f));
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
			{
				OGN_CORE_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ENTITY ITEM");
				Entity src{ *static_cast<entt::entity*>(payload->Data), m_Context.get() };
				auto& srcTreeComponent = src.GetComponent<TreeNodeComponent>();

				if (srcTreeComponent.Parent != 0)
				{
					Entity parent = m_Context->GetEntityWithUUID(srcTreeComponent.Parent);
					parent.GetComponent<TreeNodeComponent>().Children.erase(src.GetUUID());
				}

				srcTreeComponent.Parent = 0;
				srcTreeComponent.Parents.clear();
			}
			ImGui::EndDragDropTarget();
		}
	}

	bool SceneHierarchyPanel::AddNodeChild(Entity parent, Entity child)
	{
		auto& parentTreeComponent = parent.GetComponent<TreeNodeComponent>();
		auto& childTreeComponent = child.GetComponent<TreeNodeComponent>();
		auto& enttReg = m_Context->m_EntityMap;

		if (parentTreeComponent.Parents.find(child.GetUUID()) != parentTreeComponent.Parents.end())
		{
			OGN_CORE_ERROR("Cannot add {0} to child, it was the parent", child.GetTag());
			return false;
		}

		for (auto [childId, child] : parentTreeComponent.Children)
		{
			auto& treeComponent = child.GetComponent<TreeNodeComponent>();
			for (auto& [parentId, parent] : treeComponent.Parents)
			{
				auto& childIdIt = treeComponent.Parents.find(parentId);
				if (childIdIt != treeComponent.Parents.end())
				{
					RemoveConnectionsFromChild(child, child, child.GetUUID());
				}
			}
		}

		parentTreeComponent.Children.insert(std::make_pair(child.GetUUID(), child));

		UUID oldParent = childTreeComponent.Parent;
		childTreeComponent.Parent = parent.GetUUID();

		if (childTreeComponent.Parent != oldParent)
		{
			if (enttReg.find(oldParent) != enttReg.end())
			{
				Entity e = m_Context->GetEntityWithUUID(oldParent);
				e.GetComponent<TreeNodeComponent>().Children.erase(child.GetUUID());
			}
		}

		childTreeComponent.Parents.insert(std::make_pair(parent.GetUUID(), parent));

		for (auto& p : parentTreeComponent.Parents)
		{
			childTreeComponent.Parents[p.first] = p.second;
		}

		return true;
	}

	void SceneHierarchyPanel::RemoveConnectionsFromChild(Entity child, Entity nextChild, UUID parentId)
	{
		auto& childIdc = child.GetComponent<TreeNodeComponent>();
		auto& nextChildIdc = nextChild.GetComponent<TreeNodeComponent>();

		childIdc.Parents.erase(parentId);

		for (auto& c : nextChildIdc.Children)
		{
			RemoveConnectionsFromChild(child, c.second, c.second.GetUUID());
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			strcpy_s(buffer, sizeof(buffer), tag.c_str());
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
				if (tag.empty()) tag = "'No Name'";
			}
		}

		ImGui::SameLine();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			std::string search = "Search Component";
			char searchBuffer[256];
			strcpy_s(searchBuffer, sizeof(searchBuffer), search.c_str());
			if (ImGui::InputText("##SearchComponent", searchBuffer, sizeof(searchBuffer)))
				search = std::string(searchBuffer);

			DisplayAddComponentEntry<ScriptComponent>("SCRIPT");
			DisplayAddComponentEntry<CameraComponent>("CAMERA");
			DisplayAddComponentEntry<AudioComponent>("AUDIO");
			DisplayAddComponentEntry<AudioListenerComponent>("AUDIO LISTENER");
			DisplayAddComponentEntry<AnimationComponent>("ANIMATION");
			DisplayAddComponentEntry<SpriteRendererComponent>("SPRITE RENDERER");
			DisplayAddComponentEntry<StaticMeshComponent>("STATIC MESH COMPONENT");
			DisplayAddComponentEntry<CircleRendererComponent>("CIRCLE RENDERER 2D");
			DisplayAddComponentEntry<SpriteRenderer2DComponent>("SPRITE RENDERER 2D");
			DisplayAddComponentEntry<ParticleComponent>("PARTICLE");
			DisplayAddComponentEntry<TextComponent>("TEXT COMPONENT");
			DisplayAddComponentEntry<LightComponent>("LIGHTING");
			DisplayAddComponentEntry<RigidbodyComponent>("RIGIDBODY");
			DisplayAddComponentEntry<BoxColliderComponent>("BOX COLLIDER");
			DisplayAddComponentEntry<SphereColliderComponent>("SPHERE COLLIDER");
			DisplayAddComponentEntry<CapsuleColliderComponent>("CAPSULE COLLIDER");

			if(!m_SelectedEntity.HasComponent<Rigidbody2DComponent>())
				DisplayAddComponentEntry<Rigidbody2DComponent>("2D RIGIDBODY");

			DisplayAddComponentEntry<BoxCollider2DComponent>("2D BOX COLLIDER");
			DisplayAddComponentEntry<CircleCollider2DComponent>("2D CIRCLE COLLIDER");
			DisplayAddComponentEntry<RevoluteJoint2DComponent>("2D REVOLUTE JOINT");

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar();
		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("TRANSFORM", entity, [&](auto& component)
		{
				DrawVec3Control("Translation", component.Translation);
				glm::vec3 rotation = glm::degrees(component.Rotation);
				DrawVec3Control("Rotation", rotation, 1.0f);
				component.Rotation = glm::radians(rotation);
				DrawVec3Control("Scale", component.Scale, 0.01f, 1.0f);
		});

		DrawComponent<StaticMeshComponent>("STATIC MESH", entity, [](auto& component)
			{
				std::string label = "None";
				bool isMeshValid = false;
				ImGui::Text("Model");
				ImGui::SameLine();

				if (component.Model != 0)
				{
					if (AssetManager::IsAssetHandleValid(component.Model) && AssetManager::GetAssetType(component.Model) == AssetType::StaticMesh)
					{
						const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.Model);
						label = metadata.Filepath.filename().string();
						isMeshValid = true;
					}
					else
					{
						label = "Invalid";
					}
				}

				ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
				buttonLabelSize.x += 20.0f;
				const auto buttonLabelWidth = glm::max<float>(100.0f, buttonLabelSize.x);
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						AssetHandle handle = *static_cast<AssetHandle*>(payload->Data);
						if (AssetManager::GetAssetType(handle) == AssetType::StaticMesh)
						{
							component.Model = handle;
						}
						else
						{
							OGN_CORE_WARN("Wrong asset type!");
						}
					}
					ImGui::EndDragDropTarget();
				}


				if (isMeshValid)
				{
					ImGui::SameLine();
					const ImVec2 xLabelSize = ImGui::CalcTextSize("X");
					const float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
					if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
					{
						component.Model = 0;
						isMeshValid = false;
					}
				}

				if (isMeshValid)
				{
					std::shared_ptr<Model> model = AssetManager::GetAsset<Model>(component.Model);

					ImGui::Separator();
					ImGui::Text("Material");

					DrawVec2Control("Tiling Factor", model->GetMaterial()->BufferData.TilingFactor, 0.01f, 1.0f);
					ImGui::ColorEdit4("Color", glm::value_ptr(model->GetMaterial()->BufferData.Color));
					DrawVecControl("Metallic", &model->GetMaterial()->BufferData.Metallic, 0.01f, 0.0f, 1.0f);
					DrawVecControl("Roughness", &model->GetMaterial()->BufferData.Roughness, 0.01f, 0.0f, 1.0f);

					if (ImGui::Button("Refresh Shader"))
						model->GetMaterial()->RefreshShader();
				}
			});

		DrawComponent<CameraComponent>("CAMERA", entity, [](auto& component)
		{
			auto& camera = component.Camera;
			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionType[2] = { "Perspective", "Orthographic" };
			const char* currentProjectionType = projectionType[static_cast<int>(camera.GetProjectionType())];

			bool isSelected = false;
		if (ImGui::BeginCombo("Projection", currentProjectionType))
		{
			for (int i = 0; i < 2; i++)
			{
				isSelected = currentProjectionType == projectionType[i];
				if (ImGui::Selectable(projectionType[i], isSelected))
				{
					currentProjectionType = projectionType[i];
					component.Camera.SetProjectionType(static_cast<ProjectionType>(i));
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		const char* aspectRatioType[5] = { "Free", "16/10", "16/9", "21/9", "4/3" };
		const char* currentAspectRatioType = aspectRatioType[static_cast<int>(camera.GetAspectRatioType())];
		
		if (ImGui::BeginCombo("Aspect Ratio", currentAspectRatioType))
		{
			for (int i = 0; i < 5; i++)
			{
				isSelected = currentAspectRatioType == aspectRatioType[i];
				if (ImGui::Selectable(aspectRatioType[i], isSelected))
				{
					currentAspectRatioType = aspectRatioType[i];
					camera.SetAspectRatioType(static_cast<SceneCamera::AspectRatioType>(i));
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (component.Camera.GetProjectionType() == ProjectionType::Perspective)
		{
			float perspectiveFov = glm::degrees(camera.GetPerspectiveFov());
			camera.SetPerspectiveFov(glm::radians(perspectiveFov));

			if (ImGui::DragFloat("FOV", &perspectiveFov, 0.1f, 0.01f, 10000.0f))
				camera.SetPerspectiveFov(glm::radians(perspectiveFov));

			float perspectiveNearClip = camera.GetPerspectiveNearClip();
			if (ImGui::DragFloat("Near Clip", &perspectiveNearClip, 0.1f))
				camera.SetPerspectiveNearClip(perspectiveNearClip);

			float perspectiveFarClip = camera.GetPerspectiveFarClip();
			if (ImGui::DragFloat("Far Clip", &perspectiveFarClip, 0.1f))
				camera.SetPerspectiveFarClip(perspectiveFarClip);
		}

		if (component.Camera.GetProjectionType() == ProjectionType::Orthographic)
		{
			float orthoSize = camera.GetOrthographicSize();
			camera.SetOrthographicSize(orthoSize);

			if (ImGui::DragFloat("Ortho Size", &orthoSize, 0.1f, 1.0f, 100.0f))
				camera.SetOrthographicSize(orthoSize);

			float orthoNearClip = camera.GetOrthographicNearClip();
			if (ImGui::DragFloat("Near Clip", &orthoNearClip, 0.1f, -1.0f, 10.0f))
				camera.SetOrthographicNearClip(orthoNearClip);

			float orthoFarClip = camera.GetOrthographicFarClip();
			if (ImGui::DragFloat("Far Clip", &orthoFarClip, 0.1f, 10.0f, 100.0f))
				camera.SetOrthographicFarClip(orthoFarClip);
		}});

		DrawComponent<AnimationComponent>("ANIMATION", entity, [](auto& component)
		{
				for (auto& anim : component.Animations)
				{
					ImGui::Text(anim->GetName().c_str());
				}
		});

		DrawComponent<RigidbodyComponent>("RIGIDBODY", entity, [](auto& component)
		{
				ImGui::Text("Use Gravity"); ImGui::SameLine();
				ImGui::Checkbox("##UseGravity", &component.UseGravity);

				ImGui::Text("Rotate   "); ImGui::SameLine();
				ImGui::Checkbox("X", &component.RotateX); ImGui::SameLine();
				ImGui::Checkbox("Y", &component.RotateY); ImGui::SameLine();
				ImGui::Checkbox("Z", &component.RotateZ);

				ImGui::Text("Kinematic"); ImGui::SameLine();
				ImGui::Checkbox("##Kinematic", &component.Kinematic);

				DrawVecControl("Mass", &component.Mass, 0.05f, 0.0f, 1000.0f, 0.0f);
				DrawVec3Control("Center Mass", component.CenterMassPosition);

		});

		DrawComponent<BoxColliderComponent>("BOX COLLIDER", entity, [](auto& component)
		{
				DrawVec3Control("Offset", component.Offset, 0.025f, 0.0f);
				DrawVec3Control("Size", component.Size, 0.025f, 0.5f);
				DrawVecControl("StaticFriction", &component.StaticFriction, 0.025f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("DynamicFriction", &component.DynamicFriction, 0.025f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("Restitution", &component.Restitution, 0.025f, 0.0f, 1000.0f, 0.0f);
		});

		DrawComponent<SphereColliderComponent>("SPHERE COLLIDER", entity, [](auto& component)
			{
				DrawVec3Control("Offset", component.Offset, 0.025f, 0.0f);
				DrawVecControl("Radius", &component.Radius, 0.025f, 0.0f, 10.0f, 1.0f);
				DrawVecControl("StaticFriction", &component.StaticFriction, 0.025f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("DynamicFriction", &component.DynamicFriction, 0.025f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("Restitution", &component.Restitution, 0.025f, 0.0f, 1000.0f, 0.0f);
			});

		DrawComponent<CapsuleColliderComponent>("CAPSULE COLLIDER", entity, [](auto& component)
			{
				ImGui::Checkbox("Horizontal", &component.Horizontal);
				DrawVec3Control("Offset", component.Offset, 0.01f, 0.0f);
				DrawVecControl("Radius", &component.Radius, 0.01f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("Height", &component.Height, 0.01f, 0.0f, 1000.0f, 1.0f);
				DrawVecControl("StaticFriction", &component.StaticFriction, 0.025f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("DynamicFriction", &component.DynamicFriction, 0.025f, 0.0f, 1000.0f, 0.5f);
				DrawVecControl("Restitution", &component.Restitution, 0.025f, 0.0f, 1000.0f, 0.0f);
			});

		DrawComponent<AudioComponent>("AUDIO SOURCE", entity, [entity, scene = m_Context](auto& component)
			{
				std::string label = "None";

				bool isAudioValid = false;
				ImGui::Text("Audio Source");
				ImGui::SameLine();
				if (component.Audio != 0)
				{
					if (AssetManager::IsAssetHandleValid(component.Audio) && AssetManager::GetAssetType(component.Audio) == AssetType::Audio)
					{
						const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.Audio);
						label = metadata.Filepath.filename().string();
						isAudioValid = true;
					}
					else
					{
						label = "Invalid";
					}
				}

				ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
				buttonLabelSize.x += 20.0f;
				const auto buttonLabelWidth = glm::max<float>(100.0f, buttonLabelSize.x);

				ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						AssetHandle handle = *static_cast<AssetHandle*>(payload->Data);
						if (AssetManager::GetAssetType(handle) == AssetType::Audio)
						{
							component.Audio = handle;
							const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.Audio);
							component.Name = metadata.Filepath.filename().string();
						}
						else
						{
							OGN_CORE_WARN("Wrong asset type!");
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (isAudioValid == false)
					return;

				std::shared_ptr<AudioSource> audio = AssetManager::GetAsset<AudioSource>(component.Audio);

				if (audio->IsLoaded)
				{
					auto &name = component.Name;
					char buffer[256];
					ImGui::Text("Name");
					ImGui::SameLine();
					strcpy_s(buffer, sizeof(buffer), name.c_str());
					if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
					{
						name = std::string(buffer);
						audio->SetName(name.c_str());
					}

					if (ImGui::Button("Play")) audio->Play();
					ImGui::SameLine();
					if (ImGui::Button("Stop")) audio->Stop();
					ImGui::SameLine();

					ImGui::Checkbox("Play At Start", &component.PlayAtStart);
					ImGui::SameLine();
					ImGui::Checkbox("Looping", &component.Looping);
					DrawVecControl("Volume", &component.Volume, 0.025f, 0.0f, 1.0f, 1.0f);
					DrawVecControl("Pitch", &component.Pitch, 0.025f, 0.0f, 1.0f, 1.0f);
					DrawVecControl("Panning", &component.Panning, 0.025f, -1.0f, 1.0f, 0.0f);

					ImGui::Separator();
					ImGui::Checkbox("Spatialize", &component.Spatializing);

					if (component.Spatializing)
					{
						DrawVecControl("Min Distance", &component.MinDistance, 0.1f, 0.0f, 10000.0f, 0.0f);
						DrawVecControl("Max Distance", &component.MaxDistance, 0.1f, 0.0f, 10000.0f, 0.0f);
					}
				}
			});

		DrawComponent<TextComponent>("TEXT", entity, [](auto& component) 
			{
			if(!component.FontAsset)
				component.FontAsset = Font::GetDefault();
					
				ImGui::Button("DROP FONT", ImVec2(80.0f, 30.0f));

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
						std::filesystem::path fontPath = Project::GetActiveAssetFileSystemPath(path);
						if (fontPath.extension() == ".ttf" || fontPath.extension() == ".otf")
						{
							if(component.FontAsset) component.FontAsset.reset();
							component.FontAsset = std::make_shared<Font>(fontPath);
						}
					}
				}
				
				ImGui::InputTextMultiline("Text String", &component.TextString);
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
				
				DrawVecControl("Kerning", &component.Kerning, 0.01f);
				DrawVecControl("Line Spacing", &component.LineSpacing, 0.01f);
			});

		DrawComponent<ParticleComponent>("PARTICLE", entity, [](auto& component)
			{
				float columnWidth = 100.0f;

				ImGui::ColorEdit4("Color Begin", glm::value_ptr(component.ColorBegin));
				ImGui::ColorEdit4("Color End", glm::value_ptr(component.ColorEnd));
				DrawVec3Control("Velocity", component.Velocity, 0.01f, 0.0f, columnWidth);
				DrawVec3Control("Velocity Variation", component.VelocityVariation, 0.01f, 0.0f, columnWidth);
				DrawVec3Control("Rotation", component.Rotation, 0.01f, 0.0f, columnWidth);

				DrawVecControl("Size Begin", &component.SizeBegin, 0.01f, 0.0f, 1000.0f, 0.5f, columnWidth);
				DrawVecControl("Size End", &component.SizeEnd, 0.01f, 0.0f, 1000.0f, 0.0f, columnWidth);
				DrawVecControl("Size Variation", &component.SizeVariation, 0.1f, 0.0f, 1000.0f, 0.3f, columnWidth);
				DrawVecControl("Z Axis", &component.ZAxis, 0.1f, -1000.0f, 1000.0f, 0.0f, columnWidth);
				DrawVecControl("Life Time", &component.LifeTime, 0.01f, 0.0f, 1000.0f, 1.0f, columnWidth);
			});

		DrawComponent<SpriteRendererComponent>("SPRITE RENDERER", entity, [](auto& component)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

				if (!component.Texture)
					ImGui::Button("DROP TEXTURE", ImVec2(80.0f, 30.0f));
				else ImGui::ImageButton(reinterpret_cast<ImTextureID>(component.Texture->GetRendererID()), ImVec2(80.0f, 80.0f), ImVec2(0, 1), ImVec2(1, 0));

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
						std::filesystem::path texturePath = Project::GetActiveAssetFileSystemPath(path);
						if (texturePath.extension() == ".png" || texturePath.extension() == ".jpg")
							component.Texture = Texture2D::Create(texturePath);
					}
				}

				if (component.Texture)
				{
					ImGui::SameLine();
					if (ImGui::Button("Delete", ImVec2(80.0f, 30.0f)))
					{
						component.Texture->Delete();
						component.Texture = {};
						return;
					}
					ImGui::Text("Path: %s", component.Texture->GetFilepath().c_str());
				}
			});

		DrawComponent<SpriteRenderer2DComponent>("SPRITE RENDERER 2D", entity, [](auto& component)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

				std::string label = "None";
				bool isTextureValid = false;
				ImGui::Text("Texture");
				ImGui::SameLine();
				if (component.Texture != 0)
				{
					if (AssetManager::IsAssetHandleValid(component.Texture) && AssetManager::GetAssetType(component.Texture) == AssetType::Texture2D)
					{
						const AssetMetadata& metadata = Project::GetActive()->GetEditorAssetManager()->GetMetadata(component.Texture);
						label = metadata.Filepath.filename().string();
						isTextureValid = true;
					}
					else
					{
						label = "Invalid";
					}
				}

				ImVec2 buttonLabelSize = ImGui::CalcTextSize(label.c_str());
				buttonLabelSize.x += 20.0f;
				const auto buttonLabelWidth = glm::max<float>(100.0f, buttonLabelSize.x);

				ImGui::Button(label.c_str(), ImVec2(buttonLabelWidth, 0.0f));
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						AssetHandle handle = *static_cast<AssetHandle*>(payload->Data);
						if (AssetManager::GetAssetType(handle) == AssetType::Texture2D)
						{
							component.Texture = handle;
						}
						else
						{
							OGN_CORE_WARN("Wrong asset type!");
						}

					}
					ImGui::EndDragDropTarget();
				}

				if (isTextureValid)
				{
					ImGui::SameLine();
					const ImVec2 xLabelSize = ImGui::CalcTextSize("X");
					const float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
					if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
						component.Texture = 0;
					DrawVec2Control("Tilling Factor", component.TillingFactor, 0.025f, 1.0f);
					ImGui::Text("Flip");
					ImGui::SameLine();
					ImGui::Checkbox("X", &component.FlipX);
					ImGui::SameLine();
					ImGui::Checkbox("Y", &component.FlipY);
				}
			});

		DrawComponent<LightComponent>("LIGHTING", entity, [](auto& component)
			{
				const char* lightTypeString[3] = { "Spot", "Point", "Directional" };
				const char* currentLightTypeString = lightTypeString[static_cast<int>(component.Light->Type)];

				if (ImGui::BeginCombo("Type", currentLightTypeString))
				{
					for (int i = 0; i < 3; i++)
					{
						bool isSelected = currentLightTypeString == lightTypeString[i];
						if (ImGui::Selectable(lightTypeString[i], isSelected))
						{
							currentLightTypeString = lightTypeString[i];
							component.Light->Type = static_cast<LightingType>(i);
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				switch (component.Light->Type)
				{
					case LightingType::Directional:
					{
						ImGui::ColorEdit3("Color", glm::value_ptr(component.Light->m_DirLightData.Color));
						DrawVecControl("Strength", &component.Light->m_DirLightData.Strength, 0.01f, 0.0f, 100.0f);
						DrawVecControl("Diffuse", &component.Light->m_DirLightData.Diffuse, 0.01f, 0.0f, 1.0f, 1.0f);
						DrawVecControl("Specular", &component.Light->m_DirLightData.Specular, 0.01f, 0.0f, 1.0f, 1.0f);
						DrawVecControl("Far", &component.Light->GetShadow()->Far, 1.0f, -1000.0f, 1000.0f, 50.0f);
						DrawVecControl("Near", &component.Light->GetShadow()->Near, 1.0f, -1000.0f, 1000.0f, -10.0f);
						DrawVecControl("Size", &component.Light->GetShadow()->Size, 1.0f, -1000.0f, 1000.0f, 50.0f);

						if (component.Light->GetShadow()->GetFramebuffer())
						{
							uint32_t texture = component.Light->GetShadow()->GetFramebuffer()->GetDepthAttachmentRendererID();
							float size = ImGui::GetContentRegionAvail().x;
							ImGui::Image(reinterpret_cast<ImTextureID>(texture), ImVec2(size, size), ImVec2(0, 1), ImVec2(1, 0));
						}
						break;	
					}
#if 0
					case LightingType::Spot:
					{
						ImGui::ColorEdit3("Color", glm::value_ptr(component.Light->Color));

						float angle = glm::degrees(component.Light->InnerConeAngle);
						ImGui::DragFloat("Inner Cone", &angle);
						component.Light->InnerConeAngle = glm::radians(angle);

						angle = glm::degrees(component.Light->OuterConeAngle);
						ImGui::DragFloat("Outer Cone", &angle);
						component.Light->OuterConeAngle = glm::radians(angle);

						ImGui::DragFloat("Exponent", &component.Light->Exponent, 0.01f, 0.0f, 1.0f);
						break;
					}

					case LightingType::Point:
					{
						ImGui::ColorEdit3("Color", glm::value_ptr(component.Light->Color));
						DrawVecControl("Ambient", &component.Light->Ambient, 0.01f, 0.0f);
						DrawVecControl("Specular", &component.Light->Specular, 0.01f, 0.0f);
						DrawVecControl("Intensity", &component.Light->Intensity, 0.01f, 0.0f);
						DrawVecControl("Size", &component.Light->SpreadSize, 0.1f, 0.1f, 10000.0f);
						break;
					}
#endif
				}
				
			});

		DrawComponent<CircleRendererComponent>("CIRCLE RENDERER", entity, [](auto& component)
			{
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
				ImGui::DragFloat("Thickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
				ImGui::DragFloat("Fade", &component.Fade, 0.025f, 0.0f, 1.0f);
			});

		DrawComponent<Rigidbody2DComponent>("RIGID BODY 2D", entity, [](auto& component)
		{
			ImGui::Checkbox("Enabled", &component.Enabled);
			const char* bodyTypeString[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentBodyTypeString = bodyTypeString[static_cast<int>(component.Type)];

			if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = currentBodyTypeString == bodyTypeString[i];
					if (ImGui::Selectable(bodyTypeString[i], isSelected))
					{
						currentBodyTypeString = bodyTypeString[i];
						component.Type = static_cast<Rigidbody2DComponent::BodyType>(i);
					}
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			DrawVecControl("Gravity Scale", &component.GravityScale, 0.01f);
			DrawVecControl("Rotational Inertia", &component.RotationalInertia, 0.01f);
			DrawVecControl("Linear Damping", &component.LinearDamping, 0.025f, 0.0f, 1000.0f);
			DrawVecControl("Angular Damping", &component.AngularDamping, 0.025f, 0.0f, 1000.0f);
			DrawVecControl("Mass", &component.Mass, 0.01f);
			DrawVec2Control("Mass Center", component.MassCenter, 0.01f);

			ImGui::Text("Freeze Position");
			ImGui::SameLine();
			ImGui::Checkbox("X", &component.FreezePositionX);
			ImGui::SameLine();
			ImGui::Checkbox("Y", &component.FreezePositionY);

			ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
			ImGui::Checkbox("Awake", &component.Awake);
			ImGui::SameLine();
			ImGui::Checkbox("Allow Sleeping", &component.AllowSleeping);
			ImGui::Checkbox("Bullet", &component.Bullet);

			});

		DrawComponent<BoxCollider2DComponent>("BOX COLLIDER 2D", entity, [](auto& component)
			{
				ImGui::DragInt("Group Index", &component.Group, 1.0f, -1, 16, "Group Index %d");

				DrawVec2Control("Offset", component.Offset, 0.01f, 0.0f);
				glm::vec2 size = component.Size * glm::vec2(2.0f);
				DrawVec2Control("Size", size, 0.01f, 0.5f);
				component.Size = size / glm::vec2(2.0f);

				float width = 118.0f;
				b2Fixture* fixture = static_cast<b2Fixture*>(component.RuntimeFixture);
				if (DrawVecControl("Density", &component.Density, 0.01f, 0.0f, 100.0f, 1.0f, width))
				{
					if(fixture) fixture->SetDensity(component.Density);
				}
				if(DrawVecControl("Friction", &component.Friction, 0.02f, 0.0f, 100.0f, 0.5f, width))
				{
					if(fixture) fixture->SetFriction(component.Friction);
				}
				if(DrawVecControl("Restitution", &component.Restitution, 0.01f, 0.0f, 100.0f, 0.5f, width))
				{
					if(fixture) fixture->SetRestitution(component.Restitution);
				}
				if(DrawVecControl("Threshold", &component.RestitutionThreshold, 0.01f, 0.0f, 100.0f, 0.0f, width))
				{
					if(fixture) fixture->SetRestitutionThreshold(component.RestitutionThreshold);
				}
			});

		DrawComponent<CircleCollider2DComponent>("CIRCLE COLLIDER 2D", entity, [](auto& component)
			{
				ImGui::DragInt("Group Index", &component.Group, 1.0f, -1, 16, "Group Index %d");

				DrawVec2Control("Offset", component.Offset, 0.01f, 0.0f);
				DrawVecControl("Radius", &component.Radius, 0.01f, 0.5f);

				float width = 118.0f;
				b2Fixture* fixture = static_cast<b2Fixture*>(component.RuntimeFixture);
				if (DrawVecControl("Density", &component.Density, 0.01f, 0.0f, 100.0f, 1.0f, width))
				{
					if (fixture) fixture->SetDensity(component.Density);
				}
				if (DrawVecControl("Friction", &component.Friction, 0.02f, 0.0f, 100.0f, 0.5f, width))
				{
					if (fixture) fixture->SetFriction(component.Friction);
				}
				if (DrawVecControl("Restitution", &component.Restitution, 0.01f, 0.0f, 100.0f, 0.5f, width))
				{
					if (fixture) fixture->SetRestitution(component.Restitution);
				}
				if (DrawVecControl("Threshold", &component.RestitutionThreshold, 0.01f, 0.0f, 100.0f, 0.0f, width))
				{
					if (fixture) fixture->SetRestitutionThreshold(component.RestitutionThreshold);
				}
			});

		DrawComponent<RevoluteJoint2DComponent>("REVOLUTE JOINT 2D", entity, [&](auto& component)
			{
				std::string label = "Connected Body";

				if (component.ConnectedBodyID != 0)
					label = "Connected";

				ImGui::Button(label.c_str());
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_SOURCE_ITEM"))
					{
						OGN_CORE_ASSERT(payload->DataSize == sizeof(Entity), "WRONG ENTITY ITEM");
						Entity src{ *static_cast<entt::entity*>(payload->Data), m_Context.get() };
						if (component.ConnectedBodyID == 0)
						{
							UUID uuid = src.GetUUID();
							component.ConnectedBodyID = uuid;
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (component.ConnectedBodyID != 0)
				{
					ImGui::SameLine();
					const ImVec2 xLabelSize = ImGui::CalcTextSize("X");
					const float buttonSize = xLabelSize.y + ImGui::GetStyle().FramePadding.y * 2.0f;
					if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
					{
						component.ConnectedBodyID = 0;
					}
				}

				b2RevoluteJoint* joint = static_cast<b2RevoluteJoint*>(component.Joint);
				if (ImGui::Checkbox("Limit", &component.EnableLimit))
				{
					if (joint)
						joint->EnableLimit(component.EnableLimit);
				}
				DrawVec2Control("Anchor", component.AnchorPoint);
				if (DrawVecControl("Lower Angle", &component.LowerAngle, 0.0f))
				{
					if (joint)
						joint->SetLimits(glm::radians(component.LowerAngle), glm::radians(component.UpperAngle));
				}
				if (DrawVecControl("Upper Angle", &component.UpperAngle, 0.0f))
				{
					if (joint)
						joint->SetLimits(glm::radians(component.LowerAngle), glm::radians(component.UpperAngle));
				}
				if (DrawVecControl("Max Torque", &component.MaxMotorTorque, 0.0f))
				{
					if (joint)
						joint->SetMaxMotorTorque(component.MaxMotorTorque);
				}
				if (ImGui::Checkbox("Motor", &component.EnableMotor))
				{
					if (joint)
						joint->EnableMotor(component.EnableMotor);
				}
				if (DrawVecControl("Motor Speed", &component.MotorSpeed, 0.0f))
				{
					if (joint)
						joint->SetMotorSpeed(component.MotorSpeed);
				}
			});

		DrawComponent<ScriptComponent>("SCRIPT", entity, [entity, scene = m_Context](auto& component) mutable
			{
				bool scriptClassExist = ScriptEngine::EntityClassExists(component.ClassName);
				bool isSelected = false;

				if (!scriptClassExist)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

				auto& scriptStorage = ScriptEngine::GetScriptClassStorage();
				std::string currentScriptClasses = component.ClassName;

				// drop-down
				if (ImGui::BeginCombo("Script Class", currentScriptClasses.c_str()))
				{
					for (int i = 0; i < scriptStorage.size(); i++)
					{
						isSelected = currentScriptClasses == scriptStorage[i];
						if (ImGui::Selectable(scriptStorage[i].c_str(), isSelected))
						{
							currentScriptClasses = scriptStorage[i];
							component.ClassName = scriptStorage[i];
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				if (ImGui::Button("Detach"))
				{
					component.ClassName = "Detached";
					isSelected = false;
				}

				bool detached = component.ClassName == "Detached";

				// fields
				bool isRunning = scene->IsRunning();
				if (isRunning)
				{
					std::shared_ptr<ScriptInstance> scriptInstance = ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
					if (scriptInstance && !detached)
					{
						auto& fields = scriptInstance->GetScriptClass()->GetFields();

						for (const auto& [name, field] : fields)
						{
							if (field.Type == ScriptFieldType::Float)
							{
								float data = scriptInstance->GetFieldValue<float>(name);
								if (ImGui::DragFloat(name.c_str(), &data, 0.25f))
								{
									scriptInstance->SetFieldValue<float>(name, data);
								}
							}
							if (field.Type == ScriptFieldType::Int)
							{
								int data = scriptInstance->GetFieldValue<int>(name);
								if (ImGui::DragInt(name.c_str(), &data, 1))
								{
									scriptInstance->SetFieldValue<int>(name, data);
								}
							}
							if (field.Type == ScriptFieldType::Vector2)
							{
								glm::vec2 data = scriptInstance->GetFieldValue<glm::vec2>(name);
								if (ImGui::DragFloat2(name.c_str(), glm::value_ptr(data), 0.25f))
								{
									scriptInstance->SetFieldValue<glm::vec2>(name, data);
								}
							}
							if (field.Type == ScriptFieldType::Vector3)
							{
								glm::vec3 data = scriptInstance->GetFieldValue<glm::vec3>(name);
								if (ImGui::DragFloat3(name.c_str(), glm::value_ptr(data), 0.25f))
								{
									scriptInstance->SetFieldValue<glm::vec3>(name, data);
								}
							}
							if (field.Type == ScriptFieldType::Vector4)
							{
								glm::vec4 data = scriptInstance->GetFieldValue<glm::vec4>(name);
								if (ImGui::DragFloat4(name.c_str(), glm::value_ptr(data), 0.25f))
								{
									scriptInstance->SetFieldValue<glm::vec4>(name, data);
								}
							}
						}
					}
				} // !IsRunning
				else
				{
					if (scriptClassExist && !detached)
					{
						std::shared_ptr<ScriptClass> entityClass = ScriptEngine::GetEntityClass(component.ClassName);
						const auto& fields = entityClass->GetFields();
						auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);

						for (const auto& [name, field] : fields)
						{
							if (entityFields.find(name) != entityFields.end())
							{
								ScriptFieldInstance& scriptField = entityFields.at(name);

								if (field.Type == ScriptFieldType::Float)
								{
									float data = scriptField.GetValue<float>();
									if (ImGui::DragFloat(name.c_str(), &data, 0.25f))
									{
										scriptField.SetValue<float>(data);
									}
								}
								if (field.Type == ScriptFieldType::Int)
								{
									int data = scriptField.GetValue<int>();
									if (ImGui::DragInt(name.c_str(), &data, 1))
									{
										scriptField.SetValue<int>(data);
									}
								}
								if (field.Type == ScriptFieldType::Vector2)
								{
									glm::vec2 data = scriptField.GetValue<glm::vec3>();
									if (ImGui::DragFloat2(name.c_str(), glm::value_ptr(data), 0.25f))
									{
										scriptField.SetValue<glm::vec2>(data);
					}
				}
								if (field.Type == ScriptFieldType::Vector3)
								{
									glm::vec3 data = scriptField.GetValue<glm::vec3>();
									if (ImGui::DragFloat3(name.c_str(), glm::value_ptr(data), 0.25f))
									{
										scriptField.SetValue<glm::vec3>(data);
									}
								}
								if (field.Type == ScriptFieldType::Vector4)
								{
									glm::vec4 data = scriptField.GetValue<glm::vec4>();
									if (ImGui::DragFloat4(name.c_str(), glm::value_ptr(data), 0.25f))
									{
										scriptField.SetValue<glm::vec4>(data);
									}
								}
			}
							else
							{
								if (field.Type == ScriptFieldType::Float)
								{
									float data = 0.0f;
									if (ImGui::DragFloat(name.c_str(), &data))
									{
										ScriptFieldInstance& fieldInstance = entityFields[name];
										fieldInstance.Field = field;
										fieldInstance.SetValue<float>(data);
									}
								}

								if (field.Type == ScriptFieldType::Int)
								{
									int data = 0;
									if (ImGui::DragInt(name.c_str(), &data))
									{
										ScriptFieldInstance& fieldInstance = entityFields[name];
										fieldInstance.Field = field;
										fieldInstance.SetValue<int>(data);
									}
								}

								if (field.Type == ScriptFieldType::Vector2)
								{
									glm::vec2 data(0.0f);
									if (ImGui::DragFloat2(name.c_str(), glm::value_ptr(data)))
									{
										ScriptFieldInstance& fieldInstance = entityFields[name];
										fieldInstance.Field = field;
										fieldInstance.SetValue<glm::vec2>(data);
									}
								}

								if (field.Type == ScriptFieldType::Vector3)
								{
									glm::vec3 data(0.0f);
									if (ImGui::DragFloat3(name.c_str(), glm::value_ptr(data)))
									{
										ScriptFieldInstance& fieldInstance = entityFields[name];
										fieldInstance.Field = field;
										fieldInstance.SetValue<glm::vec3>(data);
									}
								}

								if (field.Type == ScriptFieldType::Vector4)
								{
									glm::vec4 data(0.0f);
									if (ImGui::DragFloat4(name.c_str(), glm::value_ptr(data)))
									{
										ScriptFieldInstance& fieldInstance = entityFields[name];
										fieldInstance.Field = field;
										fieldInstance.SetValue<glm::vec4>(data);
									}
								}
							}
						}
					}
				}

				if (!scriptClassExist)
					ImGui::PopStyleColor();
			});

		DrawComponent<AudioListenerComponent>("AUDIO LISTENER", entity, [](auto& component)
			{
				ImGui::Checkbox("Enable", &component.Enable);
			});
	}

	template<typename T>
	bool SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName)
	{
		if (ImGui::MenuItem(entryName.c_str()))
		{
			m_SelectedEntity.AddComponent<T>();
			ImGui::CloseCurrentPopup();

			return true;
		}

		return false;
	}
}
