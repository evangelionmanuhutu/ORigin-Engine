// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef SCRIPT_INSTANCE_H
#define SCRIPT_INSTANCE_H

#include "ScriptField.h"
#include "ScriptClass.h"

extern "C" {
    typedef struct _MonoMethod MonoMethod;
    typedef struct _MonoObject MonoObject;
    typedef struct _MonoClassField MonoClassField;
}

namespace origin
{
    class OGN_API Entity;
    class OGN_API ScriptClass;

    // script field + data storage
    struct OGN_API ScriptFieldInstance
    {
        ScriptField Field;
        ScriptFieldInstance()
        {
            memset(m_Buffer, 0, sizeof(m_Buffer));
        }

        template<typename T>
        T GetValue()
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            return *(T *)m_Buffer;
        }

        template<typename T>
        void SetValue(T value)
        {
            static_assert(sizeof(T) <= 16, "Type too large!");
            memcpy(m_Buffer, &value, sizeof(T));
        }

    private:
        char m_Buffer[16];

        friend class ScriptEngine;
        friend class ScriptInstance;
    };

    class OGN_API ScriptInstance
    {
    public:
        ScriptInstance(std::shared_ptr<ScriptClass> scriptClass, Entity entity);

        void InvokeOnCreate();
        void InvokeOnUpdate(float time);

        std::shared_ptr<ScriptClass> GetScriptClass() { return m_ScriptClass; }
        MonoObject *GetMonoObject() { return m_Instance; }

        template<typename T>
        T GetFieldValue(const std::string &name)
        {
            static_assert(sizeof(T) <= 24, "Type too large!");

            bool success = GetFieldValueInternal(name, s_FieldValueBuffer);
            if (!success)
                return T();

            return *(T *)s_FieldValueBuffer;
        }

        template<typename T>
        void SetFieldValue(const std::string &name, const T &value)
        {
            static_assert(sizeof(T) <= 24, "Type too large!");
            SetFieldValueInternal(name, &value);
        }

    private:
        bool GetFieldValueInternal(const std::string &name, void *buffer);
        bool SetFieldValueInternal(const std::string &name, const void *value);

    private:
        std::shared_ptr<ScriptClass> m_ScriptClass;

        MonoObject *m_Instance = nullptr;
        MonoMethod *m_OnConstructor = nullptr;
        MonoMethod *m_OnCreateMethod = nullptr;
        MonoMethod *m_OnUpdateMethod = nullptr;

        inline static char s_FieldValueBuffer[24];
        friend class ScriptEngine;
    };
}

#endif