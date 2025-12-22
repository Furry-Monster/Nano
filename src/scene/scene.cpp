#include "scene.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cmath>
#include "../math/matrix4.h"
#include "../math/quaternion.h"
#include "../render/render_pass.h"
#include "../render/static_mesh.h"
#include "../render/vulkan_rhi.h"
#include "scene_node.h"

#define _4MB 4194304

static float           sLODScale, sLODScaleHW;
static float4          sCameraPositionWS(-330.0f, 330.0f, -330.0f), sCameraTargetPositionWS(0.0f, 80.0f, 0.0f);
static matrix4         sProjectionMatrix, sViewMatrix, sModelMatrix;
static GlobalConstants sGlobalConstantsData;
static Buffer *sGlobalConstantsBuffer, *sBVHBuffer, *sEchoBuffer, *sVisBuffer64, *sNaniteMesh, *sVisiableClusterSHWH,
    *sWorkArgsBuffer[2], *sMainAndPostNodeAndClusterBatches;
static Texture2D*  sVisualizationTexture;
static RenderPass *sInitPass, *sNodeAndClusterCullPasses[4], *sClusterCullPass, *sHWRasterizePass, *sVisuaizePass;
static SceneNode*  sFSQNode;

static int     sCurrentMipLevelIndex = 0;
static int     sAvaliableMipLevels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
unsigned char* LoadFileContent(const char* inFilePath, size_t& outFileSize)
{
    FILE* file = fopen(inFilePath, "rb");
    if (file == nullptr)
    {
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    outFileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* fileContent = new unsigned char[outFileSize];
    fread(fileContent, 1, outFileSize, file);
    fclose(file);
    return fileContent;
}
void InitScene(int inCanvasWidth, int inCanvasHeight)
{
    StaticMesh::Init();
    sProjectionMatrix.Perspective(90.0f, float(inCanvasWidth) / float(inCanvasHeight), 10.0f, 10000.0f);
    sViewMatrix.LookAt(sCameraPositionWS, sCameraTargetPositionWS, float4(0.0f, 1.0f, 0.0f));
    matrix3 scaleMatrix;
    scaleMatrix.LoadIdentity();
    matrix3 lt3x3 = scaleMatrix * quaternion(180.0f, 0.0f, 0.0f).toMatrix3();
    sModelMatrix.LoadIdentity();
    sModelMatrix.SetLeftTop3x3(lt3x3);

    float ViewToPixels = 0.5f * sProjectionMatrix.v[5] * float(inCanvasHeight);
    sLODScale          = ViewToPixels / 1.0f;
    sLODScaleHW        = ViewToPixels / 32.0f;

    sGlobalConstantsData.SetProjectionMatrix(sProjectionMatrix.v);
    sGlobalConstantsData.SetViewMatrix(sViewMatrix.v); // translated world
    sGlobalConstantsData.SetModelMatrix(sModelMatrix.v);
    sGlobalConstantsData.SetMisc0(sAvaliableMipLevels[sCurrentMipLevelIndex], 0, 0, 0);
    float4 viewDirection = sCameraTargetPositionWS - sCameraPositionWS;
    viewDirection.Normalize();
    sGlobalConstantsData.SetCameraPositionWS(sCameraPositionWS.x, sCameraPositionWS.y, sCameraPositionWS.z, sLODScale);
    sGlobalConstantsData.SetCameraViewDirectionWS(viewDirection.x, viewDirection.y, viewDirection.z, sLODScaleHW);
    sGlobalConstantsBuffer =
        GenBufferObject(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, 65536);
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sGlobalConstantsBuffer->mBuffer, "GlobalConstants");
    {
        sVisualizationTexture                   = new Texture2D;
        sVisualizationTexture->mFormat          = VK_FORMAT_R32G32B32A32_SFLOAT;
        sVisualizationTexture->mImageAspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        GenImage(sVisualizationTexture,
                 inCanvasWidth,
                 inCanvasHeight,
                 VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        sVisualizationTexture->mImageView = GenImageView2D(
            sVisualizationTexture->mImage, sVisualizationTexture->mFormat, sVisualizationTexture->mImageAspectFlag);
        sVisualizationTexture->mWidth        = inCanvasWidth;
        sVisualizationTexture->mHeight       = inCanvasHeight;
        sVisualizationTexture->mChannelCount = 4;
        SetObjectName(VK_OBJECT_TYPE_IMAGE, sVisualizationTexture->mImage, "VisualizationTexture");
    }
    sEchoBuffer =
        GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, _4MB);
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sEchoBuffer->mBuffer, "EchoBuffer");

    sVisBuffer64 = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                   nullptr,
                                   inCanvasWidth * inCanvasHeight * sizeof(uint64_t));
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sVisBuffer64->mBuffer, "VisBuffer64");

    sWorkArgsBuffer[0] = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                         nullptr,
                                         _4MB);
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sWorkArgsBuffer[0]->mBuffer, "WorkArgs[0]");
    sWorkArgsBuffer[1] = GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                         nullptr,
                                         _4MB);
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sWorkArgsBuffer[1]->mBuffer, "WorkArgs[1]");
    sMainAndPostNodeAndClusterBatches =
        GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, _4MB);
    SetObjectName(
        VK_OBJECT_TYPE_BUFFER, sMainAndPostNodeAndClusterBatches->mBuffer, "MainAndPostNodeAndClusterBatches");
    sVisiableClusterSHWH =
        GenBufferObject(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, _4MB);
    SetObjectName(VK_OBJECT_TYPE_BUFFER, sVisiableClusterSHWH->mBuffer, "VisiableClusterSHWH");

    {
        size_t         fileSize    = 0;
        unsigned char* fileContent = LoadFileContent("res/mitsuba.bvh", fileSize);
        if (fileContent == nullptr || fileSize == 0)
        {
            printf("ERROR: Failed to load res/mitsuba.bvh\n");
            return;
        }
        sBVHBuffer = GenBufferObject(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, fileContent, fileSize);
        delete[] fileContent;
        SetObjectName(VK_OBJECT_TYPE_BUFFER, sBVHBuffer->mBuffer, "HierarchyBuffer");

        fileContent = LoadFileContent("res/mitsuba.nanitemesh", fileSize);
        if (fileContent == nullptr || fileSize == 0)
        {
            printf("ERROR: Failed to load res/mitsuba.nanitemesh\n");
            return;
        }
        sNaniteMesh = GenBufferObject(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, fileContent, fileSize);
        delete[] fileContent;
        SetObjectName(VK_OBJECT_TYPE_BUFFER, sNaniteMesh->mBuffer, "NaniteMesh");
    }
    // init args/visBuffer64 (complete)
    // node and cluster cull : bvh,workarg -> nextWorkArg,NodesAndCluster (incomplete)
    // cluster cull : naniteMesh,NodesAndCluster -> visiableClustersSWHW (pass through)
    // hw/sw : naniteMesh,visiableClustersSWHW -> visBuffer64 (empty)
    // cluster color(rgba32float) : visBuffer64 -> VisualizeTexture (complete)
    // swapchain VisualizeTexture -> swapchain image (complete)
    // init => RasterClear
    {
        sInitPass = new RenderPass(ERenderPassType::ERPT_COMPUTE, "Init");
        sInitPass->SetSSBO(0, sWorkArgsBuffer[0], true);
        sInitPass->SetSSBO(1, sWorkArgsBuffer[1], true);
        sInitPass->SetSSBO(2, sMainAndPostNodeAndClusterBatches, true);
        sInitPass->SetSSBO(3, sVisBuffer64, true);
        sInitPass->SetCS("shaders/Init.sb");
        sInitPass->SetComputeDispatchArgs(
            int(ceilf(float(inCanvasWidth) / 8.0f)), int(ceilf(float(inCanvasHeight) / 8.0f)), 1);
        sInitPass->Build();
    }
    // node and cluster cull
    for (int i = 0; i < 4; i++)
    {
        char szName[128] = {0};
        snprintf(szName, sizeof(szName), "NodeAndClusterCull_%d", i);
        int inputWorkArgsIndex       = i % 2;
        int outputWorkArgsIndex      = (i + 1) % 2;
        sNodeAndClusterCullPasses[i] = new RenderPass(ERenderPassType::ERPT_COMPUTE, szName);
        sNodeAndClusterCullPasses[i]->SetSSBO(0, sBVHBuffer);
        sNodeAndClusterCullPasses[i]->SetSSBO(1, sEchoBuffer, true);
        sNodeAndClusterCullPasses[i]->SetSSBO(2, sMainAndPostNodeAndClusterBatches, true);
        sNodeAndClusterCullPasses[i]->SetSSBO(3, sWorkArgsBuffer[inputWorkArgsIndex]);
        sNodeAndClusterCullPasses[i]->SetSSBO(4, sWorkArgsBuffer[outputWorkArgsIndex], true);
        sNodeAndClusterCullPasses[i]->SetUniformBufferObject(5, sGlobalConstantsBuffer);
        sNodeAndClusterCullPasses[i]->SetSSBO(6, sNaniteMesh);
        sNodeAndClusterCullPasses[i]->SetCS("shaders/NodeAndClusterCull.sb");
        sNodeAndClusterCullPasses[i]->SetComputeDispatchArgs(1, 1, 1);
        sNodeAndClusterCullPasses[i]->Build();
    }
    {
        sClusterCullPass = new RenderPass(ERenderPassType::ERPT_COMPUTE, "ClusterCull");
        sClusterCullPass->SetUniformBufferObject(0, sGlobalConstantsBuffer);
        sClusterCullPass->SetSSBO(1, sMainAndPostNodeAndClusterBatches);
        sClusterCullPass->SetSSBO(2, sVisiableClusterSHWH, true);
        sClusterCullPass->SetCS("shaders/ClusterCull.sb");
        sClusterCullPass->SetComputeDispatchArgs(1, 1, 1);
        sClusterCullPass->Build();
    }
    {
        sHWRasterizePass = new RenderPass(ERenderPassType::ERPT_GRAPHICS, "HWRasterize");
        sHWRasterizePass->SetUniformBufferObject(0, sGlobalConstantsBuffer);
        sHWRasterizePass->SetSSBO(1, sNaniteMesh);
        sHWRasterizePass->SetSSBO(2, sVisiableClusterSHWH);
        sHWRasterizePass->SetSSBO(3, sVisBuffer64, true);
        sHWRasterizePass->SetVSPS("shaders/HWRasterizeVS.sb", "shaders/HWRasterizeFS.sb");
        sHWRasterizePass->Build(inCanvasWidth, inCanvasHeight);
    }
    {
        sVisuaizePass = new RenderPass(ERenderPassType::ERPT_COMPUTE, "Visuaize");
        sVisuaizePass->SetSSBO(0, sVisBuffer64);
        sVisuaizePass->SetComputeImage(1, sVisualizationTexture, true);
        sVisuaizePass->SetCS("shaders/Visualize.sb");
        sVisuaizePass->SetComputeDispatchArgs(
            int(ceilf(float(inCanvasWidth) / 8.0f)), int(ceilf(float(inCanvasHeight) / 8.0f)), 1);
        sVisuaizePass->Build();
    }
    {
        sFSQNode               = new SceneNode;
        StaticMesh* staticMesh = new StaticMesh;
        staticMesh->SetVertexCount(4);
        staticMesh->SetPosition(0, -1.0f, -1.0f, 0.0f, 1.0f);
        staticMesh->SetTexcoord(0, 0.0f, 0.0f, 0.0f, 0.0f);
        staticMesh->SetPosition(1, 1.0f, -1.0f, 0.0f, 1.0f);
        staticMesh->SetTexcoord(1, 1.0f, 0.0f, 0.0f, 0.0f);
        staticMesh->SetPosition(2, -1.0f, 1.0f, 0.0f, 1.0f);
        staticMesh->SetTexcoord(2, 0.0f, 1.0f, 0.0f, 0.0f);
        staticMesh->SetPosition(3, 1.0f, 1.0f, 0.0f, 1.0f);
        staticMesh->SetTexcoord(3, 1.0f, 1.0f, 0.0f, 0.0f);
        staticMesh->mVBO      = GenBufferObject(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                           staticMesh->mVertexData,
                                           sizeof(StaticMeshVertexData) * 4);
        sFSQNode->mStaticMesh = staticMesh;
        staticMesh->mMaterial.Init("shaders/swapchainVS.sb", "shaders/swapchainFS.sb");
        staticMesh->mMaterial.mPrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        staticMesh->mMaterial.SetTexture2D(2, 0, sVisualizationTexture->mImageView, GenSampler());
    }
}
void RenderOneFrame(float inFrameTimeInSecond)
{
    BufferSubData(sGlobalConstantsBuffer, &sGlobalConstantsData, sizeof(GlobalConstants));
    sInitPass->Execute();
    for (int i = 0; i < 4; i++)
    {
        sNodeAndClusterCullPasses[i]->Execute();
    }
    sClusterCullPass->Execute();
    sHWRasterizePass->ExecuteIndirect(sWorkArgsBuffer[0]);
    sVisuaizePass->Execute();
    VkCommandBuffer commandBuffer = CreateCommandBuffer();
    BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    {
        SCOPED_EVENT(commandBuffer, "SwapChain");
        BeginSwapChainRenderPass(commandBuffer);
        sFSQNode->Draw(commandBuffer, GetSwapChainRenderPass(), sProjectionMatrix, sViewMatrix);
    }
    EndSwapChainRenderPass(commandBuffer);
}
void OnKeyUp(int inKeyCode)
{
    if (inKeyCode == GLFW_KEY_UP)
    {
        sCurrentMipLevelIndex++;
        if (sCurrentMipLevelIndex == sizeof(sAvaliableMipLevels) / sizeof(sAvaliableMipLevels[0]))
        {
            sCurrentMipLevelIndex = 0;
        }
    }
    else if (inKeyCode == GLFW_KEY_DOWN)
    {
        sCurrentMipLevelIndex--;
        if (sCurrentMipLevelIndex < 0)
        {
            sCurrentMipLevelIndex = sizeof(sAvaliableMipLevels) / sizeof(sAvaliableMipLevels[0]) - 1;
        }
    }
    sGlobalConstantsData.SetMisc0(sAvaliableMipLevels[sCurrentMipLevelIndex], 0, 0, 0);
}
