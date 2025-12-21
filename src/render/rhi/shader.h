#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan_core.h>

namespace Nano
{
    class Shader
    {
    public:
        Shader();
        ~Shader() noexcept;

        Shader(const Shader&)                = delete;
        Shader& operator=(const Shader&)     = delete;
        Shader(Shader&&) noexcept            = delete;
        Shader& operator=(Shader&&) noexcept = delete;

        bool loadFromFile(const char* path);

        VkShaderModule getModule() const { return m_module; }

    private:
        bool readFile(const char* path, char*& data, size_t& size);
        void cleanup();

        VkShaderModule m_module {VK_NULL_HANDLE};
    };

} // namespace Nano

#endif // !SHADER_H
