#include "vulkan_buffer.h"
#include <cstdint>
#include <stdexcept>

namespace Nano
{
    VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept :
        m_buffer(other.m_buffer), m_memory(other.m_memory), m_size(other.m_size), m_mapped(other.m_mapped)
    {
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size   = 0;
        other.m_mapped = nullptr;
    }

    VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
    {
        if (this != &other)
        {
            m_buffer = other.m_buffer;
            m_memory = other.m_memory;
            m_size   = other.m_size;
            m_mapped = other.m_mapped;

            other.m_buffer = VK_NULL_HANDLE;
            other.m_memory = VK_NULL_HANDLE;
            other.m_size   = 0;
            other.m_mapped = nullptr;
        }
        return *this;
    }

    void VulkanBuffer::create(VkDevice              device,
                              VkPhysicalDevice      physicalDevice,
                              VkDeviceSize          size,
                              VkBufferUsageFlags    usage,
                              VkMemoryPropertyFlags properties)
    {
        m_size = size;

        VkBufferCreateInfo buffer_info {};
        buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size        = size;
        buffer_info.usage       = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &buffer_info, nullptr, &m_buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer!");
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device, m_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info {};
        alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;

        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mem_properties);

        uint32_t memory_type_index = UINT32_MAX;
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        {
            if ((mem_requirements.memoryTypeBits & (1 << i)) &&
                (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                memory_type_index = i;
                break;
            }
        }

        if (memory_type_index == UINT32_MAX)
        {
            throw std::runtime_error("Failed to find suitable memory type!");
        }

        alloc_info.memoryTypeIndex = memory_type_index;

        if (vkAllocateMemory(device, &alloc_info, nullptr, &m_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, m_buffer, m_memory, 0);
    }

    void VulkanBuffer::clean(VkDevice device)
    {
        if (m_mapped)
        {
            unmap(device);
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }

        if (m_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
        }

        m_size = 0;
    }

    void* VulkanBuffer::map(VkDevice device, VkDeviceSize size, VkDeviceSize offset)
    {
        if (m_mapped)
        {
            return m_mapped;
        }

        if (vkMapMemory(device, m_memory, offset, size, 0, &m_mapped) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map buffer memory!");
        }

        return m_mapped;
    }

    void VulkanBuffer::unmap(VkDevice device)
    {
        if (m_mapped)
        {
            vkUnmapMemory(device, m_memory);
            m_mapped = nullptr;
        }
    }

} // namespace Nano
