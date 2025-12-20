#ifndef RHI_H
#define RHI_H

#include <vulkan/vulkan_core.h>
#include <cstdint>

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

        VkDevice getDevice() const;
        VkQueue  getQueue() const;

        void createBuffer();
        void createTexture();

        VkInstance   m_instance {VK_NULL_HANDLE};
        uint32_t     m_enabled_instance_ext_cnt {0};
        const char** m_enabled_instance_exts {nullptr};
        uint32_t     m_prefered_layer_cnt {0};
        char**       m_prefered_layers {nullptr};
        VkSurfaceKHR m_surface {VK_NULL_HANDLE};

        VkDebugReportCallbackEXT            m_debug_report_callback {VK_NULL_HANDLE};
        PFN_vkCreateDebugReportCallbackEXT  m_vkCreateDebugReportCallbackEXT {VK_NULL_HANDLE};
        PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT {VK_NULL_HANDLE};

        VkDevice         m_device {VK_NULL_HANDLE};
        VkPhysicalDevice m_physical_device {VK_NULL_HANDLE};
        uint32_t         m_enabled_device_ext_cnt {0};
        const char**     m_enabled_device_exts {VK_NULL_HANDLE};
        uint32_t         m_graphic_queue_family_index {0};
        uint32_t         m_present_queue_family_index {0};
        VkQueue          m_graphic_queue {VK_NULL_HANDLE};
        VkQueue          m_present_queue {VK_NULL_HANDLE};

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
    };

} // namespace Nano

#endif // !RHI_H