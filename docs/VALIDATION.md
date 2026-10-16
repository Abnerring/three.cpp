# 验证记录

## 当前追加验证：AnimationClipCreator 差分落地，2026-10-17

- 差分补齐：threecpp.threejs-animation-clip-creator-parity **51 个场景 / 44 条轨道 / 884 个时间与数值条目**逐位一致，另有 **7 条与上游一致的抛错路径**（duration 0 使 times 为空，上游 KeyframeTrack 抛 THREE.KeyframeTrack: no keyframes…，原生核心抛同一文本）。两个随机工厂在 JS 侧打桩 Math.random、在 C++ 侧注入同一序列。

- 该差分抓到我的移植里一个真实缺陷（比上一轮那个更隐蔽）：CreateShakeAnimation 里 `temporary.set( random()*2-1, random()*2-1, random()*2-1 )` 三次抽取被写成同一表达式，**C++ 不规定函数实参求值顺序**（MSVC 从右向左），而固定源码按 x、y、z 顺序抽取。已改为显式分三次求值并加注释说明；这与本项目此前记录过的 `a[i] = a[--i]` 属同一类陷阱。

- 随后发现并修掉一个**验证覆盖漏洞**（比数值差异更值得记录）：kind 4 的生成语句原本紧跟在同一行的 `//` 注释之后，整条语句因此被当作注释吞掉，**四个 CreateVisibilityAnimation 场景从未被比较过**。修正后场景数由 47 升到 51、条目 860 升到 884，全部逐位一致。此前记录的“duration 0 被排除”随之作废：查清原因是**表示差异**而非数值差异 —— 上游 BooleanKeyframeTrack 的 ValueBufferType 是 Array（`[true,false,true]`），原生轨道通过 `number()` 暴露 `[1,0,1]`，比较脚本已加入仅对 boolean 生效的精确归一化，duration 0 场景现已纳入并通过。

## 当前追加验证：CameraUtils（上一轮主动收敛后补完），2026-10-17

- 新增 `examples/jsm/utils/CameraUtils`。签名适配只有一处并写入头文件：固定版本通过 JS 鸭子类型读取 `camera.near/far/fov/aspect`，C++ 中这些属性属于 `PerspectiveCamera`，因此原生端口接受具体类型；`MathUtils.RAD2DEG` 以同一字面量（`180.0 / MATH_PI`）在文件内定义，`Math.atan` 走 `JSNumberMath::atan`，模块级临时对象为 thread_local。
- 差分：`threecpp.threejs-camera-utils-parity` **6 个场景逐位一致**（fov、四元数、离轴投影矩阵与其逆矩阵），覆盖 `estimateViewFrustum` 开关、非 1 的 aspect、不同 near/far 与倾斜角点。单测 1 case / 22 assertions（fov 不被改写、四元数单位长度、矩阵末行 `(0,0,-1,0)`、逆矩阵乘回单位阵、估计分支改变 fov）。
- 该差分立刻抓到探针自身的一处错误：我把 `PerspectiveCamera` 的构造参数写成 `(near, aspect, far, 2000)`，而固定版本是 `(fov, aspect, near, far)`；修正后 6 个场景全部一致。
- 审计：1,251 个模块，partial-unverified **277**（+1）、missing **664**（−1）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。

## 当前追加验证：RectAreaLight 数据表簇，2026-10-17

- 新增 2 个模块：`examples/jsm/lights/RectAreaLightTexturesLib`（314 KB，几乎全是内嵌数据表）与 `RectAreaLightUniformsLib`（子代理实现、主会话复核）。
- 大表表示：`constexpr double LTC_MAT_1[16384]` / `LTC_MAT_2[16384]`（各 64·64·4），逐字保留上游数字字面量（上游写作 `- 0.11134` 的空格已去除，属同一字面量，文件头有说明）。抽取脚本对每个 token 做了格式校验、条目数断言，并用 V8 求值逐项 `Object.is` 比对（0 处不符）。纹理构造完全按上游：`Float32Array` 转换、`DataUtils::toHalfFloat` 逐元素、`DataTexture(64,64,RGBA,Float|HalfFloat,UVMapping,ClampToEdge,ClampToEdge,Linear,Nearest,1)` 与 `needsUpdate = true`。
- 差分：`threecpp.threejs-rect-area-light-parity` **39 个描述字段 / 65,536 个像素值（4 张纹理 × 16384）/ 65,717 项比较**全部一致（含 Uint16 半精度原始位、`-0`/NaN/±Inf 编码，无容差）。比较还覆盖类对象形状（typeof/name/length/自有键顺序/普通调用抛 TypeError/`new` 实例键）、每张纹理的全部字段与完整数据数组，以及 `UniformsLib` 前后键的变化、四个键的插入顺序与 `UniformsLib.LTC_x === RectAreaLightTexturesLib.LTC_x` 身份关系、二次 `init()` 的重建语义。单测 3 cases / 155 assertions。
- 子代理另做了**反向对照**：把期望值中某个像素改成 0 后脚本立即以 `15360 !== 0` 失败，证明该比较不是空转；主会话独立重跑得到完全相同的比较数（65,717）。
- `UniformsLib` 无需改动：固定版本的 `src/renderers/shaders/UniformsLib.js` 只有小写 `ltc_1`/`ltc_2`，四个 `LTC_*` 键是该 addon 在运行时写入的，原生对象接受这一写入，因此本轮未触碰 `src/**`。
- 审计：1,251 个模块，partial-unverified **276**（+2）、missing **665**（−2）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。
- 本轮另有一次主动收敛：我在上下文预算不足时开始移植 `examples/jsm/utils/CameraUtils`，发现固定版本的 `CameraUtils.frameCorners` 通过 JS 鸭子类型读取 `camera.near/far/fov/aspect`，而 C++ 中这些属性属于 `PerspectiveCamera`（需要一处有据可查的签名适配），评估后**删除未完成的文件、保持工作树干净**，留待下一轮连同差分一起完成，而不是提交半成品。

## 当前追加验证：jsm/shaders 全簇（52 个模块）与 MeshSurfaceSampler，2026-10-17

