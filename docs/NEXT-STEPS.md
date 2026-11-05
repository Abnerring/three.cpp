# NEXT-STEPS — 接手指南（由多轮移植沉淀，最后更新：2026-11-06）

本文件是**精简接手入口**。完整历史在 `docs/HANDOFF.md`（>1MB，按时间从新到旧），
进度真源是 `docs/port-status.json`（由 `utils/port-audit.mjs` 生成，勿手改）。

## 1. 总进度（审计口径，2026-11-06）

| 范围 | missing | partial-unverified | 其他 |
|---|---|---|---|
| `src` | **0** | 465 | vulkan-port-pending 290 |
| `examples/jsm` | **322** | 154 | dependency-unverified 20 |

上游固定为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9`（`upstream.json`）；审计脚本**拒绝其他 commit**。

四类合计 1,251 个模块；`partial-unverified` 表示已有对应代码，`vulkan-port-pending` 也可能已有部分实现。二者均不代表全功能通过，文件覆盖率不能换算成完成率。普通 src 的对应文件缺失数已为 0；Vulkan 分类内的渲染器主执行链及已有模块的完整行为验收仍待完成。

## 2. 一轮标准工作流

1. `utils/port-audit.mjs <pinnedDir>` 刷新 `docs/port-status.json`（每轮必做，提交前核对计数）。
2. 实现（`src/**` 或 `examples/jsm/**`）。
3. 加单测：`src` 类进 `THREECPPTests`；**addon 类进 `THREECPPAddonTests`**（`test/CMakeLists.txt` 的 `if(THREECPP_BUILD_ADDONS)` 块内，addon 测试链 `threecpp::addons`）。
4. 差分 parity：探针（C++ 输出 JSON 到 stdout 最后一行）+ `compare-*.mjs`（导入 pinned 上游，`assert.deepEqual`/1e-12 容差）。
5. 构建：`cmake --build build --target THREECPPAddonTests <ProbeTarget>`；跑对应过滤测试 + `ctest -R '<name>-parity'`。
6. HANDOFF：新条目（LF、UTF-8 无 BOM）**前置**拼接到 `docs/HANDOFF.md`，校验旧字节逐字节保留，`git -c core.autocrlf=false add -- docs/HANDOFF.md`，提交信息用 `-F` 文件。

## 3. 差分编写范式

- 上游模块 `import ... from 'three'` → 把 `from 'three'` 重写为 `file:///.../build/three.module.js`。
- 无相对导入 → 可包成 `data:` URL 导入。
- **含相对导入**（如 `./BufferGeometryUtils.js`）→ 把重写源码写入 pinned 目录的临时文件，`import` 后 `finally` 删除（保持 pinned 工作树 `dirty = 0`）。
- 探针 stdout 可能混入 TSL 警告 → 比较脚本只解析**最后一行**。

## 4. 已踩过的关键陷阱（务必先看）

- 字面量 `0` 匹配 `set(const char*, ...)` 重载（空指针崩溃）→ 传 `JSValue(0.0)`。
- `struct Event` 在 `core/EventDispatcher.h`，**没有** `core/Event.h`。
- `SphereGeometry` 参数在 `parameters` 结构体里。
- `Object3D` **不可拷贝**、`traverse` 非 const；`Matrix4` 无 `operator==`（用 `std::equal` + `elements()` span）。
- `JSMember` 必须立即物化（`.toNumber()`），否则读到后续修改值。
- `Array.prototype.sort` 回调返回**数值**（负/0/正），不是布尔。
- C++ 列表初始化**禁止隐式收缩**（`double → float` 需显式 `static_cast`）。
- `arg(args,0,nullptr)` 会传 null（与 `arg(args,0)` 不同）。

## 5. 首批目标（按杠杆排序）

1. **传统 Vulkan 渲染路径** — managed 场景已按原始遍历、矩阵、排序和回调连接 WebGLObjects/Attributes/BindingStates、源码 program/material/texture 与实际 Vulkan draw。保留 typed 兼容入口。继续完整 public WebGLRenderer、未覆盖的材质/灯光/图元、阴影、环境、渲染目标、后处理及窗口呈现。
2. **现代 Vulkan 执行链** — 已有原生节点图及真实 GLSL 编译器；继续 Renderer/NodeBuilder/TSL 到同一 Vulkan 后端的完整绘制、计算与存储资源闭环，以及 WGSL/Tint 接入。CPU 节点图通过不代表现代 GPU 执行完成。
3. **普通 src 的行为验收与宿主缺口** — ObjectLoader、NodeObjectLoader、BufferGeometryLoader 已有原生动态管线；canonical curves/extras 已采用源码算法。继续 URL 图像/异步、typed facade、未覆盖的 managed 资源与宿主路径验收。所有模块仍须按完整行为验收，不能以存在文件或选中测试通过宣告完成。
4. **examples 与宿主适配** — 尚缺 322 个模块；补齐原生平台依赖并逐项差分，最后验收 Windows/Linux/macOS。AST、Linker、TranspilerUtils 已有实现，勿重复创建。
5. **验证与性能** — 已提交基线 a96368d2 的完整 core 1784 用例 / 219362 断言、生产 GPU 7/7 已通过。旧 full-core GC 慢路径尚未优化；局部纹理稀疏绑定优化不代表 GC 问题已解决。后续提交须记录实际测试范围、平台与验证层是否启用。

## 6. 近期已完成（避免重复劳动）
以下为历史轮次记录，其中“下一步／尚未”以第 5 节及最新 HANDOFF/VALIDATION 为准。


- 本轮曲线与 GPU 边界：14 个 canonical Curve 类构造器/模块导入、全部自身方法、动态长度缓存/切线/Frenet/Path 指令/Shape 洞与 JSON；移除 loader 内旧数值快照适配。新增 3 组 572 个真实源码差分通过，14 个单测/977 条断言通过；完整 core 1759/218466、addons 1441/39577 通过，24 项相关 CTest 通过。补齐 Number 的 binary64 常量及不可变描述符，修复 Frenet 初始法线选择。GPU Prepared/PipelineState/recordDraw 已解耦，原有 3 组 GPU 回归通过；尚未声称动态加载场景直接出图。
- 上一阶段加载与几何：ObjectLoader、NodeObjectLoader、BufferGeometryLoader 原生动态管线，21 个几何导出（20 个新增源码构造实现）、曲线/Shape JSON 链、ShapeUtils，以及 Array.forEach/reverse。7 组共 403 个真实源码差分已通过；旧加载器/几何与生命周期 10 项回归已通过。旧 typed facade、URL 图像宿主和 Vulkan 连接仍有缺口；其 Curve 数值适配已由本轮源码算法替换。
- 此前 Vulkan/Line2/MaterialX 轮：Vulkan 直接采样 RenderTarget GPU 附件（含线性/sRGB/HalfFloat、跨目标传递、上传回调中读回/释放后的恢复）；Line2NodeMaterial 节点图通过 209 个差分场景，MaterialXNoise/MaterialXNodes 通过 233 个差分场景。MaterialX 的验证包含算法体、延迟分支/循环及 4 个 NodeBuilder 基类构建场景，尚不证明 Vulkan 噪声着色器出图。该轮普通 src 缺失只剩 NodeObjectLoader（本轮已补入对应文件），完整渲染执行链和 322 个 addons 仍待完成。
- 此前 PBR 轮：原生 LightingModel 基类、PhysicalLightingModel 全部钩子及 helper、MeshStandard/Physical/SSSNodeMaterial 已实现并通过所选源码差分；EON、LTC、DFGLUT、EnvironmentBRDF 已修正依赖行为。此项证明节点逻辑，不代表完整 Vulkan PBR 出图。
- `examples/jsm`（部分验证，多为 partial）：ColorEnvironment、ColorUtils、DebugEnvironment、GeometryUtils、GroundedSkybox、MaterialXLog、MorphAnimMesh、MorphBlendMesh、OBJExporter、RectAreaLightHelper、SceneUtils（全部 helper）、ShadowMesh、STLExporter（ascii+binary）、TranspilerUtils、Gyroscope。
- `src` 聚合/映射：`Three.TSL`、`Three.WebGPU`、`Three.WebGPU.Nodes`、`nodes/TSL`、`nodes/Nodes`、`NodeMaterials`（19 个构造器映射，已覆盖 pinned 导出表）。
- **已确认既有实现**：`BufferGeometryUtils.mergeGroups`（r81 更正）。

## 7. `transpiler/AST.js` 类清单（下一目标脚手架）

上游 `examples/jsm/transpiler/AST.js`（694 行 / 8,868B）全部为 `export class X extends ASTNode`，共 **30** 个：

ASTNode, Comment, Program, VariableDeclaration, Uniform, Varying, FunctionParameter, FunctionDeclaration, Expression, Ternary, Operator, Unary, Number, String, Conditional, FunctionCall, Return, Discard, Continue, Break, Accessor, StaticElement, DynamicElement, AccessorElements, For, While, Switch, SwitchCase, StructMember, StructDefinition

（文件顶部 `import { toFloatType, isBuiltinType } from './TranspilerUtils.js';` —— 对应 C++ 已完成 `TranspilerUtils`，可作为联调点。）

移植建议：
- 每个类是**纯数据节点**（字段 + getter，如 `isNumericExpression`），适合按类移植；`ASTNode` 基类先做（含 `isASTNode`、`linker { reference, accesses, assignments }`、`parent`）。
- 先叶子类（`Comment` / `Uniform` / `Varying` / `Literal` 等），再容器类（`Program` / `FunctionDeclaration` / `Switch` 等）。
- 差分按「构造 → 字段值 → 序列化/`toString`」数据级比较；完成后与 `TranspilerUtils::isExpression`（读 `isFor`/`isWhile` 等标志）联调。

## 8. `ASTNode` 及首批叶子类字段（已确认，可直接实现）

### `ASTNode`（`:3` 起，~104 行）
- `isASTNode = true`
- `linker = { reference: null, accesses: [], assignments: [] }`
- `parent = null`
- `get isNumericExpression() { return false; }`（基类恒 false，子类可覆盖）
- `get hasAssignment()`：`this.isAssignment === true` → true；否则 `parent === null` → false；否则递归 `parent.hasAssignment`
- `getType()`：`return this.type || null`
- `getProgram()`：沿 `parent` 上溯，`isProgram === true` 时返回该节点，否则 null
- `getParent(parents = [])`：递归把祖先 push 进数组并返回
- `initialize()`：遍历**自身全部可枚举属性**，若值为 ASTNode 则设 `parent = this`；若为数组，则对其中每个 ASTNode 设 `parent = this`

### `Comment`（`:107`）
- ctor `(comment)`：`comment`、`isComment = true`，随后 `initialize()`

### `Uniform`（`:168`）
- ctor `(type, name)`：`type`、`name`、`isUniform = true`，随后 `initialize()`

### `Varying`（`:185`）
- ctor `(type, name)`：`type`、`name`、`isVarying = true`，随后 `initialize()`

**实现提示**：`initialize()` 的上游语义是「扫描自身属性找子节点/数组」，C++ 里等价做法是**基类提供 `void adopt(ASTNode* child)` / `void adopt(std::vector<ASTNode*>&)` 并在每个子类构造器末尾显式登记自己的子节点**（已确认的这三个叶子类没有子节点，`initialize()` 为 no-op）。`linker.reference/accesses/assignments` 在 C++ 中用裸指针表示（与 `parent` 一致，生命周期由 AST 树持有者管理）。

## 9. `ShadowNode` 调研（835 行 / 24,331B，依赖面很广，需专门轮次）

上游：`src/nodes/lighting/ShadowNode.js`，`class ShadowNode extends ShadowBaseNode`。

### 顶层依赖（多数组件需先确认已移植）

| 依赖 | 用途 |
|---|---|
| `NodeMaterial`、`QuadMesh` | VSM 模糊用的全屏 quad 材质/网格 |
| `ShadowBaseNode` + `shadowPositionWorld` | 基类与投影世界坐标 |
| `TSLCore`（float/vec2/vec3/vec4/int/Fn）| 构建 TSL 节点 |
| `ReferenceNode.reference`、`TextureNode.texture`、`CubeTextureNode.cubeTexture` | 引用/采样 |
| `Normal.normalWorld`、`Position.positionLocal` | 法线/位置 |
| `MathNode`（mix/sqrt）、`OperatorNode.add` | 数学 |
| `DepthTexture`、`RenderTarget`（推断）| 阴影贴图 |
| `LoopNode.Loop`、`ScreenNode.screenCoordinate` | VSM 采样循环 |
| `UniformGroupNode.renderGroup`、`UniformNode.uniform` | uniform |
| `ViewportDepthNode` 的 4 个深度转换函数 | 对数深度缓冲路径 |
| `Lights.lightShadowMatrix` | 阴影矩阵 |
| `RendererUtils` reset/restore | 临时改渲染器/场景状态 |
| `ShadowFilterNode`（Basic/PCF/VSM）| 阴影过滤 |
| `EquirectUV.equirectDirection`、`ContextNode.context` | 点光/环境相关 |
| `constants`：Compatibility/GreaterEqualCompare/HalfFloatType/LessEqualCompare/LinearFilter/NearestFilter/PCFShadowMap/RGFormat/VSMShadowMap… | |

### 构造字段（已确认）

```js
constructor( light, shadow = null ) {
  super( light );
  this.shadow = shadow || light.shadow;
  this.shadowMap = null;
  this.vsmShadowMapVertical = null;
  this.vsmShadowMapHorizontal = null;
  this.vsmMaterialVertical = null;
  this.vsmMaterialHorizontal = null;
  this._node = null;
  this._currentShadowType = null;
  this._cameraFrameId = new WeakMap();
  this.isShadowNode = true;
  this.depthLayer = 0;
}
static get type() { return 'ShadowNode'; }
```

### 已读到的关键结构

- 模块级：`VSMPassVertical` / `VSMPassHorizontal`（两个 `Fn`，VSM 均值/方差两趟）、`_shadowFilterLib = [BasicShadowFilter, PCFShadowFilter, null, VSMShadowFilter]`、`_rendererState`、`_quadMesh = new QuadMesh()`。
- 方法（截至 200 行）：`setupShadowFilter(builder,{filterFn,depthTexture,shadowCoord,shadow,depthLayer})`（frustum test `.select(shadowNode, float(1))`）、`setupShadowCoord(builder, shadowPosition)`（正交/对数深度两路径、`oneMinus` 翻转、reversedDepthBuffer 分支）、`getShadowFilterFn(type)`、`setupRenderTarget(shadow,builder)`（`new DepthTexture(mapSize.w, mapSize.h)` …）。

### 结论与建议

**不建议单轮硬啃**：真实移植需要 ~15 个内部组件协同（渲染器状态、RT、TSL 图），且差分只能用「节点图/材质状态」级比较，工作量大。建议：
1. 先确认上表依赖中哪些**尚未移植**（很大可能是 `QuadMesh`、`RendererUtils`、`ShadowFilterNode`、`ViewportDepthNode`、`Lights` 访问器、`EquirectUV`）；
2. 按依赖顺序逐个小模块落地（每轮 1–2 个），最后组装 ShadowNode；
3. 与 `PointShadowNode` 一并规划（它依赖 ShadowNode）。

### ShadowNode 依赖实测状态（2026-11-05，`docs/port-status.json` 查询结果）

| 状态 | 依赖 | 体量 |
|---|---|---|
| **vulkan-port-pending（未移植）** | `renderers/common/QuadMesh.js` | 3,230 |
| **vulkan-port-pending（未移植）** | `renderers/common/RendererUtils.js` | 5,802 |
| partial（已有头/实现） | `nodes/lighting/ShadowBaseNode.js` | 6,605 |
| partial | `nodes/lighting/ShadowFilterNode.js` | 4,761 |
| partial | `nodes/accessors/ReferenceNode.js` | 8,537 |
| partial | `nodes/accessors/TextureNode.js` | 28,223 |
| partial | `nodes/accessors/CubeTextureNode.js` | 5,336 |
| partial | `nodes/accessors/Normal.js` | 5,333 |
| partial | `nodes/accessors/Position.js` | 3,773 |
| partial | `nodes/accessors/Lights.js` | 4,557 |
| partial | `nodes/math/MathNode.js` | 33,978 |
| partial | `nodes/math/OperatorNode.js` | 17,835 |
| partial | `textures/DepthTexture.js` | 2,974 |
| partial | `nodes/utils/LoopNode.js` | 9,510 |
| partial | `nodes/display/ScreenNode.js` | 5,890 |
| partial | `nodes/core/UniformGroupNode.js` | 4,398 |
| partial | `nodes/core/UniformNode.js` | 5,858 |
| partial | `nodes/display/ViewportDepthNode.js` | 11,202 |
| partial | `nodes/utils/EquirectUV.js` | 1,630 |
| partial | `nodes/core/ContextNode.js` | 8,173 |

**结论**：ShadowNode 的**唯一硬阻塞**是 `QuadMesh` + `RendererUtils` 两个 renderer 模块（共约 9KB）。其余依赖已有移植骨架，但需逐个确认「partial 是否足以支撑 ShadowNode 使用」。
**建议顺序**：① 移植 `QuadMesh`(3,230) → ② 移植 `RendererUtils`(5,802) → ③ 复核 ShadowBaseNode/ShadowFilterNode/Lights/ViewportDepthNode 是否完备 → ④ 组装 `ShadowNode` → ⑤ `PointShadowNode`。

### 更正（r108 实测）：所谓两个「硬阻塞」其实都已移植

- **`QuadMesh`**：仓库里 `src/renderers/common/QuadMesh.{h,cpp}` **已存在且完整**（JSValue 原生模块：`nativeQuadMeshModule()` / `nativeQuadMeshConstructor()` / `nativeQuadMeshBinding()`，内部用 `builtinConstructor` 构造 `QuadGeometry` 与 `QuadMesh`，模块持有 `Float32BufferAttribute`、`OrthographicCamera`、`_camera`、`_geometry` 等）。`RTTNode.cpp` 已在调用它。审计把它记为 `vulkan-port-pending` 属于**渲染器后端的 Vulkan 验证待办**，**不代表源码未移植**。
- **`RendererUtils`**：见上方实测（`nativeRendererUtilsModule()` 被 `RTTNode` 引用）。
- **结论**：r106/r107 的「ShadowNode 唯一硬阻塞是这两个 renderer 模块」**不成立**。ShadowNode 的前置基本齐备，可直接进入组装阶段（仍需确认 partial 模块的实现完备度）。
- **教训**：判断某模块是否已移植，**必须直接看仓库是否有对应文件与导出函数**，不能仅凭 `port-status.json` 的 `vulkan-port-pending` 状态推断；审计状态与源码存在性是两件事。

## 10. examples/jsm 小模块筛选（r109 实测，≤6KB 且无缺失依赖）

按体量排序（`Bad` 表示命中宿主/TSL 关键字）：

| Bytes | 模块 | 判定 |
|---|---|---|
| 596 | `tsl/display/Sepia.js` | TSL（需 JSValue 运行时风格）|
| 735 | `loaders/materialx/parse/MaterialXParser.js` | ❌ 依赖 **`DOMParser`**（宿主边界）|
| 1357 | `helpers/RapierHelper.js` | ❌ 依赖 rapier 物理库（未移植）|
| 1627 | `transpiler/Transpiler.js` | 需先做 decoder（GLSLDecoder 35KB 等）|
| 2230 | **`generators/city/InstancedMeshGenerator.js`** | ✅ **可行**（见下）|
| 2723 | `environments/DebugEnvironment.js` | 已完成 |
| 3230 | `renderers/common/QuadMesh.js` | 已完成（r108 更正）|
| 2327 | `loaders/materialx/MaterialXArchive.js` | ❌ fflate/Blob |
| 1544 | `capabilities/WebGPU.js` | ❌ navigator |
| 1476 | `textures/FlakesTexture.js` | ❌ canvas |
| 1534 | `webxr/Text2D.js` | ❌ canvas |
| 1035 | `inspector/Storage.js` | ❌ localStorage |
| 690 | `offscreen/jank.js` | ❌ DOM |

### 下一实体目标：`generators/city/InstancedMeshGenerator.js`（2,230B）

可行性复核（r52 曾判定「风险高」，r109 复核**判定可行**）：
- 依赖仅 `InstancedMesh` + `MathUtils.ceilPowerOfTwo`；
- C++ 侧 `InstancedMesh(geometry, material, JSValue count)`、`Mesh::Count`（可赋 `count`）、`setMatrixAt`、`instanceMatrix->needsUpdate`（JSMember/BufferAttribute 可写）、`boundingBox`（可置空）、`computeBoundingSphere()` **全部存在**；
- `parameters`（`Object.assign({}, defaults, parameters)`）→ C++ 用 `JSValue` 对象或 `JsonValue` 保存参数并序列化 key；
- 三个导出函数 `createInstances` / `updateInstances` / 类 `InstancedMeshGenerator`（`build(placements)` / `dispose()` / `_parametersKey` 缓存 / `_createGeometry`/`_createMaterial` 回调 → `std::function`）。
- 建议实现：C++ 类 + `std::function` 工厂回调；`placements` 为 `std::vector<Matrix4>`；差分用两个 placements 场景比较 mesh 的 capacity/count/visible/instanceMatrix/name/castShadow 与重建语义。

## 11. 长会话总结（r49–r118，2026-11-05）

### 当前计数（审计口径）

| 范围 | missing | partial-unverified | 其他 |
|---|---|---|---|
| `src` | 13 | 606 | vulkan-port-pending 290 |
| `examples/jsm` | 322 | 154 | dependency-unverified 20 |

主套件 `THREECPPTests` 1645 cases / 214,902 assertions；addon 套件 `THREECPPAddonTests` **1365 cases / 38,903 assertions**（本长会话起点 1292 左右）。所有差分 `ctest -R '<name>-parity'` 通过。

### 本长会话新增（全部已提交、逐轮验证）

**examples/jsm addon (16)**：Gyroscope、ColorEnvironment、ColorUtils、MorphAnimMesh、MaterialXLog、DebugEnvironment、GroundedSkybox、TranspilerUtils、GeometryUtils、MorphBlendMesh、ShadowMesh、RectAreaLightHelper、STLExporter(ascii+binary)、OBJExporter、SceneUtils(全部 helper + 差分)、AST、Linker、InstancedMeshGenerator。

**src 聚合/映射 (6)**：`Three.TSL`/`Three.WebGPU`/`Three.WebGPU.Nodes` 聚合头、`NodeMaterials` 真实映射、`nodes/TSL`、`nodes/Nodes`。

**transpiler 子系统**：`AST`（30 类全量）+ 差分；`Linker` + 差分；`TranspilerUtils` + 差分。

### 关键经验（本会话新增，务必先读）

1. **新 API 先读头文件，禁止臆测**（r110/r105 的失败皆因臆测 `JsonValue`/`SwitchCase` API）。
2. **AST 类禁止花括号初始化列表作构造参数（MSVC C2665）**：一律 `std::vector<ASTNode*> x{...}` 具名变量。
3. **判断模块是否已移植要看仓库文件与导出函数**，不能只看 `port-status.json` 的 `vulkan-port-pending`（r108 更正）。
4. **批量字符串替换前先读取真实文本并断言替换命中**；失败轮必须自动回滚（本会话多次生效）。
5. **提交前必须跑全套**（r111 曾把红测试带进提交，随后单独修正）。
6. `Array.sort` 回调返回**数值**；`JSMember` 必须立即物化；字面量 `0` 会命中 `set(const char*)`。

### 新会话建议路径

1. **`ShadowNode`(24,331)**：依赖地图见 §9，阻塞已澄清（§9 更正）；建议分 3–4 轮：先读完整 835 行并列出**方法清单与字段**，再分段实现 `setupShadowCoord` / `setupShadowFilter` / `setupRenderTarget` / VSM passes / 主 `setup`，每段配单测（TSL 图可用 `toString`/节点类型做弱差分）。
2. **`PhysicalLightingModel`(32,829)**：单点解锁 `MeshStandard/Physical/SSSNodeMaterial`，同样建议分段。
3. 普通 src 验收：继续未覆盖的渲染、资源及宿主行为。NodeObjectLoader 的 ObjectLoader 结构阻塞已经解除；不要再按旧阻塞清单重写这些加载器。
4. examples 侧：剩余小模块基本都受宿主/TSL/wasm 限制（§10 表），新增大模块需先读源码评估。

### ShadowNode 方法清单（r119 提取，行号为 pinned 源）

| 行 | 方法 | 说明 |
|---|---|---|
| 143 | `constructor(light, shadow=null)` | 字段已确认（§9）|
| 259 | `setupShadowFilter(builder, {filterFn, depthTexture, shadowCoord, shadow, depthLayer})` | frustum test 后 `.select(shadowNode, float(1))` |
| 280 | `setupShadowCoord(builder, shadowPosition)` | 正交/对数深度两路径、`oneMinus` 翻转、reversedDepthBuffer 分支 |
| 327 | `getShadowFilterFn(type)` | `_shadowFilterLib[type]` |
| 334 | `setupRenderTarget(shadow, builder)` | 建 `DepthTexture(mapSize)` 与 RT |
| 355–595 | **`setupShadow(builder)`（约 240 行，最重）** | 主 TSL 图：阴影坐标、PCF/VSM 分支、滤波采样、bias 等 |
| 596 | `setup(builder)` | 调用 setupShadow 并返回节点 |
| 642 | `renderShadow(frame)` | 渲染阴影贴图（改渲染器/场景状态 → render → 恢复）|
| 666 | `updateShadow(frame)` | 更新 shadow matrix/RT（含 `_cameraFrameId` 缓存）|
| 716 | `vsmPass(renderer)` | 两个 VSM 模糊 pass（用 `_quadMesh` + `vsmMaterialVertical/Horizontal`）|
| 737 | `dispose()` | 释放 RT/材质 |
| 750 | `_reset()` | 清理缓存状态 |
| 790 | `updateBefore(frame)` | 帧前更新入口 |
| 131 | `static get type()` | `'ShadowNode'` |

**分段实现建议**（每段可独立提交 + 单测）：
1. **A 段**：类骨架 + 字段 + `static type` + `getShadowFilterFn` + `dispose`/`_reset` 的空实现语义；
2. **B 段**：`setupShadowCoord`（纯数学，可用数值/节点类型差分）；
3. **C 段**：`setupRenderTarget` + `renderShadow`/`vsmPass`（需渲染器接口，做「可构建 + 状态正确」级验证）；
4. **D 段**：`setupShadow`（240 行 TSL 图，最后做）+ `setup`/`updateShadow`/`updateBefore` 串联。

#### A 段规格（r120 实测，可直接实现）

- **构造器**（L143）：`super(light)` 后依次设 `shadow = shadow || light.shadow`、`shadowMap = null`、`vsmShadowMapVertical/Horizontal = null`、`vsmMaterialVertical/Horizontal = null`、`_node = null`、`_currentShadowType = null`、`_cameraFrameId = new WeakMap()`、`isShadowNode = true`、`depthLayer = 0`。
- **`static get type()`**（L131）：返回 `'ShadowNode'`。
- **`getShadowFilterFn(type)`**（L327）：`return _shadowFilterLib[ type ];`，其中 `_shadowFilterLib = [ BasicShadowFilter, PCFShadowFilter, null /* PCFSoftShadowMap removed */, VSMShadowFilter ]`（模块级常量，来自 `ShadowFilterNode` 的三个导出）。
- **`dispose()`**（L737 起）与 **`_reset()`**（L750 起）：释放/清空 `shadowMap`、`vsmShadowMapVertical/Horizontal`、`vsmMaterialVertical/Horizontal`、`_node` 等缓存（具体逐行语义参见 pinned 行 737–800；A 段先落字段与空实现骨架，行为在 C/D 段补全）。
- **实现风格**：本端口 `src/nodes/**` 一律为 **JSValue 托管类**（`native<Name>Module/Constructor/Binding` + `makeClass/builtinConstructor` + 生成的 `*Sources.inc` 类源），继承通过基类 prototype 实现；`ShadowBaseNode` 已有 `nativeShadowBaseNodeModule/Constructor/Binding` 与 C++ 包装类可参考。

#### `setupShadow` 完整实现规格（r129，L355–595，ShadowNode 唯一剩余抛错成员）

**段 1：RT 与过滤（L357–380）**
`const { depthTexture, shadowMap } = this.setupRenderTarget(shadow, builder)`；`shadowMapType = renderer.shadowMap.type`；
`hasTextureCompare = renderer.hasCompatibility(Compatibility.TEXTURE_COMPARE)`；若 `shadowMapType === PCFShadowMap && hasTextureCompare` → `depthTexture.minFilter = depthTexture.magFilter = LinearFilter`，否则两者 = `NearestFilter`；
`shadow.camera.coordinateSystem = camera.coordinateSystem`；`shadow.camera._reversedDepth = renderer.reversedDepthBuffer`；`shadow.camera.updateProjectionMatrix()`。

**段 2：VSM 两趟（L384–448，仅 `shadowMapType === VSMShadowMap && !shadow.isPointLightShadow`）**
- `depthTexture.compareFunction = null`；
- `shadowMap.depth > 1` 时复用 `shadowMap._vsmShadowMapVertical/Horizontal`（不存在则 `builder.createRenderTarget(w,h,{...,depth})` 并命名 `VSMVertical`/`VSMHorizontal`）；否则直接 `builder.createRenderTarget(w,h,{format: RGFormat})` 两次；
- `shadowPassVertical = texture(depthTexture)`（数组纹理时 `.depth(depthLayer)`）；`shadowPassHorizontal = texture(vsmShadowMapVertical.texture)` 同理；
- `samples/radius/size` 为 `reference('blurSamples'|'radius'|'mapSize', 'float'|'vec2', shadow).setGroup(renderGroup)`；
- `sharedContext = context(builder.getSharedContext())`；
- `vsmMaterialVertical = vsmMaterialVertical || new NodeMaterial()`，设 `contextNode = sharedContext`、`fragmentNode = VSMPassVertical({samples, radius, size, shadowPass, depthLayer, ...})`、`name = 'VSMVertical'`；Horizontal 同理。

**段 3：阴影坐标（L452–476）**
`shadowIntensity = reference('intensity','float',shadow).setGroup(renderGroup)`；`normalBias = reference('normalBias','float',shadow)...`；
`shadowMatrix = lightShadowMatrix(light)`；`shadowNormalBias = normalWorld.mul(normalBias)`；
若 `!renderer.highPrecision || builder.material.receivedShadowPositionNode || builder.context.shadowPositionWorld` → `shadowPosition = shadowMatrix.mul(shadowPositionWorld.add(shadowNormalBias))`；
否则 `highpShadowModelMatrix = uniform('mat4').onObjectUpdate(({object}, self) => self.value.multiplyMatrices(shadowMatrix.value, object.matrixWorld))`，`shadowPosition = highpShadowModelMatrix.mul(positionLocal).add(shadowMatrix.mul(vec4(shadowNormalBias, 0)))`；
`shadowCoord = this.setupShadowCoord(builder, shadowPosition)`。

**段 4：过滤与输出（L480–585）**
`filterFn = shadow.filterNode || this.getShadowFilterFn(renderer.shadowMap.type) || null`；为 null → **抛 `THREE.WebGPURenderer: Shadow map type not supported yet.`**；
`shadowDepthTexture = (VSM && !point) ? vsmShadowMapVertical.texture : ...`；`shadowNode = this.setupShadowFilter(builder,{filterFn, shadowTexture: shadowMap.texture, depthTexture, shadow: ..., shadowCoord, depthLayer})`；
`renderer.shadowMap.transmitted === true` 时取 `shadowColor = cubeTexture(...)/texture(...)`；`shadowOutput = shadowColor ? mix(1, shadowNode.rgb.mix(shadowColor,1), shadowIntensity.mul(shadowColor.a)).toVar() : mix(1, shadowNode, shadowIntensity).toVar()`；
`this.shadowMap = shadowMap; this.shadow.map = shadowMap;`
`toInspector(...)` 两处（`${light.type} Shadow [ ${light.name || 'ID: '+light.id} ] / Color|Depth`），Depth 分支用 `perspectiveDepthToViewZ` / `orthographicDepthToViewZ` + `viewZToOrthographicDepth` + `oneMinus`。

**所需绑定清单**：`nativeDepthTextureConstructor`、`nativeNodeMaterialConstructor`（`NodeMaterial`）、`nativeContextNodeBinding("context")`、`nativeTextureNodeBinding("texture")`、`nativeCubeTextureNodeBinding("cubeTexture")`、`nativeReferenceNodeBinding("reference")`、`nativeTSLBaseBinding("mix"|"vec4")`、`nativeMathNodeBinding("mix")`、`nativeLightsBinding("lightShadowMatrix")`、`nativeNormalBinding("normalWorld")`、`nativePositionBinding("positionLocal")`、`nativeShadowBaseNodeBinding("shadowPositionWorld")`、`nativeUniformNodeBinding("uniform")`、`nativeUniformGroupNodeBinding("renderGroup")`、`nativeViewportDepthNodeBinding("perspectiveDepthToViewZ"|"orthographicDepthToViewZ"|"viewZToOrthographicDepth")`、`nativeEquirectUVBinding("equirectDirection")`、`nativeShadowFilterNodeBinding(...)`、`nativeNodeConstantsBinding("PCFShadowMap"|"VSMShadowMap"|"LinearFilter"|"NearestFilter"|"RGFormat"|"Compatibility")`、`renderer.hasCompatibility`。

**建议**：分 4 次提交（每段一次），段 4 可用「过滤函数缺失时抛错」的既有测试断言；VSM 段需先确认 `createRenderTarget` 的 `{format, depth}` 参数与 `NodeMaterial` 绑定可用。

#### 段 2 前置：`VSMPassVertical` / `VSMPassHorizontal`（r134 抽取，可直接转写）

上游位置：`ShadowNode.js` L24–100，模块级两个 `Fn`。**仓库中尚无移植**；可复用 `ShadowFilterNode.cpp` 的 `makeFn(body)` 范式（`LightingNodeRuntime` 提供 `makeFn`、`Fn` 包装）。

完整源码（已逐行抽取）：

```
const VSMPassVertical = Fn( ( { samples, radius, size, shadowPass, depthLayer } ) => {
	const mean = float( 0 ).toVar( 'meanVertical' );
	const squaredMean = float( 0 ).toVar( 'squareMeanVertical' );
	const uvStride = samples.lessThanEqual( float( 1 ) ).select( float( 0 ), float( 2 ).div( samples.sub( 1 ) ) );
	const uvStart = samples.lessThanEqual( float( 1 ) ).select( float( 0 ), float( - 1 ) );
	Loop( { start: int( 0 ), end: int( samples ), type: 'int', condition: '<' }, ( { i } ) => {
		const uvOffset = uvStart.add( float( i ).mul( uvStride ) );
		let depth = shadowPass.sample( add( screenCoordinate.xy, vec2( 0, uvOffset ).mul( radius ) ).div( size ) );
		if ( shadowPass.value.isArrayTexture ) {
			depth = depth.depth( depthLayer );
		}
		depth = depth.x;
		mean.addAssign( depth );
		squaredMean.addAssign( depth.mul( depth ) );
	} );
	mean.divAssign( samples );
	squaredMean.divAssign( samples );
	const std_dev = sqrt( squaredMean.sub( mean.mul( mean ) ).max( 0 ) );
	return vec2( mean, std_dev );
} );

const VSMPassHorizontal = Fn( ( { samples, radius, size, shadowPass, depthLayer } ) => {
	const mean = float( 0 ).toVar( 'meanHorizontal' );
	const squaredMean = float( 0 ).toVar( 'squareMeanHorizontal' );
	const uvStride = samples.lessThanEqual( float( 1 ) ).select( float( 0 ), float( 2 ).div( samples.sub( 1 ) ) );
	const uvStart = samples.lessThanEqual( float( 1 ) ).select( float( 0 ), float( - 1 ) );
	Loop( { start: int( 0 ), end: int( samples ), type: 'int', condition: '<' }, ( { i } ) => {
		const uvOffset = uvStart.add( float( i ).mul( uvStride ) );
		let distribution = shadowPass.sample( add( screenCoordinate.xy, vec2( uvOffset, 0 ).mul( radius ) ).div( size ) );
		if ( shadowPass.value.isArrayTexture ) {
			distribution = distribution.depth( depthLayer );
		}
		mean.addAssign( distribution.x );
		squaredMean.addAssign( add( distribution.y.mul( distribution.y ), distribution.x.mul( distribution.x ) ) );
	} );
	mean.divAssign( samples );
	squaredMean.divAssign( samples );
	const std_dev = sqrt( squaredMean.sub( mean.mul( mean ) ).max( 0 ) );
	return vec2( mean, std_dev );
} );
```

**所需绑定清单**：`nativeTSLBaseBinding("float"|"int"|"vec2"|"sqrt"|"add"|"sub"|"mul"|"div")`（或对应 MathNode/OperatorNode 绑定）、`nativeLoopNodeBinding("Loop")`、`nativeScreenNodeBinding("screenCoordinate")`；节点方法：`lessThanEqual`/`select`/`toVar`/`sub`/`div`/`mul`/`add`/`addAssign`/`divAssign`/`max`/`sample`/`depth`/`value`/`x`/`y`。

**段 2 其余部分**（RT 与材质，L384–448）：`compareFunction=null`；`shadowMap.depth>1` 复用 `_vsmShadowMapVertical/Horizontal` 或 `builder.createRenderTarget(w,h,{format: RGFormat, ...})`；两个 `texture()` 采样；`reference('blurSamples'|'radius'|'mapSize')`；`context(builder.getSharedContext())`；两个 `NodeMaterial` 设 `contextNode`/`fragmentNode`/`name`。

**VSM pass 转写辅助（r135 确认，可直接照此写）**

- **Fn 包装范式**（复制自 `ShadowFilterNode.cpp`）：
  - `JSValue inputObject(const Args& args)`：上游 Fn 的入参是一个「单元素 iterable，元素为解构对象」→ 用 `getJSIterator(arg(args,0))`/`stepJSIterator`/`closeJSIterator`（`LightingNodeRuntime`）；
  - `JSValue makeFn(JSValue::NativeFunction body)`：`JSValue::function(body, captures, "", 0)` → `nativeTSLCoreBinding("Fn").call({}, {callback})`（需支持 captures 版本，用于垂直/水平共用同一 body）；
  - 节点操作统一 `invoke(node, "method")({args})`。
- **已确认绑定**：`nativeTSLBaseBinding("float"|"int"|"vec2")`、`nativeMathNodeBinding("sqrt")`、`nativeOperatorNodeBinding("add"|"sub"|"mul"|"div")`、`nativeLoopNodeBinding("Loop")`、`nativeScreenNodeBinding("screenCoordinate")`、`nativeTextureNodeBinding("texture")`、`nativeUniformGroupNodeBinding("renderGroup")`、`nativeReferenceNodeBinding("reference")`、`nativeContextNodeBinding("context")`、`nativeNodeMaterialConstructor()`。
- **Loop 调用**：`Loop({ start: int(0), end: int(samples), type: 'int', condition: '<' }, ( { i } ) => {...})` → `nativeLoopNodeBinding("Loop").call({}, {config, loopBody})`；`loopBody` 收到的第一个参数同样用 `inputObject` 解构出 `i`。
- **垂直/水平差异**（用一个 body + captures 布尔区分）：`vec2(0, uvOffset)` vs `vec2(uvOffset, 0)`；均值取 `depth.x` vs `distribution.x`；方差项 `depth.mul(depth)` vs `add(distribution.y.mul(distribution.y), distribution.x.mul(distribution.x))`；变量名后缀 `Vertical`/`Horizontal`。
- **插入位置**：`ShadowNode.cpp` 的 `setupShadow` 中，**段 1 之后、段 3 之前**（段 2 只对 `VSMShadowMap && !isPointLightShadow` 生效）。

### PointShadowNode 结构清单（r152，下一实现目标）

- 位置：`src/nodes/lighting/PointShadowNode.js`（12,693 B），依赖已完成的 `ShadowNode`（继承 `ShadowNode`）。
- 待办：读取完整源码后按 `ShadowNode` 的同样方式分段落实现（构造/`setupShadow` 重写/`updateShadow` 立方体面循环/`vsmPass` 是否覆盖等），复用 `ShadowNode` 的辅助函数与测试夹具范式。
- 建议顺序：①结构骨架 + `static type` + 构造字段；②`setupShadow`（点光六面 RT/滤镜）；③`updateShadow`/`renderShadow` 逐面渲染；④差分/集成测试。

#### PointShadowNode Stage B 源码要点（r155，L66–142）

**`PointShadowFilter`**（L66–95，Vogel disk 采样）：`radius/mapSize` 引用；`texelSize = radius.div(mapSize.x)`；`absDir = abs(bd3D)`；`tangent = normalize(cross(bd3D, absDir.x.greaterThan(absDir.z).select(vec3(0,1,0), vec3(1,0,0))))`；`bitangent = cross(bd3D, tangent)`；`phi = interleavedGradientNoise(screenCoordinate.xy).mul(6.28318530718)`；`sample0..4 = vogelDiskSample(i, 5, phi)`；返回 5 个 `cubeTexture(depthTexture, bd3D.add(tangent.mul(s.x).add(bitangent.mul(s.y)).mul(texelSize)))` 之和 `.mul(1/5)`。

**`pointShadowFilter`**（L97–142）：
- 签名 `Fn(({filterFn, depthTexture, shadowCoord, shadow}, builder) => …)`（**Fn body 第二参数是 builder**）；
- `shadowPosition = shadowCoord.xyz.toConst()`；`shadowPositionAbs = shadowPosition.abs().toConst()`；`viewZ = shadowPositionAbs.x.max(.y).max(.z)`；
- `shadowCameraNear/Far = uniform('float').setGroup(renderGroup).onRenderUpdate(() => shadow.camera.near/far)`；
- `bias = reference('bias','float',shadow).setGroup(renderGroup)`；`result = float(1.0).toVar()`；
- `If(viewZ.sub(far).lessThanEqual(0.0).and(viewZ.sub(near).greaterThanEqual(0.0)))` 内：
  - reversedDepthBuffer → `dp = viewZToReversedPerspectiveDepth(viewZ.negate(), near, far); dp.subAssign(bias)`
  - logarithmicDepthBuffer → `viewZToLogarithmicDepth(...); dp.addAssign(bias)`
  - 否则 → `viewZToPerspectiveDepth(...); dp.addAssign(bias)`
  - `bd3D = shadowPosition.normalize()`；`result.assign(filterFn({depthTexture, bd3D, dp, shadow}))`
- 返回 `result`。

**实现要点**：需要 `inputObject` 辅助（同 `ShadowNode.cpp`）；`If` 用 `nativeTSLBaseBinding("If")`（callable: `If(condition)(body)`）；`onRenderUpdate` 用 `invoke(uniform, "onRenderUpdate")({callback})`，callback 捕获 `shadow`；`vogelDiskSample`/`interleavedGradientNoise` 绑定待确认（`nativePostProcessingUtilsBinding`）。`setupShadowFilter` 只需 `return pointShadowFilter({filterFn, depthTexture, shadowCoord, shadow})`。

#### PassNode 构造器规格（r167，`constructor( scope, scene, camera, options = {} )`）

- `super('vec4')`（Node 基类带节点类型字符串）；
- 字段顺序：`scope`、`scene`、`camera`、`options`、`_width=1`、`_height=1`；
- `renderTarget = new RenderTarget(_width, _height, {type: HalfFloatType, ...options})`，`renderTarget.texture.name='output'`；
- `depthTexture = null`；若 `scope === PassNode.DEPTH || options.depthBuffer !== false`：
  - `options.depthTexture !== undefined ? options.depthTexture : new DepthTexture()`；
  - 若 `renderTarget.stencilBuffer === true` → `depthTexture.format = DepthStencilFormat`、`type = UnsignedInt248Type`；
  - `depthTexture.isRenderTargetTexture = true`、`name='depth'`、`renderTarget.depthTexture = depthTexture`；
- `overrideMaterial=null`、`transparent=true`、`opaque=true`、`lighting=null`；
- `autoClear/autoClearColor/autoClearDepth/autoClearStencil`（options 覆盖，默认 true）；
- `contextNode=null`、`_contextNodeCache=null`；
- `_textures = { output: renderTarget.texture }`，`depthTexture !== null` 时加 `depth`；
- `_textureNodes={}`、`_linearDepthNodes={}`、`_viewZNodes={}`、`_previousTextures={}`、`_previousTextureNodes={}`；
- `_cameraNear = uniform(0)`、`_cameraFar = uniform(0)`；
- `_mrt=null`、`_layers=null`、`_resolutionScale=1`、`_viewport=null`、`_scissor=null`；
- `isPassNode=true`、`updateBeforeType = NodeUpdateType.FRAME`、`global=true`。
- **静态 getter**：`PassNode.COLOR`/`PassNode.DEPTH`（值见 L1082/L1093，Stage D 时读取）。

#### NodeObjectLoader 历史阻塞分析（r177；结构阻塞已解除）

- 上游 `src/loaders/nodes/NodeObjectLoader.js`（169 行，逻辑约 60 行）**继承 `ObjectLoader`**，仅重写 5 处：构造（`nodes/nodeMaterials/_nodesJSON`）、`setNodes`/`setNodeMaterials`、`parse`/`parseAsync`（暂存 `json.nodes` 后调 `super.parse`）、`parseNodes`（`NodeLoader`）、`parseMaterials`（`NodeMaterialLoader`）。
- 当前 ObjectLoader、NodeObjectLoader、BufferGeometryLoader 已有原生动态管线与源码继承派发；typed Result 保留兼容。上述结构阻塞已解除，无需重复重写 loader 或新增 JSValue 与 typed 材质桥接。
