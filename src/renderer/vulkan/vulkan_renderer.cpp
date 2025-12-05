#include "vulkan_renderer.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include "window/window.h"

namespace Nano
{
    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : Renderer(window)
    {
        // Initialize instance
        m_instance.init("Nano", VK_MAKE_VERSION(1, 0, 0), true);

        // Create surface
        m_surface = m_instance.createSurface(window->getGLFWWindow());

        // Initialize device
        m_device.init(m_instance.getHandle(), m_surface);

        // Initialize swapchain
        auto swapchain_support = m_device.getSwapchainSupport(m_surface);
        m_swapchain.init(m_device.getDevice(),
                         m_device.getPhysicalDevice(),
                         m_surface,
                         window->getGLFWWindow(),
                         m_device.getQueueFamilyIndices(),
                         swapchain_support);

        // Initialize command pool
        m_command_pool.init(m_device.getDevice(), m_device.getQueueFamilyIndices().graphics_family.value());

        // Create sync objects
        createSyncObjects();
    }

    VulkanRenderer::~VulkanRenderer() noexcept
    {
        if (m_device.getDevice() != VK_NULL_HANDLE)
        {
            waitIdle();
            cleanupSyncObjects();
            m_command_pool.clean(m_device.getDevice());
            m_swapchain.clean(m_device.getDevice());
        }
        m_device.clean();

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance.getHandle(), m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }

        m_instance.clean();
    }

    void VulkanRenderer::beginFrame()
    {
        if (m_frame_in_flight)
        {
            return; // Frame already in flight
        }

        vkWaitForFences(m_device.getDevice(), 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

        uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(m_device.getDevice(),
                                                m_swapchain.getSwapchain(),
                                                UINT64_MAX,
                                                m_image_available_semaphores[m_current_frame],
                                                VK_NULL_HANDLE,
                                                &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapchain();
            m_frame_in_flight = false;
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        if (m_images_in_flight[image_index] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_device.getDevice(), 1, &m_images_in_flight[image_index], VK_TRUE, UINT64_MAX);
        }
        m_images_in_flight[image_index] = m_in_flight_fences[m_current_frame];

        m_frame_in_flight = true;
    }

    void VulkanRenderer::endFrame()
    {
        if (!m_frame_in_flight)
        {
            return;
        }

        // This should be called after recording command buffers
        // For now, we just mark frame as complete
        m_frame_in_flight = false;
        m_current_frame   = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::createSyncObjects()
    {
        m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
        m_images_in_flight.resize(m_swapchain.getImageCount(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphore_info {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_device.getDevice(), &semaphore_info, nullptr, &m_image_available_semaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateSemaphore(m_device.getDevice(), &semaphore_info, nullptr, &m_render_finished_semaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateFence(m_device.getDevice(), &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }
    }

    void VulkanRenderer::cleanupSyncObjects()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(m_device.getDevice(), m_image_available_semaphores[i], nullptr);
            vkDestroySemaphore(m_device.getDevice(), m_render_finished_semaphores[i], nullptr);
            vkDestroyFence(m_device.getDevice(), m_in_flight_fences[i], nullptr);
        }
        m_image_available_semaphores.clear();
        m_render_finished_semaphores.clear();
        m_in_flight_fences.clear();
        m_images_in_flight.clear();
    }

    void VulkanRenderer::recreateSwapchain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window->getGLFWWindow(), &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(m_window->getGLFWWindow(), &width, &height);
            glfwWaitEvents();
        }

        if (m_device.getDevice() != VK_NULL_HANDLE)
        {
            waitIdle();
        }

        auto swapchain_support = m_device.getSwapchainSupport(m_surface);
        m_swapchain.recreate(m_device.getDevice(),
                             m_device.getPhysicalDevice(),
                             m_surface,
                             m_window->getGLFWWindow(),
                             m_device.getQueueFamilyIndices(),
                             swapchain_support);

        m_images_in_flight.resize(m_swapchain.getImageCount(), VK_NULL_HANDLE);
    }

} // namespace Nano
