#pragma once

#include <vulkan/vulkan_core.h>

namespace Nano
{
    class VulkanDevice;

    class VulkanBuffer
    {
    public:
        VulkanBuffer()           = default;
        ~VulkanBuffer() noexcept = default;

        VulkanBuffer(VulkanBuffer&&) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&&) noexcept;
        VulkanBuffer(const VulkanBuffer&)            = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        void create(VkDevice              device,
                    VkPhysicalDevice      physicalDevice,
                    VkDeviceSize          size,
                    VkBufferUsageFlags    usage,
                    VkMemoryPropertyFlags properties);
        void clean(VkDevice device);

        VkBuffer       getBuffer() const { return m_buffer; }
        VkDeviceMemory getMemory() const { return m_memory; }
        VkDeviceSize   getSize() const { return m_size; }

        void* map(VkDevice device, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void  unmap(VkDevice device);

    private:
        VkBuffer       m_buffer {VK_NULL_HANDLE};
        VkDeviceMemory m_memory {VK_NULL_HANDLE};
        VkDeviceSize   m_size {0};
        void*          m_mapped {nullptr};
    };

} // namespace Nano