- 新增 53 个模块：`examples/jsm/shaders` 全部 52 个（子代理实现、主会话复核）与 `examples/jsm/math/MeshSurfaceSampler`（主会话实现）。
- shaders 簇差分：`threecpp.threejs-jsm-shaders-parity` **52 个模块 / 67 个导出 / 269 个字段 / 373 个 uniform / 128 段 GLSL 字符串（132,235 字符）、8,029 项比较**全部一致（字段顺序按有序数组比较，字符串逐字符 `\uXXXX` 转义以保证逐字节）；单测 3 cases / 912 assertions。覆盖含 `SubsurfaceScatteringShader`（组合 `ShaderChunk` 的 head/body 拆分与 `replaceAll`）与 `VelocityShader`（组合 `UniformsLib` 组）这类组合型模块。
- MeshSurfaceSampler 差分：`threecpp.threejs-mesh-surface-sampler-parity` **41 个场景 / 1,024 个 float32 分布项 / 210 个采样点**逐位一致；单测 3 cases / 46 assertions。
- 该差分过程中确认了一处上游怪癖并写进单测：`SphereGeometry(1,8,6)` 在给定抽样序列下的第三个样本连**上游本身**也返回 NaN（退化面的法线插值），本移植逐位复现；我最初的单测期望"所有样本法线单位长度"因此是错的，已按 oracle 修正为"该样本为 NaN，其余样本单位长度"。
- 另一处单测缺陷同样由 oracle 修正：箱体第一个样本的 `uv.y` 恰为 1（角点），我原先写成严格小于 1。
- 重要的验证约定（子代理发现，已写入 PORTING.md）：`examples/jsm/**` 用裸说明符 `'three'` 导入核心，在固定 checkout 中解析到**预构建产物** `build/three.cjs`，其字符串（如 `ShaderChunk.meshphong_vert` 的空行）与 `src/` 不一致；对照源码语义时应把说明符改写到 `<checkout>/src/Three.js`（用 base64 `data:` URL 导入，checkout 不被修改），而复现模块自身所见的实例身份时应让两侧读同一实例。
- 回归：addons 全套 **139 cases / 23,734 assertions** 通过；差分网络 **128/128 通过**。审计：1,251 个模块，partial-unverified **274**（+53）、missing **667**（−53）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。
- 过程记录：子代理先登记 CMake 再创建文件，期间 `cmake -S . -B build` 数次因 "Cannot find source file" 失败，属并发中间态；等待其落地后配置与构建均正常。`MeshSurfaceSampler` 自身还修掉一个真实缺陷：空几何路径原本会解引用空属性句柄（访问冲突），现按上游文本抛 JSTypeError。

## 当前追加验证：exp/log/log2 与色彩/MathUtils 对齐，2026-10-17

- 承接上一轮登记的非三角函数边界，`JSNumberMath` 新增 `exp`/`log`/`log2`（子代理实现、主会话复核），逐行取自同一份固定 `ieee754.cc`；`log2` 保留上游的 `ivln2hi`/`ivln2lo` 硬编码拆分与 Dekker 拆分/`spadd`，`k_log1p` 按约定改为文件局部 `kernelLog1p`。
- 新函数差分：`threecpp.threejs-math-exp-log-parity` **2,869 个场景**（971 exp、948 log、948 log2），与固定 Node v26.5.0 全局 `Math` **逐位一致，0 处不符**；覆盖 0/±0/±1/subnormal/±1e300/1e308/±Infinity/NaN、各分支边界（`o_threshold`/`u_threshold`、`2^-20` 快速路径、`sqrt(2)` 抵消位、次正规缩放）与 356 个伪随机输入。子代理另做了**灵敏度反向对照**：同一比较接受 `exp(1)`、拒绝其 ±1 ulp 邻居、拒绝 `"-0"` 与 `0` 混同，证明“0 处不符”是真实的位级结论。单测 3 cases / 47 assertions。
- 调用点差分：新增 `threecpp.threejs-color-math-parity`，**146 个场景逐位一致**（60 个 `Color` sRGB 转换场景 + `damp` + `isPowerOfTwo`/`floorPowerOfTwo`/`ceilPowerOfTwo`），并复用既有 `threecpp.threejs-color-management-parity` 验证 sRGB 传递函数。
- 该差分立刻抓到一处探针缺陷：`D NaN 1 1 1` 场景下 MSVC 的 `operator>>` 不接受 `"NaN"` 拼写，导致四个参数静默保持 0；改为统一走 `parse()` 后通过（属验证脚本问题，非实现问题）。
- 按上游 r187 移除 `Color` 的四个历史 gamma API（`copyGammaToLinear`/`copyLinearToGamma`/`convertGammaToLinear`/`convertLinearToGamma`），仓库内无其他调用点。
- 回归：核心单测 **662 cases / 205,175 assertions** 通过；addons 全套 **133 cases / 22,776 assertions** 通过；差分网络 **126/126 通过**；port-audit 无模块状态变化。
- 过程中的一处操作失误已修复并记录教训：用 PowerShell 文本往返改写 `docs/PORTING.md` 时引入了 BOM 并损坏了内容（278 行丢失），已从 git 恢复并改用 edit 工具重新应用；同时确认 `MathUtils.cpp`/`ColorManagement.cpp`/`VulkanRenderer.cpp` 的 BOM 是仓库既有状态，非本次引入。

## 当前追加验证：核心三角函数全面对齐 V8 fdlibm，2026-10-17

- 承接上一轮 Matrix4 的发现，把 `src/` 内全部三角函数调用对齐固定上游：`JSNumberMath` 新增 `tan`/`asin`/`atan`/`atan2`（子代理实现，主会话复核），随后 **117 处调用点（29 个文件）** 改为 `JSNumberMath`，`src/` 内不再有 `std::sin/cos/tan/asin/acos/atan/atan2`。
- 四个新函数的差分：`threecpp.threejs-math-trig-parity` **2,491 个场景**（360 tan、834 asin、348 atan、948 atan2）与固定 Node v26.5.0 的全局 `Math` **逐位一致，0 处不符**；子代理另跑 50 万个随机位型输入同样 0 处不符；单测 4 cases / 43 assertions。
- 调用点的差分：新增 `threecpp.threejs-math-core-trig-parity`，**921 个场景逐位一致**（六种 Euler order 的 `setFromEuler`、`setFromAxisAngle`、`slerpQuaternions`、`setFromRotationMatrix`、`Euler.setFromRotationMatrix`、`Matrix3.makeRotation`、`Matrix4.makeRotationX/Y/Z/Axis/FromEuler`、`setFromSphericalCoords`/`setFromCylindricalCoords`、`Vector3.angleTo`、`Vector2.angle`、`Spherical`/`Cylindrical.setFromVector3`、`Vector4.setAxisAngleFromQuaternion`）。
- 回归：核心单测 **662 cases / 205,175 assertions** 全部通过（未出现因末位变化而失效的固化期望）；addons 全套 **130 cases / 22,729 assertions** 通过；差分网络 **124/124 通过**（含两个新测试）；port-audit 未变更即通过（本轮不新增模块）。
- 过程中确认的一处上游事实：固定 r187 的 `Vector2` 没有 `setAngle`（只有 `angle`/`angleTo`/`setLength`/`rotateAround`），C++ 端同样没有，探针据此删除了该场景而不是补一个上游不存在的 API。
- 仍属已知边界的非三角函数：`Color.cpp`、`ColorManagement.cpp` 与 `MathUtils.cpp` 的 `std::pow/exp/log/log2`（上游为 `Math.*`）尚未对齐，已列为下一轮目标；`Math.sqrt` 与 `Math.sqrt` 同为正确舍入，无需替换。

