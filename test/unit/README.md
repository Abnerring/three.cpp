# 测试

`JSCollections.tests.cpp` 与三个集合/Symbol/GC 差分程序执行真实 Node 内建，覆盖键身份、顺序、动态迭代、IteratorClose、描述符、回调重入和管理图。GC oracle 单独记录两种 clear 后旧表更早释放的原生策略边界，其余字段严格比较；原始 Node/native 快照输出到构建目录。`JSString.tests.cpp` 验证 StringData、UTF-16 索引、startsWith 与 Symbol.match 顺序。

`WebGLCapabilitiesTests` 及两个差分程序直接导入固定上游，验证 WebGLExtensions/WebGLCapabilities、共享 warn/warnOnce、String/iterator 动态方法，以及四个能力函数的共同闭包保活。能力查询使用注入的命令对象，实际 Vulkan 设备格式与扩展支持需要另行接入。

目录对应 three.js 的 `test/unit/`：模块测试位于 `src/`，入口为 `three.source.unit.cpp`，Catch2、回归数据和跨语言对照工具位于 `utils/`。

`THREECPPTests` 使用仓库中的 Catch2，链接实际 `threecpp` 库。覆盖数学、事件、场景图、相机、场景和共享资源序列化。

`MaterialClipping.tests.cpp` 与 `ThreeJsMaterialClippingParityProbe` 验证原生裁剪数组的共享身份、字段替换后的旧引用、copy/clone/JSON、管理数组操作及 GC。普通 JS 数组和任意元素仍是未实现边界；测试不把快照转换视为共享。

`NativeMaterialsMathTests` 与对应 probe 验证材质 uniform 所需矩阵、颜色、纹理变换及固定 Node/V8 的 sin/cos，覆盖动态 getter/写入顺序、异常、重入、纹理 matrix/elements 别名与原生所有权。新数学单测和既有裁剪数学差分同时运行，避免扩展协议破坏已有行为。

`WebGLMaterialsTests` 与 `ThreeJsWebGLMaterialsParityProbe` 对照固定上游的全部材质 uniform 刷新分支及雾，包括 Physical、动态方法、异常的部分写入、重入、闭包参数保活、共享矩阵临时对象和真实原生材质/纹理。差分的输入通过 stdin 传递；仅失败时向独立临时目录写诊断，不在源码树生成文件。此模块仍是 CPU 刷新协议，完整材质 shader、GPU uniform 上传与采样器绑定需另行接入。

`VulkanClippingTests`、`VulkanClippingLifetimeTests` 使用实际 GPU 检查全局/局部裁剪、交集、相机变换、实例/形变位置、上传回调、缓存、dispose 重入和借用矩阵退役。`compare-vulkan-clipping.mjs <probe> <checkout>` 对照固定上游投影模块、renderer 分支与片元不等式的解析像素结果；没有浏览器 WebGL 驱动对照，也不涵盖尚未实现的阴影 pass 或 alphaToCoverage。

`compare-vulkan-clipping-camera-identity.mjs <lifetime-test> <checkout>` 验证 render 异常后在同一地址销毁并重建相机的缓存身份；实际 Vulkan 投影与像素对照固定上游的两个独立 Camera。相机不可变 id 防止新对象误用已销毁对象的投影。

`JSWeakValueTests` 独立检查弱根、对象/heap/线程身份与原生交叉引用生命周期。`THREECPPTests` 另覆盖 NativeEventLoop 的并发投递、取消、计时、检查点、异常和重入。`NativeEventLoopReentryTests` 在独立进程检查最后根释放时的原生析构重入，将原生析构重入与主单测夹具隔离；不放宽断言。`threecpp.native-event-loop-parity` 与实际 Node 比较正常零延迟 timer/Promise/microtask 顺序，不能据此证明完整 DOM 调度等价。

`JSWeakLifetimeTests` 检查 WeakRef 的任务保活、FinalizationRegistry 的弱目标与 token、cleanup 排队、独立任务执行、异常与重入；差分使用实际 Node `--expose-gc`。`EventDispatcherRuntimeTests` 在独立进程验证类构造与模块根，覆盖同 heap 修改保留、最后应用根释放和新 heap 初始化；链接当前完整核心静态库，不引入 UniformsLib 测试对象。对应 EventDispatcher 差分执行固定上游模块。

`JSWeakMapTests` 独立检查条件图边、长依赖链、native 所有权、析构后重标记、线程隔离和 WebGLProperties 缓存行为。WeakMap 差分使用实际 Node，Properties 差分执行固定上游模块；WeakMap 的 Symbol 键与 iterator 构造尚未接入，不能据此声明完整 ECMAScript WeakMap。该目标与全局渲染 TLS 隔离，以验证最后一个应用根释放后的 heap 生命周期。

`GLBufferAttributeTests` 单独检查 opaque 资源身份、动态字段、构造原型、更新和生命周期。运行时模块根测试同时检查同一 heap 的 GC 后保留状态，以及普通管理图的最后应用根释放后销毁 heap；包含 owning JSValue 的原生 payload 仍有运行时关闭边界。Array.sort/unshift 与 WebGLAttributes 的差分分别执行实际 Node 和固定上游模块，覆盖访问器、holes、异常、重入和调用顺序；这些 CPU 测试不能代替 Vulkan 资源及像素验收。

