# three.cpp

按 three.js 源码移植的 C++20 库，渲染后端使用 Vulkan。

参考版本：three.js r187dev / `11738a9d55e502fb484cefffcea3a66dc44e9fe9`，见 [upstream.json](upstream.json)。完整移植仍在进行；已有模块和演示不代表全部 API 或渲染行为已等价。

当前包含数学与空间查询、事件与场景图、相机与计时器、普通/交错属性存储、曲线/路径/形状、几何体、纹理、灯光、传统材质、辅助对象、渲染目标、骨骼/蒙皮/实例化 CPU 对象、线/点/Sprite、LOD、关键帧轨道/动画片段/属性绑定、动画播放与混合及拾取。加载基础包含缓存、管理器、URL 处理、动画/几何/材质解析与原生 Promise；文件/网络传输尚未接入。Vulkan 已有实际离屏图形管线、深度、索引绘制、基础材质、排序、雾、Morph、实例矩阵/颜色与实例 Morph、2D DataTexture 的 map/alphaMap 采样和像素读回。新增 CPU 类不代表其 GPU 路径已接入；各模块仍有待移植和待验证边界。

## 目录

已有代码使用 three.js 的目录层级，同一模块的 C++ 头文件与实现放在一起。

| three.js | three.cpp |
| --- | --- |
| `src/Three.js`、`src/Three.Core.js` | `src/Three.h`、`src/Three.Core.h` |
| `src/<模块>/<类>.js` | `src/<模块>/<类>.h`、同目录 `.cpp`（需要独立实现时） |
| `examples/jsm/<模块>/<类>.js` | 同目录 `.h`、`.cpp`，独立 `threecpp::addons` 库 |
| `src/renderers/shaders/ShaderLib/meshbasic.glsl.js` | 同目录 `meshbasic.vert.glsl`、`meshbasic.frag.glsl` |
| `test/unit/src/<模块>/` | 同目录 C++ 单元测试 |
| `test/unit/utils/` | 测试框架、回归数据和差分工具 |
| `examples/`、`utils/`、`docs/` | 示例、开发工具、文档 |

Vulkan 使用 `src/renderers/VulkanRenderer.h/.cpp`；C++ 基础类型、JSON 和容器适配位于 `src/core/`。CMake、版本锁定和许可证为必要的项目文件。未移植的模块目录不建立空占位。

## 构建

需要 C++20 编译器及支持浮点 `std::from_chars` / `std::to_chars` 的标准库、CMake 3.20+、Vulkan 1.1 运行时、Vulkan 头文件和 glslangValidator。CMake 会检查浮点 charconv 能力。差分测试使用 Node.js v26.5.0（V8 14.6.202.34-node.24）；排序比较调用顺序严格绑定此版本。Node 仅为开发测试依赖。

```powershell
cmake -S . -B build/native -DTHREEJS_SOURCE_DIR=<three.js checkout>
cmake --build build/native --config Debug --parallel
ctest --test-dir build/native -C Debug --output-on-failure
./build/native/bin/Debug/animation_keyframes.exe
./build/native/bin/Debug/vulkan_basic.exe ./build/native/vulkan-basic.ppm
```

SDK 非标准安装时显式传入 `-DTHREECPP_VULKAN_INCLUDE_DIR=<include>`、`-DTHREECPP_GLSLANG_VALIDATOR=<exe>`。CPU 开发可设置 `-DTHREECPP_BUILD_VULKAN=OFF`。消费库使用 `threecpp::threecpp`，渲染使用 `threecpp::vulkan`，公共入口为 `src/Three.h`。

已移植 addons 默认构建为 `threecpp::addons`，通过对应头文件显式使用，例如 `#include "helpers/VertexNormalsHelper.h"`；不加入 `Three.Core.h`。当前包含 VertexNormalsHelper 与 VertexTangentsHelper，CPU 几何更新已有差分验证，线段 GPU 绘制仍待实现。只消费核心库可设置 `-DTHREECPP_BUILD_ADDONS=OFF`。

差分测试严格检查 upstream commit；当前不支持自动漂移到其它 three.js 版本。GPU 测试需要真实 Vulkan 设备，运行失败会如实报告。

已执行的构建、差分与 GPU 检查及平台限制见 [docs/VALIDATION.md](docs/VALIDATION.md)。

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

动画播放示例见 [examples/animation_keyframes.cpp](examples/animation_keyframes.cpp)。`AnimationMixer` 接收原生场景对象，`clipAction` 返回保留身份的动作句柄；每帧调用 `mixer.update(deltaTime)` 更新对象。构建后运行 `animation_keyframes` 可观察位置插值和停止后的原始状态恢复。

旧代码的算法、默认值和边界行为按 three.js 修正，原版接口优先。数学分量直接访问 `v.x`、`color.r`；Euler/Quaternion 直接赋值触发旋转回调；`needsUpdate = true` 触发版本更新。BufferAttribute 使用共享 TypedArray 视图，并支持 Float16 属性；运行时语义使用独立 `JSValue` 基础。已有曲线、路径、形状与 earcut 三角化，以及离散、线性、三次、四元数和贝塞尔插值器。禁止引入 CEF、嵌入式浏览器或 JS 桥接。实施顺序、适配边界及待移植范围见 [docs/PORTING.md](docs/PORTING.md)。

```powershell
node utils/port-audit.mjs <three.js checkout>
```

生成的 [docs/port-status.json](docs/port-status.json) 枚举核心与官方扩展，[docs/api-inventory.json](docs/api-inventory.json) 记录 API 和分析边界。文件存在只说明已经开始移植，不代表实现完整或通过一致性验收。

three.js 原始许可保存在 [THREEJS-LICENSE](THREEJS-LICENSE)。原生算法依赖与许可位置见 [upstream.json](upstream.json)，TimSort 的完整许可见 [ArraySort.LICENSE](src/core/ArraySort.LICENSE)。本仓库原有许可见 [LICENSE](LICENSE)。
