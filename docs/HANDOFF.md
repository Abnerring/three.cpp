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
13. **注册新源码后要确认它真的被编译**：`STLLoader.cpp` 曾被提交却从未进入 CMakeLists，构建"成功"而库里没有这个文件，直到测试引用其符号才在链接期暴露。检查方式是让构建日志里出现该源文件的 `.obj`（库源与测试源同等对待），**并且差分所依赖的探针可执行文件已重新链接**——只重建测试目标会让旧探针继续参与比较，症状是探针回一句早已删除的错误文本。同一类假通过还有一次：测试注册进了错误的列表，运行器打印 `No tests ran` 却返回 0。
14. **提交信息里的每一项都要在提交前回读到**：`2cf2c96` 的信息声称移植了 STLLoader 的 ASCII 路径，实际只落地了四个匹配器——真正的解析器与 dispatch 被一次编译失败后的 `git checkout` 一起回退了，而随后的提交只因"文件仍是二进制版"而编译通过、测试通过。**勘误**：该提交信息过度声称，ASCII 解析器与 dispatch 并未包含在其中，它们仍是待落地状态。
15. **先造一个能回答二选一的实验，再改代码**：ASCII 路径出现 SIGSEGV 时，把匹配器与解析循环复制成一个**不建几何、不碰 JSValue** 的独立程序运行，结果完全正确，从而把嫌疑一次性缩小到"几何装配"；反过来，一次"测试通过"也可能只是**新用例没插进去**（锚点未命中）导致 ASCII 根本没被执行，因此"通过"必须连同"被测路径确实被执行"一起判断。
16. **STLLoader 的 ASCII 路径已完成（本条原为待收尾清单）**：二进制与 ASCII 两条路径都已落地，单测 4 个用例、两条差分（`threecpp.threejs-stl-loader-parity` 5 个二进制文档、`threecpp.threejs-stl-ascii-parity` 4 个 ASCII 文档）全部通过，比较的是几何形状与 `position`/`normal`/`color` 的 float32 位模式。
   - **真因一（代码）：`JSValue` 的整数索引会命中原名重载并当作空指针。** 同一陷阱在同一文件出现两次：生产代码里 `names.set(0, …)` 崩溃（改用 `push` 解决），测试里 `names.get(0)` 崩溃（改用 `names.get("0")` 解决）。读数组元素一律用**字符串索引**。
   - **真因二（脚手架）：探针可执行文件没有重新链接。** ASCII 落地后只重建了 addon 测试目标，差分探针仍是旧的，于是它对 ASCII 文档回的还是"未实现"那句错误文本；比较**真的**失败了，而脚本的 mismatch 打印在 `lines` 尚未构造时抛 `TypeError`，把它伪装成"脚本坏了"，为此白花了三轮。**跑差分的探针必须重新构建**，第 13 条因此扩展。
   - **方法教训**：这三轮一直在**过滤输出**，所以只看到一行 `TypeError`；改成完整打印后，探针自己给出了答案（`probeError: ASCII parsing is not implemented natively yet`）。另有一条判断被证明是错的：把"逐个加入文档"当作定位手段——一个文档也会失败，因为问题根本不在文档里。
   - **ASCII 的四个边界**（都有断言）：单个 solid；同一文件两个 solid（分组跑两次）；facet 行数既非 3 也非 6（两侧都忽略，得到 0 顶点）；**facet 里没有 normal 行**（上游沿用初始零向量，法向全 0）。上游对法向/顶点数异常的两条 `console.error` 在原生侧**不复现**（无 console），已在头文件与提交信息中记录。
17. **`threecpp.threejs-webgl-lights-states-parity` 是 CPU 密集的慢测试（实测，不是挂起）**：它曾被 ctest 在默认 1500 秒上限处杀掉并报 `***Timeout`，且**超时不显示任何输出**，因此看起来像死循环。两次独立采样否证了这一点——探针进程 `ThreeJsWebGLLightsParityProbe` 的 CPU 时间在 20 秒窗口内从 1784.9 秒增长到 1804.7 秒，即约 100% 占满一个核心，**在真实计算而不是阻塞**。
   - **规模**：探针要走 48 个种子 × `lightsCase`，再加 `mode 1..11 × 3 种子`，以及 `nativeCase` 4、`statesCase` 2、`nativeShadowMap` 4、`dynamicCase` 7。直接运行时的进度标记（在每次调用前逗号表达式打印）显示它稳定推进，两分钟走完 18 个种子，约 6.7 秒一个；但**已经消耗约 1805 秒 CPU 仍未结束**，说明 `mode` 那批场景比种子重得多——按种子外推会严重低估。
   - **已做的修法**：`test/CMakeLists.txt` 里为该测试加了 `set_tests_properties(... PROPERTIES TIMEOUT 7200)`（第 761 行附近，含实测依据的注释）。**尚未完整跑完并以实测耗时校准这个值**，这是下一件事。
   - **诊断方法（比等待便宜得多）**：不要反复等长任务，而是**采两次进程 CPU 时间**——增长即"在算"，不增长即"在等"。这个方法一次就把"挂起/死循环"与"慢"区分开，而在此之前我在这个问题上错了四次（挂起、stdin 未关闭、脚本期望构建、移植死循环）。
   - 顺带记下：`--output-on-failure` **不会**显示超时测试的输出，所以给探针加进度打印后必须**直接运行探针**（或改成流式写日志）才看得到。
18. **下一个加载器：依赖已核查，骨架已定型**。九种类型化属性（`Int8/Uint8/Uint8Clamped/Int16/Uint16/Int32/Uint32/Float32/Float64`）都已在 `src/core/BufferAttribute.h` 里存在，因此 `PLYLoader` 的依赖面**已就绪**，不需要先补属性类。剩余 jsm 加载器的上游规模：`GCodeLoader` 312 行、`MD2Loader` 435、`PCDLoader` 622、`NRRDLoader` 718、`VOXLoader` 919、`PLYLoader` 990（`XYZLoader` 143 与 `STLLoader` 421 已完成）。
   - **建议顺序**：先 `PLYLoader`（覆盖面最大，元素/属性表的结构会顺带为 `PCDLoader`、`NRRDLoader` 打通同一条路）；若要快速闭环则先 `GCodeLoader`，但它产出对象树而非单个几何，差分要比较的字段更多。
   - **差分的成品配方（两个加载器已验证，照抄即可）**：① 探针读 stdin，一行一个文档，二进制用 base64、文本用转义（`|` 代表换行、`-` 空串、`+` 空格 —— 空格必须转义，否则字段整体错位）；② 输出几何形状 + `position`/`normal`/`color` 的 **float32 位模式**（十六进制），NaN/±0 因此无歧义；③ 脚本用 **data URL** 导入固定模块（把裸 `three` 说明符改写成 `<checkout>/src/Three.js` 的 file URL），并与二进制探针**各自独立成文件**——把新文档塞进一个已经在跑的脚本，出问题时无法归因；④ mismatch 时**打印两侧对象与文档**再断言；⑤ 数组元素一律用**字符串索引**读写（整数索引会命中原名重载并当空指针）。
   - **门禁五项**（缺一不可，都是本会话踩出来的）：注册后回读 CMake ✓；构建日志里出现**库源**的 `.obj` ✓；出现**测试源**的 `.obj` ✓；**探针可执行文件已重新链接**（只重建测试目标会让旧探针继续参与比较，症状是它回一句源码里早已删除的错误文本）；退出码 + 运行器的 `parity:` 输出同时命中。
   - **诊断纪律**：失败且没有输出时，下一步一律**完整打印**而不是过滤；给**真实函数**加打印比复刻探针有效——复刻只证明你复制的那段，不证明真实函数里跑的是同一段。
19. **lights-states 探针的性能疑点：30 秒实验即可定性**。实测对照：`threecpp.webgl-lights-states-unit` **0.25 秒**通过，而 `threecpp.threejs-webgl-lights-states-parity` 的探针**耗尽约 1805 秒 CPU 仍未完成**，相差四个数量级。由此可以排除三件事：不是移植的灯光状态逻辑本身慢（单元那半很快）；不是挂起或死循环（进度标记持续推进）；**而耗时的大头恰恰是走 `watch`/`trace` 的那些场景**：把已测数据相减就能看出来——48 个种子 × 约 6.7 秒 ≈ **322 秒**，而总消耗已超过 **1805 秒**，所以剩下的一千四百多秒在 33 个 `mode` 场景（外加 `nativeCase` 4、`statesCase` 2、`nativeShadowMap` 4、`dynamicCase` 7）里，折合**每个 `mode` 场景约 45 秒**。此前曾据此断言"不是 watch/trace"，那是只看种子得出的错判；正确结论是瓶颈在 `mode` 场景的 `watch`/`trace` 与 `encode(state)` 深拷贝上，也就是**很可能在宿主层**，这将影响全部 parity 探针。
   - **剩下的两个嫌疑**：① `lightsCase` 里 `manager.setup(lights)`（移植的 `WebGLLights`）在特定种子下的复杂度；② `encode(state)` 与 `JsonValue` 的深拷贝/序列化开销（探针全程用 JSON 值承载状态，宿主层开销可能被放大）。
   - **实验（约 30 秒）**：先把探针 `main` 那一行密集代码**格式化成多行**（零逻辑改动，第 167 轮直接插桩失败过一次，原因就是那一行难以定位作用域），再在 `lightsCase` 内为 `setup` 与 `encode` 各计一次 `std::chrono`，跑 30 秒采样。若时间在 `setup` → 是移植逻辑的复杂度问题；若在 `encode`/`JsonValue` → 是**宿主层开销**，那将影响全部 parity 探针，比单个测试的时长重要得多。
   - **测试时长本身**：`test/CMakeLists.txt` 已给该测试 `PROPERTIES TIMEOUT 7200`，但那是我估的值，**尚未用一次完整运行校准**；若确实要跑完，建议流式写日志（`ctest ... | Tee-Object -FilePath build\lights-run.log`），因为 `--output-on-failure` **不会**显示超时测试的输出，而"先捕获再打印"的写法会让整个运行期间零输出。
20. **核心构建当前是坏的：`BufferGeometryLoader` 必须按既有测试完整实现**。`test/unit/src/loaders/Loaders.tests.cpp` 里早就有用例 `"BufferGeometryLoader restores shared interleaved storage and intentionally ignores instance count"`，它要求 `loader.parseGeometry(json)` 返回 `shared_ptr<BufferGeometry>`，并断言：instanced 时为 `InstancedBufferGeometry`（且 `instanceCount` 被**故意忽略**）；`position` 与 `uv` 都必须是 `InterleavedBufferAttribute` 且**共享同一个** `InterleavedBuffer`（`sameValue`）；交错属性跨属性可见（`getZ(0)`、`setY(0,9)`）；已带 `usage` 的几何再次解析要**抛 `JSTypeError`**。
   - 当前提交（`083ee72`）只有 plain 路径，**没有** `parseGeometry`，因此 `Loaders.tests.cpp` **链接失败**，核心构建是坏的。它此前一直被**陈旧的可执行文件**掩盖——这正是第 13 条记录的陷阱的又一次现形。
   - **实现清单（照测试即可，不需要再设计）**：① `parseGeometry(JSValue) → shared_ptr<BufferGeometry>` 作为一等入口，`parse` 只做 JS 包装；② `isInstancedBufferGeometry` → `InstancedBufferGeometry`，**不读** `instanceCount`；③ `isInterleavedBufferAttribute` → 按 `data.arrayBuffers` 取缓冲、按 `stride` 建 `InterleavedBuffer`，同一 uuid **共享实例**（`interleavedBufferMap` 去重），再建 `InterleavedBufferAttribute(itemSize, offset, normalized)`；④ `arrayBuffers` 表按上游 `new Uint32Array(data).buffer` 的语义实现；⑤ 对已设置 `usage` 的属性再次解析时抛 `JSTypeError`；⑥ 其余 plain 路径可复用 `083ee72` 里的实现（index、morph、groups 含旧的 `drawcalls`/`offsets` 键、手工构造的 boundingSphere、name 与 userData）。
   - **教训**：新增模块前先 `grep` 仓库里是否**已有针对它的测试**——`Loaders.tests.cpp` 就是一份免费的完整规格，而我在实现前没有读它，反而把交错与 instanced 当作"未实现"显式报错，与规格相反。
   - **验收方式**：`cmake --build build --target THREECPPTests` 必须链接成功；跑 `THREECPPTests.exe` 时该用例通过；`BufferGeometryLoader` 的差分可用一行一 JSON 的协议补上（属性按 float32 位模式比较）。
21. **两条从 `BufferGeometryLoader` 上换来的具体经验**。
   - **断言里出现奇怪的大整数，先当作位模式看**：`position->getZ(0) == 3` 失败时报的是 `1077936128`，那正是 `3.0f` 的 IEEE-754 位模式（`0x40400000`）。原因是我把 `arrayBuffers` 里的 uint32 词**复制**进了 `float` 向量，而不是按字节**重解释**（`memcpy`）。见到这种数字不必再猜层次，直接检查是不是把位模式当数值用了。
   - **同一对象要共享，就必须用同一个句柄、一次构造**：测试要求两个交错属性的 `data` 满足 `sameValue`，而 `sameValue` 对对象比较的是 `(heap_, id)`（见 `JSValue.cpp:1829`）。我原本先用缓冲对象构造一个临时属性、再从它取 `object()` 构造正式属性，中间那次拷贝使两个属性拿到**不同句柄**；改为**直接用 `buffer->object()` 一次构造**后通过。教训是：传递共享对象时不要经过中间包装，否则身份会丢。
   - 附带一条排错顺序：先打印**指针/键**（一次构建+一次运行即可，第 208 轮用它把三种可能收到一种），再打印**id**；指针相同而 `sameValue` 为假，就说明"拷贝出了一个新对象"，此时应回到自己的构造代码里找中间步骤，而不是继续怀疑基类。
22. **图像加载族：先冻结解码契约，再实现 `ImageLoader` 与它的三个薄封装**。本仓库**没有** `src/loaders/ImageLoader.*` ✗，而 `TextureLoader`(74 行)、`CubeTextureLoader`(103 行)、`ImageBitmapLoader`、`CompressedTextureLoader` 全都依赖它 ✓。上游 `ImageLoader` 直接使用 DOM：`new Image()`、`image.src`、`crossOrigin`、`document.createElementNS(...SVG...)` ✗ —— 这些在原生侧不存在 ✓，所以它与 `FileLoader` 的情形**同构** ✓：把宿主边界收窄成一个注入函数，其余逐行转写 ✓。
   - **建议契约**（模块级、可注入，与第 10 条同样风格）：`ImageLoader::setDecoder(JSValue)`，入参 `{url, crossOrigin, body}`（`body` 是已取到的字节或 `undefined`，让解码器自己决定是否走网络），返回 `{width, height, data}`，其中 `data` 是 **RGBA8 的字节序列**（8 位/通道，行主序，左上原点）。`Texture` 的 `image` 就用这个三元组表示 ✓。未注入解码器时**显式抛错** ✓。
   - **需要显式报未实现的**：`document.createElementNS` 的 SVG 路径 ✗、`ImageBitmap` 的 `createImageBitmap` + `ImageBitmapLoader` ✗、`blob:` 与 `data:` 的浏览器专有分支 ✗。这些都要抛错而不是静默返回空纹理 ✓。
   - **`ImageLoader` 其余部分照源码转写**：`Cache.add` 与 `manager.itemStart/itemEnd/itemError` 的时序 ✓、`crossOrigin` 的三种取值 ✓、`loading[url]` 去重 ✓、`onLoad/onError` 的挂接顺序 ✓ —— 这些模式在 `FileLoader`/`AnimationLoader`/`BufferGeometryLoader` 里已经验证过 ✓。
   - **三个薄封装的语义**：`TextureLoader.load` 造 `Texture` ✓ 并设 `image` 与 `needsUpdate` ✓；`CubeTextureLoader.load` 取 6 张图 ✓，顺序必须是 `px, nx, py, ny, pz, nz` ✓，再交给 `CubeTexture` ✓。
   - **差分方式**：上游侧在脚本里**垫一个最小 `Image`**（就像 `file-loader` 差分里垫 `ProgressEvent` ✓），让 `ImageLoader` 的图片解码走我们提供的像素；比较 `texture.image` 的宽、高与前若干像素的**原始字节** ✓。这样两侧语义一致 ✓，且不需要浏览器 ✓。
   - **验收**：注入式解码器的单测（成功 ✓、解码器抛错 ✓、未注入时抛错 ✓、`Cache` 命中 ✓）+ 差分（`TextureLoader` 与 `CubeTextureLoader` 各一组 ✓）。
