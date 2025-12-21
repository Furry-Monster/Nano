#include "command_buffer.h"
#include "misc/logger.h"
#include "rhi.h"

namespace Nano
{
    CommandBuffer::CommandBuffer() {}

    CommandBuffer::~CommandBuffer() noexcept { cleanup(); }

    void CommandBuffer::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_command_buffer != VK_NULL_HANDLE && m_command_pool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(rhi.getDevice(), m_command_pool, 1, &m_command_buffer);
            m_command_buffer = VK_NULL_HANDLE;

            DEBUG("  Freed command buffer");
        }

        if (m_command_pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(rhi.getDevice(), m_command_pool, nullptr);
            m_command_pool = VK_NULL_HANDLE;

            DEBUG("  Destroyed command pool");
        }

        m_is_recording = false;
    }

    bool CommandBuffer::createCommandPool()
    {
        RHI& rhi = RHI::instance();

        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex        = rhi.getGraphicsQueueFamilyIndex();

        if (vkCreateCommandPool(rhi.getDevice(), &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
        {
            ERROR("Failed to create command pool.");
            return false;
        }

        return true;
    }

    bool CommandBuffer::create(VkCommandBufferLevel level)
    {
        RHI& rhi = RHI::instance();

        if (m_command_pool == VK_NULL_HANDLE)
        {
            if (!createCommandPool())
                return false;
        }

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool                 = m_command_pool;
        alloc_info.level                       = level;
        alloc_info.commandBufferCount          = 1;

        if (vkAllocateCommandBuffers(rhi.getDevice(), &alloc_info, &m_command_buffer) != VK_SUCCESS)
        {
            ERROR("Failed to allocate command buffer.");
            return false;
        }

        return true;
    }

    bool CommandBuffer::begin(VkCommandBufferUsageFlags flags)
    {
        if (m_command_buffer == VK_NULL_HANDLE)
        {
            ERROR("Command buffer not created. Call create() first.");
            return false;
        }

        if (m_is_recording)
        {
            WARN("Command buffer is already recording.");
            return false;
        }

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags                    = flags;

        if (vkBeginCommandBuffer(m_command_buffer, &begin_info) != VK_SUCCESS)
        {
            ERROR("Failed to begin command buffer.");
            return false;
        }

        m_is_recording = true;
        return true;
    }

    bool CommandBuffer::end()
    {
        if (!m_is_recording)
        {
            WARN("Command buffer is not recording.");
            return false;
        }

        if (vkEndCommandBuffer(m_command_buffer) != VK_SUCCESS)
        {
            ERROR("Failed to end command buffer.");
            m_is_recording = false;
            return false;
        }

        m_is_recording = false;
        return true;
    }

    bool CommandBuffer::submit(VkQueue              queue,
                               VkSemaphore          wait_semaphore,
                               VkSemaphore          signal_semaphore,
                               VkPipelineStageFlags wait_stage,
                               VkFence              fence)
    {
        if (m_is_recording)
        {
            ERROR("Cannot submit command buffer while recording. Call end() first.");
            return false;
        }

        if (m_command_buffer == VK_NULL_HANDLE)
        {
            ERROR("Command buffer not created.");
            return false;
        }

        VkSubmitInfo submit_info       = {};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &m_command_buffer;

        if (wait_semaphore != VK_NULL_HANDLE)
        {
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores    = &wait_semaphore;
            submit_info.pWaitDstStageMask  = &wait_stage;
        }

        if (signal_semaphore != VK_NULL_HANDLE)
        {
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores    = &signal_semaphore;
        }

        if (vkQueueSubmit(queue, 1, &submit_info, fence) != VK_SUCCESS)
        {
            ERROR("Failed to submit command buffer.");
            return false;
        }

        return true;
    }

    bool CommandBuffer::reset(VkCommandBufferResetFlags flags)
    {
        if (m_command_buffer == VK_NULL_HANDLE)
        {
            ERROR("Command buffer not created.");
            return false;
        }

        if (m_is_recording)
        {
            ERROR("Cannot reset command buffer while recording.");
            return false;
        }

        if (vkResetCommandBuffer(m_command_buffer, flags) != VK_SUCCESS)
        {
            ERROR("Failed to reset command buffer.");
            return false;
        }

        return true;
    }

} // namespace Nano
