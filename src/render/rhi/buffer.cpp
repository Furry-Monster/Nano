#include "buffer.h"
#include <cstring>
#include "misc/logger.h"
#include "rhi.h"

namespace Nano
{
    Buffer::Buffer() {}

    Buffer::~Buffer() noexcept { cleanup(); }

    void Buffer::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_is_mapped)
        {
            unmap();
        }

        if (m_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(rhi.getDevice(), m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
            DEBUG("  Destroyed buffer");
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rhi.getDevice(), m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
            DEBUG("  Released buffer memory");
        }

        m_size = 0;
    }

    bool Buffer::create(VkBufferUsageFlags usage, size_t size, VkMemoryPropertyFlags memory_property_flags)
    {
        RHI& rhi = RHI::instance();

        if (size == 0)
        {
            ERROR("Cannot create buffer with zero size.");
            return false;
        }

        m_size = size;

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.size               = size;
        buffer_create_info.usage              = usage;
        buffer_create_info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(rhi.getDevice(), &buffer_create_info, nullptr, &m_buffer) != VK_SUCCESS)
        {
            ERROR("Failed to create buffer.");
            return false;
        }

        if (!allocateMemory(memory_property_flags))
        {
            return false;
        }

        return true;
    }

    bool Buffer::allocateMemory(VkMemoryPropertyFlags memory_property_flags)
    {
        RHI& rhi = RHI::instance();

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(rhi.getDevice(), m_buffer, &mem_requirements);

        uint32_t memory_type_index = 0;
        if (!rhi.findMemoryType(mem_requirements.memoryTypeBits, memory_property_flags, memory_type_index))
        {
            ERROR("Failed to find suitable memory type for buffer.");
            return false;
        }

        VkMemoryAllocateInfo vkMemoryAllocInfo = {};
        vkMemoryAllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkMemoryAllocInfo.allocationSize       = mem_requirements.size;
        vkMemoryAllocInfo.memoryTypeIndex      = memory_type_index;

        if (vkAllocateMemory(rhi.getDevice(), &vkMemoryAllocInfo, nullptr, &m_memory) != VK_SUCCESS)
        {
            ERROR("Failed to allocate buffer memory.");
            return false;
        }

        if (vkBindBufferMemory(rhi.getDevice(), m_buffer, m_memory, 0) != VK_SUCCESS)
        {
            ERROR("Failed to bind buffer memory.");
            return false;
        }

        return true;
    }

    bool Buffer::uploadData(const void* data, size_t size)
    {
        if (size > m_size)
        {
            ERROR("Upload size (%zu) exceeds buffer size (%zu).", size, m_size);
            return false;
        }

        void* mapped_data = map();
        if (mapped_data == nullptr)
        {
            return false;
        }

        std::memcpy(mapped_data, data, size);
        unmap();

        return true;
    }

    void* Buffer::map()
    {
        if (m_is_mapped)
        {
            WARN("Buffer is already mapped.");
            return nullptr;
        }

        RHI& rhi = RHI::instance();

        void* mapped_data = nullptr;
        if (vkMapMemory(rhi.getDevice(), m_memory, 0, m_size, 0, &mapped_data) != VK_SUCCESS)
        {
            ERROR("Failed to map buffer memory.");
            return nullptr;
        }

        m_is_mapped = true;
        return mapped_data;
    }

    void Buffer::unmap()
    {
        if (!m_is_mapped)
        {
            return;
        }

        RHI& rhi = RHI::instance();
        vkUnmapMemory(rhi.getDevice(), m_memory);
        m_is_mapped = false;
    }

} // namespace Nano