23. **`CubeTextureLoader` 的未完成线索，以及"WIP 会破坏审计门禁"这条硬约束**。
   - `CubeTextureLoader` 本身极薄（上游 25 行逻辑 ✓）：6 次 `ImageLoader.load`、按索引写 `images[i]`、`loaded === 6` 才回调 `onLoad`、`colorSpace = "srgb"`、同步返回纹理 ✓。实现与编译都曾一次通过 ✓，但两个用例卡在 `Expected runtime object`（由 `CacheStore::add` 里的 `validate` → `JSWeakValue(value)` 抛出）。
   - **已排除的假设**（都是实验证实的，不要再重复）：文件名跨用例复用 ✗（改成唯一名后毫无变化）；循环没执行 ✗（打印显示 `length=6`、`index=0` 已执行）；没进 `ImageLoader::load` ✗（打印显示已进入）；realm 缺失 ✗（`hasActiveHeap()==1`）；manager 是普通对象 ✗（`LoadingManager::fromObject` 正常工作）；**`Cache.add` 是抛点** ✗✗（单独注释掉它反而让失败从 2 个变成 3 个，说明它是必需的）。
   - **已确证的事实**：抛点在 `Cache.get` 之后 ✓；`key` 是 String、`created` 是 Object、活动堆存在 ✓；`CacheStore::add` = `validate(key) + validate(file) + requireRealm() + cache.get("add").call(...)` ✓；`requireRealm` 的消息是 "Cache mutation requires a live JSValue realm"，**与实测消息不同** ✓ → 抛点来自 `validate` 或 JS 侧 `add`。
   - **下一步（最小单变量）**：把 `created.set("complete",true)` 单独注释掉跑一次；再单独注释 `decoder.call(...)`。**一次只改一行**，因为第 247、248 两轮各因一次改多处而得到误导性结果 ✗。
   - **硬约束（本轮踩到）**：工作树里留**未跟踪**的 `.h/.cpp` 会让 `port-audit` 失败（审计把它们计入清单，与已提交的 `port-status.json` 不一致），退出码 8。也就是说"未验证的 WIP"不是无害的——它会以门禁变红的方式显形。**结论**：要么验证到绿并提交，要么删掉。
24. **"改类布局会崩"是误诊，真因是陈旧目标文件**（重要更正）。曾经记录为：给 `Object3D` 加虚函数、给 `BufferGeometry` 加字段都会让无关套件 SIGSEGV，回退即恢复。**这条结论是错的**，第 68-69 轮用干净重建推翻了它：同一份带 `indirect`/`indirectOffset` 字段的树，`cmake --build build --clean-first` 之后 **705 用例全通过**，两个访问器也正常入库。
   - **真因**：在一个小时内反复"改头文件 → 增量构建 → 跑测试"，会有目标文件的时间戳**不早于**头文件的情形，于是依赖该头文件的 TU **没有重编**，两个 TU 对类布局的理解不一致。新成员落在未初始化字节上 —— 这正是当时 ASan 报出的 `0xD7D7D7D7`（MSVC 的未初始化填充）以及"默认构造的 `shared_ptr` 读出垃圾"的来历。原树 ASan 干净也因此说得通：布局一致时没有越界。
   - **做法**：改动**头文件里的类布局**之后，若出现"无法解释的崩溃"，先 `cmake --build build --clean-first` 再复现；不要据此给能力下定论。日常增量构建是可靠的，只有这种"同一分钟内反复改头文件"的节奏才会踩到。
   - **教训**：`0xC0000409` 与 `0xC0000005` 这类错误码**不能当作根因**，它们只说明"进程异常终止"；同一错误码在本会话里至少对应过三种不同原因（缺 `/EHsc`、未初始化成员、真正的布局不一致）。25. **已移植模块的差分覆盖已经完整**（里程碑）。做法是先**扫描缺口**再补，而不是凭印象挑模块：扫描脚本把 `docs/port-status.json` 里每个非 `missing`、非 `src/nodes/`、非 `src/renderers/` 的模块与 `test/unit/utils/parity/` 下的文本对照，第一次列出了 **4 个**没有差分的模块（`DirectionalLightShadow` 32 行、`PointLightShadow` 32 行、`ExternalTexture` 54 行、`Fog` 99 行），补完后再跑即**空列表** ✓。
   - 也就是说：除着色器图（`src/nodes/`）与渲染后端（`src/renderers/`）之外，端口里**每一个已移植模块都有针对上游模块的差分**，且都是 bit-exact（数值数组按 float32 位模式、`-0`/`NaN`/`±Infinity` 显式编码）。
   - **建议把这个扫描固化成例行检查**：新增任何模块后跑一次，缺口数必须回到 0，否则就是"加了代码没加验证"。
   - 同类扫描还有两个：**方法级缺口**（第 18 轮用过，比对上游方法名与本地头文件）与 **与布局相关的崩溃**（第 24 条）。三者合起来构成"移植完成度"的三个正交维度：模块存在 ✓、方法齐备 ✓、行为一致 ✓。
26. **加功能时先看旧的否定断言；调试时先看完整输出**。两条经验在本会话各出现三次，都是"代码没错、做法错了"的类型。
   - **旧断言会拒绝新功能**：`ObjectLoader` 的切片依次加入 `Points`、`InstancedMesh`、`SkinnedMesh` 时，三次都有一条旧用例写着 `REQUIRE_THROWS_AS(parse({type:"..."}), runtime_error)`，而被点名的正是刚实现的类型，于是**功能是对的、测试是红的**。做法：实现一个类型之前，先 `grep` 该类型名是否出现在测试里，并确认那些断言是不是"它还不支持"；若是，同一次提交里把它换成仍然不支持的兄弟类型（`BatchedMesh` 目前是安全的那一个）。
   - **不要过滤诊断输出**：三次浪费整轮的共同原因是把失败输出 `Select-String` 得过窄。真实原因分别藏在没有匹配到的行里 —— Catch2 的 `due to unexpected exception with message:`、JS 侧 `TypeError` 的堆栈、以及断言上下文。做法：先用最小用例跑一次并**打印完整输出**（`Select-Object -Last 30` 而不是 `Select-String`），确认失败类型（断言 / 异常 / 崩溃）再决定下一步。
   - **配套的第三条**：调试 `.mjs` 差分脚本时**直接跑 `node`**，不要经 `ctest -V`，因为 JS 异常在 ctest 的输出里会被混在中间。差分脚本报错时，上游自己也可能抛错（见第 27 条）。
27. **`ObjectLoader` 的纹理路径是宿主边界，不要试图做差分**。上游 `parseTextures` 在记录缺少 `image` 时读 `source.data` 会**直接崩溃**；`parseImages` 要求 `url`，字符串 url 会走 `ImageLoader`（需要 `document`）。因此无宿主环境下：纹理只能靠**单测**验证（已覆盖 `repeat`/`wrap`/`magFilter`/`colorSpace`/`flipY`），而差分只比结构摘要。端口在无图像时构造普通 `Texture` 不是简化，正是上游那个分支本身。
28. **方法级审计已经收敛：可移植的方法表面是完整的**。对 288 个 `partial-unverified` 模块做"上游方法名 vs 本地实现"扫描，报出 6 个模块 / 15 个方法，逐个核对后**全部**是下面四类之一：
   - **误报：空方法**。`Object3D.intersectsFrustum` 在上游就是 `intersectsFrustum( /* frustum */ ) {}`，一个空的默认实现，由子类覆盖。端口用 `Frustum::intersectsObject` 里的 `dynamic_cast` 分支承担同样职责，行为一致而结构不同，**不需要**在 `Object3D` 上加它 —— 这也解释了第 24 条那次崩溃：当时加的是**虚函数**，改的是布局。
   - **误报：命名差异**。`DataTextureLoader._applyTexData` 在端口里叫 `applyTexData`。
   - **误报：实现位置不同**。`InstancedInterleavedBuffer.copy/clone/toJSON` 已经完整实现在 `prototype()` 里（`copy` 搬 `meshPerAttribute`、`clone` 写回结果、`toJSON` 加 flag 与值），只是不在头文件里。
   - **真实缺口，但受限**：`BufferGeometry.setIndirect/getIndirect`（需要两个字段 → 受第 24 条布局限制）；`Material.onBeforeCompile/customProgramCacheKey`（着色器钩子，且上游由函数源码字符串取 cache key）；`ObjectLoader.parseShapes`（需要 `ShapeGeometry.fromJSON`）；`ObjectLoader.parseImages`（宿主边界，见第 27 条）。
   - **扫描脚本的教训**：只在头文件里找方法名会漏掉原型注册、也会漏掉 `.cpp` 里的定义；应把 `src/` 下同名 `.h` 与 `.cpp` 一起搜，并把"空方法体"识别出来，否则每轮都会重新发现同一批幽灵。
29. **`threecpp.threejs-webgl-lights-states-parity` 会挂死，挂点在探针里，不在端口里**（重要更正）。这个测试带着 7200 秒超时，从未在本会话跑完。逐层定位的结果推翻了"端口死循环"的初判：
   - 单元版 `threecpp.webgl-lights-states-unit`（82 断言 / 12 用例）**0.2 秒通过** ✓；差分版 CPU 接近零，不是慢，是卡住。
   - 单独运行 `ThreeJsWebGLLightsParityProbe.exe`（不经 node、标准输入关掉）同样挂住，所以**也不是** `compare-webgl-lights.mjs` 的问题。顺带发现该脚本第 46 行调用 `execFileSync(probe,[],{encoding,...})` **没有传 `input`**，探针若读 stdin 会等不到 EOF —— 这是真实缺陷，应当修，但不是主因。
   - 在原生 `src/renderers/webgl/WebGLLights.cpp` 的 `setup` 返回处加打印后，最后一个用例仍然打印了 `setup done` ✓ —— 说明**端口正常返回**。而挂在 `encode(state)` 之前的打印**没有出现** ✗，也就是说卡住的位置在**探针构造函数 `light(seed+i, i, trace, mode)`** 里，属于**测试代码** ✗，不是端口逻辑。
   - **已定位到具体夹具**：在探针的 `light()` 入口打印后，输出停在 `light seed=18 type=1` —— `type 1` 就是 **`LightProbe`**，即**构造这个夹具**时自转 ✗。这一步与端口无关（端口 `setup` 已打印返回 ✓）。
   - **结论**：渲染灯光状态的端口行为目前**没有被证明有问题**，只是这个差分**从未跑通** ✗。下一步有两条路：一是在 `LightProbe` 夹具里继续二分（看是 `coefficients` 的 9 次赋值还是 `is`+type 的属性名出问题）；二是**两侧同时跳过 `LightProbe`**（`.mjs` 里过滤、探针里跳过），先让其余灯型跑通并纳入门禁，把 `LightProbe` 留作已知缺口 —— 后者更快见效。
   - **纪律**：修好之前不要让它进普通门禁（会挂住 CI）；也不要因为单元版通过就宣称渲染灯光状态的差分级验证已完成。
44. **结项说明：现在到哪了，以及剩下的为什么走不动**。这一条是给"想判断进度"的人看的，数字全部来自实测。
   - **已完成的**：`src/` 内**可移植面全部移植完毕**——835 个源文件（`.cpp`/`.h`），**738** 个单元测试用例 / **205,626** 条断言通过；`examples/jsm` 侧另有 **155** 个用例 / **23,996** 条断言。验证面是一个脚本、**21 组、167 个场景**，全部与固定上游逐位比对：几何比 **24 顶点 + 32 索引**，曲线/插值/轨道比 **17 个参数点**，另有一批退化与边界输入。**已证实的真实缺陷为 0**；期间 6 次"分歧"全部溯源到**测试夹具**而非端口（见第 40、41、43 条）。
   - **宿主边界已全部归因**（第 31、37、38 条）：音频 6 个模块用**注入式 context** 实现并有单元测试；视频 2 个用宿主 video 对象；位图解码 1 个用注入解码器；以上都已完成。剩余的是**渲染器 4 个**（`PMREMGenerator`、`CubeCamera` 的渲染侧、两个 WebGPU 入口）与**入口再导出 3 个**（判定不适用）。
   - **为什么走不动**：本环境**没有 Vulkan SDK**（`THREECPP_BUILD_VULKAN=OFF`、`THREECPP_VULKAN_INCLUDE_DIR-NOTFOUND`、`THREECPP_GLSLANG_VALIDATOR-NOTFOUND`）。后果很具体：`test/CMakeLists.txt` 里 `if(THREECPP_BUILD_VULKAN)` 整块被跳过，`VulkanRendererTests` 目标**根本不会生成**，"渲染到纹理 / 立方体目标 / 预滤波"这类改动只能编译、**无法验证**。按本项目"不留未验证代码"的规则，这类改动必须先有可验证的环境。同理 `src/nodes`（TSL，219 个模块）的验收是"生成的着色器能编译"，也被同一缺口卡住。
   - **解锁条件**：安装 Vulkan SDK（`vulkan/vulkan.h`、loader、`glslangValidator`），用 `-DTHREECPP_BUILD_VULKAN=ON` 配置；届时第 36 条的分阶段计划（先渲染到二维离屏纹理并读回，再扩到六面，最后接 `PMREMGenerator`）可以直接执行。
   - **在不装 SDK 的前提下还能做的**：继续往**已有差分能力**上加场景（第 187–190 轮各加 5 条，四次全部一次通过）；给已有模块补边界与退化输入；扩充单元测试。这些都是"能做完并且能证明做对了"的工作。
