#include "render_pass.h"

#include <spdlog/spdlog.h>

void RenderPass::SetVSPS(const char *vsPath, const char *fsPath) {
  mVertexShader = LoadShaderModule(vsPath);
  mFragmentShader = LoadShaderModule(fsPath);
}

void RenderPass::SetCS(const char *csPath) {
  mComputeShader = LoadShaderModule(csPath);
}

void RenderPass::SetComputeImage(int binding, Texture2D *image, bool isOutput) {
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = binding;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  layoutBinding.stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  mLayoutBindings.push_back(layoutBinding);

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imageInfo.imageView = image->mImageView;
  imageInfo.sampler = VK_NULL_HANDLE;
  mDescriptorImageInfos.push_back(imageInfo);

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  write.dstBinding = binding;
  write.pImageInfo = &mDescriptorImageInfos.back();
  mWriteDescriptorSets.push_back(write);

  mTextures.push_back(image);
  if (isOutput)
    mOutputTextures.push_back(image);
}

void RenderPass::SetComputeStorageImageView(int binding, VkImageView view,
                                            bool isOutput) {
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = binding;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  layoutBinding.stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  mLayoutBindings.push_back(layoutBinding);

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imageInfo.imageView = view;
  imageInfo.sampler = VK_NULL_HANDLE;
  mDescriptorImageInfos.push_back(imageInfo);

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  write.dstBinding = binding;
  write.pImageInfo = &mDescriptorImageInfos.back();
  mWriteDescriptorSets.push_back(write);

  mStandaloneStorageImageCount++;

  if (isOutput) {
    spdlog::warn(
        "SetComputeStorageImageView: isOutput layout transitions not tracked");
  }
}

void RenderPass::SetCombinedImageSampler(int binding, VkImageView imageView,
                                         VkSampler sampler) {
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = binding;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  layoutBinding.stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  mLayoutBindings.push_back(layoutBinding);

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = imageView;
  imageInfo.sampler = sampler;
  mDescriptorImageInfos.push_back(imageInfo);

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstBinding = binding;
  write.pImageInfo = &mDescriptorImageInfos.back();
  mWriteDescriptorSets.push_back(write);

  mCombinedImageSamplerCount++;
}

void RenderPass::SetSSBO(int binding, VulkanBuffer *buffer, bool isOutput) {
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = binding;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                             VK_SHADER_STAGE_COMPUTE_BIT |
                             VK_SHADER_STAGE_FRAGMENT_BIT;
  mLayoutBindings.push_back(layoutBinding);

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = buffer->mBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = buffer->mSize;
  mDescriptorBufferInfos.push_back(bufferInfo);

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.dstBinding = binding;
  write.pBufferInfo = &mDescriptorBufferInfos.back();
  mWriteDescriptorSets.push_back(write);

  mBuffers.push_back(buffer);
  if (isOutput)
    mOutputBuffers.push_back(buffer);
}

void RenderPass::SetUniformBufferObject(int binding, VulkanBuffer *ubo) {
  VkDescriptorSetLayoutBinding layoutBinding = {};
  layoutBinding.binding = binding;
  layoutBinding.descriptorCount = 1;
  layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                             VK_SHADER_STAGE_COMPUTE_BIT |
                             VK_SHADER_STAGE_FRAGMENT_BIT;
  mLayoutBindings.push_back(layoutBinding);

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = ubo->mBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = ubo->mSize;
  mDescriptorBufferInfos.push_back(bufferInfo);

  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.dstBinding = binding;
  write.pBufferInfo = &mDescriptorBufferInfos.back();
  mWriteDescriptorSets.push_back(write);

  mUniformBuffers.push_back(ubo);
}

void RenderPass::SetComputeDispatchArgs(int x, int y, int z) {
  mDispatchX = x;
  mDispatchY = y;
  mDispatchZ = z;
}

