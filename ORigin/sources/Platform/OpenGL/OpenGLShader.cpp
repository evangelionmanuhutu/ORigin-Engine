// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Origin/Core/ConsoleManager.h"
#include "OpenGLShader.h"
#include "Origin/Core/Time.h"
#include "Origin/Core/Assert.h"
#include "Origin/Profiler/Profiler.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace origin
{
    namespace Utils
    {
        static GLenum ShaderTypeFromString(const std::string &type, const std::string &filepath = std::string())
        {
            OGN_PROFILER_RENDERING();

            if (type == "vertex")                       return GL_VERTEX_SHADER;
            if (type == "fragment" || type == "pixel")  return GL_FRAGMENT_SHADER;
            if (type == "geometry")                     return GL_GEOMETRY_SHADER;
            OGN_CORE_ASSERT(false, "Unkown Shader Type '{}'", filepath);
            return 0;
        }

        static shaderc_shader_kind GLShaderStageToShaderC(GLenum stage)
        {
            OGN_PROFILER_RENDERING();

            switch (stage)
            {
            case GL_VERTEX_SHADER:    return shaderc_glsl_vertex_shader;
            case GL_FRAGMENT_SHADER:  return shaderc_glsl_fragment_shader;
            case GL_GEOMETRY_SHADER:  return shaderc_glsl_geometry_shader;
            }

            OGN_CORE_ASSERT(false, "OpenGLShader: Invalid Shader Stage");
            return (shaderc_shader_kind)0;
        }

        static const char *GLShaderStageToString(GLenum stage)
        {
            OGN_PROFILER_RENDERING();

            switch (stage)
            {
            case GL_VERTEX_SHADER:    return "GL_VERTEX_SHADER";
            case GL_FRAGMENT_SHADER:  return "GL_FRAGMENT_SHADER";
            case GL_GEOMETRY_SHADER:  return "GL_GEOMETRY_SHADER";
            }

            OGN_CORE_ASSERT(false, "OpenGLShader: Invalid Shader Stage");
            return nullptr;
        }

        static const char *GetCacheDirectory()
        {
            return "Resources/Cache/Shaders/OpenGL";
        }

        static void CreateCachedDirectoryIfNeeded()
        {
            OGN_PROFILER_RENDERING();

            std::string cachedDirectory = GetCacheDirectory();
            if (!std::filesystem::exists(cachedDirectory))
                std::filesystem::create_directories(cachedDirectory);
        }

        static const char *GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
        {
            OGN_PROFILER_RENDERING();

            switch (stage)
            {
            case GL_VERTEX_SHADER:    return ".cached_opengl.vert";
            case GL_FRAGMENT_SHADER:  return ".cached_opengl.frag";
            case GL_GEOMETRY_SHADER:  return ".cached_opengl.geom";
            }

            OGN_CORE_ASSERT(false, "OpenGLShader: Invalid Shader Stage");
            return nullptr;
        }

        static const char *GLShaderStageCachedVulkanFileExtension(uint32_t stage)
        {
            OGN_PROFILER_RENDERING();

            switch (stage)
            {
            case GL_VERTEX_SHADER:    return ".cached_vulkan.vert";
            case GL_FRAGMENT_SHADER:  return ".cached_vulkan.frag";
            case GL_GEOMETRY_SHADER:  return ".cached_vulkan.geom";
            }

            OGN_CORE_ASSERT(false, "OpenGLShader: Invalid Shader Stage");
            return nullptr;
        }

        static const char *ShaderDataTypeToString(GLenum type)
        {
            OGN_PROFILER_RENDERING();

            switch (type)
            {
            case GL_VERTEX_SHADER:    return "Vertex";
            case GL_FRAGMENT_SHADER:  return "Fragment";
            case GL_GEOMETRY_SHADER:  return "Geometry";
            }
            return nullptr;
        }

        static const char *ShaderDataTypeToString(ShaderType type)
        {
            OGN_PROFILER_RENDERING();

            switch (type)
            {
            case ShaderType::VERTEX:    return "VERTEX";
            case ShaderType::FRAGMENT:  return "FRAGMENT";
            case ShaderType::GEOMTERY:  return "GEOMETRY";
            }
            return nullptr;
        }
    }

    ShaderProgramSources OpenGLShader::ParseShader(const std::string &filepath)
    {
        OGN_PROFILER_RENDERING();

        std::ifstream stream(filepath);

        std::string line;
        std::stringstream ss[3];
        ShaderType type = ShaderType::NONE;

        while (getline(stream, line))
        {
            // Finding first line for types
            if (line.find("//type ") != std::string::npos || line.find("// type ") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos || line.find("Vertex") != std::string::npos)
                {
                    type = ShaderType::VERTEX;
                    Utils::ShaderDataTypeToString(type);
                }
                else if (line.find("fragment") != std::string::npos || line.find("Fragment") != std::string::npos)
                {
                    type = ShaderType::FRAGMENT;
                    Utils::ShaderDataTypeToString(type);
                }
                else if (line.find("geometry") != std::string::npos || line.find("Geometry") != std::string::npos)
                {
                    type = ShaderType::GEOMTERY;
                    Utils::ShaderDataTypeToString(type);
                }
            }
            else
            {
                // Fill the source code to stream
                ss[(int)type] << line << "\n";
            }
        }

        return { ss[0].str(), ss[1].str(), ss[2].str() };
    }

    OpenGLShader::OpenGLShader(const std::string &filepath, bool enableSpirv, bool recompileSpirv)
        : m_Filepath(filepath), m_RendererID(0), m_EnableSpirv(enableSpirv), m_RecompileSPIRV(recompileSpirv)
    {
        OGN_PROFILER_RENDERING();
        PUSH_CONSOLE_INFO("Trying to load Shader : {}", m_Filepath);
        OGN_CORE_TRACE("Trying to load Shader : {}", m_Filepath);

        // With SPIRV
        if (enableSpirv)
        {
            Utils::CreateCachedDirectoryIfNeeded();

            std::string source = ReadFile(filepath);
            {
                auto shaderSources = PreProcess(source);

                Timer timer;
                CompileOrGetVulkanBinaries(shaderSources);
                CompileOrGetOpenGLBinaries();
                CreateSpirvProgram();
                PUSH_CONSOLE_INFO("Shader Creation took {0} ms", timer.ElapsedMillis());
                OGN_CORE_TRACE("Shader Creation took {0} ms", timer.ElapsedMillis());
            }
        }
        else // Without SPIRV
        {
            m_ShaderSource = ParseShader(filepath);
            m_RendererID = CreateProgram(m_ShaderSource.VertexSources, m_ShaderSource.FragmentSources, m_ShaderSource.GeometrySources);
        }

        // extract shader file to string name
        size_t lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind(".");
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
    }

    OpenGLShader::OpenGLShader(const std::string &name, const std::string &filepath)
        : m_Name(name), m_Filepath(filepath), m_RendererID(0)
    {
        OGN_PROFILER_RENDERING();

        m_ShaderSource = ParseShader(filepath);
        m_RendererID = CreateProgram(m_ShaderSource.VertexSources, m_ShaderSource.FragmentSources, m_ShaderSource.GeometrySources);
    }

    OpenGLShader::OpenGLShader(const std::string &name, const std::string &vertexSrc, const std::string &fragmentSrc)
    {
        OGN_PROFILER_RENDERING();

        std::unordered_map<GLenum, std::string> sources;
        sources[GL_VERTEX_SHADER] = vertexSrc;
        sources[GL_FRAGMENT_SHADER] = fragmentSrc;

        if (!CompileOrGetVulkanBinaries(sources))
        {
            return;
        }
        if (!CompileOrGetOpenGLBinaries())
        {
            return;
        };
        CreateSpirvProgram();
    }

    OpenGLShader::OpenGLShader(const std::string &name, const std::string &vertexSrc, const std::string &fragmentSrc, const std::string &geomterySrc)
        : m_Name(name), m_RendererID(0)
    {
        OGN_PROFILER_RENDERING();

        std::unordered_map<GLenum, std::string> sources;
        sources[GL_VERTEX_SHADER] = vertexSrc;
        sources[GL_FRAGMENT_SHADER] = fragmentSrc;
        sources[GL_GEOMETRY_SHADER] = geomterySrc;

        if (!CompileOrGetVulkanBinaries(sources))
        {
            return;
        }
        if (!CompileOrGetOpenGLBinaries())
        {
            return;
        }

        CreateSpirvProgram();
    }

    OpenGLShader::~OpenGLShader()
    {
        OGN_PROFILER_RENDERING();

        glDeleteProgram(m_RendererID);
    }

    void OpenGLShader::CreateSpirvProgram()
    {
        OGN_PROFILER_RENDERING();

        uint32_t program = glCreateProgram();

        std::vector<uint32_t> shaderIDs;
        for (auto &&[stage, spirv] : m_OpenGLSPIRV)
        {
            uint32_t shaderID = shaderIDs.emplace_back(glCreateShader(stage));
            glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size() * sizeof(uint32_t));
            glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
            glAttachShader(program, shaderID);
            PUSH_CONSOLE_INFO("{0} Shader Attached", Utils::ShaderDataTypeToString(stage));
            OGN_CORE_WARN("{0} Shader Attached", Utils::ShaderDataTypeToString(stage));
        }

        glLinkProgram(program);
        glValidateProgram(program);

        int isLinked;

        glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
        if (isLinked < 0)
        {
            OGN_CORE_ERROR("Shader Linked Status : {0}", isLinked);
            PUSH_CONSOLE_ERROR("Shader Linked Status : {0}", isLinked);
        }
        else
        {
            OGN_CORE_INFO("Shader Linked Status : {0}", isLinked);
            PUSH_CONSOLE_ERROR("Shader Linked Status : {0}", isLinked);
        }

        if (isLinked == GL_FALSE)
        {
            int maxLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<GLchar> infolog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, infolog.data());

            glDeleteProgram(program);

            for (auto id : shaderIDs) glDeleteShader(id);
        }

        for (auto id : shaderIDs)
        {
            glDetachShader(program, id);
            glDeleteShader(id);
        }

        m_RendererID = program;
    }

    std::string OpenGLShader::ReadFile(const std::string &filepath)
    {
        OGN_PROFILER_RENDERING();

        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
            }
            else
            {
                PUSH_CONSOLE_ERROR("Shader: Could not read from file '{0}'", filepath);
                OGN_CORE_ERROR("Shader: Could not read from file '{0}'", filepath);
            }
        }
        else
        {
            PUSH_CONSOLE_ERROR("Shader: Could not open file");
            OGN_CORE_ERROR("Shader: Could not open file");
        }
        return result;
    }

    std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string &source)
    {
        OGN_PROFILER_RENDERING();

        std::unordered_map<GLenum, std::string> shaderSources;

        const char *typeToken = "// type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);

        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);

            if (eol == std::string::npos)
            {
                PUSH_CONSOLE_ERROR("Syntax error");
                OGN_CORE_ASSERT(false, "Syntax error");
            }
            
            size_t begin = pos + typeTokenLength + 1;
            std::string type = source.substr(begin, eol - begin);
            if (!Utils::ShaderTypeFromString(type, m_Filepath))
            {
                PUSH_CONSOLE_ERROR("Invalid shader type specified");
                OGN_CORE_ASSERT(false, "Invalid shader type specified");
            }
            

            size_t nextLinePos = source.find_first_of("\r\n", eol);
            if (nextLinePos == std::string::npos)
            {
                PUSH_CONSOLE_ERROR("Syntax Error");
                OGN_CORE_ASSERT(false, "Syntax Error");
            }
            
            pos = source.find(typeToken, nextLinePos);
            shaderSources[Utils::ShaderTypeFromString(type, m_Filepath)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
        }
        return shaderSources;
    }

    bool OpenGLShader::CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string> &shaderSources)
    {
        OGN_PROFILER_RENDERING();

        uint32_t program = glCreateProgram();
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        const bool optimize = true;
        if (optimize)
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

        auto &shaderData = m_VulkanSPIRV;
        shaderData.clear();
        for (auto &&[stage, source] : shaderSources)
        {
            std::filesystem::path shaderFilepath = m_Filepath;
            std::filesystem::path cachedPath = cacheDirectory / (shaderFilepath.filename().string() + Utils::GLShaderStageCachedVulkanFileExtension(stage));

            std::ifstream infile(cachedPath, std::ios::in | std::ios::binary);
            if (infile.is_open() && !m_RecompileSPIRV)
            {
                PUSH_CONSOLE_INFO("Get Vulkan {0} Shader Binaries", Utils::ShaderDataTypeToString(stage));
                OGN_CORE_WARN("Get Vulkan {0} Shader Binaries", Utils::ShaderDataTypeToString(stage));
                infile.seekg(0, std::ios::end);
                auto size = infile.tellg();
                infile.seekg(0, std::ios::beg);
                auto &data = shaderData[stage];
                data.resize(size / sizeof(uint32_t));
                infile.read((char *)data.data(), size);
            }
            else
            {
                OGN_CORE_WARN("Compile Vulkan {0} Shader To Binaries", Utils::ShaderDataTypeToString(stage));
                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), m_Filepath.c_str());
                bool succsess = module.GetCompilationStatus() == shaderc_compilation_status_success;

                PUSH_CONSOLE_ERROR("[OpenGLShader] Failed to compile Vulkan {0}", module.GetErrorMessage().c_str());
                OGN_CORE_ASSERT(succsess, module.GetErrorMessage());

                if (!succsess)
                {
                    return false;
                }

                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

                std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
                if (out.is_open())
                {
                    auto &data = shaderData[stage];
                    out.write((char *)data.data(), data.size() * sizeof(uint32_t));
                    out.flush();
                    out.close();
                }
            }
        }

        for (auto &&[stage, data] : shaderData)
        {
            Reflect(stage, data);
        }

        return true;
    }

    bool OpenGLShader::CompileOrGetOpenGLBinaries()
    {
        OGN_PROFILER_RENDERING();

        auto &shaderData = m_OpenGLSPIRV;
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        const bool optimize = true;
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance);
        std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();
        shaderData.clear();

        m_OpenGLSourceCode.clear();
        for (auto &&[stage, spirv] : m_VulkanSPIRV)
        {
            std::filesystem::path shaderFilepath = m_Filepath;
            std::filesystem::path cachedPath = cacheDirectory / (shaderFilepath.filename().string() + Utils::GLShaderStageCachedOpenGLFileExtension(stage));

            std::ifstream infile(cachedPath, std::ios::in | std::ios::binary);
            if (infile.is_open() && !m_RecompileSPIRV)
            {
                OGN_CORE_WARN("Get OpenGL {0} Shader Binaries", Utils::ShaderDataTypeToString(stage));
                infile.seekg(0, std::ios::end);
                auto size = infile.tellg();
                infile.seekg(0, std::ios::beg);

                auto &data = shaderData[stage];
                data.resize(size / sizeof(uint32_t));
                infile.read((char *)data.data(), size);
            }
            else
            {
                OGN_CORE_WARN("{0}", m_Filepath);
                OGN_CORE_WARN("Compile OpenGL {0} Shader To Binaries", Utils::ShaderDataTypeToString(stage));
                PUSH_CONSOLE_INFO("[OpenGLShader] Compiling {0} Shader To Binaries from {1}", Utils::ShaderDataTypeToString(stage), m_Filepath);
                spirv_cross::CompilerGLSL glslCompiler(spirv);
                m_OpenGLSourceCode[stage] = glslCompiler.compile();
                auto &source = m_OpenGLSourceCode[stage];
                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::GLShaderStageToShaderC(stage), m_Filepath.c_str());
                bool success = module.GetCompilationStatus() == shaderc_compilation_status_success;

                if (!success)
                {
                    return false;
                }

                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());
                std::ofstream outfile(cachedPath, std::ios::out | std::ios::binary);
                if (outfile.is_open())
                {
                    auto &data = shaderData[stage];
                    outfile.write((char *)data.data(), data.size() * sizeof(uint32_t));
                    outfile.flush();
                    outfile.close();
                }
            }
        }

        return true;
    }

    void OpenGLShader::Reflect(GLenum stage, const std::vector<uint32_t> &shaderData)
    {
        OGN_PROFILER_RENDERING();

        spirv_cross::Compiler compiler(shaderData);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        OGN_CORE_TRACE("OpenGLShader::Reflect - {0}", Utils::GLShaderStageToString(stage));
        OGN_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
        OGN_CORE_TRACE("    {0} resources", resources.sampled_images.size());

        if (resources.uniform_buffers.size())
        {
            OGN_CORE_TRACE("Uniform buffers:");
            for (const auto &resource : resources.uniform_buffers)
            {
                const auto &bufferType = compiler.get_type(resource.base_type_id);
                uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
                uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                int memberCount = bufferType.member_types.size();
                OGN_CORE_WARN("     Name = {0}", resource.name);
                OGN_CORE_TRACE("     Size = {0}", bufferSize);
                OGN_CORE_TRACE("  Binding = {0}", binding);
                OGN_CORE_TRACE("  Members = {0}", memberCount);
            }
        }
    }

    uint32_t OpenGLShader::CompileShader(uint32_t type, const std::string &source)
    {
        OGN_PROFILER_RENDERING();

        uint32_t shaderID = glCreateShader(type);
        const char *src = source.c_str();
        glShaderSource(shaderID, 1, &src, nullptr);
        glCompileShader(shaderID);

        int success;
        char infoLog[512];
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
            const char *m = "[OpenGLShader] Failed to Compile ";
            const char *shaderType {};

            switch (type)
            {
            case GL_VERTEX_SHADER:
                shaderType = "VERTEX TYPE";
                break;
            case GL_FRAGMENT_SHADER:
                shaderType = "FRAGMENT TYPE";
                break;
            case GL_GEOMETRY_SHADER:
                shaderType = "GEOMETRY TYPE";
                break;
            }
            size_t len = strlen(m) + strlen(shaderType) + 1;
            char *msg = (char *)malloc(len);

            if (msg != NULL)
            {
                strcpy(msg, m);
                strcat(msg, shaderType);
                OGN_CORE_ERROR(msg);
                PUSH_CONSOLE_ERROR(msg);
                free(msg);
            }
            PUSH_CONSOLE_ERROR("[OpenGLShader] {0}", infoLog);
            OGN_CORE_ASSERT(false, "[OpenGLShader] {0}", infoLog);
            return 0;
        }

        switch (type)
        {
        case GL_VERTEX_SHADER:
            OGN_CORE_TRACE("VERTEX Succesfully Compiled");
            PUSH_CONSOLE_INFO("VERTEX Succesfully Compiled");
            break;
        case GL_FRAGMENT_SHADER:
            OGN_CORE_TRACE("FRAGMENT Succesfully Compiled");
            PUSH_CONSOLE_INFO("FRAGMENT Succesfully Compiled");
            break;
        case GL_GEOMETRY_SHADER:
            OGN_CORE_TRACE("GEOMETRY Succesfully Compiled");
            PUSH_CONSOLE_INFO("GEOMETRY Succesfully Compiled");
            break;
        }

        return shaderID;
    }

    uint32_t OpenGLShader::CreateProgram(std::string vertexSrc, std::string fragmentSrc, std::string geometrySrc)
    {
        OGN_PROFILER_RENDERING();

        // Create Program
        uint32_t shaderProgram = glCreateProgram();
        uint32_t vShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
        uint32_t fShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!geometrySrc.empty())
        {
            uint32_t gShader = CompileShader(GL_GEOMETRY_SHADER, geometrySrc);
            glAttachShader(shaderProgram, gShader);
        }

        // Attach and Link Shader->Program
        glAttachShader(shaderProgram, vShader);
        glAttachShader(shaderProgram, fShader);
        glLinkProgram(shaderProgram);

        glValidateProgram(shaderProgram);

        glDeleteShader(vShader);
        glDeleteShader(fShader);

        return shaderProgram;
    }

    void OpenGLShader::Enable() const
    {
        glUseProgram(m_RendererID);
    }
    void OpenGLShader::Disable() const
    {
        glUseProgram(0);
    }

    void OpenGLShader::Reload()
    {
        // With SPIRV
        if (m_EnableSpirv)
        {
            Utils::CreateCachedDirectoryIfNeeded();
            m_RecompileSPIRV = true;
            std::string source = ReadFile(m_Filepath);
            {
                auto shaderSources = PreProcess(source);

                Timer timer;
                CompileOrGetVulkanBinaries(shaderSources);
                CompileOrGetOpenGLBinaries();
                CreateSpirvProgram();
                OGN_CORE_TRACE("Shader Creation took {0} ms", timer.ElapsedMillis());
                PUSH_CONSOLE_INFO("Shader Creation took {0} ms", timer.ElapsedMillis());
            }
            m_RecompileSPIRV = false;
        }
        else // Without SPIRV
        {
            m_ShaderSource = ParseShader(m_Filepath);
            m_RendererID = CreateProgram(m_ShaderSource.VertexSources, m_ShaderSource.FragmentSources, m_ShaderSource.GeometrySources);
        }

        // extract shader file to string name
        size_t lastSlash = m_Filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = m_Filepath.rfind(".");
        auto count = lastDot == std::string::npos ? m_Filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = m_Filepath.substr(lastSlash, count);
    }

    void OpenGLShader::SetBool(const std::string &name, bool boolean)
    {
        SetUniformBool(name, boolean);
    }

    void OpenGLShader::SetFloat(const std::string &name, float v0)
    {
        SetUniformFloat(name, v0);
    }

    void OpenGLShader::SetFloat(const std::string &name, float v0, float v1)
    {
        SetUniformFloat(name, v0, v1);
    }

    void OpenGLShader::SetFloat(const std::string &name, float v0, float v1, float v2)
    {
        SetUniformFloat(name, v0, v1, v2);
    }

    void OpenGLShader::SetFloat(const std::string &name, float v0, float v1, float v2, float v3)
    {
        SetUniformFloat(name, v0, v1, v2, v3);
    }

    void OpenGLShader::SetInt(const std::string &name, int v0)
    {
        SetUniformInt(name, v0);
    }

    void OpenGLShader::SetInt(const std::string &name, int v0, int v1)
    {
        SetUniformInt(name, v0, v1);
    }

    void OpenGLShader::SetInt(const std::string &name, int v0, int v1, int v2)
    {
        SetUniformInt(name, v0, v1, v2);
    }

    void OpenGLShader::SetInt(const std::string &name, int v0, int v1, int v2, int v3)
    {
        SetUniformInt(name, v0, v1, v2, v3);
    }

    void  OpenGLShader::SetIntArray(const std::string &name, int *values, uint32_t count)
    {
        SetUniformIntArray(name, values, count);
    }

    void OpenGLShader::SetVector(const std::string &name, const glm::vec2 &vectors)
    {
        SetUniformVector(name, vectors);
    }

    void OpenGLShader::SetVector(const std::string &name, const glm::vec3 &vectors)
    {
        SetUniformVector(name, vectors);
    }

    void OpenGLShader::SetVector(const std::string &name, const glm::vec4 &vectors)
    {
        SetUniformVector(name, vectors);
    }

    void OpenGLShader::SetMatrix(const std::string &name, const glm::mat2 &matrices)
    {
        SetUniformMatrix(name, matrices);
    }

    void OpenGLShader::SetMatrix(const std::string &name, const glm::mat3 &matrices)
    {
        SetUniformMatrix(name, matrices);
    }

    void OpenGLShader::SetMatrix(const std::string &name, const glm::mat4 &matrices)
    {
        SetUniformMatrix(name, matrices);
    }

    // private
    // BOOLEAN UNIFORM
    void OpenGLShader::SetUniformBool(const std::string &name, bool boolean)
    {
        glUniform1i(GetUniformLocation(name), boolean);
    }

    // FLOAT UNIFORM
    void OpenGLShader::SetUniformFloat(const std::string &name, float v0)
    {
        glUniform1f(GetUniformLocation(name), v0);
    }
    void OpenGLShader::SetUniformFloat(const std::string &name, float v0, float v1)
    {
        glUniform2f(GetUniformLocation(name), v0, v1);
    }
    void OpenGLShader::SetUniformFloat(const std::string &name, float v0, float v1, float v2)
    {
        glUniform3f(GetUniformLocation(name), v0, v1, v2);
    }
    void OpenGLShader::SetUniformFloat(const std::string &name, float v0, float v1, float v2, float v3)
    {
        glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
    }
    // INT UNIFORM
    void OpenGLShader::SetUniformInt(const std::string &name, int v0)
    {
        glUniform1i(GetUniformLocation(name), v0);
    }

    void OpenGLShader::SetUniformInt(const std::string &name, int v0, int v1)
    {
        glUniform2i(GetUniformLocation(name), v0, v1);
    }

    void OpenGLShader::SetUniformInt(const std::string &name, int v0, int v1, int v2)
    {
        glUniform3i(GetUniformLocation(name), v0, v1, v2);
    }

    void OpenGLShader::SetUniformInt(const std::string &name, int v0, int v1, int v2, int v3)
    {
        glUniform4i(GetUniformLocation(name), v0, v1, v2, v3);
    }

    void OpenGLShader::SetUniformIntArray(const std::string &name, int *values, uint32_t count)
    {
        glUniform1iv(GetUniformLocation(name), count, values);
    }

    // VECTOR UNIFORM
    void OpenGLShader::SetUniformVector(const std::string &name, const glm::vec2 &vector2)
    {
        glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(vector2));
    }

    void OpenGLShader::SetUniformVector(const std::string &name, const glm::vec3 &vector3)
    {
        glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(vector3));
    }

    void OpenGLShader::SetUniformVector(const std::string &name, const glm::vec4 &vector4)
    {
        glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(vector4));
    }

    void OpenGLShader::SetUniformMatrix(const std::string &name, const glm::mat2 &matrices)
    {
        glUniformMatrix3fv(GetUniformLocation(name), 1, false, glm::value_ptr(matrices));
    }

    // MATRIX UNIFORM
    void OpenGLShader::SetUniformMatrix(const std::string &name, const glm::mat3 &matrices)
    {
        glUniformMatrix3fv(GetUniformLocation(name), 1, false, glm::value_ptr(matrices));
    }

    void OpenGLShader::SetUniformMatrix(const std::string &name, const glm::mat4 &matrices)
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrices));
    }

    int OpenGLShader::GetUniformLocation(const std::string &name)
    {
        if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
            return m_UniformLocationCache[name];

        int location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location == -1)
        {
            OGN_CORE_WARN("[OpenGLShader]: '{0}' WARNING UNIFORM '{1}' does not exists or uninitialized", m_Name, name);
            PUSH_CONSOLE_INFO("[OpenGLShader]: '{0}' WARNING UNIFORM '{1}' does not exists or uninitialized", m_Name, name);
        }

        m_UniformLocationCache[name] = location;
        return location;
    }
}