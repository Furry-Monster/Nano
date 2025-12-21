#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <vulkan/vulkan_core.h>
#include <vector>
#include "render/rhi/buffer.h"
#include "render/rhi/texture.h"

namespace Nano
{

    class DescriptorSetLayout
    {
    public:
        DescriptorSetLayout();
        ~DescriptorSetLayout() noexcept;

        DescriptorSetLayout(const DescriptorSetLayout&)                = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&)     = delete;
        DescriptorSetLayout(DescriptorSetLayout&&) noexcept            = delete;
        DescriptorSetLayout& operator=(DescriptorSetLayout&&) noexcept = delete;

        bool create(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        void cleanup();

        VkDescriptorSetLayout getLayout() const { return m_layout; }

    private:
        VkDescriptorSetLayout m_layout {VK_NULL_HANDLE};
    };

    class DescriptorSet
    {
    public:
        DescriptorSet();
        ~DescriptorSet() noexcept;

        DescriptorSet(const DescriptorSet&)                = delete;
        DescriptorSet& operator=(const DescriptorSet&)     = delete;
        DescriptorSet(DescriptorSet&&) noexcept            = delete;
        DescriptorSet& operator=(DescriptorSet&&) noexcept = delete;

        bool allocate(VkDescriptorSetLayout layout, const std::vector<VkDescriptorPoolSize>& pool_sizes);
        bool updateBuffer(uint32_t binding, Buffer* buffer, VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        bool updateTexture(uint32_t         binding,
                           Texture*         texture,
                           VkSampler        sampler,
                           VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        bool updateImage(uint32_t binding, VkImageView image_view, VkImageLayout image_layout, VkDescriptorType type);

        VkDescriptorSet getDescriptorSet() const { return m_descriptor_set; }

    private:
        void cleanup();

        VkDescriptorSet       m_descriptor_set {VK_NULL_HANDLE};
        VkDescriptorPool      m_descriptor_pool {VK_NULL_HANDLE};
        VkDescriptorSetLayout m_layout {VK_NULL_HANDLE};
    };

} // namespace Nano

#endif // !DESCRIPTOR_SET_H
