#ifndef TEXTURE_H
#define TEXTURE_H

#include <vulkan/vulkan_core.h>
#include <cstdint>

namespace Nano
{
    class RHI;

    class Texture
    {
    public:
        Texture();
        ~Texture() noexcept;

        Texture(const Texture&)                = delete;
        Texture& operator=(const Texture&)     = delete;
        Texture(Texture&&) noexcept            = delete;
        Texture& operator=(Texture&&) noexcept = delete;

        bool create(uint32_t              width,
                    uint32_t              height,
                    VkFormat              format,
                    VkImageUsageFlags     usage,
                    VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        bool createFromFile(const char* path);
        bool createImageView(VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_NONE);
        bool uploadData(const void* data, size_t data_size, uint32_t width, uint32_t height);

        VkImageView getImageView() const { return m_image_view; }
        VkImage     getImage() const { return m_image; }
        VkFormat    getFormat() const { return m_format; }
        uint32_t    getWidth() const { return m_width; }
        uint32_t    getHeight() const { return m_height; }

        VkSampler createSampler(VkFilter             min_filter     = VK_FILTER_LINEAR,
                                VkFilter             mag_filter     = VK_FILTER_LINEAR,
                                VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                VkSamplerAddressMode address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                VkSamplerAddressMode address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    private:
        bool transitionImageLayout(VkCommandBuffer      command_buffer,
                                   VkImageLayout        old_layout,
                                   VkImageLayout        new_layout,
                                   VkAccessFlags        src_access_mask,
                                   VkAccessFlags        dst_access_mask,
                                   VkPipelineStageFlags src_stage,
                                   VkPipelineStageFlags dst_stage);
        bool copyBufferToImage(VkCommandBuffer command_buffer, VkBuffer buffer, uint32_t width, uint32_t height);
        bool uploadDataToImage(const void* data, size_t data_size, uint32_t width, uint32_t height);
        bool allocateMemory(VkMemoryPropertyFlags memory_property_flags);
        void cleanup();

        VkImage            m_image {VK_NULL_HANDLE};
        VkDeviceMemory     m_memory {VK_NULL_HANDLE};
        VkImageView        m_image_view {VK_NULL_HANDLE};
        VkFormat           m_format {VK_FORMAT_UNDEFINED};
        VkImageAspectFlags m_image_aspect_flags {VK_IMAGE_ASPECT_NONE};
        uint32_t           m_width {0};
        uint32_t           m_height {0};
        uint32_t           m_channel_count {0};
    };

} // namespace Nano

#endif // !TEXTURE_H