## 当前追加验证：LoftGeometry/DecalGeometry/TeapotGeometry 与核心 Matrix4 三角函数修正，2026-10-17

- 新增 3 个几何模块。`LoftGeometry`、`DecalGeometry`（含导出的 DecalVertex）由主会话实现，`TeapotGeometry` 由子代理实现、主会话复核（强制重编、跑单测与差分、检查改动范围）。
- `threecpp.threejs-loft-decal-parity`：**23 个场景 / 4,348 个属性分量逐位一致**（tube、taper、封盖、开/闭截面、`console.error` 早退路径，以及 box / 平移 box / 无法线三角面片三种贴花源）；单测 6 cases / 161 assertions。
- `threecpp.threejs-teapot-geometry-parity`：**84 个场景 / 262,656 个属性分量 / 121,512 个索引项逐位一致**（全部 32 组 bottom/lid/body/fitLid/blinn 组合、segments 含 2.9/0/−5/NaN、size 含 0/负值/1e±8），单测 7 cases / 228 assertions。子代理同时纠正了任务书中错误的构造签名（固定版本是 `size`，不含 width/height/depth）并给出源码依据。
- **核心缺陷修正**：DecalGeometry 的差分暴露出 `src/math/Matrix4.cpp` 的 `makeRotationX/Y/Z`、`makeRotationAxis`、`makeRotationFromEuler` 共 14 处使用 `std::sin/std::cos`，而固定上游使用 `Math.sin/Math.cos`（V8 fdlibm）。以 `Euler( 0.3, 0.2, 0.1 )` 为例旋转矩阵多项相差 1 ulp，只在“旋转+平移”组合下显形（单独旋转或单独平移均一致），正好解释贴花差分中 25 个末位差异。修正后该差分通过，且 **122/122 parity 测试全部通过**，数学/几何/动画回归 34/34，无任何回归。
- 修正核心文件后重跑 addons 全套：**126 cases / 22,686 assertions** 全部通过。
- 记录在案的边界：MathUtils(8)、Vector3(8)、Quaternion(5)、Matrix3(4)、Vector2(2) 仍在用 `std::sin/std::cos`，尚未逐项对齐 fdlibm；`BufferGeometry::toJSON` 与上游的参数布局差异（payload 位于 `data` 下）是库级既存差异，TeapotGeometry 探针因此只比较参数字段与 type，未削弱数值比较。
- 审计重新生成：1,251 个模块，partial-unverified 221（+3）、missing 720（−3）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。

## 当前追加验证：ColorSpaces、SimplexNoise、ConvexHull/ConvexGeometry，2026-10-17

- 新增 4 个模块。`ColorSpaces` 与 `SimplexNoise` 之外的 ConvexHull/ConvexGeometry 由子代理实现、主会话独立复核（强制重编、跑单测与差分、检查改动范围与适配说明）；`ColorSpaces` 由主会话实现。全部差分脚本导入固定 checkout 的上游模块并用 `assert.deepEqual` 比较，无容差。
- `threecpp.threejs-color-spaces-parity`：模块导出、四个实现对象、传递函数身份与扩展 sRGB 的展开身份全部一致。该差分暴露了两处**验证脚本**缺陷并修正：上游模块 import 的是 `three` 包（bundle），脚本必须读取同一模块实例，否则读到的是另一份 `ColorManagement.spaces`；ES 模块命名空间导出顺序按规范为字典序，比较改为按集合。另有自实现单测的一处错误：把 Matrix3 的存储当作行主序（实际列主序，`elements[1]` 是 n21），已按源码修正期望值。
- `threecpp.threejs-simplex-noise-parity`：**1,960 个场景逐位一致**（5 组实例表 + `noise`/`noise3d`/`noise4d` 各 1,955 个输入），两侧注入相同伪随机序列；单测 4 cases / 15,454 assertions。
- `threecpp.threejs-convex-hull-parity`：**358 个场景逐位一致**（18 组 hull 状态、14 组初始 hull、18 组 makeEmpty、18 组重复 setFromPoints、18 组 containsPoint、18 组 Face.distanceToPoint、180 次 intersectRay、18 组 ConvexGeometry、1 个空 hull、55 组抛错退化点云）；单测 14 cases / 1,152 assertions。
- addons 全套 **113 cases / 22,297 assertions** 全部通过。审计重新生成：1,251 个模块，partial-unverified 218（+4）、missing 723（−4）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。
- 过程记录：ConvexHull 子代理期间报告“全套 3 处失败均在并发写入的 SimplexNoise.tests.cpp”，该失败随后由其作者修复，最终全套通过；另有一次构建失败是并发编辑造成的竞态，重试即通过（非源码问题）。

## 当前追加验证：addons math 与 geometries 两个集群，2026-10-17

- 新增 `examples/jsm/math` 四模块（ImprovedNoise、ColorConverter、Capsule、OBB；实现由子代理完成，主会话复核并重新构建验证）与 `examples/jsm/geometries` 四模块（ParametricFunctions、ParametricGeometry、BoxLineGeometry、RoundedBoxGeometry；主会话自行实现）。两个集群各自一个差分探针，均直接导入固定 checkout 的上游模块并用 `assert.deepEqual` 比较，无容差。
- `threecpp.threejs-addon-math-parity`：**5,316 个场景逐位一致**（ImprovedNoise 717、ColorConverter/Color 1741、Capsule 151、OBB 2707）；单测 14 cases / 2,007 assertions。
- `threecpp.threejs-jsm-geometries-parity`：**111 个场景 / 34,168 个属性分量逐位一致**；单测 13 cases / 1,015 assertions。addons 全套 92 cases / 5,648 assertions 通过。
- 该差分暴露并修正了核心 `src/math/Color.cpp` 的 `setHSL` 末位缺陷：上游为 `p + ( q - p ) * 6 * ( 2 / 3 - t )`，C++ 原实现把 `( 2 / 3 - t )` 先乘，产生 1 ulp 偏差。修正后 `color-management-unit`、`color-management-native-lifetime`、`threejs-color-management-parity` 均通过，确认无回归。这是本轮唯一的核心文件改动，由主会话审阅后保留。
- 复核中发现并修正了自实现测试的两处问题：把 oracle 输出的 float32 属性值当作 double 期望值（改为按公式重算的 binary64 值），以及 RoundedBoxGeometry 缺少 `clone()` 覆写（上游 `new this.constructor().copy( this )` 会返回同类实例；C++ 基类 clone 只能返回 BufferGeometry）。两处均先由单测失败暴露，修正后差分与单测同时通过。
- 审计重新生成：1,251 个模块，partial-unverified 214（+8）、missing 727（−8）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。

