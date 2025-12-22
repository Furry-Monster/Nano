#ifndef MATERIAL_H
#define MATERIAL_H

#include <vulkan/vulkan_core.h>
#include <memory>
#include <vector>

namespace Nano
{
    class Pipeline;
    class DescriptorSet;
    class DescriptorSetLayout;
    class Shader;
    class Buffer;
    class Texture;
    class CommandBuffer;

    class Material
    {
    public:
        Material();
        ~Material() noexcept;

        Material(const Material&)                = delete;
        Material& operator=(const Material&)     = delete;
        Material(Material&&) noexcept            = delete;
        Material& operator=(Material&&) noexcept = delete;

        bool init(const char* vertex_shader_path, const char* fragment_shader_path);
        bool
        initVGF(const char* vertex_shader_path, const char* geometry_shader_path, const char* fragment_shader_path);
        bool initVTF(const char* vertex_shader_path,
                     const char* tessellation_control_shader_path,
                     const char* tessellation_evaluation_shader_path,
                     const char* fragment_shader_path);

        void setVertexInput(const std::vector<VkVertexInputBindingDescription>&   bindings,
                            const std::vector<VkVertexInputAttributeDescription>& attributes);
        void setPrimitiveTopology(VkPrimitiveTopology topology);
        void setViewport(const VkViewport& viewport);
        void setScissor(const VkRect2D& scissor);

        bool setUniformBuffer(uint32_t binding, Buffer* buffer);
        bool setTexture(uint32_t binding, Texture* texture, VkSampler sampler);

        bool bind(CommandBuffer* cmd_buffer, VkRenderPass render_pass);

        Pipeline*        getPipeline() { return m_pipeline.get(); }
        DescriptorSet*   getDescriptorSet() { return m_descriptor_set.get(); }
        VkPipelineLayout getPipelineLayout() const;

    private:
        bool createDescriptorSetLayout();
        bool createPipeline(VkRenderPass render_pass);
        void cleanup();

        std::unique_ptr<Shader> m_vertex_shader;
        std::unique_ptr<Shader> m_fragment_shader;
        std::unique_ptr<Shader> m_geometry_shader;
        std::unique_ptr<Shader> m_tessellation_control_shader;
        std::unique_ptr<Shader> m_tessellation_evaluation_shader;

        std::unique_ptr<Pipeline> m_pipeline;

        std::unique_ptr<DescriptorSetLayout> m_descriptor_set_layout;
        std::unique_ptr<DescriptorSet>       m_descriptor_set;

        std::vector<VkVertexInputBindingDescription>   m_vertex_bindings;
        std::vector<VkVertexInputAttributeDescription> m_vertex_attributes;

        VkPrimitiveTopology m_primitive_topology {VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        VkViewport          m_viewport {};
        VkRect2D            m_scissor {};

        bool m_is_initialized {false};
        bool m_pipeline_created {false};
    };

} // namespace Nano

#endif // !MATERIAL_H
