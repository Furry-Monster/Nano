#include "texture.h"
#include <cstring>
#include "buffer.h"
#include "command_buffer.h"
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
        if (m_image == VK_NULL_HANDLE)
        {
            ERROR("Texture image not created. Call create() first.");
            return false;
        }

        if (width != m_width || height != m_height)
        {
            ERROR("Upload dimensions (%u x %u) do not match texture dimensions (%u x %u).",
                  width,
                  height,
                  m_width,
                  m_height);
            return false;
        }

        return uploadDataToImage(data, data_size, width, height);
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

        size_t image_size = static_cast<size_t>(m_width) * m_height * 4; // RGBA
        if (!uploadDataToImage(pixels, image_size, m_width, m_height))
        {
            stbi_image_free(pixels);
            return false;
        }

        stbi_image_free(pixels);
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

    bool Texture::transitionImageLayout(VkCommandBuffer      command_buffer,
                                        VkImageLayout        old_layout,
                                        VkImageLayout        new_layout,
                                        VkAccessFlags        src_access_mask,
                                        VkAccessFlags        dst_access_mask,
                                        VkPipelineStageFlags src_stage,
                                        VkPipelineStageFlags dst_stage)
    {
        VkImageMemoryBarrier barrier            = {};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = old_layout;
        barrier.newLayout                       = new_layout;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = m_image;
        barrier.srcAccessMask                   = src_access_mask;
        barrier.dstAccessMask                   = dst_access_mask;
        barrier.subresourceRange.aspectMask     = m_image_aspect_flags;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        return true;
    }

    bool Texture::copyBufferToImage(VkCommandBuffer command_buffer, VkBuffer buffer, uint32_t width, uint32_t height)
    {
        VkBufferImageCopy region               = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = m_image_aspect_flags;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {width, height, 1};

        vkCmdCopyBufferToImage(command_buffer, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        return true;
    }

    bool Texture::uploadDataToImage(const void* data, size_t data_size, uint32_t width, uint32_t height)
    {
        RHI& rhi = RHI::instance();

        Buffer staging_buffer;
        if (!staging_buffer.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                   data_size,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            ERROR("Failed to create staging buffer for texture upload.");
            return false;
        }

        if (!staging_buffer.uploadData(data, data_size))
        {
            ERROR("Failed to upload data to staging buffer.");
            return false;
        }

        CommandBuffer cmd;
        if (!cmd.create())
        {
            ERROR("Failed to create command buffer for texture upload.");
            return false;
        }

        if (!cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
            ERROR("Failed to begin command buffer.");
            return false;
        }

        transitionImageLayout(cmd.getCommandBuffer(),
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              0,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT);

        copyBufferToImage(cmd.getCommandBuffer(), staging_buffer.getBuffer(), width, height);

        VkImageLayout        final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAccessFlags        final_access = VK_ACCESS_SHADER_READ_BIT;
        VkPipelineStageFlags final_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if (m_image_aspect_flags & VK_IMAGE_ASPECT_DEPTH_BIT)
        {
            final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            final_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            final_stage  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }

        transitionImageLayout(cmd.getCommandBuffer(),
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              final_layout,
                              VK_ACCESS_TRANSFER_WRITE_BIT,
                              final_access,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              final_stage);

        if (!cmd.end())
        {
            ERROR("Failed to end command buffer.");
            return false;
        }

        VkFence           fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(rhi.getDevice(), &fence_info, nullptr, &fence) != VK_SUCCESS)
        {
            ERROR("Failed to create fence for texture upload.");
            return false;
        }

        vkResetFences(rhi.getDevice(), 1, &fence);

        if (!cmd.submit(rhi.getGraphicsQueue(),
                        VK_NULL_HANDLE,
                        VK_NULL_HANDLE,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        fence))
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to submit command buffer.");
            return false;
        }

        if (vkWaitForFences(rhi.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        {
            vkDestroyFence(rhi.getDevice(), fence, nullptr);
            ERROR("Failed to wait for texture upload to complete.");
            return false;
        }

        vkDestroyFence(rhi.getDevice(), fence, nullptr);
        return true;
    }

} // namespace Nano