## 当前追加验证：BufferGeometryUtils，2026-10-17

- 新增 `examples/jsm/utils/BufferGeometryUtils`（14 个导出；`computeMikkTSpaceTangents` 因缺少第三方 MikkTSpace 依赖而显式抛错，不伪造结果）。实现由子代理完成，主会话另行复核并重新构建验证。
- 强制重编受影响的三个对象（本机 ninja 不跟踪头文件依赖）后：`THREECPPAddonTests.exe "[buffergeometryutils]"` **20 cases / 189 assertions** 通过；addons 全套 **65 cases / 2,626 assertions** 通过，无回归。
- 差分 `compare-buffer-geometry-utils.mjs`：直接导入固定 checkout 的该模块（类来自同一 checkout 的 `build/three.module.js`），**110 个场景 / 15 个操作逐字段一致**：computeMikkTSpaceTangents、computeMorphedAttributes、deepCloneAttribute、deinterleaveAttribute、deinterleaveGeometry、estimateBytesUsed、interleaveAttributes、mergeAttributes、mergeGeometries、mergeGroups、mergeVertices、remapAttribute(s)、toCreasedNormals、toTrianglesDrawMode。探针把 `std::cerr` 重定向后连同结果一起输出，脚本拦截 `console.error`/`console.warn`，因此每条失败路径的**文本**也参与比较；比较为 `assert.deepEqual`，无容差。注册为 `threecpp.threejs-buffer-geometry-utils-parity`（CTest 通过）。
- 复核确认的关键语义：mergeVertices 的 `max(tolerance, EPSILON)`/`log10`/`pow` 哈希与 `trunc(v*multiplier+additive)+","` 文本、先 `clone()` 再对全部属性与形变属性用同一 buffers 缓存执行 remapAttribute、最后 setIndex；toTrianglesDrawMode 的三条失败路径与 `clearGroups()`；computeMorphedAttributes 的 material 数组判定与 `influence === 0` 跳过；deinterleaveGeometry 的按身份去重；mergeGroups 的稳定排序。另核实固定版本**没有** InstancedInterleavedBufferAttribute 类，因此实例化分支不可达（该结论与实现说明一致）。
- 记录在案的边界：mergeVertices 在既无 index 又无 position 时与上游一样抛 TypeError（不是返回 null，任务书描述有误，实测以上游为准）；`remapAttribute` 的 buffers 缓存跨调用保留数值、对象身份会重建；`toCreasedNormals` 对上游无法终止的 `tableSize <<= 1` 循环显式抛 JSRangeError；computeMorphedAttributes 的蒙皮路径只有单测覆盖。
- 审计重新生成：1,251 个模块，partial-unverified 206（+1）、missing 735（−1）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。

## 当前追加验证：SkeletonUtils，2026-10-17

- 新增 `examples/jsm/utils/SkeletonUtils`（clone / retarget / retargetClip）。单测 4 cases / 53 assertions 通过。
- 差分探针在 C++ 与 Node 两侧用同一组字面量骨架运行同一操作，字面量漂移只会导致比较失败而不会静默通过：**3 个模式逐位一致** —— clone（层级、每个 SkinnedMesh 的重绑、骨架与骨骼的独立性、bindMatrix、源骨架未被改动）；retarget（目标骨骼 position/quaternion/scale、写回的默认值，另有 hip=spine、scale=2、hipInfluence=(1,0.5,0.25)、hipPosition、preserveBonePositions=false 的组合）；retargetClip（产出片段名、时长、4 条转换轨道的 times/values、fps=3、采样后的目标骨骼，以及 Skeleton 源形式的轨道名与时长）。注册为 `threecpp.threejs-skeleton-utils-parity`。
- 差分抓到一处真实缺陷：retargetClip 的骨骼查找误用了未回退的候选名，而上游是 `getBoneName( bone, options ) || bone.name`；修正后 3 个模式全部通过。
- 适配记录：裸骨架源在上游经 SkeletonHelper 携带 `skeleton` 供 `.bones[name]` 绑定解析，本移植改用持有同一骨架的空 SkinnedMesh；options 为值类型并把解析后的默认值写回，与上游可观察行为一致。
- 审计重新生成：1,251 个模块，partial-unverified 205（+1）、missing 736（−1）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。相关 CTest 7/7 通过。

## 当前追加验证：CurveExtras 与 JSNumberMath.pow，2026-10-17

- 新增 `examples/jsm/curves/CurveExtras`（14 个装饰曲线）。单测 5 cases / 77 assertions 通过；差分导入固定 checkout 的 `CurveExtras.js`，**1,224 个场景逐位一致**：28 次构造（默认与显式 scale）的采样点、切线、`arcLengthDivisions` 8/200 的弧长与 `type` 字符串，外加 774 组 `Math.pow` 与 212 个 `Math.sin`/`Math.cos` 输入。注册为 `threecpp.threejs-curve-extras-parity`。
- 差分前先做了独立判定实验：58,050 组 `(x, y)` 上 `std::pow` 与 Node `Math.pow` 有 **3 处位级不一致**（`Math.pow(1.000093339211477, 2)`、`Math.pow(±1.7976931348623157e308, 0.5)`、`Math.pow(0.9999999999999999, 0.5)`），因此直接使用 `std::pow` 会破坏公式一致性。
- 追溯固定 Node 源码后确认：`--use-std-math-pow` 在 Node v26.5.0 中默认为开，`v8::internal::math::pow` 只补 ECMAScript 特例与 `y === 2`/`y === 0.5` 两个编译器捷径，其余委托 `std::pow`；据此在 `src/math/JSNumberMath` 中按该包装实现 `pow`，未移植 `base::ieee754::legacy::pow`。来源 `deps/v8/src/numbers/ieee754.cc`（SHA-256 `0582ca75…`）已登记 `upstream.json`。
- 边界：通用分支与固定 Windows oracle 位级一致；其他平台的 libm 属另一份末位契约。`Math.pow` 的成功对照同时为既有的 sRGB/颜色传输等 `pow` 使用点提供了可复用实现，但这些模块自身的位级验收仍未执行。
- 审计重新生成：1,251 个模块，partial-unverified 204（+1）、missing 737（−1）、vulkan-port-pending 290、dependency-unverified 20；Acorn 解析错误 0。`addons-unit`、`nurbs-parity`、`curve-extras-parity`、`port-audit` 5/5（`-R` 筛选中 4/4）通过。

## 当前追加验证：NURBS 全家族差分与 surface/volume，2026-10-17

