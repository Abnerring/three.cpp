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