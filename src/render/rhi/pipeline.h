#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan_core.h>
#include <vector>

namespace Nano
{
    class Shader;

    struct GraphicsPipelineCreateInfo
    {
        VkRenderPass render_pass {VK_NULL_HANDLE};

        // shaders
        VkShaderModule vertex_shader {VK_NULL_HANDLE};
        VkShaderModule fragment_shader {VK_NULL_HANDLE};
        VkShaderModule geometry_shader {VK_NULL_HANDLE};
        VkShaderModule tessellation_control_shader {VK_NULL_HANDLE};
        VkShaderModule tessellation_evaluation_shader {VK_NULL_HANDLE};

        // vertex input
        std::vector<VkVertexInputBindingDescription>   vertex_bindings;
        std::vector<VkVertexInputAttributeDescription> vertex_attributes;

        // pipeline state
        VkPrimitiveTopology primitive_topology {VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
        VkPolygonMode       polygon_mode {VK_POLYGON_MODE_FILL};
        VkCullModeFlags     cull_mode {VK_CULL_MODE_BACK_BIT};
        VkFrontFace         front_face {VK_FRONT_FACE_COUNTER_CLOCKWISE};
        float               line_width {1.0f};

        // depth/stencil
        bool        depth_test_enable {true};
        bool        depth_write_enable {true};
        VkCompareOp depth_compare_op {VK_COMPARE_OP_LESS_OR_EQUAL};
        bool        stencil_test_enable {false};

        // color blend
        bool                  color_blend_enable {false};
        VkColorComponentFlags color_write_mask {VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

        // viewport/scissor (if not dynamic)
        VkViewport viewport {};
        VkRect2D   scissor {};

        // tessellation
        uint32_t patch_control_points {0}; // 0 means no tessellation

        // descriptor set layout
        VkDescriptorSetLayout descriptor_set_layout {VK_NULL_HANDLE};

        // push constants
        std::vector<VkPushConstantRange> push_constant_ranges;
    };

    struct ComputePipelineCreateInfo
    {
        VkShaderModule compute_shader {VK_NULL_HANDLE};

        // descriptor set layout
        VkDescriptorSetLayout descriptor_set_layout {VK_NULL_HANDLE};

        // push constants
        std::vector<VkPushConstantRange> push_constant_ranges;
    };

    class Pipeline
    {
    public:
        Pipeline();
        ~Pipeline() noexcept;

        Pipeline(const Pipeline&)                = delete;
        Pipeline& operator=(const Pipeline&)     = delete;
        Pipeline(Pipeline&&) noexcept            = delete;
        Pipeline& operator=(Pipeline&&) noexcept = delete;

        bool createGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);
        bool createComputePipeline(const ComputePipelineCreateInfo& create_info);

        VkPipeline       getPipeline() const { return m_pipeline; }
        VkPipelineLayout getLayout() const { return m_layout; }

    private:
        bool createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
                                  const std::vector<VkPushConstantRange>&   push_constant_ranges);
        void cleanup();

        VkPipeline       m_pipeline {VK_NULL_HANDLE};
        VkPipelineLayout m_layout {VK_NULL_HANDLE};
    };

} // namespace Nano

#endif // !PIPELINE_H
