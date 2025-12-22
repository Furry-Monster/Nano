#include "render_pass.h"
#include <cstring>
#include "misc/logger.h"
#include "render/rhi/buffer.h"
#include "render/rhi/command_buffer.h"
#include "render/rhi/descriptor_set.h"
#include "render/rhi/pipeline.h"
#include "render/rhi/rhi.h"
#include "render/rhi/shader.h"
#include "render/rhi/texture.h"

namespace Nano
{
    RenderPass::RenderPass(RenderPassType type, const char* name) : m_type(type), m_name(name ? name : "") {}

    RenderPass::~RenderPass() noexcept { cleanup(); }

    void RenderPass::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(rhi.getDevice(), m_framebuffer, nullptr);
            m_framebuffer = VK_NULL_HANDLE;
        }

        if (m_render_pass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(rhi.getDevice(), m_render_pass, nullptr);
            m_render_pass = VK_NULL_HANDLE;
        }

        m_pipeline.reset();
        m_descriptor_set.reset();
        m_descriptor_set_layout.reset();
        m_compute_shader.reset();
        m_vertex_shader.reset();
        m_fragment_shader.reset();

        m_descriptor_bindings.clear();
        m_buffers.clear();
        m_textures.clear();
        m_output_textures.clear();
        m_uniform_buffers.clear();
    }

    void RenderPass::setComputeShader(const char* compute_shader_path)
    {
        if (m_type != RenderPassType::Compute)
        {
            ERROR("Cannot set compute shader for graphics render pass.");
            return;
        }

        m_compute_shader = std::make_unique<Shader>();
        if (!m_compute_shader->loadFromFile(compute_shader_path))
        {
            ERROR("Failed to load compute shader: %s", compute_shader_path);
            m_compute_shader.reset();
        }
    }

    void RenderPass::setGraphicsShaders(const char* vertex_shader_path, const char* fragment_shader_path)
    {
        if (m_type != RenderPassType::Graphics)
        {
            ERROR("Cannot set graphics shaders for compute render pass.");
            return;
        }

        m_vertex_shader = std::make_unique<Shader>();
        if (!m_vertex_shader->loadFromFile(vertex_shader_path))
        {
            ERROR("Failed to load vertex shader: %s", vertex_shader_path);
            m_vertex_shader.reset();
            return;
        }

        m_fragment_shader = std::make_unique<Shader>();
        if (!m_fragment_shader->loadFromFile(fragment_shader_path))
        {
            ERROR("Failed to load fragment shader: %s", fragment_shader_path);
            m_fragment_shader.reset();
        }
    }

    void RenderPass::bindResource(uint32_t binding, Buffer* buffer, VkDescriptorType type)
    {
        if (buffer == nullptr)
        {
            ERROR("Cannot bind null buffer to render pass.");
            return;
        }

        VkDescriptorSetLayoutBinding layout_binding = {};
        layout_binding.binding                      = binding;
        layout_binding.descriptorCount              = 1;
        layout_binding.descriptorType               = type;
        layout_binding.stageFlags =
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        m_descriptor_bindings.push_back(layout_binding);

        if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            m_uniform_buffers.push_back(buffer);
        }
        else
        {
            m_buffers.push_back(buffer);
        }
    }

    void RenderPass::bindResource(uint32_t binding, Texture* texture, VkDescriptorType type, bool is_output)
    {
        if (texture == nullptr)
        {
            ERROR("Cannot bind null texture to render pass.");
            return;
        }

        VkDescriptorSetLayoutBinding layout_binding = {};
        layout_binding.binding                      = binding;
        layout_binding.descriptorCount              = 1;
        layout_binding.descriptorType               = type;
        layout_binding.stageFlags                   = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        m_descriptor_bindings.push_back(layout_binding);

        m_textures.push_back(texture);
        if (is_output)
        {
            m_output_textures.push_back(texture);
        }
    }

    void RenderPass::setUniformBuffer(uint32_t binding, Buffer* buffer)
    {
        if (buffer == nullptr)
        {
            ERROR("Cannot set null uniform buffer to render pass.");
            return;
        }

        VkDescriptorSetLayoutBinding layout_binding = {};
        layout_binding.binding                      = binding;
        layout_binding.descriptorCount              = 1;
        layout_binding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layout_binding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        m_descriptor_bindings.push_back(layout_binding);

        m_uniform_buffers.push_back(buffer);
    }

    void RenderPass::setComputeDispatchArgs(uint32_t x, uint32_t y, uint32_t z)
    {
        m_dispatch_x = x;
        m_dispatch_y = y;
        m_dispatch_z = z;
    }

    bool RenderPass::buildCompute()
    {
        if (!m_compute_shader)
        {
            ERROR("Compute shader not set for compute render pass.");
            return false;
        }

        if (m_descriptor_bindings.empty())
        {
            ERROR("No resources bound to compute render pass.");
            return false;
        }

        m_descriptor_set_layout = std::make_unique<DescriptorSetLayout>();
        if (!m_descriptor_set_layout->create(m_descriptor_bindings))
        {
            ERROR("Failed to create descriptor set layout for compute render pass.");
            return false;
        }

        std::vector<VkDescriptorPoolSize> pool_sizes;
        if (!m_uniform_buffers.empty())
        {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_uniform_buffers.size())});
        }
        if (!m_textures.empty())
        {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<uint32_t>(m_textures.size())});
        }
        if (!m_buffers.empty())
        {
            pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(m_buffers.size())});
        }

        if (pool_sizes.empty())
        {
            ERROR("No descriptor pool sizes for compute render pass.");
            return false;
        }

        m_descriptor_set = std::make_unique<DescriptorSet>();
        if (!m_descriptor_set->allocate(m_descriptor_set_layout->getLayout(), pool_sizes))
        {
            ERROR("Failed to allocate descriptor set for compute render pass.");
            return false;
        }

        size_t uniform_idx = 0;
        size_t texture_idx = 0;
        size_t buffer_idx  = 0;

        for (const auto& binding_info : m_descriptor_bindings)
        {
            if (binding_info.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                if (uniform_idx < m_uniform_buffers.size())
                {
                    if (!m_descriptor_set->updateBuffer(
                            binding_info.binding, m_uniform_buffers[uniform_idx], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER))
                    {
                        ERROR("Failed to update uniform buffer binding %u in compute render pass.",
                              binding_info.binding);
                        return false;
                    }
                    uniform_idx++;
                }
            }
            else if (binding_info.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            {
                if (texture_idx < m_textures.size())
                {
                    if (!m_descriptor_set->updateImage(binding_info.binding,
                                                       m_textures[texture_idx]->getImageView(),
                                                       VK_IMAGE_LAYOUT_GENERAL,
                                                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE))
                    {
                        ERROR("Failed to update storage image binding %u in compute render pass.",
                              binding_info.binding);
                        return false;
                    }
                    texture_idx++;
                }
            }
            else if (binding_info.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            {
                if (buffer_idx < m_buffers.size())
                {
                    if (!m_descriptor_set->updateBuffer(
                            binding_info.binding, m_buffers[buffer_idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER))
                    {
                        ERROR("Failed to update storage buffer binding %u in compute render pass.",
                              binding_info.binding);
                        return false;
                    }
                    buffer_idx++;
                }
            }
        }

        ComputePipelineCreateInfo pipeline_info = {};
        pipeline_info.compute_shader            = m_compute_shader->getModule();
        pipeline_info.descriptor_set_layout     = m_descriptor_set_layout->getLayout();

        m_pipeline = std::make_unique<Pipeline>();
        if (!m_pipeline->createComputePipeline(pipeline_info))
        {
            ERROR("Failed to create compute pipeline for render pass.");
            return false;
        }

        return true;
    }

    bool RenderPass::buildGraphics(uint32_t canvas_width, uint32_t canvas_height)
    {
        if (!m_vertex_shader || !m_fragment_shader)
        {
            ERROR("Graphics shaders not set for graphics render pass.");
            return false;
        }

        m_viewport_width  = canvas_width;
        m_viewport_height = canvas_height;

        if (m_descriptor_bindings.empty())
        {
            WARN("No resources bound to graphics render pass.");
        }

        if (!m_descriptor_bindings.empty())
        {
            m_descriptor_set_layout = std::make_unique<DescriptorSetLayout>();
            if (!m_descriptor_set_layout->create(m_descriptor_bindings))
            {
                ERROR("Failed to create descriptor set layout for graphics render pass.");
                return false;
            }

            std::vector<VkDescriptorPoolSize> pool_sizes;
            if (!m_uniform_buffers.empty())
            {
                pool_sizes.push_back(
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_uniform_buffers.size())});
            }
            if (!m_textures.empty())
            {
                pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<uint32_t>(m_textures.size())});
            }
            if (!m_buffers.empty())
            {
                pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(m_buffers.size())});
            }

            if (!pool_sizes.empty())
            {
                m_descriptor_set = std::make_unique<DescriptorSet>();
                if (!m_descriptor_set->allocate(m_descriptor_set_layout->getLayout(), pool_sizes))
                {
                    ERROR("Failed to allocate descriptor set for graphics render pass.");
                    return false;
                }

                size_t uniform_idx = 0;
                size_t texture_idx = 0;
                size_t buffer_idx  = 0;

                for (const auto& binding_info : m_descriptor_bindings)
                {
                    if (binding_info.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    {
                        if (uniform_idx < m_uniform_buffers.size())
                        {
                            if (!m_descriptor_set->updateBuffer(binding_info.binding,
                                                                m_uniform_buffers[uniform_idx],
                                                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER))
                            {
                                ERROR("Failed to update uniform buffer binding %u in graphics render pass.",
                                      binding_info.binding);
                                return false;
                            }
                            uniform_idx++;
                        }
                    }
                    else if (binding_info.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                    {
                        if (texture_idx < m_textures.size())
                        {
                            if (!m_descriptor_set->updateImage(binding_info.binding,
                                                               m_textures[texture_idx]->getImageView(),
                                                               VK_IMAGE_LAYOUT_GENERAL,
                                                               VK_DESCRIPTOR_TYPE_STORAGE_IMAGE))
                            {
                                ERROR("Failed to update storage image binding %u in graphics render pass.",
                                      binding_info.binding);
                                return false;
                            }
                            texture_idx++;
                        }
                    }
                    else if (binding_info.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    {
                        if (buffer_idx < m_buffers.size())
                        {
                            if (!m_descriptor_set->updateBuffer(
                                    binding_info.binding, m_buffers[buffer_idx], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER))
                            {
                                ERROR("Failed to update storage buffer binding %u in graphics render pass.",
                                      binding_info.binding);
                                return false;
                            }
                            buffer_idx++;
                        }
                    }
                }
            }
        }

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount        = 0;
        render_pass_info.subpassCount           = 1;
        render_pass_info.pSubpasses             = &subpass;

        RHI& rhi = RHI::instance();
        if (vkCreateRenderPass(rhi.getDevice(), &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
        {
            ERROR("Failed to create render pass.");
            return false;
        }

        if (canvas_width > 0 && canvas_height > 0)
        {
            VkFramebufferCreateInfo framebuffer_info = {};
            framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass              = m_render_pass;
            framebuffer_info.width                   = canvas_width;
            framebuffer_info.height                  = canvas_height;
            framebuffer_info.layers                  = 1;
            framebuffer_info.attachmentCount         = 0;
            framebuffer_info.pAttachments            = nullptr;

            if (vkCreateFramebuffer(rhi.getDevice(), &framebuffer_info, nullptr, &m_framebuffer) != VK_SUCCESS)
            {
                ERROR("Failed to create framebuffer for graphics render pass.");
                return false;
            }
        }

        GraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.render_pass                = m_render_pass;
        pipeline_info.vertex_shader              = m_vertex_shader->getModule();
        pipeline_info.fragment_shader            = m_fragment_shader->getModule();
        pipeline_info.viewport                   = {0.0f, 0.0f, float(canvas_width), float(canvas_height), 0.0f, 1.0f};
        pipeline_info.scissor                    = {{0, 0}, {canvas_width, canvas_height}};
        if (m_descriptor_set_layout)
        {
            pipeline_info.descriptor_set_layout = m_descriptor_set_layout->getLayout();
        }

        m_pipeline = std::make_unique<Pipeline>();
        if (!m_pipeline->createGraphicsPipeline(pipeline_info))
        {
            ERROR("Failed to create graphics pipeline for render pass.");
            return false;
        }

        return true;
    }

    bool RenderPass::build(uint32_t canvas_width, uint32_t canvas_height)
    {
        if (m_type == RenderPassType::Compute)
        {
            return buildCompute();
        }
        else
        {
            return buildGraphics(canvas_width, canvas_height);
        }
    }

    void RenderPass::executeCompute()
    {
        RHI& rhi = RHI::instance();

        CommandBuffer cmd;
        if (!cmd.create())
        {
            ERROR("Failed to create command buffer for compute render pass execution.");
            return;
        }

        if (!cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
            ERROR("Failed to begin command buffer for compute render pass execution.");
            return;
        }

        for (Texture* output_texture : m_output_textures)
        {
            VkImageSubresourceRange range = {};
            range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel            = 0;
            range.levelCount              = 1;
            range.baseArrayLayer          = 0;
            range.layerCount              = 1;

            VkImageMemoryBarrier barrier = {};
            barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout            = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                = output_texture->getImage();
            barrier.srcAccessMask        = 0;
            barrier.dstAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.subresourceRange     = range;

            vkCmdPipelineBarrier(cmd.getCommandBuffer(),
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);
        }

        vkCmdBindPipeline(cmd.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipeline());

        if (m_descriptor_set)
        {
            VkDescriptorSet descriptor_set = m_descriptor_set->getDescriptorSet();
            vkCmdBindDescriptorSets(cmd.getCommandBuffer(),
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    m_pipeline->getLayout(),
                                    0,
                                    1,
                                    &descriptor_set,
                                    0,
                                    nullptr);
        }

        vkCmdDispatch(cmd.getCommandBuffer(), m_dispatch_x, m_dispatch_y, m_dispatch_z);

        for (Texture* output_texture : m_output_textures)
        {
            VkImageSubresourceRange range = {};
            range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel            = 0;
            range.levelCount              = 1;
            range.baseArrayLayer          = 0;
            range.layerCount              = 1;

            VkImageMemoryBarrier barrier = {};
            barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout            = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                = output_texture->getImage();
            barrier.srcAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
            barrier.subresourceRange     = range;

            vkCmdPipelineBarrier(cmd.getCommandBuffer(),
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);
        }

        if (!cmd.end())
        {
            ERROR("Failed to end command buffer for compute render pass execution.");
            return;
        }

        VkFence           fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags             = 0;
        vkCreateFence(rhi.getDevice(), &fence_info, nullptr, &fence);

        if (!cmd.submit(
                rhi.getGraphicsQueue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, fence))
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to submit command buffer for compute render pass execution.");
            return;
        }

        if (vkWaitForFences(rhi.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to wait for compute render pass execution to complete.");
            return;
        }

        vkDestroyFence(rhi.getDevice(), fence, nullptr);
    }

    void RenderPass::executeGraphics()
    {
        RHI& rhi = RHI::instance();

        CommandBuffer cmd;
        if (!cmd.create())
        {
            ERROR("Failed to create command buffer for graphics render pass execution.");
            return;
        }

        if (!cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
            ERROR("Failed to begin command buffer for graphics render pass execution.");
            return;
        }

        if (m_framebuffer != VK_NULL_HANDLE)
        {
            VkClearValue clear_values[2] = {};
            clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clear_values[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass            = m_render_pass;
            render_pass_info.framebuffer           = m_framebuffer;
            render_pass_info.renderArea.offset     = {0, 0};
            render_pass_info.renderArea.extent     = {m_viewport_width, m_viewport_height};
            render_pass_info.clearValueCount       = 2;
            render_pass_info.pClearValues          = clear_values;

            vkCmdBeginRenderPass(cmd.getCommandBuffer(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdBindPipeline(cmd.getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipeline());

        if (m_descriptor_set)
        {
            VkDescriptorSet descriptor_set = m_descriptor_set->getDescriptorSet();
            vkCmdBindDescriptorSets(cmd.getCommandBuffer(),
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_pipeline->getLayout(),
                                    0,
                                    1,
                                    &descriptor_set,
                                    0,
                                    nullptr);
        }

        if (m_framebuffer != VK_NULL_HANDLE)
        {
            vkCmdEndRenderPass(cmd.getCommandBuffer());
        }

        if (!cmd.end())
        {
            ERROR("Failed to end command buffer for graphics render pass execution.");
            return;
        }

        VkFence           fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags             = 0;
        vkCreateFence(rhi.getDevice(), &fence_info, nullptr, &fence);

        if (!cmd.submit(
                rhi.getGraphicsQueue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, fence))
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to submit command buffer for graphics render pass execution.");
            return;
        }

        if (vkWaitForFences(rhi.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to wait for graphics render pass execution to complete.");
            return;
        }

        vkDestroyFence(rhi.getDevice(), fence, nullptr);
    }

    void RenderPass::execute()
    {
        if (m_type == RenderPassType::Compute)
        {
            executeCompute();
        }
        else
        {
            executeGraphics();
        }
    }

    void RenderPass::executeIndirect(Buffer* indirect_buffer)
    {
        if (m_type != RenderPassType::Graphics)
        {
            ERROR("executeIndirect can only be called on graphics render pass.");
            return;
        }

        if (indirect_buffer == nullptr)
        {
            ERROR("Cannot execute indirect draw with null buffer.");
            return;
        }

        RHI& rhi = RHI::instance();

        CommandBuffer cmd;
        if (!cmd.create())
        {
            ERROR("Failed to create command buffer for indirect render pass execution.");
            return;
        }

        if (!cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
            ERROR("Failed to begin command buffer for indirect render pass execution.");
            return;
        }

        if (m_framebuffer != VK_NULL_HANDLE)
        {
            VkClearValue clear_values[2] = {};
            clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clear_values[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass            = m_render_pass;
            render_pass_info.framebuffer           = m_framebuffer;
            render_pass_info.renderArea.offset     = {0, 0};
            render_pass_info.renderArea.extent     = {m_viewport_width, m_viewport_height};
            render_pass_info.clearValueCount       = 2;
            render_pass_info.pClearValues          = clear_values;

            vkCmdBeginRenderPass(cmd.getCommandBuffer(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdBindPipeline(cmd.getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipeline());

        if (m_descriptor_set)
        {
            VkDescriptorSet descriptor_set = m_descriptor_set->getDescriptorSet();
            vkCmdBindDescriptorSets(cmd.getCommandBuffer(),
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_pipeline->getLayout(),
                                    0,
                                    1,
                                    &descriptor_set,
                                    0,
                                    nullptr);
        }

        vkCmdDrawIndirect(cmd.getCommandBuffer(), indirect_buffer->getBuffer(), 0, 1, 16);

        if (m_framebuffer != VK_NULL_HANDLE)
        {
            vkCmdEndRenderPass(cmd.getCommandBuffer());
        }

        if (!cmd.end())
        {
            ERROR("Failed to end command buffer for indirect render pass execution.");
            return;
        }

        VkFence           fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags             = 0;
        vkCreateFence(rhi.getDevice(), &fence_info, nullptr, &fence);

        if (!cmd.submit(
                rhi.getGraphicsQueue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, fence))
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to submit command buffer for indirect render pass execution.");
            return;
        }

        if (vkWaitForFences(rhi.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to wait for indirect render pass execution to complete.");
            return;
        }

        vkDestroyFence(rhi.getDevice(), fence, nullptr);
    }

} // namespace Nano
