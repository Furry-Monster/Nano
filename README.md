# Nano

基于 Vulkan 的图形渲染项目。

## 构建

```bash
mkdir build
cd build
cmake ..
make
```

## 运行

构建完成后，可执行文件位于 `bin/` 目录。

## 依赖

- CMake 3.20+
- C++17 编译器
- Vulkan SDK
- GLFW

## 项目结构

- `src/` - 源代码
  - `main.cpp` - 主程序入口
  - `render/` - 渲染相关代码
  - `scene/` - 场景管理
  - `math/` - 数学库
- `shaders/` - 着色器文件
- `res/` - 资源文件
- `libs/` - 第三方库