本轮在上一轮 CPU 组合（`THREECPP_BUILD_VULKAN=OFF`，本机无 Vulkan SDK 头与 glslangValidator）基础上补齐 NURBS 家族：

- 新增 `NURBSSurface`、`NURBSVolume`（按固定源码复制 `knots.length - degree - 1` 的控制点块并重建 Vector4，缺失控制点抛上游 TypeError）。C++ 单测合并为 13 cases / 179 assertions 全部通过。
- 新增 NURBS 差分探针与 `compare-nurbs.mjs`：直接导入固定 checkout 的 `NURBSUtils.js`、`NURBSCurve.js`、`NURBSSurface.js`、`NURBSVolume.js`，同一场景在两侧执行后逐字段比较。**98 个场景全部一致**：findSpan（含域外与 p=1/2/3）、calcBasisFunctions、B-Spline 点、基函数导数矩阵、B-Spline/有理/NURBS 导数、calcKoverI、曲面与体点、曲线采样与切线、曲线 JSON 字段、clone 的 arcLengthDivisions 与独立性，以及缺失控制点的 TypeError。上一轮只登记为 partial-unverified 的 NURBSUtils/NURBSCurve 由此获得实际差分证据。
- 探针协议按“场景文本由 mjs 生成、C++ 只回显结果”实现，避免两侧各自构造输入造成假一致；过程中修正了探针网格解析把扁平点表当成行列表的问题（该错误由差分直接暴露，未进入源码）。
- 注册 CTest：`threecpp.threejs-nurbs-parity` 通过；`addons-unit`、`threejs-sort-utils-parity`、`threejs-controls-parity`、`port-audit` 同步通过（5/5，合计 6.67 秒）。
- 审计重新生成：1,251 个模块，partial-unverified 203（+2）、missing 738（−2）、vulkan-port-pending 290、dependency-unverified 20；Acorn 8.11.2 解析错误 0。

这些结果仍未接入任何几何体或渲染路径，NURBS 家族只具备 CPU 行为证据。

## 当前追加验证：addons 排序/Controls/NURBS 与审计刷新，2026-10-17

本轮所有结果来自 `THREECPP_BUILD_VULKAN=OFF` 的 CPU 组合（本机仅存在 Vulkan 运行时 DLL，`VULKAN_SDK` 指向的目录已不存在，SDK 头与 glslangValidator 均缺失，因此 Vulkan 目标未参与构建与测试）。完整 CTest 终态 **148/150**：`threecpp.threejs-webgl-lights-states-parity` 在 1,500.12 秒超时（既有已知项），`threecpp.port-audit` 因新增模块后清单过期而失败；重新运行 `node utils/port-audit.mjs ../three.js` 后该测试 5/5 通过。

- `examples/jsm/utils/SortUtils`（radixSort）按固定源码实现；C++ 单测 8 cases / 1,943 assertions 通过。差分直接导入固定 checkout 的 `examples/jsm/utils/SortUtils.js`，748 个场景、178,302 个元素、7 种 getter、reversed 与调用方 aux/内部 aux 组合逐项一致，并比较排序结果最终落在 `arr` 还是 `aux`。移植中发现一处 C++ 求值顺序缺陷：`a[ i ] = a[ -- i ]` 在 JavaScript 中先求目标索引、C++17 先求右操作数，按原语义显式改写后差分才通过；原失败组合保留在上述场景中。
- `src/extras/Controls` 单测 5 cases / 40 assertions 通过；与实际 three.js 类差分构造默认值（含 keys/mouseButtons/touches）、8 步 connect 序列（undefined/null、覆盖 disconnect 的调用计数）及 EventDispatcher 继承关系一致。
- `examples/jsm/curves/NURBSUtils` 与 `NURBSCurve` 单测 9 cases / 139 assertions 通过，参考值由固定 Node oracle 实际运行上游两个模块生成，覆盖 findSpan、基函数与导数、B-Spline 点/导数、有理导数、曲面/体点、曲线 getPoint/getTangent 与 copy/clone/JSON；越界控制点保留上游 TypeError。独立差分探针尚未建立，不登记为完成模块。
- `utils/port-audit.mjs` 重新生成：1,251 个模块，partial-unverified 201（+4）、missing 740（−4）、vulkan-port-pending 290、dependency-unverified 20；Acorn 8.11.2 解析错误 0。文件存在仍只构成存在证据。

构建环境：本机 MSVC 为中文界面，CMake 检测得到的 `/showIncludes` 前缀是乱码（缓存值 `娉ㄦ剰: 鍖呭惈鏂囦欢:`），Ninja 无法建立头文件依赖记录，修改头文件后 `cmake --build` 会报告 `ninja: no work to do` 并沿用旧对象；本轮以显式删除受影响对象强制重编规避，并把前缀固定为 ASCII 值。该限制属工具链环境问题，与源码一致性无关，但会影响后续增量构建的可信度。

## 当前追加验证：uniform 编码与原生对象接入，2026-10-14

batch19 最终组合的 Debug/Release 全目标构建均已成功。Release 完整 CTest 终态为 **151/152**，耗时 1,746.63 秒；`threecpp.threejs-webgl-lights-states-parity` 在 1,500.12 秒超时。Debug 完整 CTest 终态为 **150/152**，耗时 5,795.20 秒；`threecpp.threejs-webgl-programs-parity` 与 `threecpp.threejs-webgl-lights-states-parity` 分别在 1,500.12 / 1,500.11 秒超时，材质差分则在 1,822.71 秒通过。此轮未修改超时配置、场景或断言，完整回归尚未通过；源冻结清单保持 1,189 文件，原终态日志保存在项目外。

以下工作先在项目外完成专项验证，现已合入根项目（81 个生产/测试/依赖与构建文件）；这些专项结果仍不替代组合回归。原生编译选项开启后的根 Debug/Release 配置已成功，完整源依赖正在重建，组合测试待执行：

