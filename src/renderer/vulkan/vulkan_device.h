#pragma once

#include <vulkan/vulkan_core.h>
#include <optional>
#include <vector>

namespace Nano
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        bool isComplete() const { return graphics_family.has_value() && present_family.has_value(); }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   present_modes;
    };

    class VulkanDevice
    {
    public:
        VulkanDevice() = default;
        ~VulkanDevice() noexcept;

        VulkanDevice(VulkanDevice&&) noexcept;
        VulkanDevice& operator=(VulkanDevice&&) noexcept;
        VulkanDevice(const VulkanDevice&)            = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        void init(VkInstance instance, VkSurfaceKHR surface);
        void clean();

        VkPhysicalDevice        getPhysicalDevice() const { return m_physical_device; }
        VkDevice                getDevice() const { return m_device; }
        VkQueue                 getGraphicsQueue() const { return m_graphics_queue; }
        VkQueue                 getPresentQueue() const { return m_present_queue; }
        QueueFamilyIndices      getQueueFamilyIndices() const { return m_queue_family_indices; }
        SwapchainSupportDetails getSwapchainSupport(VkSurfaceKHR surface) const;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                     VkImageTiling                tiling,
                                     VkFormatFeatureFlags         features) const;
        VkFormat findDepthFormat() const;

        void waitIdle() const
        {
            if (m_device != VK_NULL_HANDLE)
                vkDeviceWaitIdle(m_device);
        }

    private:
        void                    selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        bool                    isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        bool                    checkDeviceExtensionSupport(VkPhysicalDevice device) const;
        SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;

        VkInstance                     m_instance {VK_NULL_HANDLE};
        VkPhysicalDevice               m_physical_device {VK_NULL_HANDLE};
        VkDevice                       m_device {VK_NULL_HANDLE};
        VkQueue                        m_graphics_queue {VK_NULL_HANDLE};
        VkQueue                        m_present_queue {VK_NULL_HANDLE};
        QueueFamilyIndices             m_queue_family_indices;
        const std::vector<const char*> m_device_extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};
    };

} // namespace Nano
