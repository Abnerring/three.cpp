# three.cpp

按 three.js 源码移植的 C++20 库，渲染后端使用 Vulkan。

参考版本：three.js r187dev / `11738a9d55e502fb484cefffcea3a66dc44e9fe9`，见 [upstream.json](upstream.json)。完整移植仍在进行；已有模块和演示不代表全部 API 或渲染行为已等价。

当前包含数学与空间查询类、EventDispatcher、Layers、Object3D、Scene、Group、相机、BufferAttribute、BufferGeometry、BoxGeometry、PlaneGeometry、Material、MeshBasicMaterial、Mesh、Raycaster。Vulkan 已有实际离屏图形管线、深度、索引绘制、基础材质、排序、雾、Morph 和像素读回。

## 构建

需要 C++20 编译器、CMake 3.20+、Vulkan 1.1 运行时、Vulkan 头文件和 glslangValidator。Node.js 用于直接运行 three.js 的差分测试。

```powershell
cmake -S . -B build/native -DTHREEJS_SOURCE_DIR=<three.js checkout>
cmake --build build/native --config Debug --parallel
ctest --test-dir build/native -C Debug --output-on-failure
./build/native/bin/Debug/vulkan_basic.exe ./build/native/vulkan-basic.ppm
```

SDK 非标准安装时显式传入 `-DTHREECPP_VULKAN_INCLUDE_DIR=<include>`、`-DTHREECPP_GLSLANG_VALIDATOR=<exe>`。CPU 开发可设置 `-DTHREECPP_BUILD_VULKAN=OFF`。消费库使用 `threecpp::threecpp`，渲染使用 `threecpp::vulkan`，公共入口为 `include/Three.h`。

差分测试严格检查 upstream commit；当前不支持自动漂移到其它 three.js 版本。GPU 测试需要真实 Vulkan 设备，运行失败会如实报告。

本轮 Debug/Release 均通过 7 项验证，结果及限制见 [docs/VALIDATION.md](docs/VALIDATION.md)。

## 使用

```cpp
#include "Three.h"

auto geometry = std::make_shared<BoxGeometry>(1, 1, 1);
auto material = std::make_shared<MeshBasicMaterial>(Color(0x4080c0));
auto mesh = std::make_shared<Mesh>(geometry, material);
Scene scene;
scene.add(mesh);
PerspectiveCamera camera(50, 1, 0.1, 100);
camera.position.set(0, 0, 4);
VulkanRenderer renderer({512, 512, false});
renderer.render(scene, camera);
auto rgba = renderer.readPixels();
```

旧代码的算法、默认值和边界行为按 three.js 修正，允许必要的 API 调整。当前保留 `x()`/`setX()` 形式；`needsUpdate = true` 使用属性代理触发版本更新。其余 C++ 适配、全部待移植范围和验证原则见 [docs/PORTING.md](docs/PORTING.md)。

```powershell
node tools/port-audit.mjs <three.js checkout>
```

生成的 [docs/port-status.json](docs/port-status.json) 枚举 upstream 全部源码模块；文件存在只说明已经开始移植，不代表实现完整或通过一致性验收。

three.js 原始许可保存在 [THREEJS-LICENSE](THREEJS-LICENSE)。本仓库原有许可见 [LICENSE](LICENSE)。