- `WebGLUniformsGroups` 按原源码实现 std140 布局、上传命令顺序、每帧缓存、动态 getter/方法、重入与 dispose。Debug/Release 各 136 场景、22,944 项实际上游比较通过。后续窄修移除函数源码换行规范化，五条可见函数文本与固定 checkout 的原 CRLF/注释逐字一致；初版证据保留。
- `VulkanUniformBuffer` 基于反射布局编码数组、结构、非方阵、row-major、64 位标量及显式运行时数组长度；输入转换在上层完成。补齐每一维 ArrayStride，真实有效 SPIR-V 的外层显式 padding 也保留。双配置原反射回归与新增检查共 10 cases / 216 assertions 通过；SPIR-V 由匹配 Vulkan 1.1 目标的静态验证器检查。
- 外部 Renderer 使用反射块模板编码已求值的纹理/实例 uniform 快照，上传长度与 descriptor range 来自反射存储，替代 C++ struct 的内存布局。所有 Vulkan 对象以新增 `Array.strides` 头重新编译，不混用旧 ABI。两种配置原四项 GPU 回归各 664 assertions、59/38/76 checks 通过；固定 MeshBasic 路径、RTX 3070 Laptop、validation=0，尚非全部材质接入。
- 真实 `UniformsGroup`、`Uniform`、Vector2/3/4、Color、Matrix3/4 的组合暴露出缺失 `equals` 和构造器 owning TLS 缓存。候选补齐六种严格比较及读取/短路次序，并使构造器与 ID 计数归属当前 realm。原基线三项退出/ID 检查失败，修复后两配置各 10 cases / 73 assertions 通过；真实对象上传/更新/dispose 对照各 9 场景 / 20,214 比较，借用 receiver/getter/NaN/负零/GC/源码元数据对照各 78 场景 / 1,018 比较通过。复制原主单测全部 87 个翻译单元，另合入已复核的 GC fixture keeper 顺序修正，Debug/Release 聚合终态各 657 cases / 205,135 assertions 全部通过；原聚合联编缺少 fixture 头的失败日志保留，补齐原文件后才登记结果。

- 原生 ESSL 编译库候选使用固定并校验完整源树的 glslang / SPIRV-Tools / SPIRV-Headers，保留原语言解析、链接与 AST 转换，所有结果独立拥有数据。项目外生产 CTest 两配置各 4/4 通过，68 份 SPIR-V 与冻结原型逐字节一致；25 个材质源码对编译成功，RawShaderMaterial 原默认无效输入仍失败。新增映射比较 674 项，原二进制/布局检查 19,961 项通过。完整 Program 命令绑定与生产 Renderer 接入仍待完成。
- `VulkanProgramUniforms` 从两个阶段的完整反射块建立独立存储，应用 uniform 与 draw 私有字段分开访问，矩阵/结构/数组使用实际偏移与步长，sampler 保存纹理单元赋值。候选两配置各 7 cases / 50 assertions 通过。原 GPU 夹具 13 个场景保留，改用真实原生 ESSL 编译与反射存储上传；两配置与重新执行的原 ESSL OpenGL 对照各 17,280 通道、最大差 0。这里只验证开发场景上传，未证明全部材质像素、真实 sampler 描述符或 Renderer 接入。
- 原生 heap 安全点候选保留逐 hook 调用、立即 owner 保活与字段快照，减少无 JS 字段数学对象的图构建。两配置与原实现对照各 77 cases / 614 assertions；Release 注入可恢复分配失败及重试 49/37，Debug 排除已确认 MSVC noexcept 容器代理分配后为 78/64。最终完整灯光差分 98 场景 / 47,607 比较通过，耗时 2,010.77 秒，仍超原 1,500 秒配置，不能作为超时已修复的证据。原未过滤 Debug 终止与主动停止的旧候选记录保留。

- `WebGLState` 及必要的 String.indexOf、共享 ReversedDepthFuncs、Vector4/Color 管理方法已另合入根项目。最初项目外双配置各 947 场景 / 360,358 比较、realm 3 cases / 9 assertions、原 7 个测试翻译单元 52 cases / 328 assertions 通过。独立复核发现 Vector4.fromArray 的 Symbol 偏移提前抛错，以及 Color.set 的借用 receiver 空操作分支误取 native payload；独立窄修保留原失败，两配置扩充至 954 场景 / 362,525 比较通过，realm 和原 7 TU 亦通过。原包三项原文件发生仅物理 LF/CRLF 转换，已核对 normalized 哈希一致并单独登记实际字节，未改写原 manifest。完整 Vulkan 状态命令适配仍待实现。

这些改动均不引入 CEF 或 JS 运行时桥接；Node/OpenGL 仅用于开发阶段对照。模块全量一致性、生成 ESSL 的生产渲染接入及 Linux/macOS 验收仍未完成。

本轮根组合初始源码冻结 1,271 文件。Debug 的核心库与 Vulkan 库均构建成功，新增所有 Vulkan CPP 使用更新后的反射头重新编译；Release 核心库成功，但其 Vulkan 依赖构建在 SPIRV-Tools 的 265 字符 MSBuild .lastbuildstate 路径处报 MSB3501。原日志完整保留，未把该配置登记成功；后续改用更短的项目外构建目录重建。两配置匹配初始组合的核心库已独立复制并登记哈希，供后续候选验证使用。

已另合入原生编译器窄追加：真实单 shader 的 validateESSLStage，以及带 attribute binding 的链接入口；原 pair 入口仍转发空绑定。项目外 Debug/Release 各 15 项 API 检查通过，各 68 份原 SPIR-V 与 377 个原 JSON 字段完全一致。此结果不代表 Program 命令层或所有 active uniform/attribute 行为已对齐；新根组合仍待重建。

追加根组合 build02（State 954 场景与编译器窄追加，尚不含后续数组/回调候选）：Debug、Release 全目标构建均成功，Release 短构建路径修复了上述 MSB3501。真实根库的原生 ESSL、SPIR-V 反射、uniform 编码及两个 GPU 对照专项各 9/9 通过，分别耗时 11.76 / 6.80 秒。测试后冻结清单 1,273 个文件哈希全部一致，终态与 LastTest 已另存项目外。完整组合回归尚未执行。

之后追加合入数学属性键/默认回调修复，共 8 个已核对前置链与候选哈希的文件；候选双配置各 1,018 场景 / 382,660 比较、realm 4 cases / 20 assertions、原 7 TU 52 cases / 328 assertions 通过。发现并保留先包装后原生注册和旧 callback 快照的两处互通失败，后续修复继续独立验证。本次仅清理已识别的单个 Python 生成 pyc 和其空目录，记录原哈希；生成器已阻止此缓存再产生，源码元数据 --check 通过。

新增 TextureUtils 上层完整接口及 15 个实现/测试/生成文件，另登记 CMake 和 Three.Core.h。项目外候选 Debug/Release 各字节数 7,060 输入 / 35,454 比较、变换与元数据 6,130 场景 / 272,835 比较、模块 realm 2 cases / 17 assertions 全部通过。变换检查保留 image 重复 getter、aspect 转换、属性目标替换、只读/抛错后的部分写入、GC 和递归重入；九条可见函数/类文本逐字比较。独立只读复核未发现现有 JSValue 类型范围内的额外缺陷。新根组合尚未重建；真实 nativeTexture 缺 image/source 等桥接，未标记 TextureUtils 全量完成。