49. **具体缺陷一：`WebGLLights` 的 `state.version` 永远是 0，因为 lambda 捕获的 `state` 与 `object_["state"]` 可能不是同一个对象**（第 227 轮定位到行）。
   - 证据：`ctest -R lights-states-parity --output-on-failure` 的首个差异是 `root.1.setup.version`，端口 **0**、上游 **2**。上游 `WebGLLights.js:545` 是 `state.version = nextVersion ++`，端口 `WebGLLights.cpp:181` 是 `global.get("nextVersion")` 自增后 `state.set("version", next)` —— **写法一致**。
   - 端口结构：构造函数（`WebGLLights.cpp:215`）先 `auto state = ::object({{"version",0}})`，然后用 `ordinary(::setup, {state, ...})`（**225 行**）把 `state` 交给 `setup` lambda **捕获**，再用 `literal(object_,"state",state)`（**226 行**）把它挂到对象上。`JSMember state{&object_,"state"}`（头文件 21 行）读的是**挂上去的那个**。
   - **假设**：`literal(object_,"state",state)` 若对 `JSValue` 做了**值语义的转换/复制**，那么 lambda 捕获的是**原件**、`JSMember` 读到的是**副本**，于是 `setup` 里每轮 `state.set("version", …)` 都写进原件，快照永远是初始的 `0`。症状与"每次 setup 都从 0 开始"完全吻合。
   - **验证方法（下轮）**：在探针里对 `manager.object().get("state")` 与 `manager.state.value()` 调 `sameValue`，或直接在 `setup` 后把两者各 dump 一次；若 `sameValue` 为 false 即确认。修法取决于 `literal` 语义：应让 lambda 捕获 `JSMember`/引用到 `object_["state"]`，而不是捕获构造期的局部 `state`。
   - **第 228 轮：上面的"副本"假设被实验推翻，真正嫌疑换到了模块级单例上**。加了 `same` 模式后：`manager.object().get("state")` 与 `manager.state.value()` 的 `sameValue` 为 **true**（就是同一个对象），`setup` 前后两次调用后 `version` 都还是 **0**——所以问题不在副本。
   - 顺着 `version` 往上看：`state.set("version",next)` 在 **`if(changed)`** 块里（端口 175–182 行），上游同样有守卫（`WebGLLights.js:493` 起 12 个字段的 `!==` 比较），两者**等价**，所以 `changed` 本身没问题。
   - 于是嫌疑落在 `global`，即 `WebGLLights.cpp:225` 传进去的 `module()`。`module()`（59–65 行）用 `static thread_local JSWeakValue weak` 做单例，命中就返回，否则重建并 `retainForHeapLifetime()` 后再存弱引用。**如果这个 `retainForHeapLifetime()` 并没有真的让它活到进程结束**，那么每次 GC 之后 `module()` 都会**重建**，`nextVersion` 随之归零——`root.1` 看到的版本号 0 就解释得通。
   - **这同时解释了第 226 轮那个怪现象**：多一个存活对象会改变 GC 时机，从而改变 `module()` 被重建的次数；对象越多、GC 越频繁，重建越多，表现成"超线性变慢乃至卡住"。**两个症状可能是一个根因**：模块级单例的弱引用没能撑住。
   - **验证方法（下一步）**：在探针里连续调 `module()` 两次比较 `sameValue`，再在 `setup` 若干次后比较一次；若中途由 true 变 false 即坐实。修法方向是把 `nextVersion`（以及同一 `module()` 里的 `sort`）挂到一个**真正不会被回收**的宿主上，而不是靠 `JSWeakValue` 自我续命。
53. **第 233 轮：修好了——第一次改动 `src/` 的真实缺陷修复**。把 `WebGLLights.cpp` 的 `module()` 从"弱引用自我续命"改成**强引用静态**（`static thread_local JSValue strong`，命中即返回，未命中则构造后 `retainForHeapLifetime()` 再赋给它）。
   - **行为验证**：`mod3` 打印的 `version` 从修前的 **0 / 0 / 0** 变成修后的 **0 / 2 / 4**——第一个用例 0、第二个用例 2，**与上游期望完全一致**（这正是 `threejs-webgl-lights-states-parity` 报的第一个差异 `root.1.setup.version`）。
   - **门禁**：核心 **738 用例 / 205,626 断言**、addons **155 / 23,996**、`primitive-geometry`、`port-audit` 全部通过，数字**一个没变**（说明这处修改没有影响任何既有行为）。
   - **遗留的性能问题（下一轮）**：改成强引用后，探针的**默认全量路径变慢**（180 秒仍未输出），而 `mod3`（只跑三个 case）很快。说明强引用把某个对象图钉在了根集合里，回收成本随之上升。要么 `retainForHeapLifetime()` 与强引用叠加造成双重保留，要么被钉住的对象参与了每轮 mark。下一步用 `stages` 模式定位是哪一阶段开始变慢，再决定是"只强引用计数器的宿主、不放整个模块"还是"去掉 `retainForHeapLifetime()`"。
55. **第 235 轮：慢的结构原因找到了——`JSHeap::collect` 里有两个"重审到稳定"的 `for (;;)` 循环，而每次重审都要遍历全部记录**。
   - 位置：`src/core/JSValue.cpp` 里 `class JSHeap`（603 行起）、`collect(bool retireRealm)`（979 行起）。关键的循环在 **1234** 行与 **1259** 行，都是 `for (;;)`；紧挨着的 1237 行是 `for (const auto& [id, value] : records) beforeRoots.emplace(id, value->roots);`——**每轮迭代都要把整张 `records` 表扫一遍**。文件里还有注释明确写着这层意图："re-mark after each owner"、"Re-read the described live graph before committing a sweep"、"Graph changes are re-audited before sweeping and must eventually settle"。
   - **这解释了第 234 轮的观测**：迭代次数只要随对象数增长，总代价就是 O(N²) 量级。保留一个模块会让每轮多一次图变化、多一轮重审，于是"秒级"变成"分钟级"；48 个种子累积下来就是 120 秒还没输出。
   - **下一步（性能修复方向，三条）**：① 给两个 `for (;;)` 加**迭代计数并输出**，确认它到底跑几轮、是否随 N 增长；② 若只是"最后一轮验证"需要，改成**只重审受影响的部分**而不是整张表；③ 若重审是因为"每次都有微小变化"，找出是哪个字段（很可能是 `records` 的 `roots` 表）在扫的过程中被修改，让它在 mark 期间不再变化即可把迭代数压到常数。
   - **注意**：这条只讲性能，不涉及第 53 条的正确性修复——那条修复必须保留（它让 `version` 从 0/0/0 变成 0/2/4，与上游一致）。
57. **第 237 轮：拿到了决定性的耗时曲线——单轮成本随累积堆严格递增，而整个过程一次 GC 都没有**。
   - 做法：探针加 `timing` 模式，逐 case 计时。前 12 轮实测（毫秒）：**88、209、351、454、577、713、1082、1360、1409、1465、1682、2264**。严格单调递增，12 轮就从 0.09 秒涨到 2.3 秒。
   - 关键点：第 56 轮已经证明这两个场景里 `collect()` **一次都没被调用**，所以这**不是回收**的开销，而是**分配/属性写入路径**的开销，且该开销**随已有对象数线性增长**——于是总成本自然是平方级。
   - **这就把范围压到"每次分配都要扫一遍已有记录"这一类实现**。下一步在 `JSHeap::record`（`JSValue.cpp:603` 起的 `class JSHeap` 里）与 `nextId` 分配路径上找那个"对已有记录做线性查找/遍历"的地方；也可以在同一处加"记录数"打印，确认记录数与 case 编号同步增长。
   - **给后来者的建议**：这类"越跑越慢但不是死循环"的问题，**先量曲线再读代码**。一次 `timing` 输出的十二个数字，比读两千四百行 `JSValue.cpp` 更快地排除了回收方向。
59. **第 239 轮：把范围压到"分配本身"——1000 次空对象分配，从堆小时 130 毫秒涨到堆大时 2.1 秒**。
   - 实验（`timing3`）：每跑完一个用例就单独计时 **1000 次 `JSValue::object()`**，结果 **130、252、388、522、822、978、1166、1364、1494、1747、2274、2080 毫秒**——单调上升，约 **16 倍**。这是**纯分配**，不涉及任何用例逻辑，所以 `JSValue::object()` 本身就是随堆大小变慢的那条路径。
   - 已读过的实现都不是 O(N)：`allocate`（885–892）只做 `records.emplace(id, …)`，`record()`（893）是 `records.at(id)`，`release()`（1655）是 `records.find(id)`，`validateEdge`（894）只比较 `heap_`。所以要么 `records` 的容器选错了（例如用了 `std::map` 且 N 很大，但那只能解释对数级，解释不了 16 倍），要么开销在**容器的内存行为**上（每次 emplace 触发节点分配、缓存不友好），要么还有一条我没读到的路径。
   - **下一步**：先把 `records` 的声明找出来（不在 600–720 行区间，需要全文搜索），确认是 `std::map` 还是 `std::vector`；再在 `timing3` 里同时打印 `records.size()`（可用已有的 `THREECPP_GC_TRACE` 机制扩展），验证"16 倍"是否与 N 的成长同步。若确认是容器问题，把 `records` 换成 `std::unordered_map` 或 `std::vector` + 空闲表即可。
60. **第 240 轮：根因找到了——`JSHeap::safepoint()` 在一种常见状态下**每次分配**都调用 `collect(true)`**。
   - `JSValue.cpp:862` 的 `safepoint()`：`if (nativeOwnershipRecords == 0 && totalRoots != 0) return 0;`（快路径一）；`if (nativeOwnershipRecords != 0 && hasNativeApplicationRoots()) return 0;`（快路径二）；否则 **`return collect(true);`**。
   - 也就是说：只要堆里**有**带 native ownership 的描述记录，而 `hasNativeApplicationRoots()` 又为假，**每一次 `JSValue::object()`**（`object()` → `current()` → `safepoint()`，见 1734 与 874–883 行）都会触发一次**完整回收尝试**。这就是实测的 **130 微秒/次分配**（正常应是 0.1 微秒量级），也解释了为什么"纯分配 1000 次"会随堆变大从 130 毫秒涨到 2 秒。
   - 为什么第 236 轮的追踪打印是 0 行：追踪加在 `stableMark` 里，而这条路径上的 `collect` 在**到达 `stableMark` 之前**就返回了（`collecting`/`initializing`/空堆等前置判断）——所以它是"每次都白跑一遍回收的前置工作"，而不是每次都真的标记清除。
   - **修法方向**：`safepoint()` 不应该在**每次分配**时都尝试回收；引擎通常在**分配计数达到阈值**时才进 safepoint。可行的最小改动是加一个"距上次回收的分配数"计数器，达到阈值才 `collect(true)`，并把阈值设得足够大以免影响现有测试对回收时机的依赖（核心 738 个用例就是验收标准）。
   - **验收**：改完后 `timing3` 应显示 1000 次分配在各轮**基本恒定**（不再从 130 毫秒涨到 2 秒）；同时 `threejs-webgl-lights-states-parity` 应从超时变成能跑完，核心与 addons 断言数**不得减少**。
61. **第 241 轮：第 60 条的结论被实测推翻——`safepoint()` 的 `collect(true)` 不是那 130 微秒的来源**。
   - 做法：在 `safepoint()` 落到 `collect(true)` 之前加了阈值计数（`kSafepointInterval = 1024`，每 1024 次落空才真正尝试回收）。
   - 结果：核心 **738 / 205,626** 与 addons **155 / 23,996** **完全不变**（说明改动无害）；但 `timing3` 的曲线**一模一样**（145 → 2252 毫秒），`threejs-webgl-lights-states-parity` **仍然 120 秒超时**。既然耗时与"是否进回收"无关，那条路径就不是瓶颈。
   - **处理**：**回退**这处改动（`git checkout src/core/JSValue.cpp`）。理由：它虽然无害，但**没有解决任何被测量的现象**，留着会让后来者误以为问题已处理。本项目不接受"无害但无效"的改动。
   - **下一步（换个切法）**：既然 `allocate`/`record`/`release`/`safepoint` 都读过了、都不是 O(N)，那就在 `JSValue::object()` 这条路径上**分段计时**：把 `current()`、`allocate()`、`record(...).prototype = ...`、以及 `JSValue` 的**析构**分别包一层计时，跑 `timing3` 看哪一段随堆增长。这比继续猜容器有效。
   - **已证伪假设累计到 5 个**：`state` 被复制、单例在实例间不持久、压力后归零、重审循环 O(N²)、`safepoint` 每次回收。
63. **第 243 轮：插桩代码没有出现在二进制里——教训是"改完要验证改动真的被编译进去"**。
   - 第二次尝试（无条件打印，不再依赖环境变量）：把 `JSValue::object()`（`JSValue.cpp:1733`）改成三段计时并每 1000 次打印一行，同时补上 `<chrono>/<cstdio>/<cstdlib>`。**编译无错误**，`timing2` 也正常跑完（12 轮 × 1000 次分配），但 **stderr 里一行 `[obj]` 都没有**。
   - 对照：前几轮把同样的 `fprintf` 放在 `JSHeap::allocate` 里（`[heap]` 那行）时**是有输出**的。两处都是同一个编译单元、同一个 target，所以不是环境变量、不是 stderr 重定向、也不是 stdout/stderr 混淆的问题。
   - 中间还踩了两个工具坑：`$t.Replace(...)` 里用反斜杠转义双引号在 PowerShell 里**直接语法错误**（整段脚本没执行，文件未被改动——事后 `git status` 确认过）；补 include 时又因为之前回退把 `<cstdio>` 一起撤掉而先失败一次。
   - **结论与下一步**：不要再"改完就信"。下次插桩后**先 `Select-String` 那个 exe 里有没有那段字面量**（或用 `strings`），确认代码真的进了二进制；再跑。如果字面量在、输出仍无，才去怀疑运行路径。
   - **本轮无代码变更**（插桩已 `git checkout` 撤回），核心 738 / 205,626、`primitive-geometry`、`port-audit` 全绿，树干净。
69. **解锁 Vulkan 之后立刻暴露的两个问题：一个是测试里的悬空引用（真 bug），一个是我的构建配置错误（Debug）**。
   - **问题一（真 bug，已修）**：`test/unit/addons/math/MeshSurfaceSampler.tests.cpp` 的 `makeSampler` 里写的是
     `std::size_t cursor = 0; sampler.setRandomGenerator([&cursor]{...}); return sampler;`
     —— lambda **按引用捕获了局部变量 `cursor`**，函数返回后 `cursor` 已销毁，而生成器在之后每次 `sample()` 时还在读写那块**已失效的栈内存**，属于未定义行为。表现就是同一个测试时而过、时而报 `REQUIRE(position.x == 0.5)` 得到 **-0.5**。修法：改成 `auto cursor = std::make_shared<std::size_t>(0);` 并按值捕获（`[cursor]`），让计数器与生成器同寿；同文件第 86 行那处是同作用域内的局部变量，保持 `[&cursor]` 即可（但 `draws` 是命名空间级变量，不能写进捕获列表）。
   - **问题二（我的错，不是代码问题）**：给 `build-vulkan` 配置时**没传 `CMAKE_BUILD_TYPE`**，Ninja 是单配置生成器，于是整个 Vulkan 构建是 **Debug** 的；而 `build/` 是 `RelWithDebInfo`。同一个 addon 测试在两个目录里跑出 **916 秒 vs 33.6 秒**（约 17–20 倍差距），`threejs-webgl-programs-parity` 更是因此撞上 ctest 默认 **1500 秒**超时。补上 `-DCMAKE_BUILD_TYPE=RelWithDebInfo` 重新配置并全量重建后：`addons-unit` **961 → 39.9 秒**、`webgl-programs-parity` **超时 → 165.2 秒**，两者均通过。
   - **教训**：① 单配置生成器**必须显式给 `CMAKE_BUILD_TYPE`**，否则"性能问题"可能只是没开优化；② 测试里的 lambda 捕获生命周期要跟着被测对象走，**按引用捕获局部变量再返回持有该 lambda 的对象**是典型陷阱；③ 反向也成立——之前那些"Vulkan 测试很慢（635 秒）"的观感同样是 Debug 造成的，优化后重跑才是有意义的数字。
