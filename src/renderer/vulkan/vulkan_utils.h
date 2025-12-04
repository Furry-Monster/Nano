#pragma once

#include <vulkan/vulkan_core.h>
#include <string>
#include <vector>

namespace Nano
{
    namespace VulkanUtils
    {
        std::vector<char> readFile(const std::string& filename);
        VkShaderModule    createShaderModule(VkDevice device, const std::vector<char>& code);
        VkImageView       createImageView(VkDevice           device,
                                          VkImage            image,
                                          VkFormat           format,
                                          VkImageAspectFlags aspectFlags,
                                          uint32_t           mipLevels = 1);
        void              transitionImageLayout(VkDevice      device,
                                                VkCommandPool commandPool,
                                                VkQueue       queue,
                                                VkImage       image,
                                                VkFormat      format,
                                                VkImageLayout oldLayout,
                                                VkImageLayout newLayout,
                                                uint32_t      mipLevels = 1);
        VkCommandBuffer   beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
        void
        endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);
    } // namespace VulkanUtils

} // namespace Nano
