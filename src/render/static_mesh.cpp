#include "static_mesh.h"

std::vector<VkVertexInputBindingDescription> StaticMesh::sVertexBindings;
std::vector<VkVertexInputAttributeDescription> StaticMesh::sVertexAttributes;

void StaticMesh::InitVertexLayout() {
  sVertexBindings.resize(1);
  sVertexBindings[0] = {};
  sVertexBindings[0].binding = 0;
  sVertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  sVertexBindings[0].stride = sizeof(StaticMeshVertex);

  sVertexAttributes.resize(4);
  for (int i = 0; i < 4; i++) {
    sVertexAttributes[i].binding = 0;
    sVertexAttributes[i].location = i;
    sVertexAttributes[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    sVertexAttributes[i].offset = static_cast<uint32_t>(i * sizeof(float4));
  }
}

void StaticMesh::SetVertexCount(int count) {
  mVertexCount = count;
  mVertexData = new StaticMeshVertex[count]();
}

void StaticMesh::SetPosition(int index, float x, float y, float z, float w) {
  mVertexData[index].position = float4(x, y, z, w);
}

void StaticMesh::SetTexcoord(int index, float x, float y, float z, float w) {
  mVertexData[index].texcoord = float4(x, y, z, w);
}

void StaticMesh::Draw(VkCommandBuffer cb) {
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cb, 0, 1, &mVBO->mBuffer, &offset);
  vkCmdDraw(cb, mVertexCount, 1, 0, 0);
}
