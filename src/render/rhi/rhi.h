#ifndef RHI_H
#define RHI_H

#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <vector>

namespace Nano
{
    class RHI final
    {
    public:
        static RHI& instance()
        {
            static RHI s_rhi;
            return s_rhi;
        }

        VkDevice         getDevice() const { return m_device; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physical_device; }
        VkQueue          getGraphicsQueue() const { return m_graphic_queue; }
        VkQueue          getPresentQueue() const { return m_present_queue; }
        VkSurfaceKHR     getSurface() const { return m_surface; }
        uint32_t         getGraphicsQueueFamilyIndex() const { return m_graphic_queue_family_index; }
        uint32_t         getPresentQueueFamilyIndex() const { return m_present_queue_family_index; }

        const VkSurfaceCapabilitiesKHR& getSurfaceCapabilities() const { return m_surface_capabilities; }
        uint32_t                        getSurfaceFormatCount() const { return m_surface_format_cnt; }
        const VkSurfaceFormatKHR*       getSurfaceFormats() const { return m_surface_formats; }
        uint32_t                        getSurfacePresentModeCount() const { return m_surface_present_mode_cnt; }
        const VkPresentModeKHR*         getSurfacePresentModes() const { return m_surface_present_modes; }

        bool
        findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags property_flags, uint32_t& memory_type_index) const;

    protected:
        RHI();
        ~RHI() noexcept;

        RHI(const RHI&)            = delete;
        RHI& operator=(const RHI&) = delete;
        RHI(RHI&&)                 = delete;
        RHI& operator=(RHI&&)      = delete;

    private:
        bool initInstance();
        bool initDebugger();
        bool initSurface();
        bool initPhysicalDevice();
        bool initLogicalDevice();
        bool initSurfaceProperties();

        bool isDeviceExtensionSupported(const char* extension_name) const;

        VkInstance               m_instance {VK_NULL_HANDLE};
        std::vector<const char*> m_additional_instance_exts;
        uint32_t                 m_prefered_layer_cnt {0};
        char**                   m_prefered_layers {nullptr};

        VkSurfaceKHR             m_surface {VK_NULL_HANDLE};
        VkSurfaceCapabilitiesKHR m_surface_capabilities {};
        uint32_t                 m_surface_format_cnt {0};
        VkSurfaceFormatKHR*      m_surface_formats {VK_NULL_HANDLE};
        uint32_t                 m_surface_present_mode_cnt {0};
        VkPresentModeKHR*        m_surface_present_modes {VK_NULL_HANDLE};

        VkDebugReportCallbackEXT            m_debug_report_callback {VK_NULL_HANDLE};
        PFN_vkCreateDebugReportCallbackEXT  m_vkCreateDebugReportCallbackEXT {VK_NULL_HANDLE};
        PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT {VK_NULL_HANDLE};

        VkDevice                           m_device {VK_NULL_HANDLE};
        VkPhysicalDevice                   m_physical_device {VK_NULL_HANDLE};
        VkPhysicalDeviceMemoryProperties   m_memory_properties {};
        std::vector<VkExtensionProperties> m_device_extensions;
        uint32_t                           m_graphic_queue_family_index {0};
        uint32_t                           m_present_queue_family_index {0};
        VkQueue                            m_graphic_queue {VK_NULL_HANDLE};
        VkQueue                            m_present_queue {VK_NULL_HANDLE};
    };

} // namespace Nano

#endif // !RHI_H