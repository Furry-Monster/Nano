#include "shader.h"
#include <glad/gl.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Nano
{
    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
    {
        std::string   vertex_code;
        std::string   fragment_code;
        std::ifstream vertex_shader_file;
        std::ifstream fragment_shader_file;

        vertex_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fragment_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            vertex_shader_file.open(vertexPath);
            fragment_shader_file.open(fragmentPath);

            std::stringstream vertex_shader_stream, fragment_shader_stream;
            vertex_shader_stream << vertex_shader_file.rdbuf();
            fragment_shader_stream << fragment_shader_file.rdbuf();

            vertex_shader_file.close();
            fragment_shader_file.close();

            vertex_code   = vertex_shader_stream.str();
            fragment_code = fragment_shader_stream.str();
        }
        catch (std::ifstream::failure& e)
        {
            throw std::runtime_error("Error: failed to read shader file: " + vertexPath + " or " + fragmentPath);
        }

        const char* vertex_shader_code   = vertex_code.c_str();
        const char* fragment_shader_code = fragment_code.c_str();

        unsigned int vertex, fragment;
        int          success;
        char         info_log[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertex_shader_code, nullptr);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, nullptr, info_log);
            glDeleteShader(vertex);
            throw std::runtime_error("Error: vertex shader compilation failed: " + std::string(info_log));
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragment_shader_code, nullptr);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, nullptr, info_log);
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            throw std::runtime_error("Error: fragment shader compilation failed: " + std::string(info_log));
        }

        m_id = glCreateProgram();
        glAttachShader(m_id, vertex);
        glAttachShader(m_id, fragment);
        glLinkProgram(m_id);

        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(m_id, 512, nullptr, info_log);
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            throw std::runtime_error("Error: shader program linking failed: " + std::string(info_log));
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader::Shader(const std::string& vertexPath, const std::string& geometryPath, const std::string& fragmentPath)
    {
        std::string   vertex_code;
        std::string   geometry_code;
        std::string   fragment_code;
        std::ifstream vertex_shader_file;
        std::ifstream geometry_shader_file;
        std::ifstream fragment_shader_file;

        vertex_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        geometry_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fragment_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            vertex_shader_file.open(vertexPath);
            geometry_shader_file.open(geometryPath);
            fragment_shader_file.open(fragmentPath);

            std::stringstream vertex_shader_stream, geometry_shader_stream, fragment_shader_stream;
            vertex_shader_stream << vertex_shader_file.rdbuf();
            geometry_shader_stream << geometry_shader_file.rdbuf();
            fragment_shader_stream << fragment_shader_file.rdbuf();

            vertex_shader_file.close();
            geometry_shader_file.close();
            fragment_shader_file.close();

            vertex_code   = vertex_shader_stream.str();
            geometry_code = geometry_shader_stream.str();
            fragment_code = fragment_shader_stream.str();
        }
        catch (std::ifstream::failure& e)
        {
            throw std::runtime_error("Error: failed to read shader file: " + vertexPath + ", " + geometryPath + " or " +
                                     fragmentPath);
        }

        const char* vertex_shader_code   = vertex_code.c_str();
        const char* geometry_shader_code = geometry_code.c_str();
        const char* fragment_shader_code = fragment_code.c_str();

        unsigned int vertex, geometry, fragment;
        int          success;
        char         info_log[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertex_shader_code, nullptr);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, nullptr, info_log);
            glDeleteShader(vertex);
            throw std::runtime_error("Error: vertex shader compilation failed: " + std::string(info_log));
        }

        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &geometry_shader_code, nullptr);
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(geometry, 512, nullptr, info_log);
            glDeleteShader(vertex);
            glDeleteShader(geometry);
            throw std::runtime_error("Error: geometry shader compilation failed: " + std::string(info_log));
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragment_shader_code, nullptr);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, nullptr, info_log);
            glDeleteShader(vertex);
            glDeleteShader(geometry);
            glDeleteShader(fragment);
            throw std::runtime_error("Error: fragment shader compilation failed: " + std::string(info_log));
        }

        m_id = glCreateProgram();
        glAttachShader(m_id, vertex);
        glAttachShader(m_id, geometry);
        glAttachShader(m_id, fragment);
        glLinkProgram(m_id);

        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(m_id, 512, nullptr, info_log);
            glDeleteShader(vertex);
            glDeleteShader(geometry);
            glDeleteShader(fragment);
            throw std::runtime_error("Error: shader program linking failed: " + std::string(info_log));
        }

        glDeleteShader(vertex);
        glDeleteShader(geometry);
        glDeleteShader(fragment);
    }

    Shader::~Shader() noexcept
    {
        if (m_id != 0)
            glDeleteProgram(m_id);
    }

    Shader::Shader(Shader&& other) noexcept : m_id(other.m_id) { other.m_id = 0; }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            if (m_id != 0)
                glDeleteProgram(m_id);
            m_id       = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    void Shader::use() const { glUseProgram(m_id); }

    void Shader::setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(m_id, name.c_str()), static_cast<int>(value));
    }

    void Shader::setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void Shader::setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
    }

    void Shader::setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2f(glGetUniformLocation(m_id, name.c_str()), value[0], value[1]);
    }

    void Shader::setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3f(glGetUniformLocation(m_id, name.c_str()), value[0], value[1], value[2]);
    }

    void Shader::setVec3Array(const std::string& name, const std::vector<glm::vec3>& values) const
    {
        glUniform3fv(glGetUniformLocation(m_id, name.c_str()), values.size(), &values[0][0]);
    }

    void Shader::setFloatArray(const std::string& name, const std::vector<float>& values) const
    {
        glUniform1fv(glGetUniformLocation(m_id, name.c_str()), values.size(), values.data());
    }

    void Shader::setIntArray(const std::string& name, const std::vector<int>& values) const
    {
        glUniform1iv(glGetUniformLocation(m_id, name.c_str()), values.size(), values.data());
    }

    void Shader::setMat4(const std::string& name, const glm::mat4& value) const
    {
        glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }

    void Shader::setModelViewProjectionMatrices(const glm::mat4& model,
                                                const glm::mat4& view,
                                                const glm::mat4& projection) const
    {
        setMat4("model", model);
        setMat4("view", view);
        setMat4("projection", projection);
    }

    void Shader::bindUniformBlock(const std::string& name, unsigned int binding_point) const
    {
        unsigned int block_index = glGetUniformBlockIndex(m_id, name.c_str());
        if (block_index != GL_INVALID_INDEX)
            glUniformBlockBinding(m_id, block_index, binding_point);
    }

} // namespace Nano
