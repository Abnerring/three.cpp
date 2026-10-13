# 验证记录

## 当前实施批次：颜色管理、图层、shader 缓存与编译基础，2026-10-13

固定上游不变。已合入 ColorManagement/WebGLUtils/Object.assign、管理 Layers 与 UniformsUtils 导出、WebGLShaderCache、Number.toFixed/NumberData、Array.filter/ArraySpeciesCreate、get/call 线程检查和 VulkanShaderCompiler。上一批三个失败项的 realm/首次初始化修复亦已合入。1,119 个生产、测试、工具、示例、构建与审计文件已冻结；两种配置的主库编译已完成，首次完整构建均终态失败：ShaderCache 两个 probe 缺少 fixture 搜索路径。原失败日志及冻结清单保留，仅修复 test/CMakeLists.txt 的两个 target include 目录，生产与原测试不变；已重新冻结 1,119 文件，等待修复后完整构建。CTest 登记 131 项，尚无本批完整 CTest 成功结论。当前审计仍为 1,251 模块（196 partial、745 missing、290 Vulkan pending、20 dependency pending），347 个模块存在 C++ 文件。

集成前 Debug、Release 各自专项通过：颜色管理 399、WebGLUtils 1,685、Object.assign 65 个实际 Node 场景；Layers/UniformsUtils 4,021 场景 / 35,122 比较，新加共享颜色管理联动 232 场景等待集成后验证；ShaderCache 7,930 场景 / 1,575,920 比较及 11 个 GC 快照；Number 37,850 精确字符串场景、1,343 协议场景及 6 个 GC 快照；filter/species 905 场景 / 28,690 比较、4 cases / 26 assertions。有效非十进制 Number 格式的 36 个未实现边界单列，不计作支持。

VulkanShaderCompiler 使用原生 glslang 进程，不调用 shell，保留诊断、链接失败和临时资源清理。Windows 两配置各 3 cases / 27 assertions；独立复核各 62 checks，覆盖 Unicode/特殊字符路径、并发、失败恢复和坏 SPIR-V。显式 close 修复最终刷盘错误被析构吞掉的问题。仅检查 SPIR-V 基本结构，Linux/macOS 未运行；项目外 GPU 验证 Renderer 副本未合入，完整 WebGLProgram/pipeline/binding 仍待实现。

模块 facade 无活 realm 时拒绝修改；ShaderLibrary 原断言转到独立线程并转回异常；灯光 oracle 显式保留与 Node 同寿命的 realm。Array species getter 提前回收的失败记录保留，按现有内建保活规则修复，实际 Node 普通函数对照仍正常回收。

## 上一实施批次：集合、能力查询与着色器库，2026-10-13

固定基准为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。已合入 Map/Set、Symbol 属性键、同步 Iterator、StringData、WebGLExtensions/WebGLCapabilities、警告协议、ShaderChunk/ShaderLib、WebGLShader 和 WebGLAnimation。原生运行时属性键入口先验证无效接收者，再转换键；Object.prototype 的两个 legacy 方法保留先转换键的例外顺序。此窄修保留双配置修复前 240 处差异及修复后的 1,070 个 Symbol 场景。

本批 1,071 个生产、测试、工具、示例、构建与审计文件已冻结，Windows x64 Debug、Release 完整构建均成功。Debug、Release 完整 CTest 终态均为 **113/116**，耗时分别 1,312.94 秒、116.11 秒；失败为主单测中的两个 ShaderLibrary 生命周期/首次初始化检查，以及 WebGLLights 单测与差分。本批完整回归尚未通过，原失败日志保留，不能把集成前专项通过登记为最终成功。源冻结清单已再次核对，全部 1,071 个哈希保持一致。

当前失败调查显示：新 ShaderLibrary 检查需要独立线程的首次 realm，主单测已有 owning TLS 根会影响最后根释放和首次初始化；LTC 的连续基础值配置需要明确保留 UniformsLib 句柄；灯光差分须像同一个 Node 进程一样，在所有场景间保留同一 realm，不能每个场景重置模块版本。窄修候选专项通过，原断言、场景和比较保持不变；修复合入下一批，不改写本批失败结论。模块解析器在没有活 realm 时的修改将明确拒绝，避免报告成功后配置立即丢失。

集成前各候选的两种配置专项通过；根库首次完整重建后的 Animation Release 已通过 628 个实际上游场景和 2 cases / 22 assertions。同行独立构建另验证 30 个实际 Node 重入/newTarget 场景及 4 个闭包 GC 场景，不能代替最后集成后的完整回归。

| 集成前双配置专项 | 每种配置的证据 |
| --- | --- |
| Map/Set / Symbol | 11,794 scenarios / 96,984 comparisons；1,070 / 10,037 |
| 集合与迭代器 GC | 9 个精确快照；2 个单独记录的 clear 旧表更早释放边界 |
| 集合独立复核 | 64 次走查、16,384 次操作、205,444 次值比较；24 个原生 GC 检查 |
| 能力查询 / 环境保活 | 493 个固定上游协议场景；8 个实际 Node GC 场景 |
| ShaderLibrary / WebGLShader | 341 scenarios / 116,012 comparisons；1,610 scenarios |
| shader 单测 / 旧 uniform 回归 | 6 cases / 48 assertions；122 scenarios / 1,993 comparisons |

Shader 源包含全部 127 个 GLSL 模块、145 个字符串导出、141 个聚合键与 18 组定义。根库两项生成器 --check 通过；完整文本差分独立导入实际上游，保留原 GLSL 空白。UniformsLib 迁移为无 owning heap 的解析器，全部消费者需要匹配重编。UniformsUtils 的 clone/slice 赋值先捕获原目标；异常、回调替换及原目标写入已有实际源码失败证据。模块初始化仍采用明确的原生惰性策略，不宣称完整 ESM 加载/TDZ。

能力查询使用注入的原生命令对象，ShaderLibrary/WebGLShader 是字符串和 CPU 协议，Animation 依赖注入的帧调度 context。实际 Vulkan 设备能力、完整 program/SPIR-V/pipeline/binding，以及平台显示刷新驱动尚未因本批完成。既有 Vulkan GPU 回归将与完整测试一起运行；Linux/macOS 尚未验收，Khronos validation layer 不可用。

材质全量差分只将 CTest TIMEOUT 设为 3,600 秒：此前完整候选 Debug 耗时 1,600 秒，超过默认 1,500 秒。案例、输入、数值比较与 GC 检查点保持原样。根库上批在原默认限制内已通过，无超时失败被改记成功。

审计 1,251 模块：196 partial、745 missing、290 Vulkan pending、20 dependency pending；345 个模块存在 C++ 文件，其中 149 个位于渲染目录。新增的大部分文件为原 GLSL 字符串模块，不代表完整渲染器完成；按全量标准完成模块仍为 0。AST 错误为 0。

根库构建日志为项目外 `implementation17-final-all-{debug,release}.log`；源冻结清单 `implementation17-source-freeze.json`。ColorManagement、WebGLUtils、ShaderCache 和其他下一批依赖仍在项目外候选，不计入本批主库验证。

## 上一实施批次：材质 uniform 刷新与 Vulkan 裁剪，2026-10-13

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64 Debug、Release 全目标构建成功；完整 CTest 各 **105/105** 通过，并发数 2，耗时分别 1,651.92 秒和 136.70 秒。Debug 的 WebGLMaterials 差分耗时 1,157.40 秒。冻结清单包含 759 个生产、测试、工具、示例、构建配置和生成审计文件；两种配置测试后文件集合与全部哈希保持一致。

| Debug / Release 验证 | 每种配置的结果 |
| --- | --- |
| CPU 主单测 | 605 cases / 204,241 assertions |
| WebGLMaterials 单测 / 实际上游差分 | 6 cases / 55 assertions；2,992 scenarios / 1,298,744 comparisons |
| NativeMaterialsMath 单测 / 实际上游差分 | 6 cases / 109 assertions；1,761 scenarios / 36,079 comparisons |
| Material 共享裁剪数组差分 | 25 scenarios / 1,322 comparisons |
| Vulkan 裁剪 / 生命周期 | 59 / 38 checks；RTX 3070 Laptop |
| Vulkan 裁剪实际上游对照 | 22 绘制场景、2 个 dispose 源码案例、1 个同地址重建相机场景 |
| 既有 GPU 回归 / NativeObject 生命周期 | 664 assertions / 817 checks |
| 全部既有单测、差分及审计 | 通过；AST 错误 0 |

WebGLMaterials 进入 CPU 主库，保留全部材质与雾的 uniform 刷新分支、动态方法调用、赋值和转换顺序、抛错后的部分写入、重入、共享矩阵临时对象及普通函数构造元数据。两个返回函数保留同一构造环境中的 renderer 与 properties；候选阶段发现并修复了原先各自只捕获一个参数的生命周期差异，原失败证据保留。该 CPU 协议尚未替换 VulkanRenderer 手工 uniform 路径，不能据此宣称 GPU Physical 等材质可用。

