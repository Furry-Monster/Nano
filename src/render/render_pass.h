#pragma once

#include "vulkan_rhi.h"

#include <string>
#include <vector>

enum class RenderPassType { Compute, Graphics };

class RenderPass {
public:
    RenderPassType mType;
    std::string mName;

    union {
        struct {
            VkShaderModule mVertexShader;
            VkShaderModule mFragmentShader;
        };
        VkShaderModule mComputeShader;
    };

    VkPipeline mPSO = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkFramebuffer mFrameBuffer = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;
    std::vector<VkWriteDescriptorSet> mWriteDescriptorSets;
    std::vector<Texture2D*> mTextures;
    std::vector<Texture2D*> mOutputTextures;
    std::vector<VulkanBuffer*> mBuffers;
    std::vector<VulkanBuffer*> mOutputBuffers;
    std::vector<VulkanBuffer*> mUniformBuffers;

    int mDispatchX = 1, mDispatchY = 1, mDispatchZ = 1;
    uint32_t mViewportWidth = 0, mViewportHeight = 0;

    RenderPass(RenderPassType type, const char* name)
        : mType(type), mName(name) {}

    void SetVSPS(const char* vsPath, const char* fsPath);
    void SetCS(const char* csPath);
    void SetUniformBufferObject(int binding, VulkanBuffer* ubo);
    void SetSSBO(int binding, VulkanBuffer* buffer, bool isOutput = false);
    void SetComputeImage(int binding, Texture2D* image, bool isOutput = false);
    void SetComputeDispatchArgs(int x, int y, int z);
    void Build(uint32_t canvasWidth = 0, uint32_t canvasHeight = 0);
    void Execute();
    void ExecuteIndirect(VulkanBuffer* indirectBuffer);
};
