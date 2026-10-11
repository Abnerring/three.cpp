# 测试

`THREECPPTests` 使用仓库中的 Catch2，链接实际 `threecpp` 库。覆盖数学、事件、场景图、相机、场景和共享资源序列化。

`ThreeJsParityProbe` / `compare.mjs` 直接比较 three.js 和 C++ 的几何、TypedArray、Morph、材质与边界行为。`ThreeJsCoreParityProbe` / `compare-core.mjs` 比较层级变换、投影与完整 JSON。参考 commit 必须与 `upstream.json` 一致。

`ThreeJsQueryParityProbe` / `compare-query.mjs` 对照平面、线段、三角形、射线、视锥查询，包括退化、非有限值、负零和 target 别名。`ThreeJsHelperParityProbe` / `compare-helper.mjs` 对照二维包围盒、球/柱坐标、Matrix2 和裁剪平面复制。`ThreeJsRaycastParityProbe` / `compare-raycast.mjs` 对照实际 Mesh 拾取结果及相机生成射线。

`test/math/generate-threejs-fixtures.mjs` 用锁定 checkout 生成数学回归数据 `ThreeJsFixtures.h`，包括全部 Euler 顺序、反向深度、四元数、颜色空间和伪随机序列。

`VulkanRendererTests` 用真正 Vulkan 图形管线绘制后读回像素，覆盖索引绘制、剔除、深度、透明混合、层级可见性、图层、drawRange/groups、雾、Morph、回调中 clear/readPixels、清屏状态及对象/Group 生命周期。测试需要 Vulkan 1.1 设备。

```powershell
cmake -S . -B build/native -DTHREEJS_SOURCE_DIR=<three.js checkout>
cmake --build build/native --config Debug --parallel
ctest --test-dir build/native -C Debug --output-on-failure
```

启用运行时 Validation Layers 的额外检查：

```powershell
./build/native/bin/Debug/VulkanRendererTests.exe --validation
```

缺少 `VK_LAYER_KHRONOS_validation` 会明确失败，不能把普通 GPU 测试通过记为验证层通过。当前测试覆盖已移植场景，不证明整个 three.js 已实现。完整范围见 `docs/PORTING.md`。
