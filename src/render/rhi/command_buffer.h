#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan_core.h>

namespace Nano
{
    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer() noexcept;

        CommandBuffer(const CommandBuffer&)                = delete;
        CommandBuffer& operator=(const CommandBuffer&)     = delete;
        CommandBuffer(CommandBuffer&&) noexcept            = delete;
        CommandBuffer& operator=(CommandBuffer&&) noexcept = delete;

        bool create(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        bool begin(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        bool end();

        bool submit(VkQueue              queue,
                    VkSemaphore          wait_semaphore   = VK_NULL_HANDLE,
                    VkSemaphore          signal_semaphore = VK_NULL_HANDLE,
                    VkPipelineStageFlags wait_stage       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VkFence              fence            = VK_NULL_HANDLE);
        bool reset(VkCommandBufferResetFlags flags = 0);

        VkCommandBuffer getCommandBuffer() const { return m_command_buffer; }
        bool            isRecording() const { return m_is_recording; }

    private:
        bool createCommandPool();
        void cleanup();

        VkCommandBuffer m_command_buffer {VK_NULL_HANDLE};
        VkCommandPool   m_command_pool {VK_NULL_HANDLE};
        bool            m_is_recording {false};
    };

} // namespace Nano

#endif // !COMMAND_BUFFER_H
