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

1. **FileLoader 已落地并闭环（本条原为待办计划）**：`src/loaders/FileLoader.{h,cpp}` 已实现并注册进核心库，配套单测 3 个用例与差分 `threecpp.threejs-file-loader-parity`（6 个场景）均已通过，差分网络因此从 131 增至 132。宿主边界被收窄为模块级可注入传输，契约冻结在本文件第 10 条。
   - 差分抓到一个真缺陷：上游非 200 错误里报告的是 `response.url` 而非请求的 url（重定向后应指向落点），实现已改为读取传输响应的 `url` 字段并在缺失时回退。这个问题读源码没有发现。
   - 两处有意保留的近似（写在头文件里）：原生传输一次性交付整个 body，因此只触发一个进度事件而非每块一次；`status === 0` 时不输出上游的 console 警告。`blob`/`document` 与非常用 charset 显式报未实现。
   - 后续加载器（`TextureLoader`/`ObjectLoader`/jsm 下的 OBJ、STL、PLY、MTL 等）都应复用这条传输契约，而不要再造宿主边界。
   - 现有基础设施均为 JSValue 图风格：`src/loaders/Loader.{h,cpp}`（`static JSValue prototype()`、`JSValue object()`、`JSMember` 访问器、`JSValue::constructor` + `defineProperty` + `retainForHeapLifetime`）、`Cache`、`LoadingManager` 已移植。
   - 传输层需注入：上游用 `fetch`/`Request`/`Response`/`ReadableStream`/`AbortController`/`ProgressEvent`/`DOMParser`/`TextDecoder`，C++ 无这些宿主对象。建议契约：注入函数接收请求描述、返回响应对象（`status`/`statusText`/`url`/`headers.get(name)`/分块 body），逐块触发 ProgressEvent 形状的回调；`document`（DOMParser）与 `blob` 显式报"未实现"错误而不是假装成功（符合 PORTING.md 的"未实现能力必须显式报错"）。
   - 可差分覆盖：缓存命中、重复请求（loading map 共享回调）、responseType（arraybuffer/text/json）、HTTP 非 200、网络错误、abort、进度事件计数与 `manager.itemStart/itemEnd/itemError` 顺序。JS 侧用桩替换 `globalThis.fetch`（必要时补 `ProgressEvent` 垫片）。
   - **该模式本项目已有先例**：WebGL 相关差分（`compare-webgl-binding-states.mjs`、`compare-webgl-uniforms.mjs`）用一套 "CPU recording transport" 顶替真实 GPU/宿主调用并录制调用序列。FileLoader 的注入式 `fetch` 应沿用同一设计：宿主能力缺失时注入可录制/可重放的传输层，而不是在核心逻辑里分支判断"有没有网络"。先读这两个脚本再动手。
   - 同目录已有 `compare-loader-foundation.mjs`（`Loader.resolveURL`/`Cache` 的 URL 与缓存键边界，含 `blob:`/`data:`/大小写/`\n`/`\u2028`/空串/非字符串）与 `compare-cache-realm.mjs`，写 FileLoader 差分时应复用其 URL/键的边界用例集，不要另起一套。
2. `examples/jsm/postprocessing`（30 个模块）**暂不可做**：需要 renderer 的 `setRenderTarget`/`getDrawingBufferSize` 等 API 面，目前只有 `VulkanRenderer`（`clear`/`render`），没有 `WebGLRenderer` 形状的基类。
3. 其他候选：`examples/jsm/utils/SceneUtils`（9 KB，对象图操作）、`examples/jsm/utils/GeometryCompressionUtils`（13.5 KB，属性打包/解包，纯 CPU）。
4. **验证缺口（已关闭）**：`JSNumberMath::acos` 目前**只有间接覆盖** —— `compare-math-core-trig.mjs` 通过 `Vector3.angleTo`、`Spherical.setFromVector3`、`Vector4.setAxisAngleFromQuaternion` 走到它，`compare-property-mixer.mjs` 只是在 JS 侧用 `Math.acos` 当 oracle；**没有直接的输入网格**（对比：`pow` 27,654 个输入、`sin/cos` 284 个、`tan/asin/atan/atan2` 2,491 个场景、`exp/log/log2` 2,869 个场景）。而 fdlibm 的 `acos` 有 `|x| > 0.5` 的分支切换，正是需要网格才能钉住的地方。补法：在 `MathTrigParityProbe` 里加一类 `acos <x>` 场景（或复用 `compare-curve-extras.mjs` 的 `N`/`O` 形式加一个 `A <x>` 并同步扩 C++ 探针），网格取 `±1`、`±1∓ulp`（`0.9999999999999999`/`1.0000000000000002`）、`±0.5`、`±0.5∓ulp`、`±0`、`±1e-8`、次正规、`|x| > 1`（NaN 路径）、NaN/±Infinity。

