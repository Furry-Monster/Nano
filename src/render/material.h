#pragma once

#include "vulkan_rhi.h"

class Material {
public:
  VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
  VkPipeline mPSO = VK_NULL_HANDLE;
  VkShaderModule mVertexShader = VK_NULL_HANDLE;
  VkShaderModule mFragmentShader = VK_NULL_HANDLE;
  VkPrimitiveTopology mPrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  Material() = default;

  void Init(const char *vsPath, const char *fsPath);
  void SetUBO(int binding, VkBuffer ubo, int uboSize);
  void SetTexture2D(int binding, int dstArrayIndex, VkImageView imageView,
                    VkSampler sampler);
  void Active(VkCommandBuffer cb, VkRenderPass renderPass);
};
