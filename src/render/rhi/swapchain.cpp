#include "swapchain.h"
#include <algorithm>
#include "misc/logger.h"
#include "rhi.h"

namespace Nano
{
    Swapchain::Swapchain() {}

    Swapchain::~Swapchain() noexcept { cleanup(); }

    void Swapchain::cleanup()
    {
        RHI& rhi = RHI::instance();

        if (m_framebuffers != nullptr)
        {
            for (uint32_t i = 0; i < m_image_cnt; ++i)
            {
                if (m_framebuffers[i] != VK_NULL_HANDLE)
                    vkDestroyFramebuffer(rhi.getDevice(), m_framebuffers[i], nullptr);
            }
            delete[] m_framebuffers;
            m_framebuffers = nullptr;

            DEBUG("  Destroyed %u framebuffers", m_image_cnt);
        }

        if (m_image_views != nullptr)
        {
            for (uint32_t i = 0; i < m_image_cnt; ++i)
            {
                if (m_image_views[i] != VK_NULL_HANDLE)
                    vkDestroyImageView(rhi.getDevice(), m_image_views[i], nullptr);
            }
            delete[] m_image_views;
            m_image_views = nullptr;

            DEBUG("  Destroyed %u image views", m_image_cnt);
        }

        if (m_images != nullptr)
        {
            delete[] m_images;
            m_images = nullptr;

            DEBUG("  Released %u swapchain images", m_image_cnt);
            m_image_cnt = 0;
        }

        if (m_render_pass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(rhi.getDevice(), m_render_pass, nullptr);
            m_render_pass = VK_NULL_HANDLE;

            DEBUG("  Destroyed render pass");
        }

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(rhi.getDevice(), m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;

            DEBUG("  Destroyed swapchain");
        }
    }

    bool Swapchain::create(uint32_t width, uint32_t height)
    {
        RHI& rhi = RHI::instance();

        VkSwapchainCreateInfoKHR swapchain_create_info = {};
        if (!initSwapchainProps(width, height, swapchain_create_info))
        {
            ERROR("Failed to create swapchain.");
            return false;
        }

        if (vkCreateSwapchainKHR(rhi.getDevice(), &swapchain_create_info, nullptr, &m_swapchain) != VK_SUCCESS)
        {
            cleanup();
            ERROR("Failed to create swapchain.");
            return false;
        }

        if (!createImages())
        {
            cleanup();
            ERROR("Failed to create swapchain.");
            return false;
        }

        if (!createRenderPass())
        {
            cleanup();
            ERROR("Failed to create swapchain.");
            return false;
        }

        if (!createFramebuffers())
        {
            cleanup();
            ERROR("Failed to create swapchain.");
            return false;
        }

        return true;
    }