5. **已修复并已覆盖（历史记录）**
11. **先核对工具产物，再判断被测对象**：FileLoader 差分前后失败七次，其中实现层真缺陷只有一个（即第 1 条的 `response.url`），其余全是脚手架问题——相对 URL 被 `Request` 拒绝、空格分隔协议无法承载含空格的 body（字段整体错位、加载次数解析成 0）、realm 句柄必须整进程存活（否则 `Cache` 变更抛错）、写入的源码与意图不符、以及 `add_test` 插入把既有测试劈成两半导致 CMake 配置失败。**每次都是"先打印、再判断"当轮推进一层，"先推理"则浪费一轮。**
12. **构建可能根本没发生（本轮最贵的教训）**：CMake 配置自一次错误的插入起持续失败，但失败信息被我自己的输出过滤器挡掉，于是构建图从未更新、构建"成功"却什么都没重编，连续数轮跑的都是旧的探针可执行文件。三条纪律由此确立：**写完文件立刻回读关键行**；**配置与构建的错误输出不得过滤**；**改动探针源文件后删除其可执行文件**（与删 `.obj` 同等对待，本文件开头已记录 MSVC 中文界面 `/showIncludes` 乱码导致 ninja 不跟踪依赖）。
13. **注册新源码后要确认它真的被编译**：`STLLoader.cpp` 曾被提交却从未进入 CMakeLists，构建"成功"而库里没有这个文件，直到测试引用其符号才在链接期暴露。检查方式是让构建日志里出现该源文件的 `.obj`（库源与测试源同等对待）。同一类假通过还有一次：测试注册进了错误的列表，运行器打印 `No tests ran` 却返回 0。
14. **提交信息里的每一项都要在提交前回读到**：`2cf2c96` 的信息声称移植了 STLLoader 的 ASCII 路径，实际只落地了四个匹配器——真正的解析器与 dispatch 被一次编译失败后的 `git checkout` 一起回退了，而随后的提交只因"文件仍是二进制版"而编译通过、测试通过。**勘误**：该提交信息过度声称，ASCII 解析器与 dispatch 并未包含在其中，它们仍是待落地状态。
15. **先造一个能回答二选一的实验，再改代码**：ASCII 路径出现 SIGSEGV 时，把匹配器与解析循环复制成一个**不建几何、不碰 JSValue** 的独立程序运行，结果完全正确，从而把嫌疑一次性缩小到"几何装配"；反过来，一次"测试通过"也可能只是**新用例没插进去**（锚点未命中）导致 ASCII 根本没被执行，因此"通过"必须连同"被测路径确实被执行"一起判断。
10. **FileLoader 传输契约（冻结，实现时照此写，不要再设计）**：上游用 fetch / Request / Response / ReadableStream / AbortController / ProgressEvent / DOMParser / TextDecoder，原生侧没有这些宿主对象，因此把**只有一处**的宿主边界收窄成一个注入函数，其余逻辑照源码逐行移植。
   - 注入方式：模块级 `FileLoader::setTransport(JSValue)`，等价于 `globalThis.fetch`；未注入时 `load()` 显式抛错（说明未配置传输），不静默失败。
   - 传输入参：一个 JSValue 对象，字段 `url`（字符串，已过 `path` 与 `manager.resolveURL`）、`method`（恒为 GET）、`headers`（普通对象，来自 `requestHeader`）、`credentials`（`include` 或 `same-origin`，取自 `withCredentials`）、`signal`（对象，含布尔字段 `aborted`，由 `abort()` 置位）。
   - 传输返回：一个 JSValue 对象，字段 `status`（数值，0 与 200 视为成功，0 时按上游输出 `FileLoader: HTTP Status 0 received.` 警告）、`statusText`、`url`、`headers`（对象，含方法 `get(name)`，用于 `X-File-Size` 与 `Content-Length`）、`chunks`（**有序数组**，每项为 Uint8Array 形状的字节序列；分块按序投递以复现上游逐块读取的行为）。
   - 进度事件：每投递一块后，向上游注册的每个 `onProgress` 回调传入 `{type: progress, lengthComputable, loaded, total}`，其中 `total` 取 `X-File-Size` 优先、否则 `Content-Length`、再否则 0，`lengthComputable` 为 total 非 0，`loaded` 为累计字节数；这与上游 ReadableStream 包装的时序一一对应。
   - responseType：`arraybuffer` 合并各块为一个字节缓冲；`text` 按 UTF-8 解码（`mimeType` 含 charset 时只接受 utf-8，其它标签显式报未实现）；`json` 解码后解析；`blob` 与 `document` 显式抛未实现错误（DOMParser 与 Blob 在原生侧不存在），不得伪造结果。
   - 其余状态机照源码：缓存键为 `file:` 前缀加 url、命中缓存时经 setTimeout 等价的零延迟投递并在其间调用 `manager.itemStart`；重复请求复用 `loading[url]` 回调列表；成功后只在此处写缓存并回调所有等待者；失败时若回调列表已被删除则只 `manager.itemError` 并重抛，否则回调每个 `onError` 再 `itemError`；最后无条件 `manager.itemEnd`。
   - 参数生效时机：`mimeType` 与 `responseType` 在发起请求时各取一次快照，之后修改不影响进行中的请求。
