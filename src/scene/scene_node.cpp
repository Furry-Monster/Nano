#include "scene_node.h"

#include "../math/quaternion.h"

SceneNode::SceneNode() : mScale(float4(1.0f)) {}

void SceneNode::SetPosition(float x, float y, float z) {
  mPosition.x = x;
  mPosition.y = y;
  mPosition.z = z;
  mNeedUpdate = true;
}

void SceneNode::SetRotation(float x, float y, float z) {
  mRotation.x = x;
  mRotation.y = y;
  mRotation.z = z;
  mNeedUpdate = true;
}

void SceneNode::SetScale(float x, float y, float z) {
  mScale.x = x;
  mScale.y = y;
  mScale.z = z;
  mNeedUpdate = true;
}

void SceneNode::Draw(VkCommandBuffer cb, VkRenderPass renderPass,
                     matrix4 &projMatrix, matrix4 &viewMatrix) {
  if (mNeedUpdate) {
    matrix3 scaleMatrix;
    scaleMatrix.LoadIdentity();
    scaleMatrix.SetScale(mScale.x, mScale.y, mScale.z);
    matrix3 lt3x3 =
        scaleMatrix *
        quaternion(mRotation.x, mRotation.y, mRotation.z).toMatrix3();
    mModelMatrix.LoadIdentity();
    mModelMatrix.SetLeftTop3x3(lt3x3);
    mModelMatrix.Translate(mPosition.x, mPosition.y, mPosition.z);
    mNormalMatrix = mModelMatrix.Invert();
    mNormalMatrix.Transpose();

    if (mUBO == nullptr) {
      mUBO = GenBufferObject(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr,
                             sizeof(matrix4) * 1024);
      mStaticMesh->mMaterial.SetUBO(0, mUBO->mBuffer, sizeof(matrix4) * 1024);

      mUBO1 = GenBufferObject(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr,
                              sizeof(matrix4) * 1024);
      mStaticMesh->mMaterial.SetUBO(1, mUBO1->mBuffer, sizeof(matrix4) * 1024);
    }

    {
      VkDevice device = GetVulkanDevice();
      void *mapped = nullptr;
      vkMapMemory(device, mUBO->mMemory, 0, sizeof(matrix4) * 1024, 0, &mapped);
      std::memcpy(mapped, &mModelMatrix, sizeof(matrix4));
      std::memcpy(static_cast<char *>(mapped) + sizeof(matrix4), &mNormalMatrix,
                  sizeof(matrix4));
      vkUnmapMemory(device, mUBO->mMemory);
    }

    {
      float ubo1data[] = {1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
                          0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  0.0f};
      VkDevice device = GetVulkanDevice();
      void *mapped = nullptr;
      vkMapMemory(device, mUBO1->mMemory, 0, sizeof(matrix4) * 1024, 0,
                  &mapped);
      std::memcpy(mapped, ubo1data, sizeof(ubo1data));
      vkUnmapMemory(device, mUBO1->mMemory);
    }

    mNeedUpdate = false;
  }

  if (mStaticMesh != nullptr) {
    mStaticMesh->mMaterial.Active(cb, renderPass);
    mStaticMesh->Draw(cb);
  }
}
