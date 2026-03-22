#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

struct GlobalConstants {
  union {
    struct {
      float mProjectionMatrix[16];
      float mViewMatrix[16];
      float mModelMatrix[16];
      unsigned int mMisc0[4];     // x: Manual MipLevel
      float mCameraPositionWS[4]; // x,y,z,w=lodScale
      float mViewDirectionWS[4];  // x,y,z,w=lodScaleHW
    };
    float mData[1024];
  };

  void SetProjectionMatrix(const float *matrix);
  void SetViewMatrix(float *matrix);
  void SetModelMatrix(const float *matrix);
  void SetMisc0(unsigned int x, unsigned int y, unsigned int z, unsigned int w);
  void SetCameraPositionWS(float x, float y, float z, float w = 0.0f);
  void SetCameraViewDirectionWS(float x, float y, float z, float w = 0.0f);
};

struct Texture {
  VkImage mImage = VK_NULL_HANDLE;
  VkDeviceMemory mMemory = VK_NULL_HANDLE;
  VkImageView mImageView = VK_NULL_HANDLE;
  VkFormat mFormat = VK_FORMAT_UNDEFINED;
  VkImageAspectFlags mImageAspectFlag = VK_IMAGE_ASPECT_NONE;
};

struct Texture2D : public Texture {
  int mWidth = 0;
  int mHeight = 0;
  int mChannelCount = 0;
};

struct VulkanBuffer {
  VkBuffer mBuffer = VK_NULL_HANDLE;
  VkDeviceMemory mMemory = VK_NULL_HANDLE;
  int mSize = 0;
};

struct ShaderParameterDescription {
  VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
};

bool InitVulkan(GLFWwindow *window, int canvasWidth, int canvasHeight);

VkCommandBuffer CreateCommandBuffer(
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
void BeginCommandBuffer(VkCommandBuffer cb, VkCommandBufferUsageFlagBits usage);
uint32_t BeginSwapChainRenderPass(VkCommandBuffer cb);
void EndSwapChainRenderPass(VkCommandBuffer cb);

VkQueue GetGraphicQueue();
VkDevice GetVulkanDevice();
VkCommandPool GetVulkanCommandPool();
VkPhysicalDevice GetPhysicalDevice();
VkRenderPass GetSwapChainRenderPass();

VulkanBuffer *GenBufferObject(VkBufferUsageFlags usage,
                              VkMemoryPropertyFlagBits memProps,
                              const void *data = nullptr, int size = 0);
void BufferSubData(VulkanBuffer *buffer, const void *data, VkDeviceSize size);

ShaderParameterDescription *GetUberPassShaderParameterDescription();

VkPipeline
CreatePSO(VkRenderPass renderPass, VkPrimitiveTopology topology,
          const std::vector<VkVertexInputBindingDescription> &bindings,
          const std::vector<VkVertexInputAttributeDescription> &attributes,
          VkShaderModule vs, VkShaderModule fs);

VkShaderModule LoadShaderModule(const char *filePath);
VkFramebuffer *GetSwapChainFrameBuffers();

void TransferImageLayout(VkCommandBuffer cb, VkImage image,
                         VkImageSubresourceRange subresourceRange,
                         VkImageLayout oldLayout, // old
                         VkAccessFlags oldAccess, VkPipelineStageFlags oldStage,
                         VkImageLayout newLayout, // new
                         VkAccessFlags newAccess,
                         VkPipelineStageFlags newStage);

void GenImage(Texture *outTex, int width, int height, VkImageUsageFlags usage,
              VkMemoryPropertyFlagBits memProps);
VkImageView GenImageView2D(VkImage image, VkFormat format,
                           VkImageAspectFlags aspect);
VkSampler
GenSampler(VkFilter minFilter = VK_FILTER_LINEAR,
           VkFilter magFilter = VK_FILTER_LINEAR,
           VkSamplerAddressMode wrapU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
           VkSamplerAddressMode wrapV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
           VkSamplerAddressMode wrapW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

void BeginEvent(VkCommandBuffer cb, const char *name);
void EndEvent(VkCommandBuffer cb);
void SetObjectName(VkObjectType type, void *object, const char *name);

struct ScopedEvent {
  VkCommandBuffer mCommandBuffer;
  ScopedEvent(VkCommandBuffer cb, const char *name) : mCommandBuffer(cb) {
    BeginEvent(cb, name);
  }
  ~ScopedEvent() { EndEvent(mCommandBuffer); }
};

#define SCOPED_EVENT_INNER(cb, name, line)                                     \
  ScopedEvent _scopedEvent_##line(cb, name)
#define SCOPED_EVENT_EXPAND(cb, name, line) SCOPED_EVENT_INNER(cb, name, line)
#define SCOPED_EVENT(cb, name) SCOPED_EVENT_EXPAND(cb, name, __LINE__)