Material.clippingPlanes 改用共享原生 Plane 数组，字段替换后旧引用仍指向旧存储，copy/clone 独立复制；NativeObject 追踪数组存储及 Plane/normal 的管理回边。此变更涉及 Material ABI，已完整重建核心库与全部消费者。普通 JS 数组、稀疏元素、任意动态元素及其完整 copy/JSON 行为仍待迁移。材质数学适配补齐 Matrix4.makeRotationFromEuler、Matrix3 运算、Vector2.negate、Color.getRGB、Texture.matrix/elements 别名与动态 updateMatrix；sin/cos 使用固定 Node/V8 fdlibm，完整自定义颜色空间和 pow 全值域位级等价仍未证明。

实际 MeshBasic Vulkan 绘制接入 WebGLClipping 的全局/局部平面、union/intersection、投影缓存和 Float32 快照；片元使用实例/形变后的视空间位置。共享相机保留真实管理身份，栈相机借用视图在退出时退役。正常完成重置相机/材质缓存，异常保留上游部分状态；dispose 的动态 WeakMap 删除支持重入。父任务另发现同地址重建相机会误用旧投影，保留修复前的黑像素失败，以不可变相机 id 校验修复，并验证重新投影后的白像素。专项 Debug、Release 各 4/4 通过，两种配置完整回归亦全部通过。

裁剪 oracle 执行固定上游投影、renderer 控制分支和片元不等式，并比较真实 Vulkan 输出；没有浏览器 WebGL 驱动像素对照。阴影 pass、alphaToCoverage、完整程序/材质以及嵌套 render 状态仍未完成。Khronos validation layer 不可用，GPU 输出 validation=0 仅为消息数。Linux/macOS 尚未验收。

父任务对材质差分脚本作独立窄修：去掉已通过 stdin 传输的重复案例文件，仅在失败时向独立临时目录写诊断。Release 完整测试确认源码树未产生这些 JSON 文件。审计为 1,251 模块：195 partial、746 missing、290 Vulkan pending、20 dependency pending；211 个模块存在 C++ 文件，其中 16 个属于渲染目录，仍没有按全量标准标记完成的模块。

项目外日志为 `implementation16-final-all-{debug,release}.log`、`implementation16-ctest-release.log`、`implementation16-LastTest-Release.log`、`implementation16-camera-{build,ctest}-{debug,release}.log` 和 `implementation16-port-audit.log`；冻结清单为 `implementation16-source-freeze.json`。Debug 完整回归日志为 `implementation16-ctest-debug.log`、`implementation16-LastTest-Debug.log`，进程终态为成功。Map/Set、Symbol/Iterator、String 与渲染能力查询的项目外候选尚未合入，不计入本批主库验证。

## 上一实施批次：Vulkan 顶点绑定、CPU 裁剪与缓存生命周期，2026-10-13

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64 Debug、Release 全目标构建成功，完整 CTest 各 **96/96** 通过；并发数 2，耗时分别 464.93 秒和 48.09 秒。冻结清单包含 738 个文件；验证期间仅更新测试 README 说明，原清单及该文件前后哈希分别保留。源码、测试代码与构建配置未变，最终文件集合和更新后清单哈希全部一致。

| 验证 | 每种配置的结果 |
| --- | --- |
| CPU 主单测 | 597 cases / 204,180 assertions |
| Vulkan GPU 既有回归 / 新顶点绑定回归 | 664 assertions / 76 checks；RTX 3070 Laptop |
| VulkanBindingCommands 内存单测 | 46 checks |
| NativeArray / NativeObject 生命周期 | 131 / 763 checks |
| WebGLClipping 单测 / 固定上游差分 | 7 cases / 52 assertions；1,486 scenarios / 225,539 comparisons |
| 裁剪数学单测 / 固定上游差分 | 7 cases / 94 assertions；191 scenarios / 16,343 comparisons |
| Array.shift 实际 Node 内建差分 | 335 scenarios / 8,175 comparisons |
| VAO 删除 / 实例数量实际上游控制流 | 2 / 7 scenarios |
| 全部既有单测、差分及审计 | 通过；AST 错误 0 |

