# 测试

目录对应 three.js 的 `test/unit/`：模块测试位于 `src/`，入口为 `three.source.unit.cpp`，Catch2、回归数据和跨语言对照工具位于 `utils/`。

`THREECPPTests` 使用仓库中的 Catch2，链接实际 `threecpp` 库。覆盖数学、事件、场景图、相机、场景和共享资源序列化。

`JSWeakValueTests` 独立检查弱根、对象/heap/线程身份与原生交叉引用生命周期。`THREECPPTests` 另覆盖 NativeEventLoop 的并发投递、取消、计时、检查点、异常和重入。`NativeEventLoopReentryTests` 在独立进程检查最后根释放时的原生析构重入，避免 UniformsLib 全局 TLS 保留 heap 改变该用例的生命周期；不放宽断言。`threecpp.native-event-loop-parity` 与实际 Node 比较正常零延迟 timer/Promise/microtask 顺序，不能据此证明完整 DOM 调度等价。

`JSWeakMapTests` 独立检查条件图边、长依赖链、native 所有权、析构后重标记、线程隔离和 WebGLProperties 缓存行为。WeakMap 差分使用实际 Node，Properties 差分执行固定上游模块；Symbol/iterator 尚未实现，不能据此声明完整 ECMAScript WeakMap。该目标与全局渲染 TLS 隔离，以验证最后一个应用根释放后的 heap 生命周期。

`GLBufferAttributeTests` 单独检查 opaque 资源身份、动态字段、构造原型、更新和生命周期。运行时模块根测试同时检查同一 heap 的 GC 后保留状态，以及普通管理图的最后应用根释放后销毁 heap；包含 owning JSValue 的原生 payload 仍有运行时关闭边界。Array.sort/unshift 与 WebGLAttributes 的差分分别执行实际 Node 和固定上游模块，覆盖访问器、holes、异常、重入和调用顺序；这些 CPU 测试不能代替 Vulkan 资源及像素验收。

BufferAttributeRuntime 单测检查上传字段的唯一存储、动态访问器、默认回调身份、Storage 与原生 handle 独立生命周期、Typed/Float16/Instanced 重建及 GC 环。NativeBufferAttributeUploadParityProbe 使用实际 C++ BufferAttribute，直接对照固定上游的 59 个上传、回调、范围、描述符和 remove 场景。

VulkanBufferCommandsTests 独立验证字节命令、错误标记、视图、转换重入和 shutdown；它使用测试内存存储，不冒充 GPU。其 3,402 个字节场景以 Node BigInt 实现规范转换 oracle，另有两个实际 three.js 控制流场景；未运行浏览器 WebGL。真实 VulkanRendererTests 另覆盖非零 view 与哨兵、初次回调改数组/版本/half 标志、无效范围继续、中途异常保留前次写、管理方法替换、原生 handle 重建、dispose 重入及实际像素。

WebGLRenderLists 差分通过 `node --expose-gc` 执行固定模块，检查池化、数组身份、默认/自定义排序、嵌套深度、动态方法、普通函数构造及模块状态。probe 启动时单独验证普通管理图的最后 heap 根释放和新 heap 初始化，主单测只检查同 heap 的状态保留。Info、两个 BufferRenderer 和 Properties 复用构造性夹具，同时保留各自全部算法差分。

启用 addons 时，`THREECPPAddonTests` 链接 `threecpp::addons`，测试位于 `addons/helpers/`。Vertex helpers 差分直接执行上游 examples/jsm 模块，检查共享资源、普通/交错属性、变换、重入、clone/dispose 和数值边界。WebGLInfo 与两个 BufferRenderer 的差分执行固定上游模块，检查动态 GL 接口调用及统计，不依赖浏览器，也不代替 Vulkan 图像验收。

`ThreeJsParityProbe` / `compare.mjs` 直接比较 three.js 和 C++ 的几何、TypedArray、Morph、材质与边界行为。`ThreeJsCoreParityProbe` / `compare-core.mjs` 比较层级变换、投影与完整 JSON。参考 commit 必须与 `upstream.json` 一致。

`ThreeJsRuntimeParityProbe` / `compare-runtime.mjs` 使用带类型的图追踪，分别保留 undefined/null、NaN/Infinity/负零、键顺序、数组空位、对象别名和循环引用；同时比较直接属性赋值、旋转回调顺序、Object3D 同步、userData 共享引用及错误类别。类型、身份、顺序和整数值精确比较；有限非整数值采用脚本内明确的 `2e-13` 容差。该测试不证明尚未覆盖的动态库语义已完成。

`threecpp.port-audit` 校验语法树提取的边界、全部模块清单、上游基准和清单一致性；测试数据存在不算实现完成证据。

`ThreeJsQueryParityProbe` / `compare-query.mjs` 对照平面、线段、三角形、射线、视锥查询，包括退化、非有限值、负零和 target 别名。`ThreeJsHelperParityProbe` / `compare-helper.mjs` 对照二维包围盒、球/柱坐标、Matrix2 和裁剪平面复制。`ThreeJsRaycastParityProbe` / `compare-raycast.mjs` 对照实际 Mesh 拾取结果及相机生成射线。

`test/unit/utils/generate-threejs-fixtures.mjs` 用锁定 checkout 生成数学回归数据 `ThreeJsFixtures.h`，包括全部 Euler 顺序、反向深度、四元数、颜色空间和伪随机序列。

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
