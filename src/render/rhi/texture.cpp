#include "texture.h"
#include <cstring>
#include "misc/logger.h"
#include "rhi.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Nano
{
    Texture::Texture() {}

    Texture::~Texture() noexcept { cleanup(); }

    void Texture::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_image_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(rhi.getDevice(), m_image_view, nullptr);
            m_image_view = VK_NULL_HANDLE;
        }

        if (m_image != VK_NULL_HANDLE)
        {
            vkDestroyImage(rhi.getDevice(), m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(rhi.getDevice(), m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }
    }

    bool Texture::create(uint32_t              width,
                         uint32_t              height,
                         VkFormat              format,
                         VkImageUsageFlags     usage,
                         VkMemoryPropertyFlags memory_property_flags)
    {
        RHI& rhi = RHI::instance();

        m_width  = width;
        m_height = height;
        m_format = format;

        // Determine image aspect flags based on format
        if (format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
            format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            m_image_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            {
                m_image_aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            m_image_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        // Create image
        VkImageCreateInfo vkImageCreateInfo = {};
        vkImageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        vkImageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
        vkImageCreateInfo.extent.width      = width;
        vkImageCreateInfo.extent.height     = height;
        vkImageCreateInfo.extent.depth      = 1;
        vkImageCreateInfo.mipLevels         = 1;
        vkImageCreateInfo.arrayLayers       = 1;
        vkImageCreateInfo.format            = format;
        vkImageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        vkImageCreateInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        vkImageCreateInfo.usage             = usage;
        vkImageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        vkImageCreateInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(rhi.getDevice(), &vkImageCreateInfo, nullptr, &m_image) != VK_SUCCESS)
        {
            ERROR("Failed to create texture image.");
            return false;
        }

        // Allocate and bind memory
        if (!allocateMemory(memory_property_flags))
            return false;

        return true;
    }

    bool Texture::allocateMemory(VkMemoryPropertyFlags memory_property_flags)
    {
        RHI& rhi = RHI::instance();

        VkMemoryRequirements mem_requirements;
        vkGetImageMemoryRequirements(rhi.getDevice(), m_image, &mem_requirements);

        VkMemoryAllocateInfo vkMemoryAllocInfo = {};
        vkMemoryAllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkMemoryAllocInfo.allocationSize       = mem_requirements.size;
        vkMemoryAllocInfo.memoryTypeIndex      = 0;

        // Find suitable memory type
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(rhi.getPhysicalDevice(), &mem_properties);

        bool found = false;
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        {
            if ((mem_requirements.memoryTypeBits & (1 << i)) &&
                (mem_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags)
            {
                vkMemoryAllocInfo.memoryTypeIndex = i;
                found                             = true;
                break;
            }
        }

        if (!found)
        {
            ERROR("Failed to find suitable memory type for texture.");
            return false;
        }

        if (vkAllocateMemory(rhi.getDevice(), &vkMemoryAllocInfo, nullptr, &m_memory) != VK_SUCCESS)
        {
            ERROR("Failed to allocate texture memory.");
            return false;
        }

        if (vkBindImageMemory(rhi.getDevice(), m_image, m_memory, 0) != VK_SUCCESS)
        {
            ERROR("Failed to bind texture memory.");
            return false;
        }

        return true;
    }

    bool Texture::createImageView(VkImageAspectFlags aspect_flags)
    {
        RHI& rhi = RHI::instance();

        if (aspect_flags != VK_IMAGE_ASPECT_NONE)
        {
            m_image_aspect_flags = aspect_flags;
        }

        VkImageViewCreateInfo vkImageViewCreateInfo           = {};
        vkImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vkImageViewCreateInfo.image                           = m_image;
        vkImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        vkImageViewCreateInfo.format                          = m_format;
        vkImageViewCreateInfo.subresourceRange.aspectMask     = m_image_aspect_flags;
        vkImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        vkImageViewCreateInfo.subresourceRange.levelCount     = 1;
        vkImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        vkImageViewCreateInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(rhi.getDevice(), &vkImageViewCreateInfo, nullptr, &m_image_view) != VK_SUCCESS)
        {
            ERROR("Failed to create texture image view.");
            return false;
        }

        return true;
    }

    bool Texture::uploadData(const void* data, size_t data_size, uint32_t width, uint32_t height)
    {
        RHI& rhi = RHI::instance();

        // TODO: Implement proper upload using staging buffer and command buffer
        // For now, this is a placeholder
        // This should:
        // 1. Create a staging buffer
        // 2. Copy data to staging buffer
        // 3. Use command buffer to transition image layout and copy from buffer to image
        // 4. Transition image to shader read layout

        WARN("Texture::uploadData is not yet fully implemented. Use createFromFile for file loading.");
        return false;
    }

    bool Texture::createFromFile(const char* path)
    {
        int      width, height, channels;
        stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            ERROR("Failed to load texture from file: %s", path);
            return false;
        }

        m_width         = static_cast<uint32_t>(width);
        m_height        = static_cast<uint32_t>(height);
        m_channel_count = 4; // STBI_rgb_alpha always returns 4 channels
        m_format        = VK_FORMAT_R8G8B8A8_UNORM;

        if (!create(m_width,
                    m_height,
                    m_format,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            stbi_image_free(pixels);
            return false;
        }

        // TODO: Implement proper upload using staging buffer
        // For now, we create the image but don't upload the data
        // This should use a staging buffer and command buffer to upload the pixel data

        stbi_image_free(pixels);
        WARN("Texture::createFromFile created image but data upload is not yet implemented.");
        return true;
    }

    VkSampler Texture::createSampler(VkFilter             min_filter,
                                     VkFilter             mag_filter,
                                     VkSamplerAddressMode address_mode_u,
                                     VkSamplerAddressMode address_mode_v,
                                     VkSamplerAddressMode address_mode_w)
    {
        RHI& rhi = RHI::instance();

        VkSamplerCreateInfo samplerInfo     = {};
        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = mag_filter;
        samplerInfo.minFilter               = min_filter;
        samplerInfo.addressModeU            = address_mode_u;
        samplerInfo.addressModeV            = address_mode_v;
        samplerInfo.addressModeW            = address_mode_w;
        samplerInfo.anisotropyEnable        = VK_FALSE;
        samplerInfo.maxAnisotropy           = 1.0f;
        samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.minLod                  = 0.0f;
        samplerInfo.maxLod                  = 0.0f;

        VkSampler sampler;
        if (vkCreateSampler(rhi.getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        {
            ERROR("Failed to create texture sampler.");
            return VK_NULL_HANDLE;
        }

        return sampler;
    }

} // namespace Nano
