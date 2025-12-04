#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

namespace Nano
{
    class VulkanCommandPool
    {
    public:
        VulkanCommandPool()           = default;
        ~VulkanCommandPool() noexcept = default;

        VulkanCommandPool(VulkanCommandPool&&) noexcept;
        VulkanCommandPool& operator=(VulkanCommandPool&&) noexcept;
        VulkanCommandPool(const VulkanCommandPool&)            = delete;
        VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;

        void init(VkDevice device, uint32_t queueFamilyIndex);
        void clean(VkDevice device);

        VkCommandPool   getCommandPool() const { return m_command_pool; }
        VkCommandBuffer beginSingleTimeCommand(VkDevice device) const;
        void            endSingleTimeCommand(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer) const;

        std::vector<VkCommandBuffer> allocateCommandBuffers(VkDevice device, uint32_t count) const;
        void freeCommandBuffers(VkDevice device, const std::vector<VkCommandBuffer>& commandBuffers) const;

    private:
        VkCommandPool m_command_pool {VK_NULL_HANDLE};
    };

} // namespace Nano
