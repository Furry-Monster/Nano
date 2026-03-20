#include "scene.h"

#include "../math/matrix4.h"
#include "../math/quaternion.h"
#include "../render/render_pass.h"
#include "../render/static_mesh.h"
#include "../render/vulkan_rhi.h"
#include "scene_node.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <spdlog/spdlog.h>

static constexpr int kBufferSize4MB = 4194304;

static float sLODScale, sLODScaleHW;
static float4 sCameraPosition(-330.0f, 330.0f, -330.0f);
static float4 sCameraTarget(0.0f, 80.0f, 0.0f);
static matrix4 sProjectionMatrix, sViewMatrix, sModelMatrix;
static GlobalConstants sGlobalConstantsData;
static VulkanBuffer *sGlobalConstantsBuffer;
static VulkanBuffer *sBVHBuffer;
static VulkanBuffer *sEchoBuffer;
static VulkanBuffer *sVisBuffer64;
static VulkanBuffer *sNaniteMesh;
static VulkanBuffer *sVisibleClusterSHWH;
static VulkanBuffer *sWorkArgsBuffer[2];
static VulkanBuffer *sMainAndPostNodeAndClusterBatches;
static Texture2D *sVisualizationTexture;
static RenderPass *sInitPass;
static RenderPass *sNodeAndClusterCullPasses[4];
static RenderPass *sClusterCullPass;
static RenderPass *sHWRasterizePass;
static RenderPass *sVisualizePass;
static SceneNode *sFSQNode;

static int sCurrentMipLevelIndex = 0;
static constexpr int kAvailableMipLevels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
static constexpr int kMipLevelCount =
    sizeof(kAvailableMipLevels) / sizeof(kAvailableMipLevels[0]);

static unsigned char *LoadFileContent(const char *path, size_t &outSize) {
  FILE *file = std::fopen(path, "rb");
  if (!file) {
    spdlog::error("Failed to open file: {}", path);
    outSize = 0;
    return nullptr;
  }
  std::fseek(file, 0, SEEK_END);
  outSize = std::ftell(file);
  std::fseek(file, 0, SEEK_SET);
  auto *content = new unsigned char[outSize];
  std::fread(content, 1, outSize, file);
  std::fclose(file);
  return content;
}