void RenderPass::Build(uint32_t canvasWidth, uint32_t canvasHeight) {
  VkDevice device = GetVulkanDevice();

  VkDescriptorSetLayout dsLayout;
  VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
  dsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dsLayoutInfo.bindingCount = static_cast<uint32_t>(mLayoutBindings.size());
  dsLayoutInfo.pBindings = mLayoutBindings.data();
  vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &dsLayout);

  VkPipelineLayoutCreateInfo plInfo = {};
  plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plInfo.setLayoutCount = 1;
  plInfo.pSetLayouts = &dsLayout;
  vkCreatePipelineLayout(device, &plInfo, nullptr, &mPipelineLayout);

  std::vector<VkDescriptorPoolSize> poolSizes;
  if (!mUniformBuffers.empty()) {
    poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                         static_cast<uint32_t>(mUniformBuffers.size())});
  }
  const uint32_t storageImageDescCount =
      static_cast<uint32_t>(mTextures.size() + mStandaloneStorageImageCount);
  if (storageImageDescCount > 0) {
    poolSizes.push_back(
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storageImageDescCount});
  }
  if (!mBuffers.empty()) {
    poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                         static_cast<uint32_t>(mBuffers.size())});
  }
  if (mCombinedImageSamplerCount > 0) {
    poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         mCombinedImageSamplerCount});
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  vkCreateDescriptorPool(device, &poolInfo, nullptr, &mDescriptorPool);

  VkDescriptorSetAllocateInfo dsAllocInfo = {};
  dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsAllocInfo.descriptorPool = mDescriptorPool;
  dsAllocInfo.descriptorSetCount = 1;
  dsAllocInfo.pSetLayouts = &dsLayout;
  vkAllocateDescriptorSets(device, &dsAllocInfo, &mDescriptorSet);

  for (auto &write : mWriteDescriptorSets) {
    write.dstSet = mDescriptorSet;
  }

  if (mType == RenderPassType::Compute) {
    VkPipelineShaderStageCreateInfo stage = {};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = mComputeShader;
    stage.pName = "main";

    VkComputePipelineCreateInfo cpInfo = {};
    cpInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cpInfo.stage = stage;
    cpInfo.layout = mPipelineLayout;
    VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
                                                &cpInfo, nullptr, &mPSO);
    if (result != VK_SUCCESS) {
      spdlog::error("Failed to create compute pipeline '{}': {}", mName,
                    static_cast<int>(result));
      mPSO = VK_NULL_HANDLE;
    }
  } else {
    mViewportWidth = canvasWidth;
    mViewportHeight = canvasHeight;

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    VkViewport viewport = {};
    viewport.width = static_cast<float>(canvasWidth);
    viewport.height = static_cast<float>(canvasHeight);
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = {canvasWidth, canvasHeight};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAttach = {};
    blendAttach.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttach;

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = mVertexShader;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = mFragmentShader;
    stages[1].pName = "main";

    // No-attachment render pass for HW rasterize (writes to SSBO only)
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkRenderPassCreateInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    vkCreateRenderPass(device, &rpInfo, nullptr, &mRenderPass);

    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = mRenderPass;
    fbInfo.width = canvasWidth;
    fbInfo.height = canvasHeight;
    fbInfo.layers = 1;
    vkCreateFramebuffer(device, &fbInfo, nullptr, &mFrameBuffer);

    VkGraphicsPipelineCreateInfo gpInfo = {};
    gpInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpInfo.renderPass = mRenderPass;
    gpInfo.stageCount = 2;
    gpInfo.pStages = stages;
    gpInfo.basePipelineIndex = -1;
    gpInfo.pVertexInputState = &vertexInput;
    gpInfo.pDynamicState = &dynamicState;
    gpInfo.pViewportState = &viewportState;
    gpInfo.pInputAssemblyState = &inputAssembly;
    gpInfo.pRasterizationState = &raster;
    gpInfo.pMultisampleState = &multisample;
    gpInfo.pColorBlendState = &colorBlend;
    gpInfo.layout = mPipelineLayout;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpInfo, nullptr,
                              &mPSO);
  }
}

