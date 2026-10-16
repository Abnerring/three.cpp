# 续接说明（Handoff）

本文件记录当前进度、已探明的事实与下一步，供后续（新上下文）直接使用，避免重复摸索。
最近一次全量验证：核心单测 662 cases / 205,175 assertions、addons 144 cases / 23,937 assertions、
差分网络 131/131 通过（排除已知长耗时用例 `webcpp.threejs-webgl-lights-states`）。

## 一、项目约定（必须遵守）

- 差分验证：探针（`test/unit/utils/parity/*ParityProbe.cpp`，stdin 逐行场景、stdout 逐行 JSON）+ 脚本（`compare-*.mjs`，导入固定 checkout 的上游模块，`assert.deepEqual`，**无容差**）。NaN/-0/±Infinity 统一编码为 `"NaN"`/`"-0"`/`"Infinity"`/`"-Infinity"`。
- `examples/jsm/**` 用裸说明符 `import ... from 'three'`，在固定 checkout 中解析到**预构建产物** `build/three.cjs`，其字符串与 `src/` 不一致（如 `ShaderChunk.meshphong_vert` 空行被剥离）。对照源码语义时用 base64 `data:` URL 把说明符改写到 `<checkout>/src/Three.js`（见 `compare-jsm-shaders.mjs`）；复现模块自身所见实例身份时则让两侧读同一实例。
- MSVC 中文界面导致 CMake 的 `/showIncludes` 前缀乱码，ninja **不跟踪头文件依赖**：改动源文件/头文件后必须删除对应 `.obj` 再构建，否则会静默使用旧对象。
- 文本编辑：**不要**用 PowerShell 往返改写含中文或反引号的文本（曾把 `` `a `` 变成 BEL、`` `t `` 变成 TAB、并引入 BOM）。用 edit/write 工具，或用 `.mjs` 脚本文件而非内联 `-e`。
- C++ 陷阱（已两次踩到）：函数实参求值顺序未定义（MSVC 从右向左），凡是上游按 x、y、z 顺序求值/抽取随机数的表达式，必须显式分步。

## 二、下一步优先级

1. **`src/loaders/FileLoader`**（8.2 KB，可解锁 72 个 jsm 加载器）
   - 现有基础设施均为 JSValue 图风格：`src/loaders/Loader.{h,cpp}`（`static JSValue prototype()`、`JSValue object()`、`JSMember` 访问器、`JSValue::constructor` + `defineProperty` + `retainForHeapLifetime`）、`Cache`、`LoadingManager` 已移植。
   - 传输层需注入：上游用 `fetch`/`Request`/`Response`/`ReadableStream`/`AbortController`/`ProgressEvent`/`DOMParser`/`TextDecoder`，C++ 无这些宿主对象。建议契约：注入函数接收请求描述、返回响应对象（`status`/`statusText`/`url`/`headers.get(name)`/分块 body），逐块触发 ProgressEvent 形状的回调；`document`（DOMParser）与 `blob` 显式报"未实现"错误而不是假装成功（符合 PORTING.md 的"未实现能力必须显式报错"）。
   - 可差分覆盖：缓存命中、重复请求（loading map 共享回调）、responseType（arraybuffer/text/json）、HTTP 非 200、网络错误、abort、进度事件计数与 `manager.itemStart/itemEnd/itemError` 顺序。JS 侧用桩替换 `globalThis.fetch`（必要时补 `ProgressEvent` 垫片）。
   - **该模式本项目已有先例**：WebGL 相关差分（`compare-webgl-binding-states.mjs`、`compare-webgl-uniforms.mjs`）用一套 "CPU recording transport" 顶替真实 GPU/宿主调用并录制调用序列。FileLoader 的注入式 `fetch` 应沿用同一设计：宿主能力缺失时注入可录制/可重放的传输层，而不是在核心逻辑里分支判断"有没有网络"。先读这两个脚本再动手。
   - 同目录已有 `compare-loader-foundation.mjs`（`Loader.resolveURL`/`Cache` 的 URL 与缓存键边界，含 `blob:`/`data:`/大小写/`\n`/`\u2028`/空串/非字符串）与 `compare-cache-realm.mjs`，写 FileLoader 差分时应复用其 URL/键的边界用例集，不要另起一套。
