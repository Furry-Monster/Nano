#include "vulkan_command_pool.h"
#include <stdexcept>

namespace Nano
{
    VulkanCommandPool::VulkanCommandPool(VulkanCommandPool&& other) noexcept : m_command_pool(other.m_command_pool)
    {
        other.m_command_pool = VK_NULL_HANDLE;
    }

    VulkanCommandPool& VulkanCommandPool::operator=(VulkanCommandPool&& other) noexcept
    {
        if (this != &other)
        {
            m_command_pool       = other.m_command_pool;
            other.m_command_pool = VK_NULL_HANDLE;
        }
        return *this;
    }

    void VulkanCommandPool::init(VkDevice device, uint32_t queueFamilyIndex)
    {
        VkCommandPoolCreateInfo pool_info {};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = queueFamilyIndex;

        if (vkCreateCommandPool(device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void VulkanCommandPool::clean(VkDevice device)
    {
        if (m_command_pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, m_command_pool, nullptr);
            m_command_pool = VK_NULL_HANDLE;
        }
    }

    VkCommandBuffer VulkanCommandPool::beginSingleTimeCommand(VkDevice device) const
    {
        VkCommandBufferAllocateInfo alloc_info {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool        = m_command_pool;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        return command_buffer;
    }

    void VulkanCommandPool::endSingleTimeCommand(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer) const
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submit_info {};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, m_command_pool, 1, &commandBuffer);
    }

    std::vector<VkCommandBuffer> VulkanCommandPool::allocateCommandBuffers(VkDevice device, uint32_t count) const
    {
        VkCommandBufferAllocateInfo alloc_info {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = m_command_pool;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = count;

        std::vector<VkCommandBuffer> command_buffers(count);
        if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        return command_buffers;
    }

    void VulkanCommandPool::freeCommandBuffers(VkDevice                            device,
                                               const std::vector<VkCommandBuffer>& commandBuffers) const
    {
        vkFreeCommandBuffers(
            device, m_command_pool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    }

} // namespace Nano
