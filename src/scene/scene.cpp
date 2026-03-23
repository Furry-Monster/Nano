#include "scene.h"

#include "input/input.h"
#include "math/matrix4.h"
#include "math/quaternion.h"
#include "render/render_pass.h"
#include "render/static_mesh.h"
#include "render/vulkan_rhi.h"
#include "scene_node.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <spdlog/spdlog.h>
#include <vector>

static constexpr int kBufferSize4MB = 4194304;

static uint32_t CountHzbMipLevels(int width, int height) {
  uint32_t levels = 1;
  int w = width;
  int h = height;
  while (w > 1 || h > 1) {
    w = std::max(1, w / 2);
    h = std::max(1, h / 2);
    levels++;
  }
  return levels;
}

static float sLODScale, sLODScaleHW;
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

static Texture2D *sHZBTexture = nullptr;
static uint32_t sHZBMipLevelCount = 1;
static std::vector<VkImageView> sHZBStorageMipViews;
static VkImageView sHZBFullSampleView = VK_NULL_HANDLE;
static VkSampler sHZBPointSampler = VK_NULL_HANDLE;
static RenderPass *sBuildHZBMip0Pass = nullptr;
static std::vector<RenderPass *> sBuildHZBDownsamplePasses;

static int sCanvasW = 1280;
static int sCanvasH = 720;
static bool sHzbFromPreviousFrameReady = false;

static int sCurrentMipLevelIndex = 0;
static constexpr int kAvailableMipLevels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
static constexpr int kMipLevelCount =
    sizeof(kAvailableMipLevels) / sizeof(kAvailableMipLevels[0]);

static bool sAutoDistanceLod = true;
static const float4 kSceneLodReference(0.f, 80.f, 0.f, 1.f);

static void SyncSceneCameraFromInput() {
  const float4 worldUp(0.f, 1.f, 0.f, 0.f);
  const float4 pos = InputCameraPosition(); // NOLINT
  const float4 viewDir = InputCameraForwardUnit();
  const float4 focus = pos + viewDir * 200.f;
  sViewMatrix.LookAt(pos, focus, worldUp);
  sGlobalConstantsData.SetViewMatrix(sViewMatrix.v);
  sGlobalConstantsData.SetCameraPositionWS(pos.x, pos.y, pos.z, sLODScale);
  sGlobalConstantsData.SetCameraViewDirectionWS(viewDir.x, viewDir.y, viewDir.z,
                                                sLODScaleHW);
}

static uint32_t LodValueFromCameraDistance() {
  float4 delta = InputCameraPosition() - kSceneLodReference;
  const float dist = std::sqrt(std::max(0.f, dot3(delta, delta)));
  constexpr float kNear = 90.f;
  constexpr float kFar = 1100.f;
  float t = (dist - kNear) / (kFar - kNear);
  t = std::clamp(t, 0.f, 1.f);
  const float fidx = t * static_cast<float>(kMipLevelCount - 1);
  int idx = static_cast<int>(fidx + 0.5f);
  idx = std::clamp(idx, 0, kMipLevelCount - 1);
  return static_cast<uint32_t>(kAvailableMipLevels[idx]);
}

static unsigned char *LoadFileContent(const char *path, size_t &outSize) {
  FILE *file = std::fopen(path, "rb");
  if (!file) {
    spdlog::error("Failed to open file: {}", path);
    outSize = 0;
    return nullptr;
  }

  if (std::fseek(file, 0, SEEK_END) != 0) {
    spdlog::error("Failed to seek file: {}", path);
    std::fclose(file);
    outSize = 0;
    return nullptr;
  }

  const long sizeLong = std::ftell(file);
  if (sizeLong < 0) {
    spdlog::error("Failed to get file size: {}", path);
    std::fclose(file);
    outSize = 0;
    return nullptr;
  }

  outSize = static_cast<size_t>(sizeLong);
  if (std::fseek(file, 0, SEEK_SET) != 0) {
    spdlog::error("Failed to rewind file: {}", path);
    std::fclose(file);
    outSize = 0;
    return nullptr;
  }

  if (outSize == 0) {
    std::fclose(file);
    return nullptr;
  }

  auto *content = new unsigned char[outSize];
  const size_t read = std::fread(content, 1, outSize, file);
  std::fclose(file);
  if (read != outSize) {
    spdlog::error("Incomplete read: {} ({}/{})", path, read, outSize);
    delete[] content;
    outSize = 0;
    return nullptr;
  }

  return content;
}

