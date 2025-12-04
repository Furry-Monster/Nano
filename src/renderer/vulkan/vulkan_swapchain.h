#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include "GLFW/glfw3.h"
#include "vulkan_device.h"

namespace Nano
{
    class VulkanSwapchain
    {
    public:
        VulkanSwapchain()           = default;
        ~VulkanSwapchain() noexcept = default;

        VulkanSwapchain(VulkanSwapchain&&) noexcept;
        VulkanSwapchain& operator=(VulkanSwapchain&&) noexcept;
        VulkanSwapchain(const VulkanSwapchain&)            = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

        void init(VkDevice                       device,
                  VkPhysicalDevice               physicalDevice,
                  VkSurfaceKHR                   surface,
                  GLFWwindow*                    window,
                  const QueueFamilyIndices&      queueFamilyIndices,
                  const SwapchainSupportDetails& swapchainSupport);
        void recreate(VkDevice                       device,
                      VkPhysicalDevice               physicalDevice,
                      VkSurfaceKHR                   surface,
                      GLFWwindow*                    window,
                      const QueueFamilyIndices&      queueFamilyIndices,
                      const SwapchainSupportDetails& swapchainSupport);
        void clean(VkDevice device);

        VkSwapchainKHR                  getSwapchain() const { return m_swapchain; }
        const std::vector<VkImage>&     getImages() const { return m_images; }
        const std::vector<VkImageView>& getImageViews() const { return m_image_views; }
        VkFormat                        getImageFormat() const { return m_image_format; }
        VkExtent2D                      getExtent() const { return m_extent; }
        uint32_t                        getImageCount() const { return static_cast<uint32_t>(m_images.size()); }

    private:
        void createSwapchain(VkDevice                  device,
                             VkPhysicalDevice          physicalDevice,
                             VkSurfaceKHR              surface,
                             GLFWwindow*               window,
                             const QueueFamilyIndices& queueFamilyIndices);
        void createImageViews(VkDevice device);
        void cleanupSwapchain(VkDevice device);

        VkSwapchainKHR           m_swapchain {VK_NULL_HANDLE};
        std::vector<VkImage>     m_images;
        std::vector<VkImageView> m_image_views;
        VkFormat                 m_image_format;
        VkExtent2D               m_extent;

        void               createSwapchain(VkDevice                       device,
                                           VkPhysicalDevice               physicalDevice,
                                           VkSurfaceKHR                   surface,
                                           GLFWwindow*                    window,
                                           const QueueFamilyIndices&      queueFamilyIndices,
                                           const SwapchainSupportDetails& swapchainSupport);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    };

} // namespace Nano
