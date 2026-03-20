#include "material.h"

#include "static_mesh.h"

void Material::Init(const char* vsPath, const char* fsPath) {
    mVertexShader = LoadShaderModule(vsPath);
    mFragmentShader = LoadShaderModule(fsPath);

    auto* params = GetUberPassShaderParameterDescription();

    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 32;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 32;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    vkCreateDescriptorPool(GetVulkanDevice(), &poolInfo, nullptr, &mDescriptorPool);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = mDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &params->mDescriptorSetLayout;
    vkAllocateDescriptorSets(GetVulkanDevice(), &allocInfo, &mDescriptorSet);
}

void Material::SetUBO(int binding, VkBuffer ubo, int uboSize) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = ubo;
    bufferInfo.offset = 0;
    bufferInfo.range = uboSize;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.dstBinding = binding;
    write.dstSet = mDescriptorSet;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(GetVulkanDevice(), 1, &write, 0, nullptr);
}

void Material::SetTexture2D(int binding, int dstArrayIndex, VkImageView imageView,
                            VkSampler sampler) {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstArrayElement = dstArrayIndex;
    write.dstBinding = binding;
    write.dstSet = mDescriptorSet;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(GetVulkanDevice(), 1, &write, 0, nullptr);
}

void Material::Active(VkCommandBuffer cb, VkRenderPass renderPass) {
    if (mPSO == VK_NULL_HANDLE) {
        mPSO = CreatePSO(renderPass, mPrimitiveType, StaticMesh::sVertexBindings,
                          StaticMesh::sVertexAttributes, mVertexShader, mFragmentShader);
    }

    auto* params = GetUberPassShaderParameterDescription();
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, mPSO);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, params->mPipelineLayout, 0, 1,
                            &mDescriptorSet, 0, nullptr);
}