void InitScene(int canvasWidth, int canvasHeight) {
  StaticMesh::InitVertexLayout();

  sProjectionMatrix.Perspective(
      90.0f, static_cast<float>(canvasWidth) / canvasHeight, 10.0f, 10000.0f);
  InputInitFromLookAt(-330.0f, 330.0f, -330.0f, 0.0f, 80.0f, 0.0f);

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
  SyncSceneCameraFromInput();
  sGlobalConstantsData.SetModelMatrix(sModelMatrix.v);
  sCanvasW = canvasWidth;
  sCanvasH = canvasHeight;
  sGlobalConstantsData.SetMisc0(LodValueFromCameraDistance(), 0u,
                                static_cast<unsigned>(canvasWidth),
                                static_cast<unsigned>(canvasHeight));

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
    unsigned char *data = nullptr;

    data = LoadFileContent("res/Cian_Dressup.bvh", fileSize);
    sBVHBuffer = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, data,
                                 static_cast<int>(fileSize));
    delete[] data;
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sBVHBuffer->mBuffer,
                  "HierarchyBuffer");

    data = LoadFileContent("res/Cian_Dressup.nanitemesh", fileSize);
    sNaniteMesh = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, data,
                                  static_cast<int>(fileSize));
    delete[] data;
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sNaniteMesh->mBuffer, "NaniteMesh");
  }

  sHzbFromPreviousFrameReady = false;
  sHZBMipLevelCount = CountHzbMipLevels(canvasWidth, canvasHeight);
  sHZBTexture = new Texture2D;
  sHZBTexture->mFormat = VK_FORMAT_R32_SFLOAT;
  sHZBTexture->mImageAspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
  GenImageWithMipLevels(sHZBTexture, canvasWidth, canvasHeight,
                        sHZBMipLevelCount,
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  sHZBTexture->mWidth = canvasWidth;
  sHZBTexture->mHeight = canvasHeight;
  sHZBTexture->mChannelCount = 1;
  SetObjectName(VK_OBJECT_TYPE_IMAGE, sHZBTexture->mImage, "HZB");

  sHZBStorageMipViews.resize(sHZBMipLevelCount);
  for (uint32_t m = 0; m < sHZBMipLevelCount; m++) {
    sHZBStorageMipViews[m] =
        GenImageView2DMipRange(sHZBTexture->mImage, VK_FORMAT_R32_SFLOAT,
                               VK_IMAGE_ASPECT_COLOR_BIT, m, 1);
  }
  sHZBFullSampleView =
      GenImageView2DMipRange(sHZBTexture->mImage, VK_FORMAT_R32_SFLOAT,
                             VK_IMAGE_ASPECT_COLOR_BIT, 0, sHZBMipLevelCount);
  sHZBPointSampler = GenSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                                VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

  {
    VkCommandBuffer clearCb = CreateCommandBuffer();
    if (clearCb != VK_NULL_HANDLE) {
      BeginCommandBuffer(clearCb, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
      VkImageSubresourceRange hzRange{};
      hzRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      hzRange.baseMipLevel = 0;
      hzRange.levelCount = sHZBMipLevelCount;
      hzRange.baseArrayLayer = 0;
      hzRange.layerCount = 1;
      TransferImageLayout(clearCb, sHZBTexture->mImage, hzRange,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_ACCESS_NONE,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_TRANSFER_BIT);
      VkClearColorValue ccv{};
      ccv.float32[0] = 1.0f;
      vkCmdClearColorImage(clearCb, sHZBTexture->mImage,
                           VK_IMAGE_LAYOUT_GENERAL, &ccv, 1, &hzRange);
      TransferImageLayout(
          clearCb, sHZBTexture->mImage, hzRange, VK_IMAGE_LAYOUT_GENERAL,
          VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
      vkEndCommandBuffer(clearCb);
      SubmitOneTimeCommandBuffer(clearCb);
    }
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
    sNodeAndClusterCullPasses[i]->SetCombinedImageSampler(7, sHZBFullSampleView,
                                                          sHZBPointSampler);
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

  {
    sBuildHZBMip0Pass = new RenderPass(RenderPassType::Compute, "BuildHZBMip0");
    sBuildHZBMip0Pass->SetSSBO(0, sVisBuffer64);
    sBuildHZBMip0Pass->SetComputeStorageImageView(1, sHZBStorageMipViews[0],
                                                  false);
    sBuildHZBMip0Pass->SetUniformBufferObject(2, sGlobalConstantsBuffer);
    sBuildHZBMip0Pass->SetCS("shaders/BuildHZBMip0.sb");
    sBuildHZBMip0Pass->SetComputeDispatchArgs(
        static_cast<int>(std::ceil(static_cast<float>(canvasWidth) / 8.0f)),
        static_cast<int>(std::ceil(static_cast<float>(canvasHeight) / 8.0f)),
        1);
    sBuildHZBMip0Pass->Build();
  }

  for (uint32_t m = 1; m < sHZBMipLevelCount; m++) {
    char nm[64];
    std::snprintf(nm, sizeof(nm), "BuildHZBDown_%u", m);
    auto *dp = new RenderPass(RenderPassType::Compute, nm);
    dp->SetComputeStorageImageView(0, sHZBStorageMipViews[m - 1], false);
    dp->SetComputeStorageImageView(1, sHZBStorageMipViews[m], false);
    dp->SetCS("shaders/BuildHZBDownsample.sb");
    const int dw = std::max(1, canvasWidth >> static_cast<int>(m));
    const int dh = std::max(1, canvasHeight >> static_cast<int>(m));
    dp->SetComputeDispatchArgs(
        static_cast<int>(std::ceil(static_cast<float>(dw) / 8.0f)),
        static_cast<int>(std::ceil(static_cast<float>(dh) / 8.0f)), 1);
    dp->Build();
    sBuildHZBDownsamplePasses.push_back(dp);
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
    mesh->mMaterial.Init("shaders/SwapchainVS.sb", "shaders/SwapchainFS.sb");
    mesh->mMaterial.mPrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    mesh->mMaterial.SetTexture2D(2, 0, sVisualizationTexture->mImageView,
                                 GenSampler());
  }

  spdlog::info("Scene initialized");
}

void OnKeyDown(int keyCode) {
  if (keyCode == GLFW_KEY_M) {
    sAutoDistanceLod = !sAutoDistanceLod;
    spdlog::info("LOD: {}", sAutoDistanceLod ? "auto (distance to reference)"
                                             : "manual (Up / Down arrows)");
  }
}

void RenderOneFrame([[maybe_unused]] float frameTime) {
  SyncSceneCameraFromInput();
  const uint32_t lodMipValue =
      sAutoDistanceLod
          ? LodValueFromCameraDistance()
          : static_cast<uint32_t>(kAvailableMipLevels[sCurrentMipLevelIndex]);
  const uint32_t hzbEnable =
      (sHzbFromPreviousFrameReady && !InputCameraMovedThisFrame()) ? 1u : 0u;
  sGlobalConstantsData.SetMisc0(lodMipValue, hzbEnable,
                                static_cast<unsigned>(sCanvasW),
                                static_cast<unsigned>(sCanvasH));
  BufferSubData(sGlobalConstantsBuffer, &sGlobalConstantsData,
                sizeof(GlobalConstants));

  uint32_t imageIndex = 0;
  if (!PrepareSwapChainFrame(&imageIndex))
    return;

  VkCommandBuffer cb = CreateCommandBuffer();
  if (cb == VK_NULL_HANDLE)
    return;

  BeginCommandBuffer(cb, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  sInitPass->UpdateDescriptorSets();
  sInitPass->RecordComputeCommands(cb);
  CmdBarrierAfterComputeWrites(cb);

  for (auto *pass : sNodeAndClusterCullPasses) {
    pass->UpdateDescriptorSets();
    pass->RecordComputeCommands(cb);
    CmdBarrierAfterComputeWrites(cb);
  }

  sClusterCullPass->UpdateDescriptorSets();
  sClusterCullPass->RecordComputeCommands(cb);
  CmdBarrierComputeToIndirectAndDraw(cb);

  sHWRasterizePass->UpdateDescriptorSets();
  sHWRasterizePass->RecordGraphicsIndirectCommands(cb, sWorkArgsBuffer[0]);
  CmdBarrierFragmentStorageToCompute(cb);

  sVisualizePass->UpdateDescriptorSets();
  sVisualizePass->RecordComputeCommands(cb);
  CmdBarrierComputeToFragmentSample(cb);

  {
    SCOPED_EVENT(cb, "BuildHZB");
    VkImageSubresourceRange hzRangeAll{};
    hzRangeAll.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    hzRangeAll.baseMipLevel = 0;
    hzRangeAll.levelCount = sHZBMipLevelCount;
    hzRangeAll.baseArrayLayer = 0;
    hzRangeAll.layerCount = 1;

    TransferImageLayout(
        cb, sHZBTexture->mImage, hzRangeAll,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    sBuildHZBMip0Pass->UpdateDescriptorSets();
    sBuildHZBMip0Pass->RecordComputeCommands(cb);

    VkImageMemoryBarrier hzBarrier{};
    hzBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hzBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hzBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hzBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    hzBarrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    hzBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    hzBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    hzBarrier.image = sHZBTexture->mImage;
    hzBarrier.subresourceRange = hzRangeAll;

    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &hzBarrier);

    for (auto *dp : sBuildHZBDownsamplePasses) {
      dp->UpdateDescriptorSets();
      dp->RecordComputeCommands(cb);
      vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 1, &hzBarrier);
    }

    TransferImageLayout(
        cb, sHZBTexture->mImage, hzRangeAll, VK_IMAGE_LAYOUT_GENERAL,
        VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  }

  sHzbFromPreviousFrameReady = true;

  {
    SCOPED_EVENT(cb, "SwapChain");
    CmdBeginSwapChainRenderPass(cb, imageIndex);
    sFSQNode->Draw(cb, GetSwapChainRenderPass(), sProjectionMatrix,
                   sViewMatrix);
    vkCmdEndRenderPass(cb);
  }

  vkEndCommandBuffer(cb);
  SubmitAndPresentSwapChainFrame(cb, imageIndex);

  InputClearCameraMovedAfterFrame();
}

void OnKeyUp(int keyCode) {
  if (!sAutoDistanceLod) {
    if (keyCode == GLFW_KEY_UP) {
      sCurrentMipLevelIndex = (sCurrentMipLevelIndex + 1) % kMipLevelCount;
    } else if (keyCode == GLFW_KEY_DOWN) {
      sCurrentMipLevelIndex =
          (sCurrentMipLevelIndex - 1 + kMipLevelCount) % kMipLevelCount;
    } else {
      return;
    }
    spdlog::info("Manual LOD mip value: {}",
                 kAvailableMipLevels[sCurrentMipLevelIndex]);
  }
}