BufferAttributeRuntime 单测检查上传字段的唯一存储、动态访问器、默认回调身份、Storage 与原生 handle 独立生命周期、Typed/Float16/Instanced 重建及 GC 环。NativeBufferAttributeUploadParityProbe 使用实际 C++ BufferAttribute，直接对照固定上游的 59 个上传、回调、范围、描述符和 remove 场景。

NativeInstancedAttributeProtocol 单测与 NativeBindingBridgeParityProbe 验证实例属性字段、动态描述符、原生 handle 重建和 GC，后者使用真实原生几何/材质/属性对照上游 BindingStates。旧值被原生或管理赋值替换后，测试在不读取管理字段的情况下检查回收。直接 C++ 引用写入仍绕过管理描述符限制；动态访问器应经管理包装调用。

VulkanBufferCommandsTests 独立验证字节命令、错误标记、视图、转换重入和 shutdown；它使用测试内存存储，不冒充 GPU。其 3,402 个字节场景以 Node BigInt 实现规范转换 oracle，另有两个实际 three.js 控制流场景；未运行浏览器 WebGL。真实 VulkanRendererTests 另覆盖非零 view 与哨兵、初次回调改数组/版本/half 标志、无效范围继续、中途异常保留前次写、管理方法替换、原生 handle 重建、dispose 重入及实际像素。

WebGLRenderLists 差分通过 `node --expose-gc` 执行固定模块，检查池化、数组身份、默认/自定义排序、嵌套深度、动态方法、普通函数构造及模块状态。probe 启动时单独验证普通管理图的最后 heap 根释放和新 heap 初始化，主单测只检查同 heap 的状态保留。Info、两个 BufferRenderer 和 Properties 复用构造性夹具，同时保留各自全部算法差分。

`WebGLGeometryObjectsTests`、`WebGLLightsTests`、`WebGLBindingStatesTests` 分别验证几何/帧上传与生命周期、灯光/场景深度状态、绑定缓存和顶点属性命令。差分执行固定上游源，包含动态字段、回调、异常、重入和类型比较；BindingStates 还覆盖枚举期间的增删、遮蔽和原型变化，以及 19 条跨线程入口拒绝路径。注入 GL 命令的 CPU 测试不代表实际 Vulkan 光照、阴影或完整顶点输入已完成。

`WebGLUniformsTests` 与 `ThreeJsWebGLUniformsParityProbe` 验证 uniform 树、35 种枚举、缓存与共享 scratch、纹理默认值、阴影排序、函数元数据、异常/重入及跨线程拒绝；Node 直接导入固定上游 WebGLUniforms。记录 GL 协议不等于实际 GPU uniform 更新。源码不会终止的 sampler 数组输入从 oracle 中明确排除，生产不使用人为循环上限。`ThreeJsArrayConcatParityProbe` 执行实际 Node 内建 concat，覆盖稀疏数组、继承属性、动态长度、constructor 读取、回调与异常，不宣称支持 Symbol.isConcatSpreadable/custom species。

`NativeArrayLifetimeTests` 在独立进程的新线程中验证九种 TypedArray、共享基础原型和 ArrayBuffer 的缓存身份、同 heap 修改保留、最后应用根释放及下一 heap 重新初始化。最后根检查在线程退出前完成，不以 TLS 析构冒充生命周期正确。 `NativeObjectLifetimeTests` 另验证七处原生对象缓存、实际 Material/Texture 管理字段回边的 GC、同 heap 修改和新 heap 独立性。删除方法后的 canonical 缓存检查只保留当前原生共享函数身份，不作为上游方法回收语义已一致的证据。

`ArrayShift.tests.cpp` 与 `ThreeJsArrayShiftParityProbe` 验证长度快照、HasProperty/Get/Set/Delete 顺序、holes/继承索引、失败前的部分写入、借用到普通对象/TypedArray、重入及 GC 中间值保活。Node 执行真实 Array.prototype.shift，保留原缺失方法的失败证据；没有用 splice 替代 shift 的实现或测试。原生包装原型、Symbol/Proxy/BigInt 的剩余边界仍单独记录。

`WebGLClippingTests`、`NativeClippingMathTests` 及两个对应 probe 对照固定上游的全局/局部平面、阴影、材质缓存、共享临时对象、矩阵/平面运算、getter、异常和重入。首次初始化场景在新线程/Node worker 中污染原型数字属性，确认字面量自有属性不触发继承 setter；跨线程入口在复制管理值前拒绝。此处验证 CPU 协议，不代表实际 Vulkan 裁剪已完成。

