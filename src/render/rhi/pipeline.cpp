#include "pipeline.h"
#include <cstring>
#include "misc/logger.h"
#include "rhi.h"

namespace Nano
{
    Pipeline::Pipeline() {}

    Pipeline::~Pipeline() noexcept { cleanup(); }

    void Pipeline::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(rhi.getDevice(), m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;

            DEBUG("  Destroyed pipeline");
        }

        if (m_layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(rhi.getDevice(), m_layout, nullptr);
            m_layout = VK_NULL_HANDLE;

            DEBUG("  Destroyed pipeline layout");
        }
    }

    bool Pipeline::createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts,
                                        const std::vector<VkPushConstantRange>&   push_constant_ranges)
    {
        RHI& rhi = RHI::instance();

        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount             = static_cast<uint32_t>(descriptor_set_layouts.size());
        layout_info.pSetLayouts            = descriptor_set_layouts.empty() ? nullptr : descriptor_set_layouts.data();
        layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
        layout_info.pPushConstantRanges    = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data();

        if (vkCreatePipelineLayout(rhi.getDevice(), &layout_info, nullptr, &m_layout) != VK_SUCCESS)
        {
            ERROR("Failed to create pipeline layout.");
            return false;
        }

        return true;
    }

    bool Pipeline::createGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
    {
        RHI& rhi = RHI::instance();

        if (create_info.render_pass == VK_NULL_HANDLE)
        {
            ERROR("Render pass is required for graphics pipeline.");
            return false;
        }

        if (create_info.vertex_shader == VK_NULL_HANDLE || create_info.fragment_shader == VK_NULL_HANDLE)
        {
            ERROR("Vertex and fragment shaders are required for graphics pipeline.");
            return false;
        }

        // Build shader stages
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(5);

        VkPipelineShaderStageCreateInfo vert_stage = {};
        vert_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_stage.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
        vert_stage.module                          = create_info.vertex_shader;
        vert_stage.pName                           = "main";
        shader_stages.push_back(vert_stage);

        if (create_info.tessellation_control_shader != VK_NULL_HANDLE &&
            create_info.tessellation_evaluation_shader != VK_NULL_HANDLE)
        {
            VkPipelineShaderStageCreateInfo tcs_stage = {};
            tcs_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            tcs_stage.stage                           = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            tcs_stage.module                          = create_info.tessellation_control_shader;
            tcs_stage.pName                           = "main";
            shader_stages.push_back(tcs_stage);

            VkPipelineShaderStageCreateInfo tes_stage = {};
            tes_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            tes_stage.stage                           = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            tes_stage.module                          = create_info.tessellation_evaluation_shader;
            tes_stage.pName                           = "main";
            shader_stages.push_back(tes_stage);
        }

        if (create_info.geometry_shader != VK_NULL_HANDLE)
        {
            VkPipelineShaderStageCreateInfo geom_stage = {};
            geom_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            geom_stage.stage                           = VK_SHADER_STAGE_GEOMETRY_BIT;
            geom_stage.module                          = create_info.geometry_shader;
            geom_stage.pName                           = "main";
            shader_stages.push_back(geom_stage);
        }

        VkPipelineShaderStageCreateInfo frag_stage = {};
        frag_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_stage.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_stage.module                          = create_info.fragment_shader;
        frag_stage.pName                           = "main";
        shader_stages.push_back(frag_stage);

        // Vertex Input State
        VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
        vertex_input_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(create_info.vertex_bindings.size());
        vertex_input_info.pVertexBindingDescriptions =
            create_info.vertex_bindings.empty() ? nullptr : create_info.vertex_bindings.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(create_info.vertex_attributes.size());
        vertex_input_info.pVertexAttributeDescriptions =
            create_info.vertex_attributes.empty() ? nullptr : create_info.vertex_attributes.data();

        // Input Assembly State
        VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology =
            create_info.patch_control_points > 0 ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : create_info.primitive_topology;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        // Viewport State
        VkPipelineViewportStateCreateInfo viewport_state = {};
        viewport_state.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount                     = 1;
        viewport_state.scissorCount                      = 1;
        viewport_state.pViewports                        = &create_info.viewport;
        viewport_state.pScissors                         = &create_info.scissor;

        // Rasterization State
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable                       = VK_FALSE;
        rasterizer.rasterizerDiscardEnable                = VK_FALSE;
        rasterizer.polygonMode                            = create_info.polygon_mode;
        rasterizer.lineWidth                              = create_info.line_width;
        rasterizer.cullMode                               = create_info.cull_mode;
        rasterizer.frontFace                              = create_info.front_face;
        rasterizer.depthBiasEnable                        = VK_FALSE;
        rasterizer.depthBiasConstantFactor                = 0.0f;
        rasterizer.depthBiasClamp                         = 0.0f;
        rasterizer.depthBiasSlopeFactor                   = 0.0f;

        // Multisample State
        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable                  = VK_FALSE;
        multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading                     = 1.0f;
        multisampling.pSampleMask                          = nullptr;
        multisampling.alphaToCoverageEnable                = VK_FALSE;
        multisampling.alphaToOneEnable                     = VK_FALSE;

        // Depth Stencil State
        VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
        depth_stencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable       = create_info.depth_test_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.depthWriteEnable      = create_info.depth_write_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.depthCompareOp        = create_info.depth_compare_op;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds        = 0.0f;
        depth_stencil.maxDepthBounds        = 1.0f;
        depth_stencil.stencilTestEnable     = create_info.stencil_test_enable ? VK_TRUE : VK_FALSE;
        depth_stencil.front                 = {};
        depth_stencil.back                  = {};

        // Color Blend State
        VkPipelineColorBlendAttachmentState color_blend_attachment = {};
        color_blend_attachment.colorWriteMask                      = create_info.color_write_mask;
        color_blend_attachment.blendEnable         = create_info.color_blend_enable ? VK_TRUE : VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending = {};
        color_blending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable                       = VK_FALSE;
        color_blending.logicOp                             = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount                     = 1;
        color_blending.pAttachments                        = &color_blend_attachment;
        color_blending.blendConstants[0]                   = 0.0f;
        color_blending.blendConstants[1]                   = 0.0f;
        color_blending.blendConstants[2]                   = 0.0f;
        color_blending.blendConstants[3]                   = 0.0f;

        // Dynamic State (currently none, but can be extended)
        VkPipelineDynamicStateCreateInfo dynamic_state = {};
        dynamic_state.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount                = 0;
        dynamic_state.pDynamicStates                   = nullptr;

        // Tessellation State
        VkPipelineTessellationStateCreateInfo  tessellation_state = {};
        VkPipelineTessellationStateCreateInfo* p_tessellation     = nullptr;
        if (create_info.patch_control_points > 0)
        {
            tessellation_state.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            tessellation_state.patchControlPoints = create_info.patch_control_points;
            p_tessellation                        = &tessellation_state;
        }

        // Create Pipeline Layout
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        if (create_info.descriptor_set_layout != VK_NULL_HANDLE)
        {
            descriptor_set_layouts.push_back(create_info.descriptor_set_layout);
        }

        if (!createPipelineLayout(descriptor_set_layouts, create_info.push_constant_ranges))
        {
            return false;
        }

        // Graphics Pipeline Create Info
        VkGraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount                   = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages                      = shader_stages.data();
        pipeline_info.pVertexInputState            = &vertex_input_info;
        pipeline_info.pInputAssemblyState          = &input_assembly;
        pipeline_info.pViewportState               = &viewport_state;
        pipeline_info.pRasterizationState          = &rasterizer;
        pipeline_info.pMultisampleState            = &multisampling;
        pipeline_info.pDepthStencilState           = &depth_stencil;
        pipeline_info.pColorBlendState             = &color_blending;
        pipeline_info.pDynamicState                = &dynamic_state;
        pipeline_info.pTessellationState           = p_tessellation;
        pipeline_info.layout                       = m_layout;
        pipeline_info.renderPass                   = create_info.render_pass;
        pipeline_info.subpass                      = 0;
        pipeline_info.basePipelineHandle           = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex            = -1;

        if (vkCreateGraphicsPipelines(rhi.getDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline) !=
            VK_SUCCESS)
        {
            ERROR("Failed to create graphics pipeline.");
            return false;
        }

        return true;
    }

    bool Pipeline::createComputePipeline(const ComputePipelineCreateInfo& create_info)
    {
        RHI& rhi = RHI::instance();

        if (create_info.compute_shader == VK_NULL_HANDLE)
        {
            ERROR("Compute shader is required for compute pipeline.");
            return false;
        }

        // Shader Stage
        VkPipelineShaderStageCreateInfo shader_stage = {};
        shader_stage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
        shader_stage.module                          = create_info.compute_shader;
        shader_stage.pName                           = "main";

        // Create Pipeline Layout
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        if (create_info.descriptor_set_layout != VK_NULL_HANDLE)
        {
            descriptor_set_layouts.push_back(create_info.descriptor_set_layout);
        }

        if (!createPipelineLayout(descriptor_set_layouts, create_info.push_constant_ranges))
        {
            return false;
        }

        // Compute Pipeline Create Info
        VkComputePipelineCreateInfo pipeline_info = {};
        pipeline_info.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.stage                       = shader_stage;
        pipeline_info.layout                      = m_layout;
        pipeline_info.basePipelineHandle          = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex           = -1;

        if (vkCreateComputePipelines(rhi.getDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline) !=
            VK_SUCCESS)
        {
            ERROR("Failed to create compute pipeline.");
            return false;
        }

        return true;
    }

} // namespace Nano