2. `examples/jsm/postprocessing`（30 个模块）**暂不可做**：需要 renderer 的 `setRenderTarget`/`getDrawingBufferSize` 等 API 面，目前只有 `VulkanRenderer`（`clear`/`render`），没有 `WebGLRenderer` 形状的基类。
3. 其他候选：`examples/jsm/utils/SceneUtils`（9 KB，对象图操作）、`examples/jsm/utils/GeometryCompressionUtils`（13.5 KB，属性打包/解包，纯 CPU）。
4. **验证缺口（已关闭）**：`JSNumberMath::acos` 目前**只有间接覆盖** —— `compare-math-core-trig.mjs` 通过 `Vector3.angleTo`、`Spherical.setFromVector3`、`Vector4.setAxisAngleFromQuaternion` 走到它，`compare-property-mixer.mjs` 只是在 JS 侧用 `Math.acos` 当 oracle；**没有直接的输入网格**（对比：`pow` 27,654 个输入、`sin/cos` 284 个、`tan/asin/atan/atan2` 2,491 个场景、`exp/log/log2` 2,869 个场景）。而 fdlibm 的 `acos` 有 `|x| > 0.5` 的分支切换，正是需要网格才能钉住的地方。补法：在 `MathTrigParityProbe` 里加一类 `acos <x>` 场景（或复用 `compare-curve-extras.mjs` 的 `N`/`O` 形式加一个 `A <x>` 并同步扩 C++ 探针），网格取 `±1`、`±1∓ulp`（`0.9999999999999999`/`1.0000000000000002`）、`±0.5`、`±0.5∓ulp`、`±0`、`±1e-8`、次正规、`|x| > 1`（NaN 路径）、NaN/±Infinity。

5. **已修复并已覆盖（历史记录）**
6. **同一类风险的系统排查结果（审计，未修）**：MeshSurfaceSampler 的越界读取修好之后，我把 `examples/jsm` 下全部 17 处 `fromBufferAttribute` 调用点过了一遍，判断哪些可能被越界下标触达（JS 得 `undefined` → NaN，原生则是越界读内存）。结论是**风险收敛到“索引缓冲区损坏”这一种输入**：
   - 安全（下标由 count 循环界定，不依赖数据内容）：DecalGeometry 的非索引分支、ConvexHull（`i < position.count`）、MeshSurfaceSampler（已修）。
   - 依赖索引数据、数据畸形时可越界：DecalGeometry 的索引分支、BufferGeometryUtils（`computeMorphedAttributes` 与顶点合并读取）、VertexNormalsHelper、VertexTangentsHelper —— 这些位置的下标直接来自 index 属性的值。
   - 与 MeshSurfaceSampler 的关键区别：那一处**无需畸形数据**即可触达（顶点数不是 3 的倍数就会走到），因此优先修；这一批需要**索引值超出 position.count** 的畸形几何，严重性低一档但属同一缺陷类别。
   - 修法与已落地的 `readInto` 相同：在读取处按 JS TypedArray 语义判界，越界返回 NaN。若要补测试，场景应构造一个索引值超出 `position.count` 的几何。：`examples/jsm/math/MeshSurfaceSampler` 在"顶点数不是 3 的倍数"的几何上会**越界读写**，而上游不会。复现：一个只有 2 个顶点的 position 属性（`count/3 = 0.67`）。上游 `new Float32Array(0.67)` 经 ToIndex 得长度 0，`build()` 的 `for (i < 0.67)` 仍跑一次，对长度 0 的数组写 `faceWeights[0]` 被**静默忽略**，于是 `distribution` 为空；`_sampleFaceIndex` 读到 `distribution[-1]` 得 `undefined`，`random() * undefined = NaN`，`_binarySearch(NaN)` 因 `start <= end`（0 <= -1）不成立而返回 -1，最后 `_sampleFace(-1)` 用负下标读属性得到 `undefined` → 采样结果是 NaN —— **全程不抛错**。C++ 侧两处不同：`std::vector<float> faceWeights(static_cast<std::size_t>(0.67))` 长度为 0 却仍写 `faceWeights[0]`（**越界写**），且我的 `sampleFaceIndex` 对空分布显式抛 `JSTypeError`（上游此处不抛）。修法：按 ToIndex 语义定长并让越界写被忽略（或先判定 `i < size`），`sampleFaceIndex` 去掉抛错、让 NaN 走到 `binarySearch` 返回 -1，`sampleFace` 对负下标按 JS 的 `undefined` 语义产出 NaN 而不是读内存。补测需要给探针加一个"2 顶点几何"的网格类型。

## 三、已完结的近期工作（勿重复）

- 数学对齐：`src/` 内三角函数与非三角初等函数（`pow/exp/log/log2`）全部走 `JSNumberMath`（fdlibm 适配），差分 2,491 + 921 + 2,869 + 146 场景逐位一致；`Math.sqrt` 与 `Math.sqrt` 同为正确舍入，无需替换。
- `Color` 的四个历史 gamma API 已按 r187 事实删除。
- addons：`jsm/shaders` 全 52 个模块（8,029 项比较）、`jsm/math/{ColorSpaces,SimplexNoise,ConvexHull,MeshSurfaceSampler}`、`jsm/geometries/{Parametric*,BoxLine,RoundedBox,ConvexGeometry,LoftGeometry,DecalGeometry,TeapotGeometry}`、`jsm/utils/{SortUtils,SkeletonUtils,BufferGeometryUtils,CameraUtils}`、`jsm/lights/RectAreaLight*`（65,717 项比较）、`jsm/animation/AnimationClipCreator`（51 场景 / 884 条目）。
- 尚未补齐的记录项：`CreateVisibilityAnimation` 的 duration 0 已纳入并通过（见 VALIDATION.md 最新一段）。
