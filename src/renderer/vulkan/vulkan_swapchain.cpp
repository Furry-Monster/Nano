#include "vulkan_swapchain.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Nano
{
    VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept :
        m_swapchain(other.m_swapchain), m_images(std::move(other.m_images)),
        m_image_views(std::move(other.m_image_views)), m_image_format(other.m_image_format), m_extent(other.m_extent)
    {
        other.m_swapchain = VK_NULL_HANDLE;
    }

    VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept
    {
        if (this != &other)
        {
            m_swapchain    = other.m_swapchain;
            m_images       = std::move(other.m_images);
            m_image_views  = std::move(other.m_image_views);
            m_image_format = other.m_image_format;
            m_extent       = other.m_extent;

            other.m_swapchain = VK_NULL_HANDLE;
        }
        return *this;
    }

    void VulkanSwapchain::init(VkDevice                       device,
                               VkPhysicalDevice               physicalDevice,
                               VkSurfaceKHR                   surface,
                               GLFWwindow*                    window,
                               const QueueFamilyIndices&      queueFamilyIndices,
                               const SwapchainSupportDetails& swapchainSupport)
    {
        createSwapchain(device, physicalDevice, surface, window, queueFamilyIndices, swapchainSupport);
        createImageViews(device);
    }

    void VulkanSwapchain::recreate(VkDevice                       device,
                                   VkPhysicalDevice               physicalDevice,
                                   VkSurfaceKHR                   surface,
                                   GLFWwindow*                    window,
                                   const QueueFamilyIndices&      queueFamilyIndices,
                                   const SwapchainSupportDetails& swapchainSupport)
    {
        cleanupSwapchain(device);
        createSwapchain(device, physicalDevice, surface, window, queueFamilyIndices, swapchainSupport);
        createImageViews(device);
    }

    void VulkanSwapchain::clean(VkDevice device) { cleanupSwapchain(device); }

    void VulkanSwapchain::createSwapchain(VkDevice device,
                                          VkPhysicalDevice /*physicalDevice*/,
                                          VkSurfaceKHR                   surface,
                                          GLFWwindow*                    window,
                                          const QueueFamilyIndices&      queueFamilyIndices,
                                          const SwapchainSupportDetails& swapchainSupport)
    {
        VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchainSupport.formats);
        VkPresentModeKHR   present_mode   = chooseSwapPresentMode(swapchainSupport.present_modes);
        VkExtent2D         extent         = chooseSwapExtent(swapchainSupport.capabilities, window);

        uint32_t image_count = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0 &&
            image_count > swapchainSupport.capabilities.maxImageCount)
        {
            image_count = swapchainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info {};
        create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface          = surface;
        create_info.minImageCount    = image_count;
        create_info.imageFormat      = surface_format.format;
        create_info.imageColorSpace  = surface_format.colorSpace;
        create_info.imageExtent      = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queue_family_indices_array[] = {queueFamilyIndices.graphics_family.value(),
                                                 queueFamilyIndices.present_family.value()};

        if (queueFamilyIndices.graphics_family != queueFamilyIndices.present_family)
        {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices   = queue_family_indices_array;
        }
        else
        {
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices   = nullptr;
        }

        create_info.preTransform   = swapchainSupport.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode    = present_mode;
        create_info.clipped        = VK_TRUE;
        create_info.oldSwapchain   = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, m_swapchain, &image_count, nullptr);
        m_images.resize(image_count);
        vkGetSwapchainImagesKHR(device, m_swapchain, &image_count, m_images.data());

        m_image_format = surface_format.format;
        m_extent       = extent;
    }

    void VulkanSwapchain::createImageViews(VkDevice device)
    {
        m_image_views.resize(m_images.size());

        for (size_t i = 0; i < m_images.size(); i++)
        {
            VkImageViewCreateInfo create_info {};
            create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image                           = m_images[i];
            create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format                          = m_image_format;
            create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &create_info, nullptr, &m_image_views[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void VulkanSwapchain::cleanupSwapchain(VkDevice device)
    {
        for (auto* image_view : m_image_views)
        {
            vkDestroyImageView(device, image_view, nullptr);
        }
        m_image_views.clear();

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
    }

    VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& available_format : availableFormats)
        {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return available_format;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& available_present_mode : availablePresentModes)
        {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return available_present_mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actual_extent.width =
                std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(
                actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }

} // namespace Nano
