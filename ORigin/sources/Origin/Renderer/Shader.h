// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include <glad\glad.h>
#include <glm\glm.hpp>

#include <string>
#include <unordered_map>

namespace origin
{
	class Shader
	{
	public:
		virtual ~Shader() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

    virtual std::string ReadFile() const = 0;
    virtual std::string GetFilepath() const = 0;
    virtual const std::string& GetName() const = 0;
    virtual void SetBool(const std::string& name, bool boolean) = 0;

    virtual void SetFloat(const std::string& name, float v0) = 0;
    virtual void SetFloat(const std::string& name, float v0, float v1) = 0;
    virtual void SetFloat(const std::string& name, float v0, float v1, float v2) = 0;
    virtual void SetFloat(const std::string& name, float v0, float v1, float v2, float v3) = 0;
    virtual void SetInt(const std::string& name, int v0) = 0;
    virtual void SetInt(const std::string& name, int v0, int v1) = 0;
    virtual void SetInt(const std::string& name, int v0, int v1, int v2) = 0;
    virtual void SetInt(const std::string& name, int v0, int v1, int v2, int v3) = 0;
    virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0;
    virtual void SetVector(const std::string& name, const glm::vec2& vector2) = 0;
    virtual void SetVector(const std::string& name, const glm::vec3& vector3) = 0;
    virtual void SetVector(const std::string& name, const glm::vec4& vector4) = 0;
    virtual void SetMatrix(const std::string& name, const glm::mat2& matrices) = 0;
    virtual void SetMatrix(const std::string& name, const glm::mat3& matrices) = 0;
    virtual void SetMatrix(const std::string& name, const glm::mat4& matrices) = 0;

    virtual bool IsSpirvEnabled() const = 0;
    virtual bool IsRecompilerSpirv() const = 0;

    static std::shared_ptr<Shader> Create(const std::string& filepath, bool enableSpirv = false, bool recompileSpirv = false);
    static std::shared_ptr<Shader> Create(const std::string& name, const std::string& filepath);
    static std::shared_ptr<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	};

  class ShaderLibrary
  {
  public:
    ShaderLibrary() = default;

    void Add(const std::shared_ptr<Shader>& shader);
    void Add(const std::string& name, const std::shared_ptr<Shader>& shader);

    std::shared_ptr<Shader> Load(const std::string& filepath);
    std::shared_ptr<Shader> Load(const std::string& name, const std::string& filepath);
    std::shared_ptr<Shader> Load(const std::string& name, const std::string& filepath, bool enableSpirv, bool recompileSpirv = false);

    std::shared_ptr<Shader> Get(const std::string& name);

    bool Exist(const std::string& name);

    std::unordered_map<std::string, std::shared_ptr<Shader>> GetMap() const { return m_ShaderMap; }
    inline size_t GetSize() const { return m_ShaderMap.size(); }

  private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_ShaderMap;
  };

}