void InitScene(int canvasWidth, int canvasHeight) {
  StaticMesh::InitVertexLayout();

  sProjectionMatrix.Perspective(
      90.0f, static_cast<float>(canvasWidth) / canvasHeight, 10.0f, 10000.0f);
  sViewMatrix.LookAt(sCameraPosition, sCameraTarget, float4(0.0f, 1.0f, 0.0f));

  matrix3 scaleMatrix;
  scaleMatrix.LoadIdentity();
  matrix3 lt3x3 = scaleMatrix * quaternion(180.0f, 0.0f, 0.0f).toMatrix3();
  sModelMatrix.LoadIdentity();
  sModelMatrix.SetLeftTop3x3(lt3x3);

  // ViewToPixels: maps world-space error to screen pixels
  float viewToPixels =
      0.5f * sProjectionMatrix.v[5] * static_cast<float>(canvasHeight);
  sLODScale = viewToPixels / 1.0f;
  sLODScaleHW = viewToPixels / 32.0f;

  sGlobalConstantsData.SetProjectionMatrix(sProjectionMatrix.v);
  sGlobalConstantsData.SetViewMatrix(sViewMatrix.v);
  sGlobalConstantsData.SetModelMatrix(sModelMatrix.v);
  sGlobalConstantsData.SetMisc0(kAvailableMipLevels[sCurrentMipLevelIndex], 0,
                                0, 0);

  float4 viewDir = sCameraTarget - sCameraPosition;
  viewDir.Normalize();
  sGlobalConstantsData.SetCameraPositionWS(sCameraPosition.x, sCameraPosition.y,
                                           sCameraPosition.z, sLODScale);
  sGlobalConstantsData.SetCameraViewDirectionWS(viewDir.x, viewDir.y, viewDir.z,
                                                sLODScaleHW);

  sGlobalConstantsBuffer =
      GenBufferObject(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, 65536);
  SetObjectName(VK_OBJECT_TYPE_BUFFER, sGlobalConstantsBuffer->mBuffer,
                "GlobalConstants");

  {
    sVisualizationTexture = new Texture2D;
    sVisualizationTexture->mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    sVisualizationTexture->mImageAspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    GenImage(sVisualizationTexture, canvasWidth, canvasHeight,
             VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    sVisualizationTexture->mImageView = GenImageView2D(
        sVisualizationTexture->mImage, sVisualizationTexture->mFormat,
        sVisualizationTexture->mImageAspectFlag);
    sVisualizationTexture->mWidth = canvasWidth;
    sVisualizationTexture->mHeight = canvasHeight;
    sVisualizationTexture->mChannelCount = 4;
    SetObjectName(VK_OBJECT_TYPE_IMAGE, sVisualizationTexture->mImage,
                  "VisualizationTexture");
  }

  sEchoBuffer = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr,
                                kBufferSize4MB);
  SetObjectName(VK_OBJECT_TYPE_BUFFER, sEchoBuffer->mBuffer, "EchoBuffer");

  sVisBuffer64 = GenBufferObject(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      nullptr, canvasWidth * canvasHeight * static_cast<int>(sizeof(uint64_t)));
  SetObjectName(VK_OBJECT_TYPE_BUFFER, sVisBuffer64->mBuffer, "VisBuffer64");

  sWorkArgsBuffer[0] = GenBufferObject(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, kBufferSize4MB);
  SetObjectName(VK_OBJECT_TYPE_BUFFER, sWorkArgsBuffer[0]->mBuffer,
                "WorkArgs[0]");

  sWorkArgsBuffer[1] = GenBufferObject(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, kBufferSize4MB);
  SetObjectName(VK_OBJECT_TYPE_BUFFER, sWorkArgsBuffer[1]->mBuffer,
                "WorkArgs[1]");

  sMainAndPostNodeAndClusterBatches = GenBufferObject(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      nullptr, kBufferSize4MB);
  SetObjectName(VK_OBJECT_TYPE_BUFFER,
                sMainAndPostNodeAndClusterBatches->mBuffer,
                "MainAndPostNodeAndClusterBatches");

  sVisibleClusterSHWH = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                        nullptr, kBufferSize4MB);
  SetObjectName(VK_OBJECT_TYPE_BUFFER, sVisibleClusterSHWH->mBuffer,
                "VisibleClusterSHWH");

  {
    size_t fileSize = 0;
    unsigned char *data = LoadFileContent("res/mitsuba.bvh", fileSize);
    sBVHBuffer = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, data,
                                 static_cast<int>(fileSize));
    delete[] data;
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sBVHBuffer->mBuffer,
                  "HierarchyBuffer");

    data = LoadFileContent("res/mitsuba.nanitemesh", fileSize);
    sNaniteMesh = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, data,
                                  static_cast<int>(fileSize));
    delete[] data;
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sNaniteMesh->mBuffer, "NaniteMesh");
  }

  // Init pass
  {
    sInitPass = new RenderPass(RenderPassType::Compute, "Init");
    sInitPass->SetSSBO(0, sWorkArgsBuffer[0], true);
    sInitPass->SetSSBO(1, sWorkArgsBuffer[1], true);
    sInitPass->SetSSBO(2, sMainAndPostNodeAndClusterBatches, true);
    sInitPass->SetSSBO(3, sVisBuffer64, true);
    sInitPass->SetCS("shaders/Init.sb");
    sInitPass->SetComputeDispatchArgs(
        static_cast<int>(std::ceil(static_cast<float>(canvasWidth) / 8.0f)),
        static_cast<int>(std::ceil(static_cast<float>(canvasHeight) / 8.0f)),
        1);
    sInitPass->Build();
  }

  // Node and cluster cull passes (4 BVH traversal iterations)
  for (int i = 0; i < 4; i++) {
    char name[128];
    std::snprintf(name, sizeof(name), "NodeAndClusterCull_%d", i);
    int inputIdx = i % 2;
    int outputIdx = (i + 1) % 2;

    sNodeAndClusterCullPasses[i] =
        new RenderPass(RenderPassType::Compute, name);
    sNodeAndClusterCullPasses[i]->SetSSBO(0, sBVHBuffer);
    sNodeAndClusterCullPasses[i]->SetSSBO(1, sEchoBuffer, true);
    sNodeAndClusterCullPasses[i]->SetSSBO(2, sMainAndPostNodeAndClusterBatches,
                                          true);
    sNodeAndClusterCullPasses[i]->SetSSBO(3, sWorkArgsBuffer[inputIdx]);
    sNodeAndClusterCullPasses[i]->SetSSBO(4, sWorkArgsBuffer[outputIdx], true);
    sNodeAndClusterCullPasses[i]->SetUniformBufferObject(
        5, sGlobalConstantsBuffer);
    sNodeAndClusterCullPasses[i]->SetSSBO(6, sNaniteMesh);
    sNodeAndClusterCullPasses[i]->SetCS("shaders/NodeAndClusterCull.sb");
    sNodeAndClusterCullPasses[i]->SetComputeDispatchArgs(1, 1, 1);
    sNodeAndClusterCullPasses[i]->Build();
  }

  // Cluster cull pass
  {
    sClusterCullPass = new RenderPass(RenderPassType::Compute, "ClusterCull");
    sClusterCullPass->SetUniformBufferObject(0, sGlobalConstantsBuffer);
    sClusterCullPass->SetSSBO(1, sMainAndPostNodeAndClusterBatches);
    sClusterCullPass->SetSSBO(2, sVisibleClusterSHWH, true);
    sClusterCullPass->SetCS("shaders/ClusterCull.sb");
    sClusterCullPass->SetComputeDispatchArgs(1, 1, 1);
    sClusterCullPass->Build();
  }

  // HW rasterize pass
  {
    sHWRasterizePass = new RenderPass(RenderPassType::Graphics, "HWRasterize");
    sHWRasterizePass->SetUniformBufferObject(0, sGlobalConstantsBuffer);
    sHWRasterizePass->SetSSBO(1, sNaniteMesh);
    sHWRasterizePass->SetSSBO(2, sVisibleClusterSHWH);
    sHWRasterizePass->SetSSBO(3, sVisBuffer64, true);
    sHWRasterizePass->SetVSPS("shaders/HWRasterizeVS.sb",
                              "shaders/HWRasterizeFS.sb");
    sHWRasterizePass->Build(canvasWidth, canvasHeight);
  }

  // Visualize pass
  {
    sVisualizePass = new RenderPass(RenderPassType::Compute, "Visualize");
    sVisualizePass->SetSSBO(0, sVisBuffer64);
    sVisualizePass->SetComputeImage(1, sVisualizationTexture, true);
    sVisualizePass->SetCS("shaders/Visualize.sb");
    sVisualizePass->SetComputeDispatchArgs(
        static_cast<int>(std::ceil(static_cast<float>(canvasWidth) / 8.0f)),
        static_cast<int>(std::ceil(static_cast<float>(canvasHeight) / 8.0f)),
        1);
    sVisualizePass->Build();
  }

  // Full-screen quad for swapchain blit
  {
    sFSQNode = new SceneNode;
    auto *mesh = new StaticMesh;
    mesh->SetVertexCount(4);
    mesh->SetPosition(0, -1.0f, -1.0f, 0.0f, 1.0f);
    mesh->SetTexcoord(0, 0.0f, 0.0f);
    mesh->SetPosition(1, 1.0f, -1.0f, 0.0f, 1.0f);
    mesh->SetTexcoord(1, 1.0f, 0.0f);
    mesh->SetPosition(2, -1.0f, 1.0f, 0.0f, 1.0f);
    mesh->SetTexcoord(2, 0.0f, 1.0f);
    mesh->SetPosition(3, 1.0f, 1.0f, 0.0f, 1.0f);
    mesh->SetTexcoord(3, 1.0f, 1.0f);

    mesh->mVBO = GenBufferObject(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        mesh->mVertexData, sizeof(StaticMeshVertex) * 4);
    sFSQNode->mStaticMesh = mesh;
    mesh->mMaterial.Init("shaders/swapchainVS.sb", "shaders/swapchainFS.sb");
    mesh->mMaterial.mPrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    mesh->mMaterial.SetTexture2D(2, 0, sVisualizationTexture->mImageView,
                                 GenSampler());
  }

  spdlog::info("Scene initialized");
}