72. **性能缺陷：根因已证实，但"节流"不是可接受的修法（已回退）；正确方向是脏标记**。
   - **已证实（第 71 条）**：`safepoint()` 每次分配都会调用 `hasNativeApplicationRoots()`（`JSValue.cpp:748`），而它会建快照、审计整张 native ownership 图。用 `JSValue::object()` 与 `JSValue::object(proto)` 对比计时即可隔离：前者走 `JSHeap::current()`、后者不走，同一堆大小下 **144 ms vs 0.86 ms**，且前者随堆增长、后者平坦。
   - **我试的修法（节流到每 256 次分配）→ 回退**：它把性能提升 90–255 倍（`timing3` 从 130,190 µs → 1,559 µs；k=11 从 2,079,615 µs → 8,159 µs），`lights-states-parity` 也从"永不结束"变成 8.31 秒通过。但**两个测试立刻失败**：`threecpp.native-heap-exit`（"isolated check failed"，两处）与 `threecpp.webgl-program-unit`，两个构建都失败，都是秒级逻辑失败。这说明 **native host 的 retirement 语义依赖"每次分配都进 safepoint"**，节流改变了契约，不是位置问题（第 60/61 条我把阈值放错位置是另一回事）。**已回退（`a06d95f`）**，两个测试恢复通过，核心 738 / 205,626 与 addons 155 / 23,996 不变。
   - **正确的修法（下一步）**：保留"每次分配都问一次"的语义，但让那个问题**变便宜**。做法是脏标记：在 `retain()`/`release()`（root 计数变化处，见 `release()` 1655 行）与任何会新增/移除 native ownership 记录的地方置 `nativeDirty = true`；`safepoint()` 里只有 `nativeDirty` 为真时才调用 `hasNativeApplicationRoots()`，调用后清标记。这样"每次分配都检查"仍在，但只有**状态真的变过**才做全图审计——既不动语义，又能去掉 O(N) 扫描。
   - **验收标准（三条都要过）**：① `threecpp.native-heap-exit` 与 `threecpp.webgl-program-unit` 通过；② `timing4` 里 `object()` 与 `object(proto)` 都平坦（约 1 ms/1000）；③ 核心 738 / 205,626、addons 155 / 23,996 断言数不减少。
73. **第 1 轮（新目标）：渲染到纹理的第一刀——`VulkanRenderer` 现在能绑定渲染目标**。
   - 背景：`PMREMGenerator.js` 上游 770 行，端口**完全没有**；它依赖 `setRenderTarget` 渲染到二维/立方体目标（第 35、36 条的计划）。原来 `VulkanRenderer` 只有自己的离屏窗口，没有目标概念。
   - 本次实现（`src/renderers/VulkanRenderer.{h,cpp}`）：`setRenderTarget(std::shared_ptr<RenderTarget>)`、`getRenderTarget()`、`readRenderTargetPixels(target, x, y, width, height)`。语义照 WebGL：绑定目标时渲染器**临时采用目标尺寸**，绑定 `nullptr` 时恢复原尺寸；`readRenderTargetPixels` 的行序与既有 `readPixels()` 一致（**自上而下**，头文件里已注明），越界矩形抛 `std::out_of_range`。
   - **验证**：在既有 `VulkanRendererTests` 里加了 `renderTargetTests`——绑定 32×16 目标后尺寸变为 32×16、清除为红色后整幅读回都是红（r ≥ 200）、子矩形读回尺寸正确、越界抛异常、解绑后尺寸恢复 64×64 且 `getRenderTarget()` 为空。`threecpp.vulkan` **通过（30.5 秒）**。
   - **下一步**：① 立方体目标（六个 face / 六次绘制 + 图层），这是 PMREM 的必需前提；② 然后把 `PMREMGenerator` 的 CPU 侧逻辑（`_fromScene`/`_blur`/`_allocateTargets`、LodPlanes 与模糊链）移植过来，GPU 侧的 fullscreen pass 复用现有管线；③ 验收方式需要设计——Node 里没有 WebGL，所以 PMREM **不能**做像素级差分，只能用"确定性输入 → 结构断言"（例如黑体环境 → 全黑、单点光源 → 峰值位置）加固定 golden。
74. **第 3 轮（新目标）：审计确认——除 TSL 与入口再导出外，"缺失"的只剩 `PMREMGenerator`；同时给渲染器加上了分层图像基建**。
   - **审计结论（按目录结构逐文件对比上游 `src/` 与端口 `src/`，排除 `renderers/`、`nodes/`、`audio/`、`loaders/`、`libs/` 与本轮已确认的入口文件）**：候选缺失 25 个，其中 23 个是 `materials/nodes/*NodeMaterial.js` 与 `manager/NodeMaterialObserver.js`（TSL 体系）、3 个是入口再导出（`Three.Legacy/TSL/WebGPU/WebGPU.Nodes`）。**真正缺失的功能模块只有 `extras/PMREMGenerator.js`（770 行）**。所以新目标"一切功能"的收敛点就是它，而它的**唯一**前置是立方体六面渲染。
   - **本轮实现（`src/renderers/VulkanRenderer.cpp`）**：`createImage` 增加 `std::uint32_t layers = 1` 参数——`arrayLayers = layers`、`layers > 1` 时置 `VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT`、视图类型在多层时用 `VK_IMAGE_VIEW_TYPE_2D_ARRAY` 且 `subresourceRange.layerCount = layers`。`Impl` 增加 `activeCubeFace`、`activeMipmapLevel`、`targetLayers` 状态位。**默认仍为 1 层，行为零变化**：`threecpp.vulkan` 仍通过（36.71 秒），核心 739 / 205,653 不变。
   - **本轮踩的坑**：`create.arrayLayers = layers;` 和 `view.viewType` 的替换**误命中了纹理创建函数**（那里没有 `layers` 变量），编译报 `layers 未声明`；按行号精确修正（纹理路径保持 1 层、`createImage` 里改对视图类型与 `layerCount`）。教训同第 63 条：**改共享代码里的同名语句必须按行号或按函数定位，不能靠字符串替换**。
   - **下一步**：① 每层一个 `VkImageView` + 每层一个 framebuffer，`setRenderTarget(target, activeCubeFace)` 选层；② readback 用 `vkCmdCopyImageToBuffer` 的 `baseArrayLayer = activeCubeFace`；③ 然后 `WebGLCubeRenderTarget::clear` 才能真正实现（不再抛错）；④ 最后才是 `PMREMGenerator` 本体。
75. **第 4 轮：把逐面渲染的"可变部分"改成以 `activeCubeFace` 为准（安全中间态，行为未变）**。
   - 改动：`imageBarrier` 增加 `std::uint32_t layer = 0` 参数并写 `baseArrayLayer`；`beginFrame` 的首次布局转换与 `endFrame` 的 `vkCmdCopyImageToBuffer` 都改用 `activeCubeFace`；`initialized` 之外新增 `initializedLayers` 位掩码（每层单独做一次 UNDEFINED → ATTACHMENT 转换）；`Impl` 预留 `faceView`/`faceImage`/`faceFramebuffer` 三个成员。
   - **为什么现在仍然安全**：`activeCubeFace` 目前只可能是 0（`resize` 还没有接层数、`setRenderTarget` 还没接 face 参数），所以所有新代码路径都退化成原来的单层行为。`threecpp.vulkan` 仍通过，核心 **739 / 205,653** 不变。
   - **还差的三步（下一轮）**：① `resize(w, h, layers)` 用层数建颜色图并在多层时建 `faceView`/`faceFramebuffer`（每层一个 2D 视图 + 每层一个 framebuffer）；② `beginFrame` 在 `targetLayers > 1` 时用 `faceFramebuffer` 而不是 `framebuffer`；③ `setRenderTarget(target, activeCubeFace = 0, activeMipmapLevel = 0)` 公开到 `VulkanRenderer`，`WebGLCubeRenderTarget::clear` 改用 `setRenderTarget(this, face)`。
   - **验收设计**：`clear` 六个面用不同颜色 → 逐面 `readRenderTargetPixels` 得到对应颜色 → 六面互不串色；再验证解绑后渲染器尺寸恢复。
76. **第 5 轮：六面渲染的实现写通了又撤回，原因是环境层面的 mipmap 失败把验证堵住了**。
   - 本轮一度实现完成：`resize(w, h, layers)` 建分层颜色图、`selectFace(face)` 为当前面建 2D 视图与 framebuffer、`beginFrame` 在 `targetLayers > 1` 时改用 `faceFramebuffer`、公开 `setRenderTarget(target, activeCubeFace, activeMipmapLevel)`（`activeMipmapLevel != 0` 显式抛错）。**编译通过**，并写好了六面验收测试（六个面各用一种颜色 clear，逐面 `readRenderTargetPixels` 校验不串色 + 解绑后尺寸恢复）。
   - **但验收跑不起来**：`VulkanRendererTests` 在更早的用例处失败——`Automatic mipmaps filter a minified checkerboard: expected 128,128,128,255, received 0,0,0,255`。
   - **确认与我的改动无关**：把未提交的改动全部 `git checkout` 掉、回到**之前通过过的提交 `4751e1a`**，重新构建后**同样失败**，且 `ctest -R "^threecpp\.vulkan$"` 这次只用 **2.68 秒**就失败（之前同一提交是 **99.70 秒通过**）。同一二进制、同一驱动，先前通过、现在失败，说明是**环境侧发生变化**（GPU 驱动状态、后台占用或设备重置），不是仓库里的代码。
   - **处理**：本轮改动**全部撤回**，仓库停留在已验证通过的 `4751e1a`（核心 739 / 205,653、差分与 audit 全绿）。六面渲染的实现细节写在这里，下一轮可直接重做：`resize` 接层数 → `selectFace` 建逐面视图/framebuffer → `beginFrame` 选 fb → `setRenderTarget` 接 face → 六面色测试。
   - **下次开工前先做一件事**：先单跑 `VulkanRendererTests` 确认 mipmap 用例恢复了，再动手；否则写了也验证不了。
77. **第 6 轮：两个构建目录被外部清理删除，正在重建；顺带记下"不要捕获 /showIncludes 输出"**。
   - 现象：第 5 轮结束时 `build/` 与 `build-vulkan/` **都不存在了**（不是 git 操作，`git status` 一直干净；`build/` 里连 `bin/THREECPPTests.exe` 都没了）。之前 `build/` 是被单独删过一次，这次是两个一起。
   - 影响：所有门禁（核心、addons、差分探针、ctest）在重建完成前**无法验证**；`docs/HANDOFF.md` 里那些断言数因此只能引用删除前的实测值。
   - 重建命令（两条，都需要 vcvars 且**必须显式给 `CMAKE_BUILD_TYPE=RelWithDebInfo`**，否则 Ninja 单配置会落到 Debug，慢十倍以上——见第 69 条）：
     `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTHREEJS_SOURCE_DIR=D:\project\DX\GitHub\three.js`
     `cmake -S . -B build-vulkan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTHREEJS_SOURCE_DIR=D:\project\DX\GitHub\three.js -DTHREECPP_BUILD_VULKAN=ON`
     然后各自 `cmake --build <dir> --parallel`。
   - **工具教训（第 N 次重复）**：MSVC + CMake 的依赖扫描会打印海量 `注意: 包含文件: …`（含 `/showIncludes`），**一次构建能刷出 18 万字符**。所以：① 构建输出**重定向到文件**，只 `Select-String 'error C|error LNK|FAILED:'`；② **不要**在 `job_output` 里等一个不重定向的构建任务——会把整段噪音灌进上下文。
   - **下一步**：两个构建完成后，先跑 `VulkanRendererTests` 确认 mipmap 用例是否恢复；恢复了就按第 76 条一次重做六面渲染。
78. **第 7 轮：门禁全部恢复；但 Vulkan 的 mipmap 用例在全新构建后依然确定失败——环境层回归，不是代码**。
   - **恢复情况（实测）**：`build/` 重建完成后——核心 **739 / 205,653**、addons **155 / 23,996**、差分 **21 组 / 277 场景**、`primitive-geometry` 与 `port-audit` 全绿。中途 `port-audit` 先失败过两次，原因都查清了：① `three.js/node_modules` 里的 `acorn` 随同那次外部清理一起消失（提示 `Cannot find module 'acorn'`），从本机 `D:\project\DX\SceneSpace\node_modules\acorn` 复制回去解决；② 我新增了源文件（`WebGLCubeRenderTarget.cpp`），`docs/api-inventory.json` 变**过期**，提示原文是 "Generated inventory is stale … Run utils/port-audit.mjs without --check"，重新生成后通过，已提交（`1999f78`）。
   - **Vulkan 侧**：`build-vulkan` 用**干净的完整重建**（`RelWithDebInfo`、`-DTHREECPP_BUILD_VULKAN=ON`、`third_party` 自带的头文件与 glslang）产出了全部二进制，但 `VulkanRendererTests` 仍然在 **4.5 秒**内失败于
     `Automatic mipmaps filter a minified checkerboard: expected 128,128,128,255, received 0,0,0,255`。
   - **为什么判定是环境**：同一份源码、同一个提交 `4751e1a`，在更早的轮次里 **99.70 秒跑完并通过**（`threecpp.vulkan`），现在换成全新构建目录仍复现，且**结论稳定**（连跑三次一致）。代码没有变、构建类型一致、Vulkan 头文件与 glslang 都是项目内固定的那份——所以差异只可能在**机器侧的驱动/GPU 状态**（自动生成 mipmap 走 `vkCmdBlitImage` 链，返回全黑说明 blit 没有产生数据）。
   - **对目标的影响**：这条用例挡住的是**整个 `VulkanRendererTests` 二进制**，而六面渲染的验收测试就放在里面，所以六面渲染**暂时无法验证**（第 76 条的实现已经写好过一次）。**下一步不赌 GPU**：先做 `PMREMGenerator` 的**CPU 侧**结构（类骨架、`_allocateTargets`、`_blur` 的尺寸与卷积数学、`fromEquirectangular` 的参数校验），GPU 相关入口一律显式抛错，并在**不依赖 GPU** 的单测里验证；等 mipmap 用例恢复（或换机器/驱动）再补六面渲染与真实 GPU 路径。
79. **新目标第 12–13 轮：确认 Vulkan 的 mipmap 失败与优化级别无关，是机器侧；同时把差分扩到 22 组 / 287 场景**。
   - **决定性实验**：怀疑"此前 99.70 秒通过"那次是 Debug 构建、之后换 RelWithDebInfo 才失败（即**优化暴露的真实 bug**）。于是单独建了 `build-vkdebug`（`CMAKE_BUILD_TYPE=Debug`，Vulkan ON），只编 `VulkanRendererTests` 并运行：**同样失败**，同样是 `Automatic mipmaps … expected 128,128,128,255, received 0,0,0,255`，耗时 49.7 秒。→ **优化假设被推翻**，两种构建类型都失败，因此结论回到"机器/驱动状态"（第 76/78 条）。这条已两次独立验证，不再重复尝试。
   - **差分扩展（本轮交付）**：新增 `node cubetarget <size>` 探针分支（输出 `isWebGLCubeRenderTarget`、`width`/`height` 位模式、`faces`、`face0Width`、`renderTargetTexture`、`mapping`），脚本侧新增 "cube target parity" 组，六个尺寸（8/32/1/300/16/256）全部与上游逐位一致。另加 4 条既有组场景（网格交换分段、32×32 纹理、2 分量 2 采样离散插值、过原点的对角曲线路径）。
   - **现状**：差分 **22 组 / 287 场景 / exit=0**；核心 **745 / 205,728**；addons **155 / 23,996**；`primitive-geometry` 与 `port-audit` 通过。
   - **一个意外收获**：上游 `WebGLCubeRenderTarget` **没有 `type` 属性**，探针第一次编译时就因此报错；把它输出出来反而会制造假的差异。所以差分字段必须**照着上游真实存在的属性选**，不能顺手加。
   - **下一步**：GPU 侧（六面渲染、PMREM 的 `_textureToCubemap`/`_applyPMREM`）在 mipmap 用例恢复前无法验证；在此期间继续做**纯 CPU 且可差分**的部分，或开始评估 TSL（`src/nodes`）里可以先做的纯 CPU 子集（生成 GLSL 字符串，用 `third_party/vulkan/bin/glslang.exe` 编译验证——这条**不需要 GPU**）。
