# RHI 使用文档

RHI (Render Hardware Interface) 是对 Vulkan API 的封装层，提供了更简单易用的接口。

## 核心概念

RHI 采用单例模式，所有组件通过 `RHI::instance()` 访问。所有资源类都遵循 RAII 原则，析构时自动清理资源。

## 初始化

RHI 在构造时自动初始化 Vulkan 实例、设备、队列等。需要先初始化 Window：

```cpp
#include "render/window.h"
#include "render/rhi/rhi.h"

// 初始化窗口（必须在RHI之前）
Window::instance();  // Window也是单例

// RHI会自动初始化
RHI& rhi = RHI::instance();
```

## 主要组件

### 1. Swapchain（交换链）

管理渲染目标和呈现。

```cpp
#include "render/rhi/swapchain.h"

Swapchain swapchain;

// 创建交换链（需要先初始化Window和RHI）
if (!swapchain.create(1280, 720))
{
    // 错误处理
}

// 获取下一帧图像
uint32_t image_index = swapchain.acquireNextImage();
if (image_index != UINT32_MAX)
{
    // 使用 image_index 进行渲染
    // ...
  
    // 呈现图像
    swapchain.present(image_index);
}
```

### 2. Buffer（缓冲区）

用于存储顶点数据、索引数据、Uniform数据等。

```cpp
#include "render/rhi/buffer.h"

Buffer vertex_buffer;

// 创建缓冲区
if (!vertex_buffer.create(
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // 用途
    1024 * sizeof(float),                // 大小
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
{
    // 错误处理
}

// 上传数据
float vertices[] = { /* ... */ };
vertex_buffer.uploadData(vertices, sizeof(vertices));

// 或者手动映射
void* data = vertex_buffer.map();
memcpy(data, vertices, sizeof(vertices));
vertex_buffer.unmap();
```

### 3. Texture（纹理）

管理图像和纹理资源。

```cpp
#include "render/rhi/texture.h"

Texture texture;

// 从文件加载
if (!texture.createFromFile("texture.png"))
{
    // 错误处理
}

// 创建图像视图
texture.createImageView();

// 创建采样器
VkSampler sampler = texture.createSampler();

// 或者手动创建
texture.create(
    512, 512,                                    // 宽高
    VK_FORMAT_R8G8B8A8_UNORM,                   // 格式
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

// 上传数据
uint8_t pixels[512 * 512 * 4] = { /* ... */ };
texture.uploadData(pixels, sizeof(pixels), 512, 512);
```

### 4. Shader（着色器）

加载和管理着色器模块。

```cpp
#include "render/rhi/shader.h"

Shader vertex_shader;
Shader fragment_shader;

// 从文件加载（SPIR-V格式，.sb扩展名）
if (!vertex_shader.loadFromFile("shader.vert.sb"))
{
    // 错误处理
}

if (!fragment_shader.loadFromFile("shader.frag.sb"))
{
    // 错误处理
}

// 获取着色器模块
VkShaderModule vs_module = vertex_shader.getModule();
VkShaderModule fs_module = fragment_shader.getModule();
```

### 5. Pipeline（管线）

创建图形或计算管线。

```cpp
#include "render/rhi/pipeline.h"

Pipeline pipeline;

// 准备管线创建信息
GraphicsPipelineCreateInfo info = {};
info.render_pass = swapchain.getRenderPass();
info.vertex_shader = vertex_shader.getModule();
info.fragment_shader = fragment_shader.getModule();

// 顶点输入
info.vertex_bindings = {
    {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
};
info.vertex_attributes = {
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
    {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
};

// 视口和裁剪
info.viewport = {0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f};
info.scissor = {{0, 0}, {1280, 720}};

// 创建管线
if (!pipeline.createGraphicsPipeline(info))
{
    // 错误处理
}
```

### 6. CommandBuffer（命令缓冲区）

录制和提交渲染命令。

```cpp
#include "render/rhi/command_buffer.h"

CommandBuffer cmd;

// 创建命令缓冲区
if (!cmd.create())
{
    // 错误处理
}

// 开始录制
if (!cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
{
    // 错误处理
}

// 录制命令（使用Vulkan API）
vkCmdBindPipeline(cmd.getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());
vkCmdBindVertexBuffers(cmd.getCommandBuffer(), 0, 1, &vertex_buffer.getBuffer(), offsets);
vkCmdDraw(cmd.getCommandBuffer(), vertex_count, 1, 0, 0);

// 结束录制
cmd.end();

// 提交到队列
VkFence fence;
// ... 创建fence ...
cmd.submit(rhi.getGraphicsQueue(), VK_NULL_HANDLE, VK_NULL_HANDLE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, fence);
```

### 7. DescriptorSet（描述符集）

管理着色器资源绑定。

```cpp
#include "render/rhi/descriptor_set.h"

// 创建描述符集布局
DescriptorSetLayout layout;
std::vector<VkDescriptorSetLayoutBinding> bindings = {
    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
};
layout.create(bindings);

// 分配描述符集
DescriptorSet descriptor_set;
std::vector<VkDescriptorPoolSize> pool_sizes = {
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32}
};
descriptor_set.allocate(layout.getLayout(), pool_sizes);

// 更新绑定
descriptor_set.updateBuffer(0, &uniform_buffer);
descriptor_set.updateTexture(1, &texture, sampler);
```

## 完整示例

参考 `demo_rhi.cpp` 查看完整的使用示例。

## 注意事项

1. **初始化顺序**：必须先初始化 Window，再使用 RHI
2. **资源管理**：所有资源类在析构时自动清理，无需手动释放
3. **错误处理**：所有 `create` 方法返回 `bool`，需要检查返回值
4. **线程安全**：RHI 单例不是线程安全的，应在单线程中使用
5. **生命周期**：确保资源在使用期间保持有效，不要过早销毁

## 常见问题

**Q: 如何检查设备是否支持某个扩展？**
A: 扩展检查在 RHI 初始化时自动进行，不支持会报错。

**Q: 如何选择合适的内存类型？**
A: 使用 `RHI::findMemoryType()` 方法，传入内存类型过滤器和属性标志。

**Q: 可以创建多个 Swapchain 吗？**
A: 可以，但通常一个窗口只需要一个 Swapchain。

**Q: CommandBuffer 可以复用吗？**
A: 可以，使用 `reset()` 方法重置后可以重新录制。