void RenderOneFrame(float /*frameTime*/) {
  BufferSubData(sGlobalConstantsBuffer, &sGlobalConstantsData,
                sizeof(GlobalConstants));

  sInitPass->Execute();
  for (int i = 0; i < 4; i++) {
    sNodeAndClusterCullPasses[i]->Execute();
  }
  sClusterCullPass->Execute();
  sHWRasterizePass->ExecuteIndirect(sWorkArgsBuffer[0]);
  sVisualizePass->Execute();

  VkCommandBuffer cb = CreateCommandBuffer();
  BeginCommandBuffer(cb, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  {
    SCOPED_EVENT(cb, "SwapChain");
    BeginSwapChainRenderPass(cb);
    sFSQNode->Draw(cb, GetSwapChainRenderPass(), sProjectionMatrix,
                   sViewMatrix);
  }
  EndSwapChainRenderPass(cb);
}

void OnKeyUp(int keyCode) {
  if (keyCode == GLFW_KEY_UP) {
    sCurrentMipLevelIndex = (sCurrentMipLevelIndex + 1) % kMipLevelCount;
  } else if (keyCode == GLFW_KEY_DOWN) {
    sCurrentMipLevelIndex =
        (sCurrentMipLevelIndex - 1 + kMipLevelCount) % kMipLevelCount;
  }
  sGlobalConstantsData.SetMisc0(kAvailableMipLevels[sCurrentMipLevelIndex], 0,
                                0, 0);
  spdlog::info("LOD Mip Level: {}", kAvailableMipLevels[sCurrentMipLevelIndex]);
}
