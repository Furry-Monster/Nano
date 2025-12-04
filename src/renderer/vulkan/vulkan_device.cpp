#include "vulkan_device.h"
#include <set>
#include <stdexcept>

namespace Nano
{

    VulkanDevice::~VulkanDevice() noexcept { clean(); }

    VulkanDevice::VulkanDevice(VulkanDevice&& other) noexcept :
        m_instance(other.m_instance), m_physical_device(other.m_physical_device), m_device(other.m_device),
        m_graphics_queue(other.m_graphics_queue), m_present_queue(other.m_present_queue),
        m_queue_family_indices(other.m_queue_family_indices)
    {
        other.m_instance        = VK_NULL_HANDLE;
        other.m_physical_device = VK_NULL_HANDLE;
        other.m_device          = VK_NULL_HANDLE;
        other.m_graphics_queue  = VK_NULL_HANDLE;
        other.m_present_queue   = VK_NULL_HANDLE;
    }

    VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other) noexcept
    {
        if (this != &other)
        {
            clean();
            m_instance             = other.m_instance;
            m_physical_device      = other.m_physical_device;
            m_device               = other.m_device;
            m_graphics_queue       = other.m_graphics_queue;
            m_present_queue        = other.m_present_queue;
            m_queue_family_indices = other.m_queue_family_indices;

            other.m_instance        = VK_NULL_HANDLE;
            other.m_physical_device = VK_NULL_HANDLE;
            other.m_device          = VK_NULL_HANDLE;
            other.m_graphics_queue  = VK_NULL_HANDLE;
            other.m_present_queue   = VK_NULL_HANDLE;
        }
        return *this;
    }

    void VulkanDevice::init(VkInstance instance, VkSurfaceKHR surface)
    {
        m_instance = instance;
        selectPhysicalDevice(instance, surface);

        QueueFamilyIndices indices = findQueueFamilies(m_physical_device, surface);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info {};
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount       = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features {};
        device_features.samplerAnisotropy         = VK_TRUE;
        device_features.geometryShader            = VK_TRUE;
        device_features.multiDrawIndirect         = VK_TRUE;
        device_features.drawIndirectFirstInstance = VK_TRUE;

        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_feature {};
        dynamic_rendering_feature.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_feature.dynamicRendering = VK_TRUE;

        VkDeviceCreateInfo create_info {};
        create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos       = queue_create_infos.data();
        create_info.pEnabledFeatures        = &device_features;
        create_info.pNext                   = &dynamic_rendering_feature;
        create_info.enabledExtensionCount   = static_cast<uint32_t>(m_device_extensions.size());
        create_info.ppEnabledExtensionNames = m_device_extensions.data();

        if (vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(m_device, indices.graphics_family.value(), 0, &m_graphics_queue);
        vkGetDeviceQueue(m_device, indices.present_family.value(), 0, &m_present_queue);

        m_queue_family_indices = indices;
    }

    void VulkanDevice::clean()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
    }

    SwapchainSupportDetails VulkanDevice::getSwapchainSupport(VkSurfaceKHR surface) const
    {
        return querySwapchainSupport(m_physical_device, surface);
    }

    uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    VkFormat VulkanDevice::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                               VkImageTiling                tiling,
                                               VkFormatFeatureFlags         features) const
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format!");
    }

    VkFormat VulkanDevice::findDepthFormat() const
    {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void VulkanDevice::selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

        if (device_count == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device, surface))
            {
                m_physical_device = device;
                break;
            }
        }

        if (m_physical_device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensions_supported = checkDeviceExtensionSupport(device);

        bool swapchain_adequate = false;
        if (extensions_supported)
        {
            SwapchainSupportDetails swapchain_support = querySwapchainSupport(device, surface);
            swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
        }

        VkPhysicalDeviceFeatures supported_features;
        vkGetPhysicalDeviceFeatures(device, &supported_features);

        return indices.isComplete() && extensions_supported && swapchain_adequate &&
               supported_features.samplerAnisotropy;
    }

    QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

            if (present_support)
            {
                indices.present_family = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions(m_device_extensions.begin(), m_device_extensions.end());

        for (const auto& extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    SwapchainSupportDetails VulkanDevice::querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

        if (format_count != 0)
        {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0)
        {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &present_mode_count, details.present_modes.data());
        }

        return details;
    }

} // namespace Nano