另合入 SPIR-V 矩阵数组窄修及四项新矩阵检查：按实际 OpMemberDecorate / OpGroupMemberDecorate 读取 MatrixStride 和主序；底层 SPIRV-Reflect 不支持的分组注解只在私有反射副本展开，应用原 SPIR-V 保持不变。独立复核发现合法空组误报，第二补丁将其保留为 no-op。原三组反射/uniform 测试与新增 ESSL mat2[2]、行/列主序非方阵数组、嵌套结构数组内 64 位矩阵、合法空组共 21 cases / 580 assertions 双配置通过；所有输入先经 Vulkan 1.1 验证，检查实际物理字节，未放宽 Buffer 拒绝规则。两个补丁前置链及全部记录哈希已核对，新根组合验收待执行。

第五版回调互通 13 个文件已核对基线与候选哈希后合入：Property 注册记录、独立快照与弱观察器修复先包装后注册和旧函数跟随新注册的问题。独立复核补充 setter 抛错/嵌套注册条件恢复，以及 observer 的 std::function 复制须在提交前进行。最终源两配置专项各 40 cases / 310 assertions、realm 4/20、原 7 TU 52/328 通过；完整 1,054 场景 / 394,098 比较属于 observer-copy 前置前的已冻结源，最终根组合将另行运行，不冒充同一哈希的完整验收。早期共享输入被重写导致长度失败、旧探针占用导致重链失败均保留，后续已冻结独立输入。完整纯 C++ owner 保活动态管理回调仍有已测缺口，后续独立实现继续推进。

根组合 COMBINED03 冻结 1,292 个文件，650 个根 CPP 已在不修改内容的前提下强制重新编译，避免 MSBuild 沿用旧头文件 ABI 的对象。Debug/Release 全目标构建终态均为 0；Release 完整 CTest 已通过 172/172，耗时 2,330.96 秒，结束后 1,292 个文件哈希全部一致。Lights 差分耗时 2,050.27 秒；Debug 完整 CTest 仍在运行，尚无双配置完整组合通过结论。9,000 秒为本轮正确性观察窗口，部分项目保留显式超时；原 1,500 秒性能门槛仍未通过，不以延长观察窗口替代优化。

项目外 WebGLMorphtargets 候选按固定原源码生成三个静态 C++ 函数；最终源经 clean-first 实际重编，Debug/Release 各 493 场景（含 170 个 getter、部分写入、异常和回调重入场景）与实际 three.js 输出一致，函数文本逐字比较。额外使用实际 Node --expose-gc 验证三个所有权边界、33 个对象活性字段，两配置均一致。两处原失败完整保留：整调用 frame 额外保留未捕获的 object/program/uniforms/influences，以及函数声明 id 被误判为引用导致 update 函数多保活。修复将活动调用 frame 与保留的共享闭包变量分开，保留回调之间的实时赋值。一次仅修改生成 .inc 而 MSBuild 复用旧对象的结果未登记为新源验收，后续均强制重编两个翻译单元。生成器 --check 与从固定 checkout 重新抽取的 Acorn AST 均逐字复现产物。

此形变候选尚未合入根项目：当前差分使用明确的开发纹理导入夹具，验证的是上层协议；真实原生 DataArrayTexture 桥、公共 Vector2/Vector4 构造器接入及 Vulkan 形变纹理/采样仍待完成。不得据此标记模块全量完成。

另建真实 BufferGeometry 形变字段候选。旧库在 Debug/Release 各实际复现四项不一致：map/array 身份及 C++ 与包装对象两个方向的修改可见性。首版候选完整重编 Debug 核心库后，24 个字段与实际 Node 一致；两个 GC 边界、8 个活性字段也一致。该首版源码、库和探针单独封存，未将其当作后续源码通过证据。

后续形变字段描述符、任意 morphTargetsRelative 值与消费逻辑候选的完整 Debug 重编成功，53 个旧用例实际为 26 通过、27 失败，独立探针退出 1。错误为普通内部对象调用仅供 native host 使用的槽接口，全部原源码、二进制与日志保留。修复正在项目外继续验证；还发现并修正准备中的 for-in/自身键枚举区别、空数组丢键、数组 push 和 clone/toJSON 动态调用顺序、删除重建字段顺序。新候选尚无通过结论。独立 for-in 枚举器在 Debug/Release 各 37 个实际 Node 场景与 GC 保活/释放检查通过，其语言层证据不替代新的 Geometry ABI 联合验收。

## 当前实施批次：程序生成、正则表达式与 Vulkan 接口，2026-10-14

在 batch18 双配置 130/131 的失败基线上，已按依赖顺序合入 Unicode 默认大写、callable builtin 构造钩子、String/RegExp、WebGLProgram、WebGLPrograms、SPIR-V 接口反射、VulkanPipelineLayout 与 Cache 无 owning TLS facade。全部窄补丁与文件哈希核对完成；String 候选携带的旧 JSValue 全文件未覆盖根，原 ArraySpeciesCreate/filter 及 intrinsic species getter 保持。测试与生产源已登记 CMake，第三方来源/哈希/许可证登记 upstream.json，审计重新生成，AST 错误为 0。

初始联编清单冻结 1,172 文件，Windows Debug/Release 全目标构建均终态成功，日志前缀 implementation19-initial。新增专项 CTest 两配置均终态 14/15，分别耗时 1,015.09 秒和 95.19 秒；唯一失败为 WebGLProgram 原真实 Texture 退出检查（6 cases / 34 assertions 中各失败 1）。原主单测两配置各 649 cases / 205,001 assertions，保留 8 个生命周期失败（原 5 个加 Cache 的 3 个检查）。此阶段尚含 DefaultLoadingManager 的旧 owning TLS，以及真实 Texture/native payload→JSValue→heap 的退出环。全部 1,172 个冻结哈希及文件集合再次核对一致，未改写失败断言或结果。

后续已合入 DefaultLoadingManager/Loader 弱原型缓存、AbortSignal/AbortController 弱原型缓存、真实材质 shaderSource 对照及 VulkanShaderModules，并登记 CMake。Manager 候选以原主单测全部 84 个源文件加 Cache/Manager/Abort 新检查在同一进程双配置通过 657 cases / 205,135 assertions；实际 Node 421 场景 / 11,781 比较加 4 个 GC 快照，既有 Loader foundation 14,904 / 15,324 和 parsers 103 / 4,350 亦双配置通过。这些是集成前候选证据。根项目后续清单冻结 1,183 文件，匹配联编的四项集成专项两配置均终态 4/4，耗时 Debug 128.86 秒、Release 7.98 秒；主单测 657 / 205,135、Manager 421 / 11,781、真实 shaderSource 26 / 3,835 / 52 份源码和 ShaderModules 4 / 90 全部通过。测试后 1,183 个哈希及文件集合核对一致，详细 LastTest 日志单独保留。通用堆退出候选尚在项目外验证，完整集成回归尚未执行。