MeshBasic 的实际 Vulkan 绘制接入 WebGLBindingStates.setup，保留交错布局、索引、默认属性与 divisor。删除缓冲名称只解绑当前 VAO，其余 VAO 保留已绑定存储；最终引用释放后退役 GPU 资源。buffer 与 vertex 命令共享错误标记，内部状态隔离可修改原型。整数 pointer 与浮点 shader 不匹配、缺失有效绑定或越界时不提交图元，但保留上层回调与逻辑统计。更新了原先错误要求 C++ 抛错的断言，保留无效输入及恢复检查。删除和越界规则依据 [OES_vertex_array_object](https://registry.khronos.org/webgl/extensions/OES_vertex_array_object/) 与 [WebGL 范围检查规范](https://registry.khronos.org/webgl/specs/latest/1.0/#ATTRIBS_AND_RANGE_CHECKING)，并用实际上游命令记录及 Vulkan 像素验证；没有实际浏览器像素对照证据。

InstancedBufferGeometry 的数量读取保留两次动态 _maxInstanceCount 访问、undefined→Infinity、instanceCount 读取及转换顺序；索引上传回调改变 count 后，无效绘制仍按上游更新统计，严格零实例不提交。原始失败日志保留。NativeArray 与 NativeObject 构造器、原型和方法缓存改用弱 TLS 加 heap 内在根；独立工作线程在退出前验证同 heap 身份、修改可见性、最终根释放与新 heap 隔离。原生 payload 的管理字段回边仍需显式 GC，其他模块 owning 根及完整类原型方法删除后的可达性仍待迁移。

WebGLClipping 与所需 Matrix3、Vector3、Plane 管理方法进入 CPU 主库，覆盖平面投影、材质缓存、阴影阶段、uniform 身份、getter、异常和重入。Array.shift 按长度快照、HasProperty/Get/Set/Delete 和最终 length 写入实现，保留 holes、继承索引及部分失败。Symbol/Proxy 等运行时边界、完整原生动态字段、完整 shader/material、光照、阴影、渲染目标和现代渲染仍未完成。Material 共享裁剪数组和实际 GPU 裁剪候选尚未合入，不计入本批主库验证。

Khronos validation layer 仍不可用；普通 GPU 输出 validation=0 仅为消息数。独立代码复核子任务被平台风险检测中止，未产生复核结论，也未计入上述测试证据。Linux/macOS 尚未验收。

审计仍为 1,251 模块：195 partial、746 missing、290 Vulkan pending、20 dependency pending；210 个模块存在 C++ 文件，其中 15 个属于渲染目录。文件存在与测试通过均不等于全量完成，没有模块按全量标准标记完成。项目外日志为 `implementation15-final-all-{debug,release}.log`、`implementation15-ctest-{debug,release}.log`、`implementation15-LastTest-{Debug,Release}.log` 和 `implementation15-port-audit.log`；冻结清单为 `implementation15-source-freeze.json`，README 更新前清单为 `implementation15-source-freeze-before-readme-docs.json`。

## 上一实施批次：uniform 协议与数组 concat，2026-10-13

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。717 个生产、测试、工具、示例与构建文件冻结后，Windows x64 Debug、Release 全目标构建成功，完整 CTest 各 **85/85** 通过；并发数 2，耗时分别 435.48 秒和 48.94 秒。测试后逐项核对文件哈希与文件集合，均未改变。

| 验证 | 每种配置的结果 |
| --- | --- |
| WebGLUniforms 单测 | 9 cases / 49 assertions |
| WebGLUniforms 实际固定上游差分 | 1,389 scenarios / 143,528 comparisons |
| Array.concat 实际 Node 内建差分 | 33 scenarios |
| CPU 主单测 | 591 cases / 204,153 assertions |
| Vulkan GPU 既有回归 | 664 assertions；RTX 3070 Laptop；validation=0 |
| 全部既有单测、差分及审计 | 通过；AST 错误 0 |

WebGLUniforms 进入实际主库，覆盖全部 35 种 uniform 枚举、结构/纯数组、共享 setter 与矩阵 scratch、缓存、纹理单元、阴影采样器排序、元数据、getter/异常/重入。C++ 入口在复制前验证线程归属，独立旧版外线程 object() 复现由 accepted/exit 2 变为 rejected/exit 0。默认纹理使用真实原生类，NativeObject 窄补丁补充子类型标志与 DepthTexture.compareFunction；借用深度访问器到普通纹理会抛 TypeError。该模块使用弱缓存与 heap 内在根，不新增 owning TLS JSValue。

concat 按长度快照、HasProperty/Get、自有属性创建与最终 length 写入实现；覆盖 holes、继承索引、getter 增删、长度变化、constructor 读取、异常、GC、重入和属性描述符。Uniforms 阴影排序动态调用该方法，没有绕开可替换的数组方法。Symbol.isConcatSpreadable/custom species、完整 iterator 与现有原生纹理字段类型边界仍未完成。Uniforms 的实际源非终止输入从 oracle 明确排除，未用人为循环上限替换生产语义。

上述记录的是 CPU uniform 调用协议；Vulkan uniform 资源、完整 shader/material、光照与裁剪未因此完成。Vulkan 顶点绑定、Clipping 数学适配与 NativeArray TLS 生命周期修复仍在项目外候选中，不计入本批主库通过结果。Vulkan 候选发现删除缓冲需区分当前与其他 VAO，按照 Khronos 规范修正资源持有并补 Vulkan 像素检查；实际 Edge 无头浏览器对照启动被自动审批以 `blocked by policy` 拒绝，浏览器结果未计入证据。Khronos validation layer 仍不可用，validation=0 是消息数，不是验证层验收。

清单仍为 1,251 模块：195 partial、746 missing、290 Vulkan pending、20 dependency pending；当前 209 个模块存在 C++ 文件，其中 14 个属于渲染目录。状态分类不是工作量完成率，没有模块按全量标准标记完成。项目外主库日志为 `implementation14-final-all-{debug,release}.log`、`implementation14-ctest-{debug,release}.log`、`implementation14-LastTest-{Debug,Release}.log`，冻结清单为 `implementation14-source-freeze.json`。新模块先行专项日志与候选原失败证据分别保留，没有放宽既有主库断言。

## 上一实施批次：资源生命周期、渲染统计与状态模块，2026-10-13

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev；Windows x64、MSVC 19.51、Node 26.5.0 / V8 14.6。704 个生产、测试、工具与构建文件冻结后，Debug、Release 全目标构建成功，完整 CTest 各 82/82 通过；并发数 2，耗时分别 434.51 秒和 49.12 秒。测试结束核对文件哈希与文件集合均未改变。

| 验证 | 每种配置的结果 |
| --- | --- |
| CPU 主单测 | 591 cases / 204,153 assertions |
| 弱生命周期独立单测 / 实际 Node 差分 | 27 cases / 261 assertions；3,483 scenarios / 17,341 fields |
| 几何与对象单测 / 上游差分 | 24 cases / 303 assertions；650 scenarios / 29,107 comparisons |
| 光照、状态、数学协议单测 | 12 cases / 82 assertions |
| 光照与状态 / 数学协议差分 | 98 / 47,607；56 / 4,182 scenarios / comparisons |
| BindingStates 单测 / 上游差分 | 12 cases / 138 assertions；4,165 scenarios / 284,375 comparisons |
| 实例属性原生桥接 | 主单测新增 4 cases / 37 assertions；23 实际上游差分场景 |
| EventDispatcher 独立单测 / 上游差分 | 6 cases / 30 assertions；11 实际上游差分场景 |
| Vulkan GPU | 664 assertions；RTX 3070 Laptop；validation=0 |
| 全部既有单测、差分及审计 | 通过；AST 错误 0 |

VulkanRenderer 接入实际 WebGLGeometries/WebGLObjects 与 WebGLInfo。几何注册、帧更新、线框缓存、dispose 顺序和计数使用上游模块，派生解码不重复执行原始上传。Info 保留活对象、动态 reset/update、共享纹理逻辑计数和 dispose 回调期间当前绘制；派生缓存弱持原始上传 data。生命周期 facade 只退役派生资源，设备销毁前关闭逃逸闭包的原生访问。GPU 测试宿主显式推进任务边界和 GC，保留旧资源释放断言并新增当前任务的 WeakRef 保活检查；生产 render/prune 不结束任务或主动 GC。SkinnedMesh 不支持绘制的旧夹具补入空 Skeleton，因为原版对象更新在材质可见性检查前访问 skeleton。

WeakRef/FinalizationRegistry 已接入原生宿主任务循环。cleanup 是独立任务，每轮最多一个 registry，返回后执行微任务检查点；异常分别记录任务和检查点阶段。cleanup 内直接请求检查点不会提前清除当前任务的弱目标；待执行队列使用 list splice 恢复，析构恢复路径不分配内存。新增回归覆盖异常后继续、重入、注入时钟及普通管理图的最后 heap 根释放。

WebGLBindingStates 已进入主构建，保留缓存、属性启停、divisor、integer pointer、默认属性与释放协议。实际 Node 差分发现枚举时新增继承键被过早缓存，已改为初始候选键快照与逐键存活检查；新增 64 场景覆盖增删、遮蔽、属性标志和原型变化。19 条跨线程入口在复制管理值前拒绝访问。原生实例属性补入类型标志与活 meshPerAttribute；描述符元数据不再额外保留旧值，直接原生 A→B 后无管理读取也可回收 A。同行独立检查两配置各通过 8 场景 / 52 检查。直接 C++ 引用赋值仍绕过管理 readonly/frozen，完整子类原型和构造器表面仍待完成。

EventDispatcher 的规范类构造器支持 new/newTarget，模块根保留同 heap 修改并随最后应用根释放；runtimeObject 不读取可修改的 prototype.constructor。数学回归曾发现 Vector3.toArray 提前转换偏移键，已修复 RHS 读取与键转换顺序；既有原生对象差分恢复为 46 scenarios / 183 comparisons，保留原失败日志。没有删除或放宽既有断言。

上述 CPU 模块不证明完整 Vulkan 光照、阴影或 BindingStates 顶点输入已完成。交错提交、完整材质/后处理、现代渲染/nodes/TSL、原生动态字段全覆盖及 Linux/macOS 仍待实现或验收。显式 `--validation` 因缺少 `VK_LAYER_KHRONOS_validation` 失败；普通 GPU 输出的 validation=0 只表示消息数，不能作为验证层通过证据。下一批 uniforms 与 Vulkan 顶点绑定候选在项目外开发，未计入本批通过结果。

重新生成的清单仍为 1,251 模块：195 partial、746 missing、290 Vulkan pending、20 dependency pending；渲染目录即使已有 C++ 实现仍归为 Vulkan pending。因此状态计数不等于已有文件覆盖率：当前 208 个模块存在 C++ 文件，其中 13 个属于渲染目录；文件存在也不等于逻辑完成。没有模块按全量标准标记完成。

项目外完整构建日志为 `implementation13-final-all-{debug,release}.log`，完整回归为 `implementation13-ctest-{debug,release}.log` 和 `implementation13-LastTest-{Debug,Release}.log`；冻结清单为 `implementation13-source-freeze.json`，审计日志为 `implementation13-port-audit.log`。保留本批专项与原始失败日志，包括 `implementation13-core-regression-ctest-release.log`、`implementation13-core-regression-LastTest-release.log` 和显式验证层失败日志。

## 上一实施批次：统一属性上传状态与 Vulkan 字节命令，2026-10-12

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev；Windows x64、MSVC 19.51、Node 26.5.0 / V8 14.6。冻结源码与测试后，Debug、Release 全目标构建成功，完整 CTest 各 70/70 通过；并发数 2，耗时分别 250.01 秒和 37.54 秒。首次全目标构建因 GPU 测试漏迁移一处 gpuType 比较而失败；改用管理值的 strictEquals 后，两种配置完整重建及回归通过，原始失败日志保留。

| 验证 | 每种配置的结果 |
| --- | --- |
| CPU 主单测 | 587 cases / 204,116 assertions |
| VulkanBufferCommands 独立单测 | 12 cases / 103 assertions |
| 原生 BufferAttribute 上传差分 | 59 scenarios / 9,720 comparisons；执行固定上游 |
| 字节命令差分 | 3,402 个规范字节场景及 2 个固定上游控制流场景 / 132,720 comparisons |
| Vulkan GPU | 615 assertions；RTX 3070 Laptop；validation=0 |
| 既有 WebGLAttributes 差分 | 778 scenarios / 26,841 comparisons |
| 全部既有单测、差分及审计 | 通过；AST 错误 0 |

普通 BufferAttribute 的 usage、gpuType、version、needsUpdate、updateRanges 和上传回调改为同一管理对象中的活字段。原生 handle 与共享 Storage 分离；包装对象只弱引用原生 handle，可在应用释放 handle 后按原类型重建，并保持包装身份、Storage 和属性 ID。新增测试覆盖默认回调、描述符、Typed/Float16/Instanced 重建、GC 和上传控制流。普通属性不提前拥有 half 标志。

VulkanRenderer 使用 WebGLAttributes 的上层算法，通过 VulkanBufferCommands 执行实际 VkBuffer 上传。命令使用真实 TypedArray 的原生视图边界，保留 WebIDL 转换、错误标记和转换重入；无效 GL 范围记录错误并继续上层流程，转换异常仍抛出。每次有效写立即保留，后续异常不回滚先前写。GPU 回归覆盖非零视图和哨兵、首次回调改变数组/版本/half 标志、无效范围继续、部分失败、方法替换、handle 重建及 dispose 重入，并比较实际像素。

原始上传缓冲在设备销毁前统一 shutdown；派生 float/uint32 绘制缓冲按写入 revision 转换，数据来自 Vulkan memory。仍有额外上传缓冲、CPU 读回及转换成本，完整原生格式绑定尚待实现。独立真实 GPU 资源 probe 已确认首次上传 3、未更新版本仍为 3、geometry.dispose 后为 0、重新上传恢复 3；释放原生 handle 后弱引用立即失效。释放管理包装并 GC 后、下一次 render/pruneResources 之前，原生派生缓存仍强持有 data，使原始上传资源暂留；此生命周期边界尚待对齐。上传回调删除当前缓存时，Vulkan 仍显式报错；上游缺失顶点缓存的跳过绑定行为待接入。原生 Float16Array、GLBufferAttribute 原生资源导入、完整交错绘制和全类动态字段/事件适配仍未完成。

字节命令的 3,402 个规范场景使用 Node BigInt oracle；独立命令单测使用内存 Storage，两者不代表实际浏览器 WebGL。真实 GPU 证据来自 VulkanRendererTests。Khronos 验证层不可用，Linux/macOS、完整材质/光照/阴影/后处理及现代渲染/nodes/TSL 尚未验收。

清单仍为 1,251 模块：195 partial、746 missing、290 Vulkan pending、20 dependency pending；没有模块按全量标准标记完成。项目外日志为 `implementation12-final-all-{debug,release}.log`、`implementation12-ctest-{debug,release}.log` 与 `implementation12-LastTest-{Debug,Release}.log`；直接 GPU 复核另存 `implementation12-gpu-{debug,release}.log`。原始构建失败日志为 `implementation12-build-all-{debug,release}.log`。最终测试期间生产源码与测试未再修改。

## 上一实施批次：弱键缓存、属性上传逻辑与渲染列表，2026-10-12

固定基准 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev；Windows x64、MSVC 19.51、Node 26.5.0 / V8 14.6。最终源码与测试冻结后，Debug、Release 全目标构建成功，完整 CTest 均为 67/67 通过。并发数为 2，耗时分别为 320.78 秒和 38.39 秒。下列计数在两种配置中一致。

| 验证 | 每种配置的结果 |
| --- | --- |
| CPU 主单测 | 580 cases / 204,043 assertions |
| 独立 WeakMap / Properties / 模块根单测 | 23 cases / 413 assertions |
| 独立 GLBufferAttribute 单测 | 9 cases / 126 assertions |
| JSWeakValue / 原生析构重入 / addons | 分别 11/483、5/75、15/185 cases/assertions |
| Vulkan GPU | 592 assertions；validation=0 |
| 原生 WeakMap 差分 | 124 Node scenarios / 1,139 comparisons |
| Array.sort / unshift 差分 | 127 Node scenarios / 87,469 comparisons |
| GLBufferAttribute 差分 | 514 scenarios / 7,102 comparisons |
| WebGLProperties 差分 | 413 scenarios / 5,783 comparisons |
| WebGLAttributes 差分 | 778 scenarios / 26,841 comparisons |
| WebGLRenderLists 差分 | 223 scenarios / 43,704 comparisons |
| WebGLInfo 差分 | 1,261 scenarios / 70,560 comparisons |
| 两种 BufferRenderer 差分 | 1,354 scenarios / 32,783 comparisons |
| 既有差分与审计 | 全部通过；AST 错误 0 |

WeakMap 使用 heap 内条件图边，只有 map 与 key 均可达才标记 value；按 key ID 唤醒的工作队列覆盖长链、多个 map 和 native 所有权。弱表延迟到首次 set 分配，原生析构后的完整重新标记完成后才剪除失效键。独立只读审查另以随机图和全扫描 oracle 验证固定点，Debug/Release 各 8 场景 / 14,388 检查通过。Symbol 键、iterator 初始化和完整 WeakMap 内建库仍未实现。

GLBufferAttribute 保留 opaque buffer 身份和动态字段。WebGLAttributes 使用该弱表保留交错 data 身份、版本比较、上传类型/大小、范围排序合并、回调和 remove 顺序；WebGLRenderLists 保留池化对象、三个分类数组、默认/自定义排序、回调重入和嵌套深度缓存。Array.sort/unshift 的原型继承索引、holes、UTF-16 比较、部分失败及比较器修改另经只读审查，额外 756 个实际 Node 场景在两配置中一致。

真实上游检查发现普通闭包函数应可构造，已修正 Info、两个 BufferRenderer、Properties、Attributes、RenderLists；类方法和箭头比较器仍不可构造。差分包含 name/length/prototype 描述符、new/newTarget、构造返回值、异常和闭包状态，保留全部原有算法场景。

另一处修正是模块函数或构造器只有弱缓存时会在同 heap 的 GC 后丢失用户修改。retainForHeapLifetime 通过 heap 内标记根保留 WeakMap、GLBufferAttribute 构造器及默认排序函数，本身不增加 heap 的共享所有权。独立目标验证普通管理图的同 heap 状态保留、最后应用根释放及新 heap 初始化；GC 与 native finalizer 重入另经 Debug/Release 各 7 场景 / 22 检查。已确认 native payload 中 owning JSValue 仍可能形成 heap 强环，完整 native 图关闭策略待实现；未以普通管理图测试掩盖此边界。

上述 webgl 模块本批完成的是 C++ CPU 调用逻辑，尚未接入 VulkanRenderer 的统一属性缓存、排序与命令提交。GLBufferAttribute 的 Vulkan 原生 buffer 导入、完整交错绘制、原生 Float16Array、Symbol.hasInstance、完整材质/光照/阴影/后处理、现代渲染与 nodes/TSL 仍待实现。GPU 断言数与上一批相同；未启用 Khronos 验证层，Linux/macOS 未验收。

当前清单为 1,251 模块：195 partial、746 missing、290 Vulkan pending、20 dependency pending；没有模块标记为全量完成。项目外最终构建日志为 `implementation11-final-all-{debug,release}.log`，完整回归为 `implementation11-ctest-{debug,release}.log`，快照为 `implementation11-LastTest-{Debug,Release}.log`。测试期间生产源码未再修改。

## 上一实施批次：弱身份、原生调度、绘制辅助与首批 addons，2026-10-12

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；冻结生产源码和测试拆分后，两种配置全目标均重新构建成功，分别完整通过 59/59 CTest。最终回归并发数为 2，Debug 耗时 286.40 秒，Release 34.84 秒。下表计数在两种配置中一致。

| 验证 | 结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 主单测 | 559 cases / 203,760 assertions | 新增绘制辅助和原生调度，以及全部既有回归 |
| 独立原生析构重入 | 5 cases / 75 assertions | 取消、注销、关闭期间释放最后根及析构回调重入 |
| JSWeakValue | 11 cases / 483 assertions | 弱根、heap/线程/记录身份、失效和原生交叉引用生命周期 |
| Addons 单测 | 15 cases / 185 assertions | VertexNormalsHelper、VertexTangentsHelper 与包装/原生所有权扩展 |
| Vulkan GPU | 592 assertions；validation=0 | 同字节数 TypedArray 替换、原始字节及范围上传、缓存类型和属性回调重入 |
| WebGLInfo 差分 | 1,237 scenarios / 69,408 comparisons | 字面量属性描述符、动态常量/转换、捕获 render 身份和 reset 顺序 |
| 两种 BufferRenderer 差分 | 1,252 scenarios / 26,900 comparisons | 接收者、模式/索引缓存、多次绘制、异常和重入顺序 |
| Vertex helpers 差分 | 120 scenarios / 10,220 comparisons | 固定容量、变换、交错/Float16/normalized、clone/dispose 和转换重入 |
| 原生调度顺序差分 | 4 Node scenarios | 正常零延迟 timer、Promise 和 microtask 顺序 |
| 全部既有差分及审计 | 通过；AST 错误 0 | 未删除或缩减既有几何、动画、运行时与 GPU 夹具 |

NativeEventLoop 单测合计仍为 20 cases / 1,083 assertions：主进程内 15 / 1,008，独立析构进程内 5 / 75。首次两配置完整 58 项均为 57/58，通过项包含 GPU 和全部差分；主单测中的五个新增析构用例失败，共八条失败断言。已实证 MSVC 在新 worker 初始化已链接模块的动态 TLS，UniformsLib 的全局 JSValue 保留 heap，因此单纯换线程无法创造最后根释放条件。五项移入仅引用原生运行时的独立目标，仍链接当前完整核心静态库；不放宽原断言，也不用显式 GC 替换被测析构时序。另以预热 Promise、UniformsLib 和 WebGLInfo 的完整库独立验证常规 15 项。

原生调度修复取消/注销时先清调度数据、再摘除和释放根；关闭先拒绝所属线程 API 重入，再逐个释放根。独立微任务检查点执行期间禁止原生 pump/wait，等待通过 wake revision 保留注入时钟唤醒，避免通知丢失。JSWeakValue 不保留 heap 或 GC root，访问前验证单调线程身份和不复用的记录 ID；它不提供完整 WeakRef 或 WeakMap 语义。

Vulkan 属性上传保留初次缓存的组件类型和字节宽度，后续同字节数 TypedArray 替换按原始字节处理；部分上传分别换算源/目标偏移。属性更新先快照名称，再逐个重读当前字段，覆盖上传回调替换、删除、追加及显式 null。新增二十条 GPU 断言；既有资源及像素回归一并通过。

两个 addons 通过独立 `threecpp::addons` 库和显式头文件使用；差分中有符号零精确比较，有限浮点阈值为 1e-12。NativeObjectExtension 将实际 owning slot 和 JSValue root 接入核心 GC，保持 Object3D ABI；仅 parent 原生拥有 helper 时仍保留源包装字段，不可达 JS/原生交叉环可回收。核心不引用 addons 类或构造器；独立不链接 addons 对象文件的核心程序已链接并运行通过。任意 object 类型、Normals flag 任意 JS 值、字段删除/描述符替换及完整动态方法仍待补齐。

三个 webgl 辅助模块已按上游实现 CPU 调用逻辑，尚未替代 VulkanRenderer 的完整统计与命令路径。线/点/Sprite 的 GPU 绘制、完整纹理/材质/光照/阴影/后处理，以及现代渲染、nodes/TSL 仍待实现。NativeEventLoop 的入口检查点属于原生宿主策略，Node 差分不能证明完整 DOM 调度；自动驱动、网络 I/O 和平台输入/音频仍待实现。Khronos 验证层、Linux/macOS 和完整渲染图像对比尚未验收。

当前审计为 1,251 模块：194 partial、747 missing、290 Vulkan pending、20 dependency pending；AST 错误 0。没有模块标记全量完成。构建和日志位于项目外：最终构建为 `implementation10-isolation-final-all-{debug,release}.log`，最终回归为 `implementation10-isolation-ctest-snapshot-{debug,release}.log` 和 `implementation10-isolation-LastTest-Snapshot-{Debug,Release}.log`；保留首次失败的 `implementation10-ctest-snapshot-*` 与对应 LastTest 日志。

## 上一实施批次：辅助对象、相机、加载器与原生异步基础，2026-10-12

新增 13 个 helpers、7 个核心/相机模块和 7 个加载模块。Loader.loadAsync 接入原生 Promise 与显式微任务队列。新容器 ABI 全量重编完成；Release 最终完整 52/52 CTest 通过。Debug 最终完整运行 51/52，唯一失败为新增 WebGLInfo 文件使审计清单过期；重新生成清单后，该审计专项 1/1 通过。全部 52 项已分别通过，不将 Debug 结果表述为一次完整 52/52。

首轮两配置均为 51/52；primitive-geometries 差分发现 BufferGeometry.toJSON 提前执行 stringify，丢失 -0、NaN/Infinity。修复 Geometry 和 detached IBA 的结构快照，groups 保留上游独立 stringify 规则；新增 125 条数值/位模式断言、14 条快照/stringify 区别断言，并重新编译所有受影响目标。Release 该差分最终通过 121 cases / 1,556,574 comparisons，耗时 21.00 秒。首轮 Debug 大型用例如实记录 255.39 秒和失败，未截断或删除夹具；完整回归日志分别保留首轮与修复后结果。

两配置最终 CPU 均为 520 cases / 202,259 assertions；GPU 均为 572 assertions，RTX 3070 Laptop，validation=0。Release 完整结果来自 `implementation9-ctest-snapshot-release.log` 和 `implementation9-LastTest-Snapshot-Release.log`；Debug 为 `implementation9-ctest-snapshot-debug.log`、`implementation9-LastTest-Snapshot-Debug.log` 和 `implementation9-audit-final-debug.log`。Debug primitive-geometries 最终通过 121 cases / 1,556,574 comparisons，耗时 251.85 秒。Khronos 验证层、Linux/macOS 和完整渲染图像对比仍未验收。

原生 Promise 的 34 个同步执行器、thenable、链式反应、catch/finally、循环拒绝、微任务顺序、newTarget 原型回退与原型修改场景已与 Node v26.5.0 直接差分通过。Promise/OrderedMap 独立 Debug 单测共 6 cases / 65,582 assertions 通过。仍缺少 Promise 聚合方法、Symbol/species、宿主未处理拒绝报告与自动事件循环。

OrderedMap 改为自有稳定双向节点、哈希查找与数值键有序索引，修复大型几何数组转入 JSValue 时的线性查找，以及 MSVC Debug 持久 list iterator 导致的平方级销毁/删除；保留数值键顺序、引用及双向迭代器稳定性、copy/move/swap 与删除/重插入行为。32,768 键用例从 15.757 秒降至 0.201 秒；原始大型几何差分已在两配置重新通过。只读审查另执行 30,000 次随机操作、30 个复制异常点并发现 move 的错误 noexcept，已取消移动构造的该保证，移动赋值改为 clear/swap。

新增模块的独立证据：Helpers 为 107 scenarios / 35,438 comparisons 与 10 cases / 50 assertions；Core/Camera 为 108 scenarios / 17,786 comparisons 与 16 cases / 182 assertions；Loader foundation 为 14,904 scenarios / 15,324 comparisons，parser/loadAsync 为 103 scenarios / 4,350 comparisons，loader 单测为 10 cases / 65 assertions。Unicode 官方 NormalizationTest 另通过 100,170 个 NFC 比较。上述模块在最终容器 ABI 后的两配置完整运行中再次通过。

当前审计为 1,251 模块：192 partial、749 missing、290 Vulkan pending、20 dependency pending；AST 错误 0。全部模块仍未标记全量完成。构建与日志保存在项目外，使用 `implementation9-*` 前缀。

## 上一实施批次：交错属性、基础对象与 Vulkan 实例化，2026-10-12（回归未全过）

新增 VertexAttribute 协议、InterleavedBufferAttribute、Line/LineLoop/LineSegments/Points/Sprite 与 LOD；CPU 几何、包围体、拾取和原生所有权路径已接入。Vulkan 增加实际实例矩阵/颜色/morph 绘制、资源及绑定缓存、回调重入时的资源保留和 LOD 自动选择。线/点/Sprite 仅接入渲染列表筛选，实际 GPU 绘制仍明确拒绝。

独立验证已通过：InterleavedBufferAttribute 46 场景 / 7,075 次差分；基础对象 552 场景 / 15,030 次差分；LOD 176 场景 / 2,948 次差分。新增矩阵借用回调、返回/异常后快照和 GC 单测通过。上游实例化 oracle 执行实际 WebGL 模块，覆盖 8 个绑定、6 个生命周期、14 个数量及 2 个动态转换场景。

冻结源码后 Debug 全目标构建成功。CTest 46/47 通过：CPU 为 472 cases / 136,210 assertions，GPU 为 572 assertions；primitive-geometries 差分暴露大型数组序列化性能回退，持续运行超过 400 秒后主动终止该进程，CTest 如实记录失败。Release 构建也在发现问题后主动终止，未完成验收。性能修复及新模块在下一批重新完整验证。Khronos 验证层不可用，validation=0；Linux/macOS 尚未验收。

本批审计为 1,251 模块：165 partial、776 missing、290 Vulkan pending、20 dependency pending；AST 错误 0。全部模块仍未标记全量完成。构建和日志位于项目外，使用 `implementation8-*` 前缀。

## 上一完整验收批次：灯光、材质、骨骼与共享资源，2026-10-12

基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；冻结源码后 Debug 与 Release 全目标均构建成功，分别完整通过 43/43 CTest。下表计数在两种配置中一致。

| 验证 | 结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 450 cases / 135,881 assertions | 新增灯光、材质、蒙皮/实例化、构造、uniform、交错缓冲与原生所有权回归 |
| Vulkan GPU | 464 assertions；validation=0 | 既有像素/资源回归，以及可见实例化和蒙皮对象的显式拒绝路径 |
| Lights 差分 | 235 scenarios / 36,563 comparisons | 默认值、copy/clone、阴影资源、更新顺序、dispose 与球谐系数 |
| Material 差分 | 226 scenarios / 105,451 exact comparisons | 18 类传统材质的字段、参数、版本、克隆、uniform 与序列化 |
| Skinning / Instancing 差分 | 79 scenarios / 85,189 comparisons | 骨骼矩阵、pose、骨骼纹理、CPU 蒙皮、实例属性、包围体、拾取及真实动画绑定 |
| RenderTarget 差分 | 62 scenarios / 8,577 exact comparisons | MRT、共享源/图片、深度所有权、resize/dispose、clone 回调替换数组 |
| Uniforms 差分 | 122 scenarios / 1,993 exact comparisons | Uniform/UniformsGroup、Array.map、UniformsUtils 克隆/合并/颜色空间 |
| UniformsLib 差分与生成检查 | 16 groups / 84 uniforms | 默认值、键顺序、194 个对象的身份关系、克隆别名、修改隔离与回收 |
| Construct 差分 | 217 scenarios / 4,108 comparisons | 类/函数构造器、newTarget、原型、描述符、返回值、异常、捕获与 GC |
| InterleavedBuffer 差分 | 43 scenarios / 4,665 comparisons | 普通/实例交错缓冲、九类 TypedArray、共享视图、重叠修改、原始 NaN、元数据与回调 |
| 全部既有差分与审计 | 通过；审计 5 项；AST 错误 0 | 1,251 模块：158 partial、783 missing、290 Vulkan pending、20 dependency pending |

传统材质及灯光已补入 CPU 对象行为，ShaderMaterial 复用按上游目录导出的 UniformsUtils；UniformsLib 由固定源码 AST 生成独立默认对象。Uniform/UniformsGroup 与 InterleavedBuffer 使用受管理的活对象图。JSValue 的原生 construct 实现构造调用语义，不执行 JavaScript 源码；现有全部类的构造器仍需逐项迁移，Symbol/species、完整装箱及其他动态边界仍待完成。

NativeArray 统一 TypedArray View 和 ArrayBuffer 的包装身份，并将属性数组、九类纹理数据、骨骼矩阵/纹理、轨道、切线、插值器与 PropertyMixer 的真实所有权槽纳入图。单测验证仅原生 owner 存活时保留包装扩展属性，外部 ArrayBuffer 保持共享存储，释放最后外部引用后回收数组回指环，以及字段替换后旧存储回收。Scene.fog、Skeleton.boneMatrices 等缺失的所有权/包装追踪也已补齐；未登记类型和纯 C++ 强引用环仍有原有边界。

合并构建修正 Fog/FogExp2 字符串字面量重载歧义；Frustum/Box3 补齐对象自身包围体优先级、回调修改几何后的生命周期和空包围体异常。RenderTarget.copy 保留 clone 回调前的赋值目标数组，随后按上游重新读取当前 textures。早期并行编辑产生的过期目标文件经冻结源码并重编处理，最终两种配置的结果均来自冻结后的完整运行，未放宽断言。

Vulkan 仍未实现新增灯光、完整材质、渲染目标、实例化、蒙皮或 UBO 的 GPU 路径。实例化/蒙皮在可见性、层、视锥和材质过滤后，对实际提交项明确报错；新增 31 条 GPU 断言验证该边界。GPU 为 RTX 3070 Laptop，Khronos 验证层不可用，validation=0。Linux/macOS 和完整 WebGL/WebGPU 图像差分未验收。全部模块仍未标记全量完成。

构建和日志位于项目外，增量链接关闭。最终 Debug 日志为 `implementation7-frozen-debug.log`、`implementation7-ctest-debug.log` 和 `implementation7-LastTest-Debug.log`；Release 为 `implementation7-all-release.log`、`implementation7-ctest-release.log` 和 `implementation7-LastTest-Release.log`。

## 上一实施批次：动画播放与原生生命周期，2026-10-12

基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；Debug 与 Release 全目标均构建成功，各 34 项 CTest 最终通过。Debug 完整运行 34/34；Release 首轮 33/34，修正下述旧生命周期测试后重编并复测 CPU 通过，其余已通过项未重复执行。

| 验证 | 两种配置的结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 397 cases / 135,313 assertions | 动画播放、运行时分发、原生所有权及全部既有回归 |
| Vulkan GPU | 433 assertions；validation=0 | 保留上一批像素与资源生命周期回归 |
| AnimationAction 差分 | 533 scenarios / 171,592 exact comparisons | 播放、循环、倒放、fade/warp、权重、事件与边界 |
| AnimationMixer 差分 | 165 scenarios / 105,928 exact comparisons | 原生对象、共享绑定、多根、重入、缓存与基础值 for-in |
| Mixer 内部缓存差分 | 118 scenarios / 226,398 exact comparisons | 活动/空闲交换、计数、更新顺序、动态方法与访问器 |
| AnimationObjectGroup 差分 | 88 scenarios / 210,267 comparisons | 添加/移除/取消缓存、共享绑定、统计与身份 |
| AnimationRuntime 差分 | 15 scenarios | 活字段、原型、共享数组、插值方法覆写及回调内 GC；Node 使用 --expose-gc |
| Runtime core 差分 | 547 scenarios / 16,146 comparisons | Array 基础方法、访问器、部分修改、Function.call、EventDispatcher |
| 其余差分与审计 | 全部通过；审计 5 项；AST 错误 0 | 1,251 模块中 115 partial、826 missing、290 Vulkan pending、20 dependency pending |

AnimationAction、AnimationMixer、AnimationObjectGroup 使用受管理的权威对象图；AnimationRuntime 将原生轨道、片段、插值器和绑定接入该图。实际场景测试覆盖位置、旋转、颜色、材质、morph、布尔/字符串轨道、子节点、共享绑定及回调重入。固定上游自身的 Group uncacheClip、部分 uncacheRoot 后重用旧 action 异常按原行为保留。

原生 evaluate 内部动态读取 interpolate_、copySampleValue_、intervalChanged_，Cubic 同样读取 getSettings_，同时保留默认 C++ virtual 派发。新增差分覆盖运行时替换方法、访问器抛错、部分缓存修改及回调触发回收；既有 50 个插值器场景、2,208 个比较仍通过。

GC 按 shared_ptr 控制块登记原生字段与真实所有权槽，保留外部原生 owner 对应包装的身份和动态字段，回收 JS 与原生对象交叉形成的不可达环。分阶段析构允许析构函数读取和保存普通 JS 字段，重新标记后清除记录。纯 C++ 强引用环仍需显式打断；不透明 std::function 捕获不能自动追踪，未登记字段、自动 GC 调度和最终性能仍待完成。

合并回归发现旧 NativeObject 测试在仍持有 oldMaterial shared_ptr 时要求包装被回收，与新增外部所有权规则冲突。测试改为先验证 extra 字段保留，再释放最后一个外部 owner，验证 weak_ptr 失效和包装缓存清除；未通过放宽生产逻辑处理失败。早期硬编码内建对象数量的测试也改为记录当前 realm 基线，显式回收后比较。

两种配置均运行 `animation_keyframes` 示例：四次 0.25 秒更新得到 x=0.5、1、1.5、2，stop 后恢复 x=0。构建及完整日志位于项目外，增量链接保持关闭。

CMake 新增浮点 std::from_chars/to_chars 编译检查，MSVC 通过。本机 MinGW GCC 11.2 的部分动画翻译单元可编译，但其标准库缺少浮点 from_chars，完整库无法通过；独立配置已验证明确报错。没有声称通过 MinGW、Linux 或 macOS 验收。

GPU 为 RTX 3070 Laptop，Khronos 验证层仍不可用，validation=0。新增模块仍未标记完整完成；动态构造、完整装箱与 species、原生字段描述符/容器身份限制，以及完整传统/现代渲染、官方扩展和三平台验收继续见 PORTING.md。

## 上一实施批次：属性绑定、原生对象与排序，2026-10-12

基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51。冻结后的 Debug 与 Release 全目标均构建成功，分别通过全部 28 项 CTest。本节各项计数在两种配置中一致。

| 验证 | 结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 352 cases / 135,017 assertions | 新增宿主运行时、原生对象、属性绑定、排序及既有回归 |
| Vulkan GPU | 433 assertions；validation=0 | 手工 mipmap、多级旧资源保留、版本/共享 source、分组和 NaN 排序、范围合并身份、morph 缺失与零 draw |
| PropertyBinding 差分 | 532 scenarios / 17,166 comparisons | 路径/节点查找、四类读写、版本标记、Composite、默认派发表、属性与转换回调顺序 |
| NativeObject 差分 | 46 scenarios / 183 comparisons | 实际场景/数学/材质/纹理绑定、数组身份、描述符/只读长度、Euler 与遍历重入 |
| V8 排序差分 | 1,272 cases / 1,738,420 comparator calls | 最终身份及比较调用逐项精确相同；NaN、非传递/状态比较器、异常和回调追加元素 |
| Animation 差分 | 642 scenarios / 125,839 comparisons | 新增长 NaN 时间数组、getter/valueOf 顺序及修改原数组；排序/trace 使用精确比较 |
| earcut 差分 | 728 cases / 111,741 exact triangle indices | 原 NaN 多洞失败复现及 100 个新增多 NaN 孔洞用例 |
| Path/Shape 差分 | 9,472 values/identities | 新增 NaN/Infinity 面积排序、空子路径/空点 TypeError |
| Raycast 差分 | 4,350 values | 非有限距离排序、共享数组替换/删除/缩短、morph 缺失/空位/扩展与 clone/JSON |
| WebGL 调用 oracle | 16 mipmap traces；8 次属性上传 | 实际执行固定 WebGLTextures/WebGLAttributes 源码，以记录调用的 GL 对象检查分支、时序及参数；不是完整 GPU 图像对比 |
| 其余差分与审计 | 全部通过；AST 错误 0 | 保留上一批全部回归；1,251 模块中 112 partial、829 missing、290 Vulkan pending、20 dependency pending |

共享宿主对象增加弱身份缓存、内部图边与原生字段追踪。独立单测覆盖回调重置调用者后继续属性访问、GC 保留子包装动态字段、替换字段释放旧包装，以及原生析构期间分配/嵌套回收。引用图的完整跨原生 owner 循环回收、仅原生对象存活时的包装属性持久化仍未完成。

V8 TimSort 是原生 C++ 算法适配，未嵌入 V8 或 JS 引擎。固定 Node v26.5.0 / V8 14.6.202.34-node.24；源 SHA-256 和 V8/Python 完整许可证保留在 upstream.json、src/core/ArraySort.LICENSE。原 earcut NaN 失败已修复；历史批次中的失败记录保留为当时证据。

首轮合并测试曾在新增 morphMissing 脚本与旧探针二进制之间出现字段不匹配。冻结全部源码并重编后，正式 Debug 与 Release 分别 28 项全过；早期失败日志另行保留。全部构建产物和日志位于项目外，增量链接保持关闭。

GPU 为 RTX 3070 Laptop。VK_LAYER_KHRONOS_validation 仍不可用，validation=0；没有声称通过验证层。Vulkan 缩短 morph 权重数组所需的旧 uniform 尾部状态仍未实现，非零绘制明确拒绝；零绘制保留上传、回调和统计。Linux/macOS、完整 WebGL/WebGPU 图像对比、其余材质/节点/扩展与全部动态 API 继续待验收。

## 上一实施批次：纹理、几何体与动画，2026-10-11

基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；Debug 与 Release 全目标均构建成功，分别通过全部 23 项 CTest。下表两种配置结果一致。构建产物与完整日志保存在项目外。

| 验证 | 两种配置的结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 325 cases / 134,617 assertions | 纹理、关键帧/片段/混合、几何体及全部既有回归 |
| Vulkan GPU | 351 assertions；validation=0 | 原有 229 项、纹理 99 项、属性上传 17 项、分组/drawRange 6 项；真实像素读回 |
| 基本几何差分 | 121 cases / 1,556,574 comparisons | Box/Plane 与九种新增基本几何；分段、索引/法线/UV、参数和 JSON |
| 高级几何差分 | 103,833 values/identities | 多面体、Shape、Tube、Edges、Wireframe；孔洞、Frenet、克隆与 JSON |
| Extrude 差分 | 47,201 values/identities | 倒角、孔洞、挤出路径、自定义 UV；数组缩短后的小数分组、Frenet 虚调用、getter/转换/描述符次序 |
| Texture 差分 | 42 scenarios / 3,773 comparisons | 源/图片/数组身份、版本、复制、UV、材质/场景资源缓存及完整输出键序；材质纹理回调后读取后续字段 |
| Animation 差分 | 418 scenarios / 20,331 comparisons | 轨道、片段、切线、裁剪/优化、数组类型、回调重入与生命周期 |
| PropertyMixer 差分 | 338 scenarios / 119,120 comparisons | 累积/加法/恢复、严格变化检测及回调身份；包含 24,040 个 sin/acos 位级结果 |
| Storage 差分 | 35 scenarios | 原有存储/数值/重入，严格 true 更新、double 版本、更新范围/回调身份、group/drawRange 数值 |
| 既有差分与审计 | 全部通过；审计 5 项；AST 错误 0 | 数学、查询、核心、曲线、路径、插值器、半精度、earcut 与运行时回归 |

Vulkan 纹理支持 2D DataTexture 的 RGBA/RG/Red × UnsignedByte/Float/HalfFloat，map/alphaMap、UV 通道/矩阵、wrap/过滤/flipY/sRGB/预乘、自动 mipmap，以及 Source 共享、更新范围、回调与 dispose。已修复同 draw 共享 source 的两个采样器读取不同上传版本、回调修改材质 uniform 过早生效等差异。属性上传还覆盖 byte offset 与 element offset 的独立转换、count=0、负字节偏移、空回调错误和默认回调共享身份。

PropertyMixer 的严格变化比较暴露了平台 libm sin 舍入造成的额外 setValue 回调。采用固定 Node/V8 的原生 fdlibm sin/acos 后，保持严格比较与回调次数；源哈希和许可证见 upstream.json、src/math/JSNumberMath.cpp。没有嵌入 JavaScript 引擎。

Debug 增量链接曾出现 MSVC LNK1163；使用 `/p:LinkIncremental=false` 后全目标构建和测试通过。未通过修改测试条件规避该错误。

移植清单现为 1,251 模块：111 partial-unverified、830 missing、290 vulkan-port-pending、20 dependency-unverified。本批模块仍不标记全量完成。NaN 不一致排序、资源 JSON 活对象图、具体动画绑定/播放、其他材质/纹理格式、完整传统/现代渲染与官方扩展继续跟踪于 PORTING.md。

GPU 为 RTX 3070 Laptop；显式运行 --validation 确认本机缺少 VK_LAYER_KHRONOS_validation，validation=0。Linux/macOS 与完整 WebGL/WebGPU 图像差分仍未验收。

## 上一实施批次：路径、插值器与半精度，2026-10-11

基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；Debug 与 Release 全目标均构建成功，分别通过全部 17 项 CTest。下表两种配置结果一致。构建产物与完整日志保存在项目外。

| 验证 | 两种配置的结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 257 cases / 132,362 assertions | 曲线、路径/形状、插值器、半精度及全部已有回归；其中半精度含逐编码穷举，断言数不代表模块覆盖率 |
| Vulkan GPU | 229 assertions；validation=0 | 半精度顶点/颜色与缓存更新；浮点逻辑计数；尾部 wireframe 索引补零与 Uint16/32 转换；仅检查实际 fetch、drawRange 外 NaN/坏索引和合法范围恢复 |
| 新曲线差分 | 1,351 queries / 7,395 comparisons | Ellipse/Arc/Spline/CatmullRom、公共插值函数、共享点集合、缓存/复制/JSON、非法类型的历史系数 |
| Path/Shape 差分 | 3,996 values/identities | 2D/3D、闭合点身份、基类默认采样、虚调用、hole/fill-rule、三角化；getter/clone/toJSON 回调替换容器时的读取顺序与生命周期 |
| Interpolant 差分 | 50 scenarios / 2,208 comparisons | 五种插值、缓存搜索、结果类型/身份与共享视图；延迟和重复 ToNumber、Cubic stride/Wrap 字符串加法、settings 虚调用和重入 |
| DataUtils/Float16 差分 | 24 scenarios / 257,333 comparisons | 全部 65,536 个 half 解码、190,464 个有限边界编码输入；NaN payload、raw/component 语义、normalized、虚方法、变换、ArrayBuffer 共享构造与独立视图 |
| earcut 差分 | 627 cases / 74,685 exact triangle indices | 孔洞、退化、自交、hash 路径、deviation/flatten；有效用例索引顺序逐项相同 |
| Storage 差分 | 28 scenarios | 原有共享存储/转换；浮点 itemSize/count/index、尾部法线、fractional toNonIndexed、getter 顺序、count/矩阵回调与变换重入 |
| 既有 Runtime/Curve 与五组差分 | 全部通过 | Runtime 22 traces / 674；Extended Runtime 7 scenarios / 15,345 conversions；原六条曲线 1,387；几何/核心/辅助/拾取/查询回归 |
| 移植审计 | 5 项自验通过；AST 解析错误 0 | 1,251 模块；68 partial-unverified、873 missing、290 vulkan-port-pending、20 dependency-unverified；未自动认定任何模块完整 |

独立交叉审查发现的插值器转换顺序、路径默认值/虚派发/容器重入、Vulkan 尾部顶点/线框/统计差异均已加入回归并修复。存储差分直接执行固定上游；整数、原始位、回调 trace 与身份关系按脚本精确比较，有限非整数按各脚本明确的容差比较。

仍有明确未通过边界：`earcut-nan-boundary.mjs` 的多个 NaN-x 孔洞使上游排序比较器不一致，Node v26.5.0 / V8 14.6.202.34-node.24 与 C++ 排序输出不同。该独立诊断已实际运行并返回 1（首差索引位置 24：C++ 15、JS 27），未注册为通过的 CTest，也未视为修复。其他动态参数和静态适配限制见 PORTING.md。

GPU 为 RTX 3070 Laptop 实际像素测试；validation=0 表示未启用 Khronos 验证层。Linux/macOS、完整 WebGL/WebGPU 图像差分与全部 API 仍未验收。本批通过不改变任何模块的 verifiedComplete=false。

## 上一实施批次：运行时、TypedArray 与曲线，2026-10-11

固定基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；Debug 与 Release 全目标成功构建，分别通过全部 12 项 CTest。全部构建与日志位于项目外。

| 验证 | 两种配置的结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 200 cases / 2,462 assertions | 原有回归；函数/原型/描述符/转换、GC 和重入、UTF-16、共享字节/视图、TypedArray 迭代器与转换、曲线与缓存 |
| Vulkan GPU | 199 assertions；validation=0 | 原有回归；clip-z 排序与反向深度、零数量 geometry/index 上传和 hooks/draw、空 fog/morph/wireframe、共享 TypedArray 写入不提前修改快照 |
| 扩展 Runtime 差分 | 7 scenarios / 15,345 conversions | 原生函数身份/捕获/接收者、继承与访问器、只读描述符/数组长度、转换顺序、UTF-16；随机 binary64 shortest string、长 radix/decimal、精确 halfway 与远尾 sticky |
| Storage 差分 | 20 scenarios | view/buffer 身份、偏移与范围、重叠 set、整数/clamped/float 转换、NaN 原始位、溢出中点、跨类型构造、属性 copy/clone、toNonIndexed 缺失值及原数组长度 |
| Curve 差分 | 6 concrete classes / 1,387 values and identity checks | 直线和二次/三次贝塞尔 2D/3D；采样、长度/缓存、target 别名、copy/clone、边界映射与开/闭 FrenetFrames |
| 原有 Runtime 与五组 JS 差分 | 全部通过 | Runtime 22 traces / 674 comparisons；原有几何/材质、core、helper、raycast、query 验证保持通过 |
| 移植审计 | 5 项自验通过；AST 解析错误 0 | 1,251 模块；49 partial-unverified、892 missing、290 vulkan-port-pending、20 dependency-unverified；verifiedComplete 仍全部 false |

扩展 Runtime 的转换结果逐值对照 Node，包含 NaN/Infinity/负零类别。Storage 对类型、字节/视图身份、数值及错误类别精确比较；Curve 的有限值容差由 compare-curves.mjs 声明，身份与缓存关系单独比较。独立只读审查另核对 50,000 个随机 double 字符串和 880 个精确中点/sticky 输入；这些是补充审查证据，不计为完整模块验收。

GPU 实际运行于 RTX 3070 Laptop；validation=0 表示未启用 Khronos 验证层。Linux/macOS、完整 WebGL/WebGPU 图像差分、全部动态 API、材质/纹理/节点/扩展仍未验收。Curve 与运行时的静态适配和剩余边界见 PORTING.md。

## 上一实施批次：2026-10-11

基准仍为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9` / r187dev。Windows x64、MSVC 19.51；Debug 和 Release 全目标均成功构建，分别通过全部 9 项 CTest。构建产物与日志位于项目外，未重新创建仓库内 build 目录。

| 验证 | 两种配置的结果 | 本批覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 163 cases / 1,911 assertions | 原有回归，以及公开分量、width/height 别名、旋转回调/复制/移动、自替换回调、运行时图与 GC、JSON 顺序、userData 共享引用 |
| Vulkan GPU | 168 assertions；validation=0 | 原有场景，以及无 needsUpdate 保持旧像素、部分范围上传、共享属性、索引更新、上传回调时序、早期 draw 缓冲保留、dispose/弱引用、属性 ID、上传回调重入拒绝和析构后 dispose |
| Runtime 差分 | 22 typed traces / 674 comparisons | 类型、undefined/null、NaN/Infinity/负零、键顺序、空位、别名/循环、直接赋值、回调顺序、Object3D 同步、userData 身份及 TypeError/RangeError 类别 |
| 移植审计 | 5 项自验通过；AST 解析错误 0 | 755 核心 + 496 addons 模块、119 目录、8 非 JS 资源；动态 API 盲区与未验收状态显式保留 |
| 原有五组 JS 差分 | 全部通过 | 几何/材质 3,472 values、core 1,741、helper 396、raycast 606、query 718 samples / 14,301 comparisons |

Runtime 差分精确比较类型、身份、属性/回调顺序、错误类别和整数；有限非整数使用 `2e-13 * max(1, abs(expected))`。普通 JSON 不再代替运行时身份/类型证据。GC 生命周期另由 C++ 单元测试验证。

GPU 是实际 Vulkan 像素测试，尚未建立完整 WebGL/WebGPU 图像差分 oracle。当前机器仍未启用 Khronos 验证层；Linux/macOS 验收也未执行。完整材质/纹理/节点/平台与动态语义仍待移植；全部 1,251 模块的 verifiedComplete 保持 false，不能把本批通过的用例扩展为模块全量一致性证明。

## 历史基线

基准：three.js r187dev，commit `11738a9d55e502fb484cefffcea3a66dc44e9fe9`。Windows x64，MSVC 19.51，CMake 4.3.1，Node.js 26.5.0；Vulkan 设备为 NVIDIA GeForce RTX 3070 Laptop GPU。

Debug 和 Release 均成功构建，均通过全部 7 项 CTest。

后续目录重构按 three.js 层级迁移头文件、测试、工具、示例和着色器；已重新完成 Debug 构建并通过全部 7 项 CTest。`src/` 与 `test/unit/src/` 的现有子目录均对应锁定 upstream，未建立空占位目录。此次构建产物位于项目外的临时目录。

| 验证 | 结果 | 覆盖 |
| --- | --- | --- |
| CPU 单元测试 | 145 cases / 1,760 assertions | 数学、场景图、相机、事件、共享资源、拾取、Group 身份 |
| Vulkan GPU 测试 | 113 assertions | 实际图形管线与像素读回；深度、混合、剔除、排序、雾、Morph、回调清屏/读回、生命周期 |
| 几何/材质差分 | 3,472 values / 12 geometry scenarios | TypedArray、属性变换、几何、材质、Morph、复制与非有限值 |
| Core/Camera/Scene 差分 | 1,741 values | 世界变换、投影、完整 JSON、资源缓存及虚拟序列化调用 |
| Helper 差分 | 396 values | Box2、Spherical、Cylindrical、Matrix2、裁剪平面克隆 |
| Raycast 差分 | 606 values | Mesh 交点、UV/法线/重心坐标、side/groups/drawRange、层级/图层、Morph、相机射线 |
| Query 差分 | 718 samples / 14,301 comparisons | 平面、线段、三角形、射线、视锥、退化/边界、NaN/Infinity/负零、target 别名 |

数值差分直接执行锁定 checkout 的 JavaScript。有限值容差在各脚本明确声明；查询套件对 NaN、Infinity、负零分别比较。GPU 测试覆盖指定场景，未建立全部 three.js 场景的图像等价证明。

GPU 测试记录 `validation=0`：当前机器没有 `VK_LAYER_KHRONOS_validation`，未声称通过验证层检查。活动渲染回调中的嵌套 render、setSize 仍显式拒绝；clear、readPixels 已按当前 GPU 绘制顺序验证。

离屏示例在验证时生成 512×512 图像：7 次 draw、14 个三角形，PNG 已目视检查。输出路径为 `build/parity/vulkan-basic.ppm` 和 `build/parity/vulkan-basic.png`；这些图像及构建产物已在后续清理中删除，可重新生成。

以下为当时执行的验证命令；重新验证前，先按 README 配置构建目录。

```powershell
cmake --build build/parity --config Debug --parallel 6
ctest --test-dir build/parity -C Debug --output-on-failure
cmake --build build/parity --config Release --parallel 6
ctest --test-dir build/parity -C Release --output-on-failure
./build/parity/bin/Debug/vulkan_basic.exe ./build/parity/vulkan-basic.ppm
```

当时清单仅枚举 755 个核心模块：42 个已有移植入口、423 个尚缺源码、290 个 renderer 模块等待对应 Vulkan 实现。当前清单已扩展到 1,251 个核心与 addons 模块，第三方依赖单列；详见 `PORTING.md`、`API-INVENTORY.md` 与 `port-status.json`。
