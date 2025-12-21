#include "shader.h"
#include <cstdio>
#include <cstring>
#include "misc/logger.h"
#include "rhi.h"

namespace Nano
{
    Shader::Shader() {}

    Shader::~Shader() noexcept { cleanup(); }

    void Shader::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(rhi.getDevice(), m_module, nullptr);
            m_module = VK_NULL_HANDLE;

            DEBUG("  Destroyed shader module");
        }
    }

    bool Shader::readFile(const char* path, char*& data, size_t& size)
    {
        FILE* file = std::fopen(path, "rb");
        if (file == nullptr)
        {
            ERROR("Failed to open shader file: %s", path);
            return false;
        }

        std::fseek(file, 0, SEEK_END);
        long file_size = std::ftell(file);
        std::rewind(file);

        if (file_size <= 0)
        {
            ERROR("Shader file is empty or invalid: %s", path);
            std::fclose(file);
            return false;
        }

        data             = new char[file_size];
        size_t read_size = std::fread(data, 1, file_size, file);
        std::fclose(file);

        if (read_size != static_cast<size_t>(file_size))
        {
            ERROR("Failed to read shader file completely: %s (read %zu/%ld)", path, read_size, file_size);
            delete[] data;
            return false;
        }

        size = static_cast<size_t>(file_size);
        return true;
    }

    bool Shader::loadFromFile(const char* path)
    {
        RHI& rhi = RHI::instance();

        char*  shader_code = nullptr;
        size_t code_size   = 0;

        if (!readFile(path, shader_code, code_size))
        {
            return false;
        }

        VkShaderModuleCreateInfo create_info = {};
        create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize                 = code_size;
        create_info.pCode                    = reinterpret_cast<const uint32_t*>(shader_code);

        if (vkCreateShaderModule(rhi.getDevice(), &create_info, nullptr, &m_module) != VK_SUCCESS)
        {
            ERROR("Failed to create shader module from file: %s", path);
            delete[] shader_code;
            return false;
        }

        delete[] shader_code;
        return true;
    }

} // namespace Nano
