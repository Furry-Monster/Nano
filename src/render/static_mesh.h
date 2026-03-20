#pragma once

#include "../math/float4.h"
#include "material.h"
#include "vulkan_rhi.h"

#include <vector>

struct StaticMeshVertex {
    float4 position;
    float4 texcoord;
    float4 normal;
    float4 tangent;
};

class StaticMesh {
public:
    static std::vector<VkVertexInputBindingDescription> sVertexBindings;
    static std::vector<VkVertexInputAttributeDescription> sVertexAttributes;
    static void InitVertexLayout();

    Material mMaterial;
    StaticMeshVertex* mVertexData = nullptr;
    int mVertexCount = 0;
    VulkanBuffer* mVBO = nullptr;

    void SetVertexCount(int count);
    void SetPosition(int index, float x, float y, float z, float w = 1.0f);
    void SetTexcoord(int index, float x, float y, float z = 0.0f, float w = 0.0f);
    void Draw(VkCommandBuffer cb);
};
