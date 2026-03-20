#pragma once

#include "math/matrix4.h"
#include "render/static_mesh.h"

class SceneNode {
public:
  float4 mPosition;
  float4 mRotation;
  float4 mScale;
  bool mNeedUpdate = true;
  matrix4 mModelMatrix;
  matrix4 mNormalMatrix;
  StaticMesh *mStaticMesh = nullptr;
  VulkanBuffer *mUBO = nullptr;
  VulkanBuffer *mUBO1 = nullptr;

  SceneNode();
  void SetPosition(float x, float y, float z);
  void SetRotation(float x, float y, float z);
  void SetScale(float x, float y, float z);
  void Draw(VkCommandBuffer cb, VkRenderPass renderPass, matrix4 &projMatrix,
            matrix4 &viewMatrix);
};