9. **加固差分前先分辨探针是"单侧"还是"两侧配对"**：
   - 单侧（只改 JS 侧即可加固）：摘要统计、场景列表、注入序列等都在 .mjs 里，C++ 探针按既有协议被动接收。例：`compare-camera-utils.mjs`、`compare-loft-decal.mjs`、`compare-mesh-surface-sampler.mjs`、`compare-animation-clip-creator.mjs`、`compare-curve-extras.mjs`、以及 `compare-animation-runtime.mjs` 的摘要统计（第 50 轮只加了比较值计数）。
   - 两侧配对（必须同时改 C++ 探针与脚本，再重编）：`compare-runtime.mjs` 用 `const expected = {}` 加 `emit(name, value)` 收集具名 trace，C++ 探针必须产出**同名的每一项**，否则 `compare` 会报缺失 —— 因此加固它是"C++ 加发射 + JS 加 emit + 重编 + 验证"的完整闭环，不是多写几行。
   - 判断方法：打开 .mjs 看它是否有 `emit` / 固定具名键集合；若 `expected` 的键由脚本枚举而探针只是逐行应答，则通常属单侧（但仍需确认协议）。
8. **流程纪律（本轮踩到，务必遵守）**：
   - 验证命令失败时**不要提交**，即使怀疑失败原因是自己调用姿势不对 —— 先看清输出、再用正确方式复跑一次，通过后才提交。本轮我曾把脚本与 git commit 放在同一条命令里，脚本因我传错了探针二进制名（ENOENT）而失败，我却仍然提交了；事后用注册的 ctest 复跑确认改动本身没问题，但操作顺序是错的。
   - **过滤输出时不要连错误一起滤掉**：同一轮我用只匹配 files changed 的过滤器查看提交结果，恰好把失败信息挡在视野之外。凡是用 Select-String / grep 收窄输出，都要同时保留 error / Error / FAILED / Exception 等模式。
   - 探针二进制名不要凭记忆拼：`build/bin/` 下的可执行名与脚本名并不一一对应（例如动画运行时探针并非 compare-animation-runtime 对应的同名文件）。最可靠的做法是**直接用 ctest 跑注册好的用例**（它自带正确路径），或在 test/CMakeLists.txt 里查 $<TARGET_FILE:...> 的名字。
7. **差分网络的运行时覆盖清单与判断方法（采集于本轮）**：判断"某模块有没有差分覆盖"**不能 grep 导入语句** —— 这个仓库的加载方式至少有三种（字面量 url 调用、为把 three 说明符指向 src/Three.js 而使用的 base64 data URL、以及目录枚举加动态循环），静态扫描必然漏报（曾据此得出"59 个模块无覆盖"的错误结论）。可靠做法：运行 ctest --test-dir build -R parity -E webgl-lights-states -V -j 4，然后采集各探针自报的摘要行。
   - 当前规模（131 个测试全绿，无空转探针）：V8 array sort 1272 cases / 1738420 comparator calls；Addon math 5316 场景；CurveExtras 28176；RectAreaLight 65717 项比较；Advanced geometry 103833 值/身份；JSM shaders 52 模块 / 8029 项；Convex hull 358 场景；SortUtils 748 场景 / 178302 元素；MeshSurfaceSampler 73 场景；BufferGeometryUtils 110 场景 / 15 操作；Teapot 84 场景 / 262656 分量。
   - 加固候选（**按比较量而非场景数排序** —— 场景数不等于覆盖强度，这一条同样适用于本清单本身）：Runtime parity 仅 674 次比较，数量级明显低于同类；Animation runtime parity 15 个场景且**不自报比较数**，强度无法判断，应先让它自报或补足场景。先前按场景数把 Storage parity（35）、InterleavedBuffer parity（43）、Interleaved attribute parity（46）也列为偏薄是**误判**：场景少不等于比较少，例如 Extended runtime parity 只有 7 个场景却做了 15345 次转换，DataUtils parity 只有 24 个场景却覆盖全部 65536 个半精度编码。
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
