#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Nano
{
    class Shader;
    class Pipeline;
    class DescriptorSet;
    class DescriptorSetLayout;
    class Buffer;
    class Texture;
    class CommandBuffer;

    enum class RenderPassType
    {
        Graphics,
        Compute
    };

    class RenderPass
    {
    public:
        RenderPass(RenderPassType type, const char* name);
        ~RenderPass() noexcept;

        RenderPass(const RenderPass&)                = delete;
        RenderPass& operator=(const RenderPass&)     = delete;
        RenderPass(RenderPass&&) noexcept            = delete;
        RenderPass& operator=(RenderPass&&) noexcept = delete;

        void setComputeShader(const char* compute_shader_path);
        void setGraphicsShaders(const char* vertex_shader_path, const char* fragment_shader_path);

        void bindResource(uint32_t binding, Buffer* buffer, VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        void bindResource(uint32_t         binding,
                          Texture*         texture,
                          VkDescriptorType type      = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                          bool             is_output = false);

        void setUniformBuffer(uint32_t binding, Buffer* buffer);
        void setComputeDispatchArgs(uint32_t x, uint32_t y, uint32_t z);

        bool build(uint32_t canvas_width = 0, uint32_t canvas_height = 0);
        void execute();
        void executeIndirect(Buffer* indirect_buffer);

        RenderPassType     getType() const { return m_type; }
        const std::string& getName() const { return m_name; }

    private:
        void cleanup();
        bool buildCompute();
        bool buildGraphics(uint32_t canvas_width, uint32_t canvas_height);
        void executeCompute();
        void executeGraphics();

        RenderPassType m_type;
        std::string    m_name;

        std::unique_ptr<Shader> m_compute_shader;
        std::unique_ptr<Shader> m_vertex_shader;
        std::unique_ptr<Shader> m_fragment_shader;

        std::unique_ptr<Pipeline>            m_pipeline;
        std::unique_ptr<DescriptorSetLayout> m_descriptor_set_layout;
        std::unique_ptr<DescriptorSet>       m_descriptor_set;

        std::vector<VkDescriptorSetLayoutBinding> m_descriptor_bindings;
        std::vector<Buffer*>                      m_buffers;
        std::vector<Texture*>                     m_textures;
        std::vector<Texture*>                     m_output_textures;
        std::vector<Buffer*>                      m_uniform_buffers;

        uint32_t m_dispatch_x {1};
        uint32_t m_dispatch_y {1};
        uint32_t m_dispatch_z {1};

        uint32_t m_viewport_width {0};
        uint32_t m_viewport_height {0};

        VkRenderPass  m_render_pass {VK_NULL_HANDLE};
        VkFramebuffer m_framebuffer {VK_NULL_HANDLE};
    };

} // namespace Nano

#endif // !RENDER_PASS_H
