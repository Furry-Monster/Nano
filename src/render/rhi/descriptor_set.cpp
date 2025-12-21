#include "descriptor_set.h"
#include "buffer.h"
#include "misc/logger.h"
#include "rhi.h"
#include "texture.h"

namespace Nano
{
    DescriptorSetLayout::DescriptorSetLayout() {}

    DescriptorSetLayout::~DescriptorSetLayout() noexcept { cleanup(); }

    void DescriptorSetLayout::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(rhi.getDevice(), m_layout, nullptr);
            m_layout = VK_NULL_HANDLE;
            DEBUG("  Destroyed descriptor set layout");
        }
    }

    bool DescriptorSetLayout::create(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        RHI& rhi = RHI::instance();

        if (bindings.empty())
        {
            ERROR("Cannot create descriptor set layout with empty bindings.");
            return false;
        }

        VkDescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount                    = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings                       = bindings.data();

        if (vkCreateDescriptorSetLayout(rhi.getDevice(), &layout_info, nullptr, &m_layout) != VK_SUCCESS)
        {
            ERROR("Failed to create descriptor set layout.");
            return false;
        }

        return true;
    }

    DescriptorSet::DescriptorSet() {}

    DescriptorSet::~DescriptorSet() noexcept { cleanup(); }

    void DescriptorSet::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_descriptor_set != VK_NULL_HANDLE && m_descriptor_pool != VK_NULL_HANDLE)
        {
            vkFreeDescriptorSets(rhi.getDevice(), m_descriptor_pool, 1, &m_descriptor_set);
            m_descriptor_set = VK_NULL_HANDLE;
            DEBUG("  Freed descriptor set");
        }

        if (m_descriptor_pool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(rhi.getDevice(), m_descriptor_pool, nullptr);
            m_descriptor_pool = VK_NULL_HANDLE;
            DEBUG("  Destroyed descriptor pool");
        }

        m_layout = VK_NULL_HANDLE;
    }

    bool DescriptorSet::allocate(VkDescriptorSetLayout layout, const std::vector<VkDescriptorPoolSize>& pool_sizes)
    {
        RHI& rhi = RHI::instance();

        if (layout == VK_NULL_HANDLE)
        {
            ERROR("Cannot allocate descriptor set with null layout.");
            return false;
        }

        if (pool_sizes.empty())
        {
            ERROR("Cannot allocate descriptor set with empty pool sizes.");
            return false;
        }

        m_layout = layout;

        // Create descriptor pool
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount              = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes                 = pool_sizes.data();
        pool_info.maxSets                    = 1;
        pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(rhi.getDevice(), &pool_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
        {
            ERROR("Failed to create descriptor pool.");
            return false;
        }

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool              = m_descriptor_pool;
        alloc_info.descriptorSetCount          = 1;
        alloc_info.pSetLayouts                 = &layout;

        if (vkAllocateDescriptorSets(rhi.getDevice(), &alloc_info, &m_descriptor_set) != VK_SUCCESS)
        {
            ERROR("Failed to allocate descriptor set.");
            return false;
        }

        return true;
    }

    bool DescriptorSet::updateBuffer(uint32_t binding, Buffer* buffer, VkDescriptorType type)
    {
        if (m_descriptor_set == VK_NULL_HANDLE)
        {
            ERROR("Descriptor set not allocated. Call allocate() first.");
            return false;
        }

        if (buffer == nullptr)
        {
            ERROR("Cannot update descriptor set with null buffer.");
            return false;
        }

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer                 = buffer->getBuffer();
        buffer_info.offset                 = 0;
        buffer_info.range                  = buffer->getSize();

        VkWriteDescriptorSet descriptor_write = {};
        descriptor_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet               = m_descriptor_set;
        descriptor_write.dstBinding           = binding;
        descriptor_write.dstArrayElement      = 0;
        descriptor_write.descriptorType       = type;
        descriptor_write.descriptorCount      = 1;
        descriptor_write.pBufferInfo          = &buffer_info;

        vkUpdateDescriptorSets(RHI::instance().getDevice(), 1, &descriptor_write, 0, nullptr);

        return true;
    }

    bool DescriptorSet::updateTexture(uint32_t binding, Texture* texture, VkSampler sampler, VkDescriptorType type)
    {
        if (m_descriptor_set == VK_NULL_HANDLE)
        {
            ERROR("Descriptor set not allocated. Call allocate() first.");
            return false;
        }

        if (texture == nullptr)
        {
            ERROR("Cannot update descriptor set with null texture.");
            return false;
        }

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView             = texture->getImageView();
        image_info.sampler               = sampler;

        VkWriteDescriptorSet descriptor_write = {};
        descriptor_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet               = m_descriptor_set;
        descriptor_write.dstBinding           = binding;
        descriptor_write.dstArrayElement      = 0;
        descriptor_write.descriptorType       = type;
        descriptor_write.descriptorCount      = 1;
        descriptor_write.pImageInfo           = &image_info;

        vkUpdateDescriptorSets(RHI::instance().getDevice(), 1, &descriptor_write, 0, nullptr);

        return true;
    }

    bool DescriptorSet::updateImage(uint32_t         binding,
                                    VkImageView      image_view,
                                    VkImageLayout    image_layout,
                                    VkDescriptorType type)
    {
        if (m_descriptor_set == VK_NULL_HANDLE)
        {
            ERROR("Descriptor set not allocated. Call allocate() first.");
            return false;
        }

        if (image_view == VK_NULL_HANDLE)
        {
            ERROR("Cannot update descriptor set with null image view.");
            return false;
        }

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout           = image_layout;
        image_info.imageView             = image_view;
        image_info.sampler               = VK_NULL_HANDLE;

        VkWriteDescriptorSet descriptor_write = {};
        descriptor_write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet               = m_descriptor_set;
        descriptor_write.dstBinding           = binding;
        descriptor_write.dstArrayElement      = 0;
        descriptor_write.descriptorType       = type;
        descriptor_write.descriptorCount      = 1;
        descriptor_write.pImageInfo           = &image_info;

        vkUpdateDescriptorSets(RHI::instance().getDevice(), 1, &descriptor_write, 0, nullptr);

        return true;
    }

} // namespace Nano