VulkanShaderModules 在 borrowed device 上拥有选定 entry point 的 shader 句柄、接口副本和入口字符串；先完成全部反射/容器准备，再创建 GPU 资源，失败只逆序释放成功句柄。移动与 reset、错误输出句柄和实际并发创建/销毁检查双配置各 4 cases / 90 assertions。现有 VulkanRenderer 改由该所有权对象提供 pipeline stage 描述；匹配根依赖的外部 Renderer 副本双配置分别通过原 GPU 664 assertions、裁剪 59 checks、裁剪生命周期 38 checks、顶点绑定 76 checks。同行只读复核未发现新增事实缺陷。此变更仍使用原固定 MeshBasic SPIR-V，尚未将 WebGLProgram 生成源码接入 GPU。

现有固定 MeshBasic Renderer 的布局也已改为以 ShaderModules 接口、实际物理设备限额及一个颜色附件创建 VulkanPipelineLayout，替代手工的 12 项绑定与 push constant 声明；descriptor 写入、uniform ABI 和 shader 源保持。该一文件窄补丁的匹配外部 Renderer 双配置均实际通过原四项 GPU 回归（664 assertions、59/38/76 checks）。首轮启动脚本将目录与文件名作为独立参数，测试未执行；该 harness 失败另存记录，修正路径后才登记成功。该改动已根集成，完整根回归待最终组合重建。

真实材质源码夹具调用原生 WebGLPrograms→WebGLProgram 及实际 ShaderLib/ShaderChunk，与固定上游 Node 双配置各比较 26 场景 / 3,835 项 / 52 份完整源码；包含 __VERSION__/GL_ES 自定义分支。fixture 显式设置 customProgramCacheKey，不能据此证明默认 onBeforeCompile.toString 元数据或 ESSL→Vulkan 语义转换完成。根项目新差分输出目录在构建树中，不写入源码树。

后续根组合又合入原生 heap 安全点、两阶段 realm 退出与 native ownership 图固定点检查。每个 ownership hook 返回后立即保活描述的 owner 并捕获非拥有字段快照，trace、析构或后一 hook 重分配容器后不再读取旧字段地址；collector 临时 owner 不计作应用根。最终图增量及独立复核在项目外双配置通过 28 个退出/图变更用例，Program 6 cases / 34 assertions、WeakValue 11 / 483、NativeArray 131 checks 和 11 个原生模块退出检查。专项 NativeGC 聚合与原 baseline 仍同有 59 != 53 的一个失败，未改断言；这些候选证据不代替最终根组合回归。

同一组合通过独立窄补丁修复 Material.onBeforeCompile/customProgramCacheKey 的 name 与已知原源码文本。源码元数据存于函数内部 record，原 Function.prototype.toString 和动态 getter/call 保持；仅两方法提供元数据。项目外双配置各 4 cases / 26 assertions，并与锁定 Material 的实际 Node 场景比较 26 个字段；原 5 个不一致字段记录保留。根 CMake 已登记两组独立单测及差分，完整 Debug/Release 重建和 CTest 尚待终态。

集成前各配置：Unicode 大写对所有 1,114,112 个码点和 10,012 个 UTF-16 字符串精确对照 Node/Unicode 17；RegExp 93,707 场景中 93,003 精确支持、704 明确未实现，协议 1,800、GC 6；String/RegExp 新单测 11 cases / 136 assertions。有效未支持的正则语法/flags 不作为语法错误或已支持功能登记。

WebGLProgram 各配置实际源码主语料 1,084 / 176,024 比较、追加语料 288 / 65,069、GC 3 / 24，全部通过；单测保留 5/6 cases、33/34 assertions 的原 Texture 退出失败。WebGLPrograms 最终匹配 RegExp 35A 依赖，各配置 2,620 / 482,804 比较、7 cases / 278 assertions；方法独立保活 14 / 168。CPU 命令协议调用真实 nativeWebGLProgramConstructor，未以 fake Program 替代。

SPIR-V 接口拥有选定 entry point、descriptor/block、输入输出和 push constant 的独立副本，各配置 4 cases / 105 assertions。固定 Khronos SPIRV-Reflect 563dc9a16d0e86145e12a66bb61f0695ca4a14b9，源文件/许可原样保留。Vulkan 布局各配置 5 cases / 96 assertions，RTX 3070 Laptop GPU 实际创建/销毁 97 个布局、2 个 shader modules 与空布局，覆盖并发与所有创建失败边界；补齐 framebuffer 附件的 fragment resource 限额计数。GPU fixture 已按 advertised extensions 支持 portability enumeration/subset；仅 Windows 运行，未启用 Khronos validation layer。

尚未因本批完成的内容包括：原 ESSL 300 语义到 Vulkan SPIR-V 的通用转换、GL active uniforms/attributes 元数据与实际 GPU 程序绑定、完整 graphics/compute pipeline、传统 Renderer 和现代 nodes/TSL 全量接入，以及 Linux/macOS 验收。原生 glslang CLI 编译目前接收 Vulkan GLSL；直接改写 #version 会改变用户 __VERSION__/GL_ES 行为，不能视为通用实现。

## 上一实施批次：颜色管理、图层、shader 缓存与编译基础，2026-10-13

固定上游不变。已合入 ColorManagement/WebGLUtils/Object.assign、管理 Layers 与 UniformsUtils 导出、WebGLShaderCache、Number.toFixed/NumberData、Array.filter/ArraySpeciesCreate、get/call 线程检查和 VulkanShaderCompiler。上一批三个失败项的 realm/首次初始化修复亦已合入。首次完整构建均因 ShaderCache 两个 probe 缺少 fixture 搜索路径失败；仅修复 test/CMakeLists.txt 的两个 target include 目录，原失败日志保留。

修复后 Windows x64 Debug、Release 完整构建均成功；完整 CTest 均终态 **130/131**，分别耗时 **1,356.45 秒、133.97 秒**。主单测 643 cases 中 638 通过，204,961 assertions 中 204,956 通过；五处失败为 Number 原型、模块首次 realm/跨线程、Array species getter 及 ShaderLibrary 的生命周期检查。真实库独立复现确认 Cache 与 DefaultLoadingManager 的 owning TLS 在新线程入口提前创建并持有 realm；修复候选尚未合入。本批未获全绿，原断言未弱化。Layers/UniformsUtils 的 4,253 个场景及此前失败的灯光检查均已在完整回归通过。

2026-10-14 再次核对 repair1 冻结清单，全部 1,119 个文件哈希一致。构建、测试日志与冻结清单保留在项目外，前缀 implementation18-repair1。当前审计为 1,251 模块（196 partial、745 missing、290 Vulkan pending、20 dependency pending），347 个模块存在 C++ 文件。下一批候选单独集成验收，不改写本批失败结果。

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
