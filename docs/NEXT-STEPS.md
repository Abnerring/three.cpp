# NEXT-STEPS — 接手指南（由多轮移植沉淀，最后更新：2026-11-05）

本文件是**精简接手入口**。完整历史在 `docs/HANDOFF.md`（>1MB，按时间从新到旧），
进度真源是 `docs/port-status.json`（由 `utils/port-audit.mjs` 生成，勿手改）。

## 1. 总进度（审计口径，2026-11-05）

| 范围 | missing | partial-unverified | 其他 |
|---|---|---|---|
| `src` | **13** | 452 | vulkan-port-pending 290 |
| `examples/jsm` | **325** | 151 | dependency-unverified 20 |

上游固定为 `11738a9d55e502fb484cefffcea3a66dc44e9fe9`（`upstream.json`）；审计脚本**拒绝其他 commit**。

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

1. **`examples/jsm/transpiler/AST.js`**(8,868) — 纯 AST 类；完成后可与已完成 `TranspilerUtils` 联调。
2. **`src/nodes/lighting/ShadowNode.js`**(24,331) — 依赖齐备（上轮已确认），845 行左右。
3. **`src/nodes/functions/PhysicalLightingModel.js`**(32,829) — 单点解锁 `MeshStandard/Physical/SSSNodeMaterial`。
4. 其余 examples 小项：多数依赖 wasm/host（meshopt、fflate、canvas、navigator），已在 HANDOFF 中逐条排除原因。

## 6. 近期已完成（避免重复劳动）

- `examples/jsm`（部分验证，多为 partial）：ColorEnvironment、ColorUtils、DebugEnvironment、GeometryUtils、GroundedSkybox、MaterialXLog、MorphAnimMesh、MorphBlendMesh、OBJExporter、RectAreaLightHelper、SceneUtils（全部 helper）、ShadowMesh、STLExporter（ascii+binary）、TranspilerUtils、Gyroscope。
- `src` 聚合/映射：`Three.TSL`、`Three.WebGPU`、`Three.WebGPU.Nodes`、`nodes/TSL`、`nodes/Nodes`、`NodeMaterials`（15 个构造器映射；缺 4 个未移植材质）。
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