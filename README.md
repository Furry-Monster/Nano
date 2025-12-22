# Nano

基于 Vulkan 的一个 UE5-Nanite虚拟几何体系统 的粗浅复现。

代码比较混乱并且存在一些 Vulkan 错误，后续会慢慢修复，由于个人对 Vulkan API不太熟悉，周期可能较长，如果可以帮忙修复的话可以提一个PR。

![demo](demo.png)

## 构建

```bash

cd shaders
chmod +x compile.sh
./compile.sh

cd ../

mkdir build
cmake --build build
```

## 运行

构建完成后，可执行文件位于 `bin/` 目录。

## 依赖

- CMake 3.20+
- C++17 编译器,不支持MSVC
- Vulkan SDK
- GLFW
- glslc 或者 glslangValidator

## 项目结构

- `src/` - 源代码
  - `main.cpp` - 主程序入口
  - `render/` - 渲染相关代码
  - `scene/` - 场景管理
  - `math/` - 数学库
- `shaders/` - 着色器文件
- `res/` - 资源文件
- `libs/` - 第三方库