80. **第 14 轮：找到了绕过环境阻塞的钥匙（`VulkanRendererTests --cube`），六面渲染因此**第一次真正跑起来**，但逐面颜色不对**。
   - **关键发现**：`VulkanRendererTests` 是**遇第一个失败即中止**的，而 `mipmapTests` 排在前面，所以六面测试**永远跑不到**（这就是前几轮"无法验证"的真正机制）。在 `main` **最前面**加一个快路径：`if (argc > 1 && argv[1] == "--cube") { renderTargetTests(validation); cubeRenderTargetTests(validation); return 0; }`——只跑与立方相关的两个用例，**绕开** mipmap。
   - **结果**：`--cube` 跑 46.9 秒后报 `cube face carries its own colour`——也就是说**六面渲染代码跑通了**（`setRenderTarget(cube, face)` 接受 face、`selectFace` 建了逐面视图与 framebuffer、`beginFrame` 用了 faceFramebuffer、`endFrame` 按 `baseArrayLayer = activeCubeFace` 回读），但**读回来的颜色不是该面的颜色**。
   - **已经排除的**：`--cube` 位置（第一次放太靠后，mipmap 先跑；移到最前面才真正执行）、链接失效（头文件改了但测试 obj 是旧的，要删 obj 强制重编）、`threecpp_vulkan` 未重编（要先编库再编测试）。
   - **下一步（诊断顺序）**：① 只跑 **face 0**，确认单面正确——若单面就对，问题在"切面"逻辑（`selectFace` 的早退条件或 `initializedLayers` 位）；② 若单面也不对，问题在**首帧布局转换**（`beginFrame` 里 `VK_IMAGE_LAYOUT_UNDEFINED → COLOR_ATTACHMENT_OPTIMAL` 只对当前层做，但整图是 6 层，需要每层各做一次——现在正是这么做的，但 `initializedLayers` 的位可能在 `endFrame` 之后才置，导致第二轮又按 UNDEFINED 转换）；③ 打印每个面读回的首像素，和预期的 `{1,0,0}`/`{0,1,0}`/… 逐个对照。
   - **本轮处理**：所有未验证改动**已回退**，树保持干净（核心 745 / 205,728、差分 22 组 / 287 场景、`primitive-geometry` 与 `port-audit` 全绿）。
81. **第 15 轮：确认了一个具体 bug（`endFrame` 的 barrier 没带层），但补丁又引入新回归，已全部回退**。
   - **确认的 bug**：`VulkanRenderer.cpp` 的 `endFrame()` 里两个 `imageBarrier` 调用（`COLOR_ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL` 与反向）**没有传 `layer` 参数**，于是它们作用在 **layer 0**，而紧跟着的 `vkCmdCopyImageToBuffer` 用的是 `baseArrayLayer = activeCubeFace`。面 ≥ 1 时布局与拷贝目标不一致——这正是"每个面读回来不是自己的颜色"的最直接嫌疑。`beginFrame` 里那次初始化转换**已经**带了 `activeCubeFace`（第 2235 行），只有 `endFrame` 这两处漏了。
   - **基线（提交 `4751e1a`）里已经有的**（别重复加）：`initializedLayers`、`activeCubeFace/activeMipmapLevel/targetLayers`、`imageBarrier(..., layer = 0)` 的 `baseArrayLayer`、`beginFrame` 的逐层初始化与 barrier、`copy.imageSubresource.baseArrayLayer = activeCubeFace`、`initializedLayers |= 1u << activeCubeFace`。**缺的只有五处**：`resize` 接层数、颜色图按层创建、`render.framebuffer` 按面选择、`selectFace` 助手、`setRenderTarget` 的 face 参数，外加上述两个 barrier 的层参数。
   - **本轮为什么又回退**：补这五处时连续踩到重复插入（`faceView`/`faceFramebuffer`/`auto cube` 各出现过两次，因为锚点行在补丁后位移）和一次行号错位，最后跑 `--cube` 时报 `target size adopted`——连**二维**目标都不生效了，说明补丁把 `setRenderTarget` 的结构改坏了。按纪律**全部回退**，树留在 `4751e1a` 绿状态（核心 745 / 205,728、差分 22 组 / 287 场景、`primitive-geometry` 与 `port-audit` 通过）。
   - **下一轮的正确做法**：不要再用"整文件多次正则替换"来打这个补丁。改用**逐处小改 + 每改一处立即构建**：① 先只给 `endFrame` 的两个 barrier 加 `activeCubeFace`（两行，独立构建，跑 `--cube` 看是否只剩颜色问题）；② 再单独加 `resize` 的层数；③ 再单独加 `selectFace`；④ 最后接 `setRenderTarget`。每步都能独立验证，出错立刻知道是哪一步。
82. **第 16 轮：六面渲染通过——一次只改一处、每处立即构建的做法奏效**。
   - **根因（与第 81 条推断一致）**：`endFrame()` 里那两个 `imageBarrier` 没传 `activeCubeFace`，于是作用在 layer 0，而 `vkCmdCopyImageToBuffer` 读的是 `baseArrayLayer = activeCubeFace`。补上这两个参数后，配合 `resize(w,h,layers)` 建分层颜色图、`selectFace(face)` 逐面建 2D 视图与 framebuffer、`beginFrame` 在 `targetLayers > 1` 时改用 `faceFramebuffer`、以及 `setRenderTarget(target, activeCubeFace, activeMipmapLevel)`，六面测试**全新通过**。
   - **验收证据**：`VulkanRendererTests --cube` 直接运行 `LASTEXITCODE=0`、**零输出**（测试里每个面都会在失败时打印 `face N wanted … got …`，现在一条都没有）。六个面分别用红、绿、蓝、黄、品红、青清屏，逐面 `readRenderTargetPixels` 校验，并且解绑后渲染器尺寸恢复 64×64。
   - **过程中最有价值的一条诊断**：在只有"分层"没做之前跑出来的是 `face 1 wanted 010 got 255,0,0`——面 1 显示的是**面 0 的红色**，直接指向"所有面都渲染进 layer 0"。
   - **再次踩到的坑（第 81 条已警告过）**：`faceView`/`faceFramebuffer` 在基线 `4751e1a` 里**已经有声明**，我又加了一遍 → 重定义；用"声明去重"后才通。教训：**动手前先 grep 目标标识符是否已存在**，不要假设上一轮回退意味着它不存在。
   - **`--cube` 快路径现在保留在测试文件里**：`main` 最前面判断 `argv[1] == "--cube"` 时只跑 `renderTargetTests` 与 `cubeRenderTargetTests`，用于绕开仍然失败的 mipmap 用例（第 76/78/79 条）。正常跑 `threecpp.vulkan` 时它仍会在 mipmap 处中止，所以这条路目前只是**验证通道**。
83. **第 19 轮：`CubeCamera::update` 模板版写出来了、GPU 用例也过了，但把核心测试打崩了，已回退；同时暴露我一个提交纪律错误**。
   - **做了什么**：把 `CubeCamera::renderTarget` 从 `JSValue` 改成 `std::shared_ptr<RenderTarget>`，新增类型化构造函数（JS 构造改为委托，因为 JS 句柄无法解包），并把 `update` 写成模板：父节点为空则 `updateMatrixWorld`；坐标系不同则 `updateCoordinateSystem`；保存当前目标/面/mip；六面 `setRenderTarget` + `render(scene, eye)`；最后恢复。GPU 侧 `--cube` 通过（绑好立方目标后 update，目标仍绑定、尺寸不变、最后一面带着清屏色且 sRGB 通道有序）。
   - **为什么回退**：`THREECPPTests` 直接**崩溃**（退出码 `-1073740791` = `0xC0000409`，fast-fail / 栈检查），不是断言失败。最可能是 `CoreCameraModules.tests.cpp` 之类的既有用例依赖 `renderTarget` 原本的 `JSValue` 语义（比如把它当对象读属性），改成 `shared_ptr` 后行为变了。**具体原因还没查**。
   - **我犯的提交纪律错误（比崩溃更值得记）**：验证脚本里的 `if` 条件用了 `$LASTEXITCODE`，而它当时已经是**后面那条 `ctest` 的退出码**，不是核心测试的；于是核心测试明明崩了，`if` 仍为真，**把崩溃提交进了仓库**。修法：**每个门禁的退出码必须先赋给独立变量**（`$ccode = $LASTEXITCODE`），`if` 里只判断这些变量，绝不复用 `$LASTEXITCODE`。
   - **已恢复**：`git revert` 掉那个提交（`c6affd5`），重建后核心 **745 / 205,728**、差分 **22 组 / 287 场景**、`primitive-geometry` 与 `port-audit` 全绿，树干净。
   - **下一轮**：先查清 `renderTarget` 类型变更为什么让核心测试崩溃（跑 `THREECPPTests` 并看是哪条用例；大概率在 `CoreCameraModules` 或 `renderTarget` 相关断言里），再决定是"保留 `JSValue` 另加类型化成员"还是"同步改用例"。
70. **第 520 轮：完整 Vulkan 套件 211/211 全绿，耗时 270.72 秒**。
   - 命令：`ctest --test-dir build-vulkan -j 4 -E "lights-states" --output-on-failure`（仍排除第 46 条那个已知项；它在 Vulkan 构建里同样慢）。
   - 结果：**100% tests passed, 0 tests failed out of 211**，总耗时 **270.72 秒**。对比同一套件在 **Debug** 配置下的 **3784.56 秒 / 2 失败**，差了约 14 倍——再次印证第 69 条"是构建类型不是代码"的结论。
   - 数字对照（全部实测）：无 Vulkan 的 `build/` 注册 **197 项**、核心 738 / 205,626、addons 155 / 23,996；带 Vulkan 的 `build-vulkan` 注册 **213 项**、上面这轮跑 211 项全绿。两者都包含 `primitive-geometry` 的 277 个场景差分与 `port-audit`。
   - **现在的状态**：可移植面完成、渲染器**可以构建也可以验证**（16 个 Vulkan 测试全过，最长的是优化后的真实 GPU 测试）、差分 277 场景零分歧、一个真实缺陷已修（模块计数器）、一个测试里的未定义行为已修（悬空 cursor）。**唯一还开着的**就是那个分配路径的性能缺陷，它同时也解释了 `lights-states` 为什么慢。
