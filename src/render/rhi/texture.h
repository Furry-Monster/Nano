#ifndef TEXTURE_H
#define TEXTURE_H

#include <vulkan/vulkan_core.h>
namespace Nano
{
    struct Texture
    {
        VkImage            image {VK_NULL_HANDLE};
        VkDeviceMemory     memory {VK_NULL_HANDLE};
        VkImageView        image_view {VK_NULL_HANDLE};
        VkFormat           format {VK_FORMAT_UNDEFINED};
        VkImageAspectFlags image_aspect_flags {VK_IMAGE_ASPECT_NONE};
    };

    struct Texture2D : public Texture
    {
        uint32_t width {0}, height {0};
        uint32_t channel_count {0};
    };
} // namespace Nano

#endif // !TEXTURE_H