void RenderPass::UpdateDescriptorSets() {
  VkDevice device = GetVulkanDevice();
  vkUpdateDescriptorSets(device,
                         static_cast<uint32_t>(mWriteDescriptorSets.size()),
                         mWriteDescriptorSets.data(), 0, nullptr);
}

void RenderPass::RecordComputeCommands(VkCommandBuffer cb) {
  if (mType != RenderPassType::Compute) {
    return;
  }

  SCOPED_EVENT(cb, mName.c_str());

  for (auto *tex : mOutputTextures) {
    VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    TransferImageLayout(cb, tex->mImage, range, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_ACCESS_NONE, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                        VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT,
                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  }

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, mPSO);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout,
                          0, 1, &mDescriptorSet, 0, nullptr);
  vkCmdDispatch(cb, mDispatchX, mDispatchY, mDispatchZ);

  for (auto *tex : mOutputTextures) {
    VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    TransferImageLayout(
        cb, tex->mImage, range, VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  }
}

void RenderPass::RecordGraphicsIndirectCommands(VkCommandBuffer cb,
                                                VulkanBuffer *indirectBuffer) {
  SCOPED_EVENT(cb, mName.c_str());

  VkClearValue clearValues[2];
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpBegin = {};
  rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpBegin.renderPass = mRenderPass;
  rpBegin.framebuffer = mFrameBuffer;
  rpBegin.clearValueCount = 2;
  rpBegin.pClearValues = clearValues;
  rpBegin.renderArea.extent = {mViewportWidth, mViewportHeight};

  vkCmdBeginRenderPass(cb, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, mPSO);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout,
                          0, 1, &mDescriptorSet, 0, nullptr);
  vkCmdDrawIndirect(cb, indirectBuffer->mBuffer, 0, 1, 16);
  vkCmdEndRenderPass(cb);
}

void RenderPass::Execute() {
  VkDevice device = GetVulkanDevice();
  VkCommandBuffer cb = CreateCommandBuffer();
  if (cb == VK_NULL_HANDLE) {
    return;
  }
  BeginCommandBuffer(cb, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  if (mType == RenderPassType::Compute) {
    UpdateDescriptorSets();
    RecordComputeCommands(cb);
  }

  vkEndCommandBuffer(cb);

  VkFence fence;
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(device, &fenceInfo, nullptr, &fence);
  vkResetFences(device, 1, &fence);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cb;
  vkQueueSubmit(GetGraphicQueue(), 1, &submitInfo, fence);

  if (vkWaitForFences(device, 1, &fence, VK_TRUE, 1000000000000ULL) !=
      VK_SUCCESS) {
    spdlog::error("RenderPass::Execute failed: {}", mName);
  }

  vkDestroyFence(device, fence, nullptr);
  vkFreeCommandBuffers(device, GetVulkanCommandPool(), 1, &cb);
}

void RenderPass::ExecuteIndirect(VulkanBuffer *indirectBuffer) {
  VkDevice device = GetVulkanDevice();
  VkCommandBuffer cb = CreateCommandBuffer();
  if (cb == VK_NULL_HANDLE) {
    return;
  }
  BeginCommandBuffer(cb, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  UpdateDescriptorSets();
  RecordGraphicsIndirectCommands(cb, indirectBuffer);

  vkEndCommandBuffer(cb);

  VkFence fence;
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(device, &fenceInfo, nullptr, &fence);
  vkResetFences(device, 1, &fence);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cb;
  vkQueueSubmit(GetGraphicQueue(), 1, &submitInfo, fence);

  if (vkWaitForFences(device, 1, &fence, VK_TRUE, 1000000000000ULL) !=
      VK_SUCCESS) {
    spdlog::error("RenderPass::ExecuteIndirect failed: {}", mName);
  }

  vkDestroyFence(device, fence, nullptr);
  vkFreeCommandBuffers(device, GetVulkanCommandPool(), 1, &cb);
}
