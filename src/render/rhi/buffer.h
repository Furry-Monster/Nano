#ifndef BUFFER_H
#define BUFFER_H

#include <vulkan/vulkan_core.h>

namespace Nano
{
    class Buffer
    {
    public:
        Buffer();
        ~Buffer() noexcept;

        Buffer(const Buffer&)                = delete;
        Buffer& operator=(const Buffer&)     = delete;
        Buffer(Buffer&&) noexcept            = delete;
        Buffer& operator=(Buffer&&) noexcept = delete;

        bool create(VkBufferUsageFlags    usage,
                    size_t                size,
                    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        bool uploadData(const void* data, size_t size);

        void* map();
        void  unmap();

        VkBuffer getBuffer() const { return m_buffer; }
        size_t   getSize() const { return m_size; }

    private:
        bool allocateMemory(VkMemoryPropertyFlags memory_property_flags);
        void cleanup();

        VkBuffer       m_buffer {VK_NULL_HANDLE};
        VkDeviceMemory m_memory {VK_NULL_HANDLE};
        size_t         m_size {0};
        bool           m_is_mapped {false};
    };

} // namespace Nano

#endif // !BUFFER_H