`VulkanBindingCommandsTests` 使用内存存储验证 VAO、默认值、divisor、共享错误标记、删除后的引用存储及原型污染隔离。`VulkanBindingSmoke` 使用实际 GPU 验证交错/实例输入、默认 UV/颜色、两种 VAO 删除顺序和整数 pointer/浮点 shader 错误恢复。`trace-vulkan-binding-delete.mjs <checkout>` 执行固定上游三个模块，验证删除顺序与后续 setup 命令；该探针不是浏览器驱动像素 oracle。GPU 既有删除和越界断言按源命令与规范修正，仍分别检查保留存储的绿像素、解绑后的无图元以及完整最终释放。

实例几何回归覆盖索引上传回调改变 count 后的下一帧越界、零实例、容量截断、无实例属性、getter/数值转换顺序与恢复绘制。`trace-vulkan-instanced-index-bounds.mjs` 从固定 WebGLRenderer 抽取实例数量分支并调用实际 WebGLIndexedBufferRenderer/Info，验证 `_maxInstanceCount` 条件二次读取、缺失时 Infinity 及先 requested 后 maximum 的转换；GPU 场景另核对像素、回调和未重新上传的恢复行为。

启用 addons 时，`THREECPPAddonTests` 链接 `threecpp::addons`，测试位于 `addons/helpers/`。Vertex helpers 差分直接执行上游 examples/jsm 模块，检查共享资源、普通/交错属性、变换、重入、clone/dispose 和数值边界。WebGLInfo 与两个 BufferRenderer 的差分执行固定上游模块，检查动态 GL 接口调用及统计，不依赖浏览器，也不代替 Vulkan 图像验收。

`ThreeJsParityProbe` / `compare.mjs` 直接比较 three.js 和 C++ 的几何、TypedArray、Morph、材质与边界行为。`ThreeJsCoreParityProbe` / `compare-core.mjs` 比较层级变换、投影与完整 JSON。参考 commit 必须与 `upstream.json` 一致。

`ThreeJsRuntimeParityProbe` / `compare-runtime.mjs` 使用带类型的图追踪，分别保留 undefined/null、NaN/Infinity/负零、键顺序、数组空位、对象别名和循环引用；同时比较直接属性赋值、旋转回调顺序、Object3D 同步、userData 共享引用及错误类别。类型、身份、顺序和整数值精确比较；有限非整数值采用脚本内明确的 `2e-13` 容差。该测试不证明尚未覆盖的动态库语义已完成。

`threecpp.port-audit` 校验语法树提取的边界、全部模块清单、上游基准和清单一致性；测试数据存在不算实现完成证据。

`ThreeJsQueryParityProbe` / `compare-query.mjs` 对照平面、线段、三角形、射线、视锥查询，包括退化、非有限值、负零和 target 别名。`ThreeJsHelperParityProbe` / `compare-helper.mjs` 对照二维包围盒、球/柱坐标、Matrix2 和裁剪平面复制。`ThreeJsRaycastParityProbe` / `compare-raycast.mjs` 对照实际 Mesh 拾取结果及相机生成射线。

`test/unit/utils/generate-threejs-fixtures.mjs` 用锁定 checkout 生成数学回归数据 `ThreeJsFixtures.h`，包括全部 Euler 顺序、反向深度、四元数、颜色空间和伪随机序列。

`VulkanRendererTests` 用真正 Vulkan 图形管线绘制后读回像素，覆盖索引绘制、剔除、深度、透明混合、层级可见性、图层、drawRange/groups、雾、Morph、回调中 clear/readPixels、清屏状态及对象/Group 生命周期。测试需要 Vulkan 1.1 设备。

Vulkan 几何测试已使用实际 WebGLGeometries/WebGLObjects 和 WebGLInfo 管理对象。测试宿主在顶层 render 调用之间显式结束任务并 GC，嵌套 render 不推进任务；原生 renderer 的 render/prune 不替宿主清除 WeakRef 保活。资源释放断言保留，并分别验证当前任务保活和任务结束后的释放。

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

`ShaderLibrary.tests.cpp`、`ThreeJsShaderLibraryParityProbe` 与 `ThreeJsWebGLShaderParityProbe` 验证全部 shader 文本/导出、uniform 默认值与身份、初始化读取与抛错、clone 重入、普通 physical 赋值、动态 for-in 和着色器命令顺序。`generate-shaders.mjs --check` 与 `generate-uniforms.mjs --check` 检查生成内容；不据此宣称已完成 GPU shader 编译。

`WebGLAnimationTests` 和两个 oracle 验证实际固定上游的帧请求、取消、回调重入、异常部分状态、函数元数据和共享闭包保活。context 是注入的命令对象，测试不代表平台显示刷新调度已完成。

颜色管理、Layers/UniformsUtils、ShaderCache 的实际源码 oracle 覆盖共享身份、动态方法和生命周期；ColorManagement 首次 realm 用独立进程检查。Number 固定小数字符串精确比较，非十进制边界单列。filter/species oracle 同时覆盖 map/slice/splice/concat 的共用构造分支，原数组回归保留。

VulkanShaderCompilerTests 调用配置的 glslangValidator，检查链接、诊断、并发和失败恢复；不代表完整 Renderer 已使用新 Program。LayersVulkanTests 检查实际绘制时图层 getter 替换、异常和恢复。