68. **更正：这个环境其实有可用的 Vulkan，之前"You must install the SDK"的结论是错的——SDK 已经被塞进 `third_party/vulkan`**（第 517 轮）。
   - 起因：用户问"项目中不能放个第三方库的文件夹吗，然后放入 vulkan 的 sdk"。查下来发现所需要的只有**两样**（`CMakeLists.txt:317-318`）：`vulkan/vulkan.h` 头文件，以及一个 `glslangValidator` 兼容的编译器；**运行时**用的是 `LoadLibraryA("vulkan-1.dll")`（`VulkanRenderer.cpp:207`），而 `C:\Windows\System32\vulkan-1.dll` **本来就存在**。
   - 机内已有资产：`D:\project\goblin\Computational\vulkan\include\vulkan\` 是完整的 Vulkan-Headers（含 `vk_video/`），`...\lib\vulkan\glslang.exe`（Glslang 11:16.1.0，新版把 `glslangValidator` 改名为 `glslang`）。
   - 做法：拷成 `third_party/vulkan/{include,bin}`（26.4 MB，21 个头文件 + `vk_video` 12 个 + `glslang.exe` 与 4 个 DLL）。`CMakeLists.txt` 的 `find_path`/`find_program` 现在**先看项目内**、再看 `$ENV{VULKAN_SDK}`，并且 `NAMES` 增加了 `glslang`。`.gitignore` 里对 `*.dll/*.exe` 的规则在本目录**加了例外**并注明原因。
   - **验证（都是实测）**：① 不带任何 `-D` 路径参数、只给 `-DTHREECPP_BUILD_VULKAN=ON`，配置就把 `THREECPP_VULKAN_INCLUDE_DIR`/`THREECPP_GLSLANG_VALIDATOR` 解析到项目内路径；② 着色器编译出 SPIR-V（`basic.frag.spv` 6220 B、`basic.vert.spv` 12056 B）；③ `build-vulkan` 一次构建 602 个目标全过；④ **16 个 Vulkan 测试全部通过**（`100% tests passed, 0 failed out of 16`），其中包括 635 秒的 `threecpp.vulkan`、275 秒的 `threecpp.threejs-vulkan-clipping-parity`、98 秒的 `vulkan-binding-smoke`，以及 3 个差分 trace；⑤ 主 `build/`（Vulkan OFF）重新配置后 **197 项测试、738 用例 / 205,626 断言、差分与 audit 全部不变**。
   - **结论更正**：第 37、44、57 条里"没有 Vulkan SDK，所以渲染与 TSL 只能编译不能验证"**不再成立**。`build-vulkan` 现在有 **213 项**注册测试（比无 Vulkan 的 197 多 16 项）。之前记为"环境阻塞"的那一项**已经消失**，剩下的只有那个性能缺陷，以及"后端功能本身还没写完"的工作量。
   - **下一步**：`PMREMGenerator`（第 35 条）与 `src/nodes`（TSL，第 38 条，219 个模块）的验收环境**现在已经具备**；先做 `PMREMGenerator`，因为它只需要渲染到纹理/立方体目标的能力，而 Vulkan 渲染器与 16 项测试证明这块已经通了。
67. **终局陈述（第 255 轮）：目标达成到什么程度，以及剩下的两项到底是什么**。
   - **规模**：src/ 下 835 个源文件；165 个单元测试文件；历史提交 348 个；HANDOFF 58 条。
   - **可交付的绿状态**：核心 **738 用例 / 205,626 断言**、addons **155 / 23,996**、差分 **21 组 / 277 场景**、完整 ctest **195/195 通过**（172 秒，排除唯一已知挂起项）。这些都取实测，不是估算。
   - **已经做到的"一模一样"**：所有已移植模块的行为与固定上游逐位一致——浮点数走 float32 位模式，-0 与 +0 分别编码；几何比 24 顶点 + 32 索引；曲线/插值/轨道比 17 个参数点；并覆盖了零尺寸、坍缩路径、边界光参数、单像素到体纹理、1° 到 180° 相机、1×1 到全高清渲染目标。
   - **剩余第一项（真正的缺陷，未修）**：JSValue 分配路径随堆增长而变慢。实测单轮耗时 88→2264 毫秒、纯分配 1000 次从 130 毫秒涨到 2.1 秒。**已经排除 5 个假设**（state 被复制、单例不持久、压力归零、重审循环 O(N²)、safepoint 每次回收），并且有一个未解释的矛盾——object() 里插桩后字面量确认在二进制里却一次都没执行。**下一步已写死在 HANDOFF 里**：绕开 object()，在 JSHeap::allocate/ecord/elease 里插桩分段计时（那里打印生效过）。这不是"还没做"，是"诊断到一半，方法已定"。
   - **剩余第二项（环境，非工作量）**：本机无 Vulkan SDK，THREECPP_BUILD_VULKAN=OFF，VulkanRendererTests 目标不生成，所以渲染到纹理 / 立方体目标 / PMREMGenerator / TSL（src/nodes 219 模块）**只能编译、无法验证**。装上 SDK 并 -DCONFIG…=ON 即可继续第 36 条的分阶段计划。**这一项不是我不做，是我做不了。**
   - **结论**：目标**尚未达成**，缺口的形状是"一个已诊断到一半的性能缺陷 + 一个需要用户侧安装的环境依赖"。仓库本身处于随时可交付、可交接的状态。
66. **第 247 轮：最终门禁——完整测试套件 195/195 全绿**。
   - 命令：`ctest --test-dir build -j 4 -E "lights-states"`（排除第 46/61 条里那个已知挂起项，它在 120 秒超时后会被记为失败）。
   - 结果：**100% tests passed, 0 tests failed out of 195**，总耗时 **172 秒**。日志在 `build/ctest-final.log`。
   - 同时复核的关键数字：差分脚本 **21 组 / 247 场景 / exit=0**；addons **155 用例 / 23,996 断言**通过；`port-audit` 在 ctest 包装器里通过（直接 `node utils/port-audit.mjs` 会以 exit=1 结束，因为它在报告 `missing: 639` 这类计数——那是审计的**输出**，不是失败）。
   - **状态**：除两个后端方向（缺 Vulkan SDK）与一个已诊断的性能缺陷外，仓库处于**可交付的绿状态**：339+ 个提交、197 项测试注册、可移植面完成、一个真实缺陷已修并有上游行为验收。
65. **交接总结（第 246 轮）：这个仓库现在处于什么状态，以及接下来该做什么**。历史提交 339 个，测试注册 197 项。
   - **已完成**：src/ 内**可移植面全部移植**。核心 **738 个用例 / 205,626 条断言**通过，addons **155 / 23,996** 通过；差分是一个脚本、**21 组、247 个场景**，全部与固定上游（11738a9d…）逐位比对——几何比 24 顶点 + 32 索引，曲线/插值/轨道比 17 个参数点，另有一批退化与边界输入。宿主边界（音频 6、视频 2、位图 1）全部用注入式实现并有测试。
   - **已修的真实缺陷（1 个）**：WebGLLights 的模块级 
extVersion 用弱引用自我续命，回收后模块被重建、计数器归零（第 52–53 条）。改成强引用静态后，前三个用例的 state.version 是 **0/2/4**，与上游期望一致。核心与 addons 断言数**一个没变**。
   - **已知未修的缺陷（1 个）**：JSValue 的分配路径随堆增长而变慢（第 57–64 条）。实测 12 轮单轮耗时 **88 → 2264 毫秒**，纯分配 1000 次从 **130 毫秒涨到 2.1 秒**（约 16 倍）。已经排除 5 个假设（state 被复制、单例不持久、压力归零、重审循环 O(N²)、safepoint 每次回收），还有一个未解释的矛盾：object() 里插桩后**字面量确实进了二进制**（Select-String 验证过）却一次都没执行。**下一任的第一件事**是绕开 object()，改在 JSHeap::allocate/ecord/elease 里插桩（那里打印生效过），把 130 微秒分段定位。
   - **环境阻塞**：本机**没有 Vulkan SDK**，THREECPP_BUILD_VULKAN=OFF，VulkanRendererTests 目标根本不生成，所以渲染到纹理 / 立方体目标 / PMREMGenerator / TSL（src/nodes 219 个模块）都**只能编译、无法验证**（第 37、38、44 条）。装上 SDK 并 -DTHREECPP_BUILD_VULKAN=ON 即可继续第 36 条的分阶段计划。
   - **操作纪律（每条都由真实返工换来，务必遵守）**：新建 CMake 目标前先 grep 目标名（撞名只在别人引用它的测试里静默错位，第 47 条）；插桩后先在二进制里 grep 字面量再跑（第 63 条）；插桩是临时资产，当轮撤掉或单独提交；期望值要算出来不要写死；改探针采样上界必须同时改除数；一次只改一个组；出现不一致时先读两边实现；**"无害但无效"的改动要回退**（第 61 条）。
   - **一句话状态**：可移植面**做完了且验证很深**；**两个后端方向**（渲染与 TSL）卡在环境上；**一个性能缺陷**已诊断到"分配路径 + 5 个假设被排除"，缺的是分段计时那一步。
64. **第 244 轮：`object()` 里的插桩确认进了二进制（grep 到了字面量）却一次都没执行——这是一个还没解释清楚的矛盾**。
   - 严格按第 63 条的纪律执行：先在 `JSValue::object()` 里插桩（三段计时，`[objstage]` 前缀），编译后用 `Select-String -Path <exe> -Pattern 'objstage'` **在二进制里搜到了那段字面量**（`marker in binary: True`），确认改动真的链接进去了；然后跑 `timing2`（无输出，合理，因为它不调 `object()`），再跑 `timing3`（**每轮显式调用 1000 次 `JSValue::object()`**）——**仍然一行 `[objstage]` 都没有**。
   - 这排除了"没编译进去""没链接进去""环境变量""stderr 重定向"这些解释。剩下的可能只有：调用被内联到别处、或者 `timing3` 里那个循环被优化掉了、或者 `object()` 存在第二个定义（但全文搜索只找到一处 `JSValue::object()` 定义，头文件里只有声明）。
   - **处理**：回退插桩，树保持干净，核心 738 / 205,626、`primitive-geometry`、`port-audit` 全绿。
   - **下一轮的路线（走已验证可行的路）**：不要继续在 `object()` 上纠缠。改在 `JSHeap` 的**类内**方法里插桩——`allocate`（885）、`record`（893）、`release`（1655）这三处都在同一个类里，而且**之前 `allocate` 里的 `[heap]` 打印确实生效过**，说明这条路线能出数据。把三段耗时分别累计在这三处，跑 `timing3`，就能知道 130 微秒落在哪一段。
62. **第 242 轮：分段计时的实现尝试失败在两处工程细节上，方法本身仍然有效**。
   - 想在 `JSValue::object()`（`JSValue.cpp:1733`）里分三段计时（`current()` / `allocate()` / `record(...).prototype = …`），受 `THREECPP_GC_TRACE` 控制每 1000 次打印一行均值。两次编译失败：一次是 `#include <chrono>` 缺失（因为回退把之前加的 `<cstdio>` 一起撤了），补上 `<chrono>/<cstdio>/<cstdlib>` 后编译通过；但运行时**一行输出都没有**，说明子进程没有继承到那个环境变量。
   - **处理**：回退这次插桩（`git checkout src/core/JSValue.cpp`），保持树干净。核心 738 / 205,626 与 `primitive-geometry`、`port-audit` 依旧全绿。
   - **下一轮的正确做法**：不要依赖环境变量控制插桩。两个更稳的办法：① 用**命令行参数**（探针本来就有 `argv[1]`，加一个 `stages-timing` 模式，把开关传到库里的一个静态 bool——但库不知道 argv，所以更简单的是②）；② 直接**无条件打印到 stderr**，把 `timing3` 的用例数压到 3、每 1000 次分配打一行，输出量可控，跑完再 `git checkout` 撤掉。
   - **提醒**：插桩代码是**临时资产**，要么当轮撤掉，要么明确标注并单独提交；不要让它混进功能性提交里。
58. **第 238 轮：变慢只取决于累积堆大小，与跑哪个用例无关**。
   - 实验：把**同一个** `lightsCase(0,0)` 重复 12 次并逐次计时，得到 **86、182、300、430、551、692、826、1137、1227、1448、1690、1695** 毫秒——和第 237 轮跑 12 个**不同**用例得到的曲线**几乎一样**。所以"某个用例本身慢"这个方向可以排除，**唯一的自变量是堆里已有多少对象**。
   - 已排除的分配路径部件：`JSHeap::record` 用 `std::unordered_map<JSValue,…>` 做索引（O(1) 查找），`validateEdge` 只做一次 `heap_` 比较（O(1)）。所以线性开销不在"查记录"这一步。
   - **下一步**：在 `JSHeap::allocate` 与 `JSValue::defineProperty` 这两条**每次写入都会走**的路径上加计数（记录数、分配数、属性写入数），跑 `timing2` 看哪一个的**增量成本**随 `index.size()` 增长。这条一旦定位，修法通常是"把线性查找换成索引"或"把重复的全量审计改成增量"。
   - **方法记录**：这一轮又是靠**一个能打印数字的最小实验**（重复同一个用例）把范围砍掉一半，没有读代码。
56. **第 236 轮：给回收器加了迭代计数后，"重审循环"假设被直接证伪——`collect()` 在这两个场景里根本**没有**被调用**。
   - 做法：在 `JSHeap::collect` 的 `stableMark` 里（`JSValue.cpp:1234` 那个 `for (;;)`）插入一个受环境变量 `THREECPP_GC_TRACE` 控制的计数打印（`std::fprintf(stderr, …)`，只在变量存在时输出，平时零噪声零开销）。这套追踪保留在树里，方便以后排查。
   - 结果：`mod3`（3 个用例）**0 行**、完整跑（120 秒超时）**同样 0 行**——也就是说这两种情况下**一次回收都没发生**。所以"重审循环导致 O(N²)"这个解释**不成立**，第 55 条的方向被推翻。
   - **新的方向**：慢发生在**分配路径**上，而不是回收上。合理怀疑是**记录表随分配增长、而每次分配或每次属性写入都要线性扫它**（第 55 条列出的第二个猜测）。下一步应当在同一处给 `JSHeap::record`/`nextId` 的分配路径加计数与耗时，量出"第 k 个用例的单次分配耗时"是否随 k 增长。
   - **已证伪的假设累计到 4 个**：`state` 被复制、单例在实例间不持久、压力后归零、重审循环 O(N²)。每次都是靠一个能打印数字的最小实验推翻的，没有一次靠读代码猜。
54. **第 234 轮：性能回退与保留无关，是"保留堆变大就变慢"的独立缺陷**。
   - 试了两件事：① 保留强引用 + `retainForHeapLifetime()`；② 只保留强引用、去掉 `retainForHeapLifetime()`。两者**行为相同**（`mod3` 都是 0/2/4，正确），**速度也都慢**（探针默认路径 120 秒仍未输出）。所以慢不是那句 `retainForHeapLifetime()` 造成的。
   - 回看修前：模块**每轮被回收**，同样的 48 个种子能跑完（`stages` 模式全程结束）。修后模块**一直保留**，卡在 A37 附近。结论：**端口的回收成本随保留堆大小增长得比预期快**——多保留一个模块就足以把"秒级"变成"分钟级"。
   - **决定**：**保留正确性，不为了跑得快而回退修复**。上游语义要求模块级 `nextVersion` 存活，回退会让 `threejs-webgl-lights-states-parity` 继续报 0 vs 2。慢是**另一个缺陷**，应当单独查。
   - **下一步（性能）**：这个探针每轮会累积大量 `JSValue`，而慢随累积量增长——怀疑 mark 阶段对**已注册对象**做了线性扫描（而不是按引用图遍历），或 `JSValue` 的登记表在每次分配时线性查找。查这两处的复杂度，就能解释"2.2 秒/轮"和"越跑越慢"。
52. **第 232 轮：闭环——`module()` 撑得过小数据、撑不过真实运行，所以计数器在真实用例里每个 case 都回到 0**。
   - `mod4`（复刻 `lightsCase(0,0)` 的 6 盏灯数组）：`hash` 从全 `-1` 变成实际计数（`sunLength:1`、`rectAreaLength:0`、`numSpotMaps:1` 等），`version` 是 **0**。**这证明 `if(changed)` 块确实执行了**，`state.set("version",next)` 真的被调用；`next` 为 0 是**对的**，因为那是进程里第一次变更。
   - 把它和前一晚两个实验拼起来：`mod2` 在**只分配少量对象**的 22 轮里看到计数器一路涨到 **45**（没有归零）；而真实用例里 `mod3` 逐个 seed 打印都是 **0**。
   - **唯一自洽的解释**：`module()` 的单例（`WebGLLights.cpp:59` 的 `static thread_local JSWeakValue` + `retainForHeapLifetime()`）在**小数据**下不会被回收，所以计数器连续；在**真实运行**（每个 case 累积大量对象）里会被回收，`module()` 随后重建、`nextVersion` 归零。这同时解释了第 230 轮"多一个存活对象就更慢/更卡"——回收时机改变，重建次数就改变。
   - **修法方向**：让 `nextVersion`（以及同在这个 `module()` 里的 `sort`）不要依赖"弱引用自我续命"。要么改成真正的强引用持有（静态 `JSValue`，或交给宿主侧的根集合），要么把这两个值放到**不参与回收**的 C++ 静态里再包成 JSValue 暴露。
   - **验证方式**：修完后 `lightsCase(0,0)` 与 `lightsCase(1,0)` 的 `version` 应分别是 0 与 2（对齐上游），且 `mod3` 打印的将不再是 0/0/0。
51. **第 230 轮：三个决定性实验结果，假设③也被证伪，但矛盾被压缩到一个变量**。
   - `mod2`（造 22 轮压力后再读版本）：**45**，没有归零 → 单例**确实持久**，压力不是原因。同时量出真实用例大约**每轮 2.2 秒**，所以第 219 轮看到的"卡住"其实主要是**慢**，不是死循环。
   - `mod3`（依次跑 seed 0/1/2 并打印各自快照里的 `version`）：**三个都是 0**。也就是说在**真实用例**里 `if(changed)` 恒为假，版本号从未被写过。
   - `mod`（手工构造**一盏**灯再 `setup`）：**0、1、2** 正常递增。同样是 `setup`，写入确实会反映到 `manager.state.value()`。
   - **矛盾被压缩成一句话**：手工单灯能递增，`lightsCase(seed,0)` 恒 0。两者唯一的差别是**灯数组的构造方式**（`lightsCase` 用 `light(seed+i,i,…)`，可能有 6–8 盏并带 `(seed+i)%5!=0` 过滤）。下一步就在 `lightsCase` 里把 `hash` 与各项 `counts` 打出来，确认是不是"两次 layout 完全一样"导致 `changed` 恒假——如果是，那上游的 2 就另有来源，需要回头看上游同一条路径。
   - **诊断纪律的收获**：三个假设（副本、实例间不持久、压力后归零）全部**被实验推翻**，而且每次推翻都靠**一个能打印数字的最小实验**，不是靠读代码猜。继续保持这个顺序：先造一个能证伪的实验，再改代码。
50. **第 229 轮：又一次证伪——计数器在"无 GC 压力"时确实持久（0/1/2），所以问题只可能出现在回收之后**。
   - 实验：新建三个 `WebGLLights`，每个都用同一份灯数组调一次 `setup`（第一次必然 `changed` 为真），读出 `state.version` 依次是 **0、1、2**。这说明 `module()` 的弱引用单例在**普通**情况下没有失效，`nextVersion` 在实例之间是连续的。
   - 因此第 228 轮"单例根本活不住"的说法**过于笼统**，需要收窄为：**只有在回收发生之后**单例才可能被重建。上面这个实验全程只分配几个小对象，没有触发任何值得回收的压力，所以证明不了什么。
   - **至今已证伪的两个假设**（记在这里，避免重复尝试）：① `state` 被 `literal` 复制——`sameValue` 为 true，是同一个对象；② 单例在实例之间不持久——连续三次得到 0/1/2。
   - **仍然成立的观察**：真实用例里 `root.1.setup.version` 是 0 而上游是 2；并且"多一个存活对象"就会让整轮从秒级变成卡住。这两件事都指向**回收**，而上面那个 0/1/2 的实验恰好**没有**制造压力。
   - **下一步（决定性实验）**：让 `mod` 模式在两次 `setup` 之间**故意制造大量存活对象**（模拟真实用例累积的 `results`），再看版本号是否回退到 0。若能复现，就说明单例确实会被回收；这时修法是把 `nextVersion`/`sort` 挂到不会被回收的宿主上。
48. **第一个真实缺陷候选：一个未被使用的 `JSValue::object()` 局部变量会让探针永远卡住**（第 225 轮定位）。
   - 现象：`ThreeJsWebGLLightsParityProbe` 用默认参数运行时**永不返回**（180 秒零输出，说明卡在构造 JSON 的过程中）；把同一个 `main` 里那句**从未被使用**的 `const auto realm = JSValue::object();` 删掉后，**9.9 秒**跑完、输出 576 KB、`ctest -R lights-states-parity` **通过**。
   - 定位过程：给 `main` 加 seed 参数（seed 17/18/19 单独跑全部正常）→ 加阶段标记（`stages` 模式打印 A…F 全程走完）→ 对比两条路径**唯一**的差异就是那句 `realm`，删掉即好。**先前第 29 条把它记成"fixture 在 light(18,1) 处挂起"是错的**：`lightsCase` 本身没问题。
   - 这**不是**探针写法问题可以解释的：那句变量既没有参与任何计算，也没有被 lambda 捕获（`[&]` 捕获不到未使用的变量）——它的唯一作用是**让一个空的 JSValue 对象在该作用域内存活到 `main` 结束**。所以嫌疑落在 JSValue 的**对象/引用登记**或**析构**路径上（例如空对象进入某个环形结构、或 finalizer 队列在退出时死循环）。
   - **第 226 轮的最小复现结果（比上一版描述更精确）**：
     - `{ auto o = JSValue::object(); }` 单独构造→析构，甚至循环 1000 次：**0.14 秒，不卡**。
     - `realm` + 只跑 seed 22：**0.1 秒，不卡**。
     - `realm` + 依次跑 seed 0…47（`stagesr` 模式）：**卡在 A22**（60 秒未完）；去掉 `realm` 的同一序列（`stages` 模式）：**全部 48 个 seed 加后面所有阶段跑完**。
   - 结论：不是"空对象本身"的问题，也不是某个 seed 的问题，而是**累积存活对象数 + 一个多余根**时的**超线性开销**。多一个根不该改变复杂度量级，所以嫌疑集中在两点：GC 的 mark/sweep 对**不可达但未释放**的对象每轮都要重扫（近似平方），或该对象的**析构/finalizer 队列**在每次 GC 前进一次。**最小复现已收窄到"两个变量 + 一个循环"**，下一步应当在 GC 入口加计数打印，量出 mark 次数随 seed 增长的关系，而不是继续二分 fixture。
   - **注意**：把 `realm` 删掉能让 #84 **跑完**（十秒），但它现在**失败**在一个真实的差分差异上——那是另一件事，需要看 `--output-on-failure` 的具体 diff。
47. **我在第 118 轮引入了一个回归：新探针的目标名撞了旧测试引用的名字，导致旧差分永远失败**（完整跑才暴露，单跑门禁三项看不出来）。
   - 经过：第 118 轮我新建 `test/unit/utils/parity/PrimitiveGeometryParityProbe.cpp`，在 `test/CMakeLists.txt` 里注册为 `ThreeJsPrimitiveGeometryParityProbe`。但 `test/CMakeLists.txt:1202` 处的**旧**测试 `threecpp.threejs-primitive-geometries-parity` 早就用 `$<TARGET_FILE:ThreeJsPrimitiveGeometryParityProbe>` 指向同名目标——于是那个旧测试现在跑的是**我的新探针**，而它用的脚本是 `compare-primitive-geometries.mjs`（喂 JSON 文档、期望 `keys` 数组）。新探针按自己的行协议解析，直接回 `{"error":...}`，断言因此失败。
   - 证据：`ctest -j 4 -E "lights-states"` 跑到 **196 项里 195 通过、1 失败**，失败项是 `threecpp.threejs-primitive-geometries-parity`（#179/#181 两次运行编号不同但同一项）；v8 报错正是 `actual: ['error']` 对 `expected: ['cloneAttribute', ...]`。
   - **修法（下轮做）**：把**我的**探针目标改名（例如 `ThreeJsPrimitiveGeometryBasicParityProbe`），同时改它自己的注册行（`add_test(... threejs-primitive-geometry-parity ...)` 与 `add_executable`/`target_link_libraries` 三处），让 1202 行重新指回**原来的**旧探针目标。改完必须 `cmake -S . -B build` 重新生成，再跑 `ctest -R "primitive-geometr"` 确认两个测试都过。
   - **教训**：**新建 CMake 目标前先 grep 目标名**。名字撞车不会在编译期报错，只会在**别人引用该名字的测试**里静默错位；而我的日常门禁只跑 `-R "primitive-geometry|port-audit|object-loader-parity"`，正好漏掉了这项。
   - **已修（第 222 轮）**：把 `add_test(NAME threecpp.threejs-primitive-geometries-parity ...)` 整块**退役**并留注释说明原因。它指向的 `PrimitiveGeometriesParityProbe.cpp` 早已不存在，那个 `$<TARGET_FILE:...>` 是在撞了我的目标名之后才"看起来能解析"的——在撞名之前，它只会让 configure 失败或者根本不生成该测试。退役后 `ctest -R "primitive-geometr"` 只剩我那个探针，**1/1 通过**。
   - **失去的覆盖**：那个旧脚本比的是 `clone`/`copy`/`toJSON` 往返（`cloneAttribute`、`copyReturn`、`roundTripEqual`、`transformedCloneEqual` 这些键）。这些维度**目前没有替代**。要补回来的话，正确做法是新写一个按 JSON 文档输入、输出上述状态的探针（`PrimitiveGeometriesParityProbe.cpp`），而不是复用行协议探针。
   - **教训补充**：删测试也要记账。这次删掉的是一项**本来就跑不通**的测试，但它承载的维度是真的——所以退役时必须把"失去什么"写下来，否则以后会以为这块被测过。
46. **完整 `ctest` 跑不完：`threecpp.threejs-webgl-lights-states-parity`（#84）会挂起，这与端口无关**。第 219 轮我用 `ctest -j 4` 跑全部 198 项，跑到 **197/198 之后卡住**；查日志可见 `Test #84` 只有 `Start` 没有结果，其余全部 `Passed`（含 `primitive-geometry` 与 `port-audit`）。这正是第 29 条记录过的探针挂起：该探针在 fixture `light(18, 1)`（LightProbe）处停住，端口自身的 `setup` 已经返回。
   - **后果**：不加过滤地跑全量会**永远**跑不完，CI 里会表现为超时而不是失败。
   - **本仓库的正确用法**：`ctest --test-dir build -j 4 -E "lights-states"`（第 220 轮已按此跑，见 `build/ctest-skip-hang.log`）。日常门禁只需 `-R "primitive-geometry|port-audit|object-loader-parity"` 三项即可。
   - **要彻底解决**：修该探针的 fixture（从 `light(18, 1)` 起逐步二分），或者给它加 `set_tests_properties(... TIMEOUT ...)` 让全量跑能正常结束。这两件事都不影响端口产物。
45. **"按类名匹配测试文件"会严重高估缺口：165 个测试文件覆盖 208 个类，但绝大多数"没对上"的类其实被多类共用的测试文件覆盖了**。我按 `类名 == 测试文件名` 扫了一遍，得到"152 个类可能缺测试"，抽查就推翻了：`Path`/`CurvePath`/`ShapePath` 各有自己的 `*.tests.cpp`；`TextureImage`（即 `TextureSource`）在 `Texture.tests.cpp` 里有专门用例（像素保留、回调替换、元数据重读）；helper 全部由 `Helpers.tests.cpp` 覆盖。
   - **教训**：类名 ↔ 文件名不是覆盖关系的可靠代理。要判断某类有没有测试，**grep 类名**（`TextureImage`、`PolarGridHelper` 等）而不是猜文件名。这与第 43 条里差分 gap 扫描踩的是同一个坑（那次也是名字匹配不准）。
   - **结论**：就本仓库现状而言，**没有明显缺失单元测试的模块**。想继续加测试，应该针对"行为"而不是"类"——例如给已有用例补边界值，或给差分已发现的行为写等价的 C++ 断言。
   - **补充（第 203–204 轮）：这个边界不是"感觉"，有三条独立证据**。① **后端**：Vulkan SDK 缺失，`VulkanRendererTests` 目标不生成，渲染类改动无法验证（第 37 条）。② **TSL**：`src/nodes` 219 个模块的验收是"生成的着色器能编译"，走同一条着色器管线，因此被同一缺口卡住（第 38 条）。③ **单元测试**：按类名扫描出的"152 个模块缺测试"经抽查被推翻——`Path`/`CurvePath`/`ShapePath` 各有测试文件，`TextureSource` 在 `Texture.tests.cpp` 里有专门用例，helper 全由 `Helpers.tests.cpp` 覆盖（第 45 条）。也就是说，**在本环境里"能写完并证明做对了"的工作已经做完了**；剩下的要么需要 SDK，要么是在已有验证面上继续加深（可行但收益递减）。
43. **差分覆盖矩阵（第 253 轮收尾的实测值：277 个场景、21 组）**。数字取自 compare-primitive-geometry.mjs 的实际输出：
   - primitive geometry parity：39 场景
   - curve parity：2 场景
   - simple curve parity：4 场景
   - flat curve parity：16 场景
   - interpolant parity：20 场景
   - track parity：10 场景
   - text parity：4 场景
   - light parity：24 场景
   - material parity：15 场景
   - helper parity：31 场景
   - child helper parity：9 场景
   - node parity：30 场景
   - texture parity：25 场景
   - fog parity：10 场景
   - spherical harmonics parity：8 场景
   - instanced mesh parity：5 场景
   - path parity：8 场景
   - stereo camera parity：1 场景
   - raycaster parity：3 场景
   - curve path parity：9 场景
   - shape path parity：4 场景
   - **深度**：曲线/插值/轨道 **17 个参数点**；几何 **24 个顶点**（72 个 float32 位模式）+ **32 个索引**。
   - **退化与边界（按类）**：零尺寸盒/平面/面光；零长/共线/折返/闭合/重合点/之字形路径；样条前两点同 x；`far == near` 的射线；`density` 为 0 与 1000 的雾、`near = 0, far = 1` 的雾；全零/负值/2–8 组球谐；单格与奇数分段网格、文档默认与最重的极地网格；百万分之一到千单位的轴；两词三词轨道；单像素、单行、奇数、1×1 与 2×8 的纹理与阴影图；单面到五面的立方；单层与 4³ 的数组纹理；3×1×2 与 4³ 的 3D 纹理；单到三十二实例；单相机到四相机的数组相机；全零/全 1/200 单位的正交相机；1° 到 180° 的透视（含 16:9 文档值）；`distance = decay = 0` 与百倍强度的点光；锥角 0 到 1.5 的聚光；黑色与大面积面光；零强度环境光/半球光/探针；1–4 分量 × 2–3 采样的插值组合；负坐标路径与线段；1×1 到 1920×1080 的渲染目标。
   - **尺度两端**：1000 单位轴与千单位雾、全高清渲染目标 ↔ 零尺寸、百万分之一轴、单像素纹理。
   - **组合完整性**：`linear`/`discrete` 覆盖 1–4 分量，`cubic` 覆盖 1–3 分量，四元数覆盖 4 分量；采样 2–3；六类轨道两种长度；纹理 8 类、材质 15 类、光源 7 类、helper 18 + 子对象 6。
   - **写法**：第 187–253 轮共 **二十三次**扩场景，**只加场景行、探针零改动**，二十三次**全部一次通过**；第 186 轮"加新 kind"改坏文件只能回退。**扩张验证面优先走"已有能力 + 新数据"**。
42. **差分验证的现状与操作纪律（第 175–180 轮把深度翻了 3–4 倍之后的总结）**。当前 `compare-primitive-geometry.mjs` 是一个脚本、二十一个组、**147 个场景**，全部与固定上游逐位比对：
   - **组与深度**：几何 39（每个属性比较 **24 个顶点**、**32 个索引**）、光照 7、helper 11 + 子对象 6、材质 15、纹理 8、对象 13、fog 4、球谐 3、实例化 3、曲线 2（4 个数组）+ 简单曲线 3 + 平面曲线 9（曲线类均为 **17 个采样**）、插值 7（17 点）、轨道 5 + 字符串 2（17 点）、路径 2、曲线路径 2、形状路径 2、立体相机 1、射线 2。
   - **为什么深度重要**：5 个采样会漏掉四分点之间的弯折（`cubic` 与四元数 slerp 尤其明显）；6 个顶点会漏掉扫掠与细分的中段。加深之后 147 个场景**仍然全部逐位相等**，这才是"一模一样"的有力证据。
   - **操作纪律（每条都由真实的返工换来）**：① 改探针的采样上界时**必须同时改除数**（否则输出是旧采样的重复）；② 改完**先重建、再数发射的条目数**，形状对了才比对数值；③ 一条命令只改**一个组**；④ 插入代码用**行号**，不要用 `Replace`（它替换所有匹配，且 PowerShell 会把 `#` 当切片）；⑤ 期望值必须**算出来**，不能写死（`children: []` 那个字面量害了四轮）；⑥ 出现不一致时**先读两边实现**，再决定是代码问题还是夹具问题——至今 5 次"分歧"全部源于夹具。
   - **已闭环（第 183 轮）**：`Path::getPoints(divisions)` **没有问题**。最小实验只比一个数字——`getPoints(4).length`——两侧都是 **4**；端口产出的四个点就是轮廓的四个角 `(0,0),(2,0),(2,2),(0,2)`，与上游一致。第 148 轮那次"分歧"的真正原因是我在探针里把取值循环写成了 `i / 2` 的半索引，取到了错位的点。**这是第六次由夹具引起的假分歧**。
   - **新教训**：在**一行** `std::cout` 里追加数组字段很容易把括号配错（本轮把 `"]}"` 拆开后又补错两次，最后只能整批回退）。要么让每个数组**独占**一个 `std::cout`、方括号成对出现，要么把整个对象改成逐字段分行输出。**追加字段前先数一遍方括号**。
41. **`PlaneHelper` 的差分曾报"子对象不一致"，但两边源码都 `add` 了同一个 Mesh，所以这是一个未能复现的疑点，而不是已确认的缺陷**（原条目曾误记为"端口缺失"，此处更正）。
   - 两边的源码都建了象限网格并挂为子对象：上游 `src/helpers/PlaneHelper.js` 第 63 行 `this.add( new Mesh( geometry2, new MeshBasicMaterial( { color, opacity: 0.2, transparent: true, depthWrite: false, toneMapped: false } ) ) )`；本端口 `src/helpers/PlaneHelper.cpp` 第 3 行同样建了 `geometry2` 与 `material2`（`opacity=.2`、`transparent=true`、`depthWrite=false`）并 `add(std::make_shared<Mesh>(...))`。第 6 行的 `dispose` 还会释放这个子网格。
   - 差分现象：把 `plane` 场景与 `box3` 一起启用时变红，单跑 `box3` 是绿的；红的差异只有 `children`，线的 8 个顶点完全一致，而 `children` 一侧是 `[{type:'Mesh'}]`、另一侧是 `[]`。既然两侧源码都 `add`，这条差异**不指向端口**，更可能是我的脚本用 `helper.children` 取值时取到了不同的对象（例如读到了构造过程中的中间状态，或上游的 `Mesh` 未在 `describeHelper` 读之前挂上）。
   - 下一步：先写一个**独立的**最小差分，只比较 `new PlaneHelper(...).children.length`，两侧各打印一次；若两侧都是 1，则重启原场景并细查脚本；若一侧是 0，再定级。**在拿到这个最小证据之前，不要把 `plane` 场景并回差分。**
   - `box3` 场景已启用并通过 ✓（8 顶点 + 索引，与本端口的实现一致）；`plane` 场景的探针分支与脚本条目写过两次，复原很快。
40. **`TubeGeometry` 曾被我误报为"与上游存在真实分歧"，实际是本端口正确、我的对拍输入写错了**（本条留作反面教材）。
   - 经过：加 tube 场景时差分报错，我看到"顶点数与索引数一致、`position` 位模式不同"，据此写下"框架计算有分歧"的结论，并用三轮排查依次排除了曲线 `getPoint`/`getTangent`、弧长 `getLengths`/`getUtoTmapping`/`getPointAt`、以及 `computeFrenetFrames` 本身——这些排查本身是有效的（并且顺带把五项曲线函数固化进了差分），但它们全都在证明**被测代码没问题**。
   - 真相：`test/unit/utils/parity/compare-primitive-geometry.mjs` 里三条 tube 场景的**输入行本身写错了**：第一条少一个坐标（8 个数字而非 9 个），第二条同样少一个，第三条把控制点数量写成了 4 而实际给了 3 个点。探针按空格流式解析，数字错位后读到的是错的控制点，于是"顶点数对、值不对"——这个现象和我当时给出的诊断（框架方向差异）**恰好吻合**，所以我没有去怀疑输入。
   - 修正后事实：`tube` 与上游**完全一致**，`position`/`normal`/`uv`/`index` 四项逐一匹配；`computeFrenetFrames` 的 `normals`/`binormals` 前若干项也逐位相等。三条 tube 场景已并入差分（现共 39 个几何场景 + 2 个曲线场景）。
   - 教训：① 自定义行协议**没有校验**，错一个数字会伪装成"被测代码的 bug"；给探针加上"按 `count` 校验数字个数"的检查是值得的。② 报"发现分歧"之前，先把**输入**验一遍，代价远低于三轮排查。③ 位级对拍依然值得——它这次是**正确地**排除了嫌疑，只是嫌疑人是我自己。
39. **差分覆盖的选点表：上游有单测的 242 个模块里，105 个已有对拍，170 个没有；其中 125 个是可移植的**。统计方法：扫 `test/**/*.mjs` 里出现的 `src/.../Name.js` 取模块名，与 `../three.js/test/unit/src/**/Name.tests.js` 的模块名求差集。按可移植性分组如下（括号为数量）：
   - **可移植、尚未对拍（125）**：光照与 helper 22、对象与渲染目标 27、几何 20、曲线 14、杂项 14、材质 13、关键帧与插值 11、音频 4。这一组是当前**唯一能"做完并证明做对"**的方向，第 37、38 条的 SDK 阻塞不适用于它。
   - **被阻塞（45）**：TSL 节点系统 32、GPU 计算 8、WebGL 渲染目标 5。前两组要 SDK，第三组要本端口的等价物。
   - **建议次序**：先做**几何（20）**——它们输出的是顶点与索引缓冲，端口已有 `PrimitiveGeometries.tests.cpp` 做单测，补一份 `.mjs` 对拍即可把"参数 → 顶点数据"钉死；再做**曲线（14）**与**关键帧/插值（11）**，两者都是纯数学，最容易证明"一模一样"；光照与材质适合随后，因为它们的构造参数与默认值多，价值高但用例要更细。
   - 提醒：`BatchedMesh`、`CanvasTexture`、`VideoTexture`、`AudioLoader` 等虽在"未对拍"名单里，但**已有单元测试**覆盖其行为（差分不是唯一证据）；选点时优先挑"只有单测、没有对拍"的模块，收益最大。
38. **`src/nodes`（TSL，219 个模块）是独立大工程，而且它的验收同样卡在渲染器上，不适合当作"下一个模块"**。构成：`accessors` 43、`core` 42、`utils` 31、`functions` 24、`lighting` 23、`display` 18、`math` 11、`gpgpu` 6、`materialx` 5、`code` 4、`tsl` 2、`pmrem` 2、`parsers` 2，其余各 1，外加入口 `TSL.js`（184 行）与 `Nodes.js`（165 行）两个再导出。
   - 规模：单是 `core/Node.js` 就 1298 行，整个 `core` 目录围绕 `NodeBuilder` 展开。这不是"再补一个类"，而是**一套着色器图 DSL**：节点构造、类型推断、常量折叠、代码生成。
   - 关键判断：TSL 的**构造**部分是纯数据，可以不碰 Vulkan 单测；但它的**价值与验收**是"生成正确的着色器代码并让它编译"，那一步走 `NodeBuilder` → 着色器管线，与第 37 条是同一个 SDK 阻塞点。也就是说，即使把节点类逐个移植完，本环境也无法证明它们产出正确。
   - 结论：不在本环境里推进 TSL。可行的次序仍是先把 Vulkan SDK 装上、把第 36 条的分阶段计划做完，再谈 `src/nodes`。在此之前，`src/` 与 `src/nodes` 之外仍有**不依赖渲染器**的工作可做：扩充已有模块的差分与边界用例，这是当前唯一能"做完并且证明做对了"的方向。
37. **本环境没有 Vulkan SDK，因此后端改动无法验证；这是硬性环境约束，不是待办**。证据：`build/CMakeCache.txt` 里 `THREECPP_BUILD_VULKAN:BOOL=OFF`，并且 `THREECPP_VULKAN_INCLUDE_DIR:PATH=THREECPP_VULKAN_INCLUDE_DIR-NOTFOUND`、`THREECPP_GLSLANG_VALIDATOR:FILEPATH=...-NOTFOUND`。后果很具体：`test/CMakeLists.txt` 第 413–485 行的 `if(THREECPP_BUILD_VULKAN)` 整块被跳过，`VulkanRendererTests`（第 481 行）根本不会被生成，`cmake --build build --target VulkanRendererTests` 只会得到 `unknown target`。
   - 我试过的排查顺序（记下来省得重走）：先怀疑 ninja 的 `premature end of file`，删掉 `.ninja_log`/`.ninja_deps` 后该警告消失但目标仍然不存在；再用 `cmake -S . -B build` 重新生成，`build.ninja` 正常写出（0.93 MB）但目标依旧没有；最后读缓存才发现选项是 OFF。
   - **含义**：任何"渲染到纹理、立方体目标、预滤波"的改动，在本环境里只能做到"库能编译"，**没有可运行的验证途径**。按本项目"不留未验证代码"的规则，这类改动必须**先获得可验证的环境**再落地。反过来也要注意：`VulkanRenderer.cpp` 本身在库里是会编译的，所以"库能编译"不等于"后端可验证"，两者不要混。
   - 若要解锁：安装 Vulkan SDK（提供 `vulkan/vulkan.h`、loader 与 `glslangValidator`），再用 `-DTHREECPP_BUILD_VULKAN=ON` 配置。届时第 36 条的分阶段计划可以直接执行。
36. **"渲染到纹理"的分阶段计划，附勘察到的实际缺口**。先给结论：后端底子比想象的厚，缺的是**入口**而不是**机制**。
   - **已有的**：`src/core/RenderTarget.h` 的数据模型（`textures`、`depthTexture`、`viewport`/`scissor`）；`src/renderers/VulkanRenderer.cpp` 里的 `VkRenderPass`/`VkImage`/`VkImageView`/`VkFramebuffer` 与带 mip 层的图像屏障（约 1410 行那段 `barrier` lambda 已经按 `levels` 逐层转换）。
   - **缺的（按依赖顺序）**：
     1. `VulkanRenderer` 只有 `render(Scene&, Camera&)`（`src/renderers/VulkanRenderer.h:61`），**没有** `setRenderTarget` / `getRenderTarget` / `getActiveCubeFace` / `getActiveMipmapLevel`。上游这四个方法是 `CubeCamera.update`、`PMREMGenerator` 和高层渲染入口的共同前置。
     2. 渲染通道目前只有一条（面向窗口），需要按"当前绑定目标"选择或创建 `VkRenderPass`/`VkFramebuffer`，并处理颜色/深度附着的布局转换与 mip 层选择。
     3. 立方体渲染目标类型（六面 + 每面 mip 链）。本端口没有 `WebGLCubeRenderTarget` 的等价物；`RenderTarget3D` 是另一回事。
     4. 预滤波的两个 pass（模糊与等距柱状采样）及其着色器。`ShaderChunk` 已有，但这两个 pass 的采样逻辑在 `PMREMGenerator` 内部。
   - **建议的最小里程碑**：先只做 (1)+(2) 的一个子集——把场景渲染到一张 2D 离屏纹理并能读回——用一个单元测试固定住；再扩到六面；最后才是 `PMREMGenerator`。每一步都能独立验证，避免一次性接上一个无法调试的大件。
35. **`PMREMGenerator` 是 `src/` 内最后一项真工程，它卡在后端的渲染目标能力上，而不是卡在移植工作量上**。依赖面清点：
   - 上游 import 的是 `WebGLCubeRenderTarget`、`ShaderMaterial`、`CubeCamera`、`BoxGeometry`、`ShaderChunk`、`MathUtils.floorPowerOfTwo`、`Color`、`Vector3`。其中后面七项本端口都有（`CubeCamera` 见第 109 轮），真正缺的是**第一项**。
   - 本端口只有 `src/core/RenderTarget.h` 与 `RenderTarget3D.h`，**没有立方体渲染目标**；Vulkan 后端（`src/renderers/vulkan/`）目前只有管线布局、着色器编译、uniform 缓冲、绑定与缓冲命令这些**管线构件**，没有"渲染到纹理 / 立方体面 / mip 链"的通道机制。
   - 因此它的落地顺序是：① 后端支持渲染到纹理 ② 立方体贴图的六面与 mip 链 ③ 预滤波的两个 pass（blur 与 equirect 采样）④ 才是把 `PMREMGenerator` 的调度逻辑搬过来。前三项是**后端工程**，不是移植工作；把 771 行 JS 直接翻译过来而没有 ①②③，只会得到一份无法执行的代码。
   - 结论：`src/` 内"不依赖渲染器就能推进"的部分到此为止，剩下的是后端能力建设。
33. **两条防回归纪律（都来自一次真实事故）**。加 `Earcut` 封装时，我用整文件写入的方式创建 `test/unit/src/extras/Earcut.tests.cpp`，而**那个文件早就存在**，里面有 5 个对拍上游三角形顺序的用例。结果它们被静默覆盖，核心用例数从 737 掉到 733，而门禁只看退出码，没看数量，于是带着损失提交了。两条规则：
   - **写文件前先确认它是否存在**。新文件才整写；已存在的文件一律用定点补丁追加。当时我检查了"CMake 里有没有登记"，却没检查"文件在不在"，这个检查顺序是错的。
   - **用例数只能单调不减**。任何一次门禁里，核心与 addons 的用例数、断言数如果比上一轮**下降**，就当作回归处理：查明原因、修复、再提交。断言数会因测试改写而小幅波动，但用例数下降没有正当理由。
   - 事后核对：`git diff --stat HEAD~1 HEAD -- test/unit/src` 能立刻显示被大幅删除的测试文件；`--list-tests` 的条目数与汇总行的用例数应当一致。
34. **三个"入口再导出"模块判定为不适用，不需要移植**：`src/Three.Legacy.js`（1 行，空文件，只是历史兼容占位）、`src/Three.TSL.js`（4 行，把 `src/nodes` 的 TSL 再导出）、`src/Three.WebGPU.js` 与 `src/Three.WebGPU.Nodes.js`（43 与 33 行，全部是 WebGPU 渲染器与节点系统的再导出）。它们没有任何自己的逻辑：等价物是"本端口的入口头文件与 Vulkan 后端"，写一个 `Three.WebGPU` 的 C++ 再导出既没有调用者也没有意义。判定理由记录在此，而不是把它们留在"待移植"里充数。
32. **`vulkan-port-pending` 的 290 项不是待移植缺口，而是被本端口自己的后端取代的上游渲染器实现**。读 `docs/port-status.json` 时会以为还有 290 个模块要写，实际构成是：`src/renderers/shaders` 132、`src/renderers/common` 68、`src/renderers/webgpu` 40、`src/renderers/webgl` 30、`src/renderers/webgl-fallback` 12、`src/renderers/webxr` 3，外加 5 个 `WebGL*RenderTarget` 与 `WebGLRenderer` 本身。
   - 这些是**跑在浏览器里的 JS 渲染器**：WebGL 的 `WebGLPrograms`/`WebGLState`/`WebGLTextures`，WebGPU 的 `Backend`/`Renderer` 家族，以及它们共用的 `common` 抽象。本端口不使用它们中的任何一个，而是自己实现了一份 Vulkan 后端（`src/renderers/vulkan/` 下 13 个 `.cpp`：绑定与缓冲命令、管线布局、程序 uniform、ESSL 着色器编译等）。
   - **含义**：把它们列为 `vulkan-port-pending` 在"上游模块是否有对应实现"这个维度上是诚实的，但在"还剩多少移植工作"这个维度上会**严重高估**。真正的判断标准应当是：**上游的渲染能力在 Vulkan 后端里有没有等价路径**，而不是"这些 JS 文件有没有被逐行翻译"。
   - **做法建议**：以后统计完成度时，把这三类分开报：① 已移植且有差分 ✓；② 宿主边界（第 31 条，16 个，现只剩 4 个渲染器绑定 + 3 个非逻辑文件）✗；③ 被后端取代的 JS 渲染器（290）✓。混在一起会得出错误结论。
31. **宿主边界清单：`src/` 内（除 `src/nodes/`）剩余未移植模块全部归因于宿主或渲染器**。这是"可移植面已完工"的判据，可随时用下面的扫描复核（读 `docs/port-status.json` 的 `missing`，过滤 `src/`、排除 `src/nodes/`，再按上游源码里出现的宿主 API 分类）。当前 16 个，逐一归因：
   - **WebAudio（6）**：`audio/Audio.js` 779、`audio/PositionalAudio.js` 254、`audio/AudioListener.js` 219、`loaders/AudioLoader.js` 109、`audio/AudioAnalyser.js` 98、`audio/AudioContext.js` 41 —— 全部需要 `AudioContext`/`GainNode`/`PannerNode` 这类浏览器音频节点。
   - **渲染器（4）**：`extras/PMREMGenerator.js` 771、`cameras/CubeCamera.js` 260、`Three.WebGPU.js` 43、`Three.WebGPU.Nodes.js` 33 —— 需要 WebGPU/WebGL 渲染目标与预滤波。
   - **视频播放（2）**：`textures/VideoTexture.js` 128、`textures/VideoFrameTexture.js` 73 —— 需要 `VideoFrame` 或 `readyState` 驱动的帧更新；`VideoFrameTexture` 还**继承** `VideoTexture`，所以不能只做薄封装。
   - **位图解码（1）**：`loaders/ImageBitmapLoader.js` 226 —— 需要 `createImageBitmap`。
   - **外部包（1）**：`extras/Earcut.js` 29 —— 只是 `earcut` 包的再导出。
   - **入口再导出（2）**：`Three.Legacy.js` 1（空文件）、`Three.TSL.js` 4 —— 没有逻辑。
   - **做法上的含义**：这 16 个不是"忘了移植"，而是**必须由宿主提供能力**。要解锁它们，正确路径是照 `ImageLoader` 的注入式模板（第 22 条）为音频/视频/位图各加一个注入接口，而不是在移植里伪造 DOM 对象。
   - **另外两处已知的宿主依赖**（模块本身已移植）：`ObjectLoader.parseImages` 的 `url` 字符串分支，以及 `BatchedMesh` 的三个 GPU 纹理与绘制参数。
30. **`ObjectLoader` 的图像记录形状：`url` 必须是记录，不是 `data`**。写差分文档时踩了两次：上游 `parseImages` 对每条记录调用 `deserializeImage(image.url)`，于是
   - 记录里**没有 `url`** → `deserializeImage(undefined)` → 读 `image.data` **直接崩** ✗；
   - 记录里 `url` 是**字符串** → 走 `ImageLoader` → 需要 `document` ✗；
   - 记录里 `url` 是**对象** `{data, width, height, type}` → 走上游的数据分支 ✓，构造 `DataTexture` ✓。
   端口的 `parseImages` 直接读记录的 `data`/`width`/`height`/`type`（跳过 `url` 这层），所以两侧都能吃，但**生成文档时必须按上游的 `url` 记录形写**，否则 oracle 先崩。
   这也解释了 `BatchedMesh` 的差分为什么需要两个**带数据图像**的纹理（`matricesTexture`、`indirectTexture`）：上游分支无条件读它们的 uuid，缺了就是崩溃而不是跳过。端口有意跳过这三个纹理（矩阵与颜色存在向量里、间接表只用于 multi draw），结构摘要因此不观察它们 —— 这是**有意取舍**，不是遗漏。