    bool Swapchain::initSwapchainProps(uint32_t width, uint32_t height, VkSwapchainCreateInfoKHR& swapchain_create_info)
    {
        RHI& rhi = RHI::instance();

        uint32_t selected_surface_format_index = 0;
        bool     format_found                  = false;
        for (uint32_t i = 0; i < rhi.getSurfaceFormatCount(); ++i)
        {
            const VkSurfaceFormatKHR& format = rhi.getSurfaceFormats()[i];
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            {
                selected_surface_format_index = i;
                format_found                  = true;
                break;
            }
        }

        if (!format_found && rhi.getSurfaceFormatCount() > 0)
        {
            WARN("No suitable surface format found, use default.");
            selected_surface_format_index = 0;
        }
        else if (!format_found)
        {
            ERROR("No suitable surface format found.");
            return false;
        }

        m_format = rhi.getSurfaceFormats()[selected_surface_format_index].format;

        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (uint32_t i = 0; i < rhi.getSurfacePresentModeCount(); ++i)
        {
            if (rhi.getSurfacePresentModes()[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                // if support, use mailbox for v-sync.
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        const VkSurfaceCapabilitiesKHR& capabilities = rhi.getSurfaceCapabilities();
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            m_extent = capabilities.currentExtent;
        }
        else
        {
            m_extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            m_extent.height =
                std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
        {
            image_count = capabilities.maxImageCount;
        }

        swapchain_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface          = rhi.getSurface();
        swapchain_create_info.minImageCount    = image_count;
        swapchain_create_info.imageFormat      = m_format;
        swapchain_create_info.imageColorSpace  = rhi.getSurfaceFormats()[selected_surface_format_index].colorSpace;
        swapchain_create_info.imageExtent      = m_extent;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.preTransform     = capabilities.currentTransform;
        swapchain_create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode      = present_mode;
        swapchain_create_info.clipped          = VK_TRUE;
        swapchain_create_info.oldSwapchain     = VK_NULL_HANDLE;

        uint32_t queue_family_indices[2] = {rhi.getGraphicsQueueFamilyIndex(), rhi.getPresentQueueFamilyIndex()};
        if (rhi.getGraphicsQueueFamilyIndex() == rhi.getPresentQueueFamilyIndex())
        {
            swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            swapchain_create_info.queueFamilyIndexCount = 0;
            swapchain_create_info.pQueueFamilyIndices   = nullptr;
        }
        else
        {
            swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            swapchain_create_info.queueFamilyIndexCount = 2;
            swapchain_create_info.pQueueFamilyIndices   = queue_family_indices;
        }

        return true;
    }

    bool Swapchain::createImages()
    {
        RHI& rhi = RHI::instance();

        vkGetSwapchainImagesKHR(rhi.getDevice(), m_swapchain, &m_image_cnt, nullptr);
        m_images = new VkImage[m_image_cnt];
        vkGetSwapchainImagesKHR(rhi.getDevice(), m_swapchain, &m_image_cnt, m_images);

        m_image_views = new VkImageView[m_image_cnt];
        for (uint32_t i = 0; i < m_image_cnt; ++i)
        {
            VkImageViewCreateInfo view_create_info           = {};
            view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_create_info.image                           = m_images[i];
            view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            view_create_info.format                          = m_format;
            view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            view_create_info.subresourceRange.baseMipLevel   = 0;
            view_create_info.subresourceRange.levelCount     = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(rhi.getDevice(), &view_create_info, nullptr, &m_image_views[i]) != VK_SUCCESS)
            {
                ERROR("Failed to create swapchain image view %u.", i);
                return false;
            }
        }

        return true;
    }

    bool Swapchain::createRenderPass()
    {
        RHI& rhi = RHI::instance();

        VkAttachmentDescription color_attachment = {};
        color_attachment.format                  = m_format;
        color_attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment            = 0;
        color_attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &color_attachment_ref;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass          = 0;
        dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask       = 0;
        dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount        = 1;
        render_pass_info.pAttachments           = &color_attachment;
        render_pass_info.subpassCount           = 1;
        render_pass_info.pSubpasses             = &subpass;
        render_pass_info.dependencyCount        = 1;
        render_pass_info.pDependencies          = &dependency;

        if (vkCreateRenderPass(rhi.getDevice(), &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
        {
            ERROR("Failed to create render pass.");
            return false;
        }

        return true;
    }

    bool Swapchain::createFramebuffers()
    {
        RHI& rhi = RHI::instance();

        m_framebuffers = new VkFramebuffer[m_image_cnt];
        for (uint32_t i = 0; i < m_image_cnt; ++i)
        {
            VkFramebufferCreateInfo framebuffer_info = {};
            framebuffer_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass              = m_render_pass;
            framebuffer_info.attachmentCount         = 1;
            framebuffer_info.pAttachments            = &m_image_views[i];
            framebuffer_info.width                   = m_extent.width;
            framebuffer_info.height                  = m_extent.height;
            framebuffer_info.layers                  = 1;

            if (vkCreateFramebuffer(rhi.getDevice(), &framebuffer_info, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
            {
                ERROR("Failed to create framebuffer %u.", i);
                return false;
            }
        }

        return true;
    }

    uint32_t Swapchain::acquireNextImage(VkSemaphore image_available_semaphore)
    {
        RHI& rhi = RHI::instance();

        uint32_t image_index;
        VkResult result = vkAcquireNextImageKHR(
            rhi.getDevice(), m_swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            ERROR("Failed to acquire swapchain image.");
            return UINT32_MAX;
        }

        return image_index;
    }

    bool Swapchain::present(uint32_t image_index, VkSemaphore render_finished_semaphore)
    {
        RHI& rhi = RHI::instance();

        VkPresentInfoKHR present_info   = {};
        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = (render_finished_semaphore != VK_NULL_HANDLE) ? 1 : 0;
        present_info.pWaitSemaphores =
            (render_finished_semaphore != VK_NULL_HANDLE) ? &render_finished_semaphore : nullptr;
        present_info.swapchainCount = 1;
        present_info.pSwapchains    = &m_swapchain;
        present_info.pImageIndices  = &image_index;

        VkResult result = vkQueuePresentKHR(rhi.getPresentQueue(), &present_info);
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            ERROR("Failed to present swapchain image.");
            return false;
        }

        return true;
    }

    VkImage Swapchain::getImage(uint32_t index) const
    {
        if (index >= m_image_cnt)
        {
            ERROR("Index of image out of bounds");
            return VK_NULL_HANDLE;
        }
        return m_images[index];
    }

    VkImageView Swapchain::getImageView(uint32_t index) const
    {
        if (index >= m_image_cnt)
        {
            ERROR("Index of image view out of bounds");
            return VK_NULL_HANDLE;
        }
        return m_image_views[index];
    }

    VkFramebuffer Swapchain::getFramebuffer(uint32_t index) const
    {
        if (index >= m_image_cnt)
        {
            ERROR("Index of framebuffer out of bounds");
            return VK_NULL_HANDLE;
        }
        return m_framebuffers[index];
    }

} // namespace Nano
