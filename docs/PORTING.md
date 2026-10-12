# three.js → three.cpp 移植约束

目标是完整复现 three.js 逻辑，渲染后端替换为 Vulkan。首个能运行的示例不代表目标完成。

参考版本记录在 `upstream.json`：three.js r187dev，commit `11738a9d55e502fb484cefffcea3a66dc44e9fe9`。参考源码改变时必须显式更新基准、差分结果和版本记录。

范围包含全部 `src` 与 `examples/jsm`，扩展携带的第三方库单独跟踪依赖和许可。editor、manual 网页应用不作为原生应用重写。要求 Windows、Linux、macOS 分别验收。禁止引入 CEF、嵌入式浏览器和 JS 运行时桥接；Node 仅用于开发阶段的上游差分与审计。浏览器专属接口由 C++ 原生适配逐项实现，未实现的能力保持待办。

## 一致性要求

- 文件路径按 upstream 分类；同一模块的 `.h` 与 `.cpp` 同放 `src` 对应目录，不另设 `include` 树。
- 测试使用 `test/unit/src` 和 `test/unit/utils`；工具使用 `utils`；示例使用 `examples`；着色器使用 `src/renderers/shaders/ShaderLib`。尚未移植的目录不创建空占位。
- 保持类名、方法名、默认值、返回自身语义、算法、计算顺序和边界行为。
- JavaScript Number 对应 IEEE 754 binary64；Float32Array 和整数 TypedArray 保留独立存储类型与转换规则。
- 矩阵保持列主序、右手坐标和相机负 Z 朝向。Vulkan 的 Y 轴及深度范围转换只发生在渲染后端。
- Object3D 的 rotation/quaternion 双向同步、矩阵更新标记、父子更新顺序必须对照源码。
- copy/clone 区分值复制与共享几何、材质、纹理等资源引用；序列化资源按 UUID 去重。
- 材质颜色使用 three.js 的工作颜色空间；输出转换、混合、雾与透明排序按源码验证。
- 未实现的渲染功能必须显式报错，不能接受属性后静默输出错误结果。
- 每项移植必须对照 upstream 单元测试，并补充实际运行 three.js 与 C++ 的差分验证；存在头文件不能视为完成。

## C++ 适配

原版接口优先，允许修改旧接口。Vector2/3/4 与 Color 的分量使用公开数值字段，例如 `v.x`、`color.r`；Euler/Quaternion 使用公开赋值属性，在直接写入时触发原版旋转回调，批量操作保留原版回调次数与 `update=false` 行为。`Object3D::children` 用 shared_ptr 持有子节点，parent 为非拥有指针。对象不允许直接复制，以 copy/clone 保持身份、监听器和旋转回调的有效性。需要更新版本的属性使用 C++ 赋值代理。矩阵 elements 与部分其余属性尚待接口迁移。

当前 `JsonValue` 覆盖 JSON 数据，不支持 JavaScript 的函数、undefined、循环引用或任意动态原型。堆上共享对象的 `toNonIndexed()` 无索引分支返回自身；栈对象使用此分支会遇到 C++ shared_from_this 的所有权约束。C++ 关键字 `static` 属性对应 `isStatic`，JSON 仍为 `static`。

`JSValue` 是独立的运行时基础：支持 undefined/null、binary64 数值、UTF-16 字符串语义、对象身份、赋值别名、键顺序、稀疏数组、原生函数及接收者/显式捕获、原型链、属性描述符/访问器、SameValue、Reflect.set、基础强制转换及受管理的循环回收。GC 追踪原型、函数捕获、访问器和转换回调；回调重入保留调用对象与描述符快照。`trace()` 明确记录类型、负零、非有限值、属性顺序和图身份；`toJSON()` 支持 getter、toJSON 回调及 lone-surrogate 转义。@@toPrimitive 使用受管理的内部适配钩子，未实现 Symbol 属性。Symbol/BigInt/Proxy、super/bind/跨 realm 构造、完整装箱对象与内建库、跨线程堆引用仍待实现；原生函数能力不执行 JS 源码。现有全部类的动态属性和 TypedArray 尚未统一接入此对象图，不能据此宣称完整库语义已完成。

动画依赖的 Array push/pop/slice/indexOf/splice、Function.call、Object 原型基础方法及运行时 EventDispatcher 已接入；差分验证 holes、继承索引、访问器、强制转换、只读属性的部分修改、监听器快照、抛错和重入。slice/splice 观察 constructor 读取及其异常；Symbol/custom species、Array 方法对基础值 receiver 的完整装箱仍待实现。回收目前通过 collectGarbage 显式触发，尚无自动调度策略。原生 payload 分阶段析构：全部别名缓存先退役，保留 JS 记录供析构函数读取或保存普通字段，重新标记后才清除不可达记录；已经开始析构的 C++ 对象不能复活。每个 owner 析构后重新标记，性能仍需优化。

`ArrayBuffer` 与 `TypedArray` 已支持共享字节、独立视图身份、只读 length/byteLength/byteOffset/buffer、subarray 与 slice、重叠 set、整数模转换、Uint8Clamped 饱和/偶数舍入、Float32 溢出，以及同类型 set 的原始 NaN 位保留。BufferAttribute 直接接收 TypedArray 时保留视图身份；带类型子类、copy 与 clone 分配独立存储。TypedArray.get 的越界结果是 undefined；普通 BufferAttribute 数值读取越界仍用 NaN 表示其参与数值运算的结果。跨 itemSize 的组件读取按原数组地址计算。ArrayBuffer 的 detach/resize、DataView、原生 Float16Array、BigInt 数组及其运行时对象图接入仍待完成。

BufferAttribute.itemSize/count 与组件索引使用 double，保留非整除 count、负数、非整数与非有限输入；TypedArray 决定实际下标是否可读写。version/usage/gpuType/updateRanges/onUploadCallback/isFloat16BufferAttribute 使用同一管理对象的活字段，保留任意值、描述符、继承 getter 和方法替换；needsUpdate 仅在严格 true 时按上游转换并递增版本。更新范围是实际管理数组，数组和范围对象分别保留身份，不另设原生权威 vector。C++ 数值读取使用 toNumber，布尔读取使用 toBoolean，回调通过 onUpload 或 JSValue::function 显式捕获注册。copyAt 保留逐项读写与重叠效果；toNonIndexed 按原 index 数组长度与浮点 itemSize 逐项展开，法线计算保留不足三个顶点的尾部处理。BufferGeometry 的 groups/drawRange 也保留 double，默认 drawRange.count 是 Infinity；网格拾取仍要求有效非负整数顶点索引，未对齐绘制索引偏移显式报错。属性 bufferSubData 的无效字节范围记录 WebGL 错误并不写该次数据；上层继续后续范围、clear、callback 和 version。原有数学构造参数采用值语义，不保留传入向量的 JavaScript 引用身份。

属性变换固定进入循环时的 count，并在逐次读取分量后使用矩阵，保留 getter 回调修改 count/矩阵的时机。Vector2/3/4 与 Color.fromBufferAttribute 使用 double 下标，按 x/y/z/w 顺序逐项读取与赋值，避免 C++ 函数实参求值次序改变回调顺序。BufferAttribute 变换使用模块临时向量的 thread_local 适配；整个库的所有模块临时状态和跨线程语义尚未统一验收。

DataUtils 按上游查表、clamp 和 Float32 中间转换实现半精度转换。Float16BufferAttribute 构造参数是 Uint16 原始编码，getX/Y/Z/W 与对应 setter 转换半精度，getComponent/setComponent 和 copyAt 按上游保留原始存储语义；变换通过虚方法调用。normalized 按当前数组类型应用上游规则。Vulkan 上传解码 Uint16 半精度顶点，并遵循 HALF_FLOAT 忽略 normalized 的 GPU 规则。NaN 转半精度的原始结果匹配当前固定 Node/V8 oracle 的 0xfe00；ECMAScript 不保证跨引擎 NaN payload 相同。警告目前输出到 stderr，完整 utils 日志接管机制仍待移植。

已增加 Curve、LineCurve/LineCurve3、QuadraticBezierCurve/QuadraticBezierCurve3、CubicBezierCurve/CubicBezierCurve3、EllipseCurve/ArcCurve、SplineCurve、CatmullRomCurve3 及公共 Interpolations 的算法。控制点和样条点集合使用 shared_ptr 保留共享身份；采样返回共享点对象，闭合路径可复用首点身份。CurvePath/Path/Shape/ShapePath 保留曲线/长度缓存、JSON、多边形孔洞分类和 fillRule 读取；ShapeUtils 使用同版本 earcut 3.0.2 三角化，保留输入重复端点移除与索引次序。通过 Curve 基类调用路径无参采样时，仍使用路径默认值 12/40。

Curve<Vector2/Vector3> 是维度的静态适配；混合维度、无 target 的点/切线返回值与数值 fromJSON 尚不能覆盖任意动态参数。抽象 Curve.getPoint 当前抛 logic_error，与 JS 的警告及 undefined 不同。CurvePath 引用返回方法无法表达 null，提供 getPointOrNull 保留空结果；空 getLength 的 undefined 映为 NaN，特殊空采样的 undefined 点映为 nullptr。Ellipse 无限角度循环未作超时改写；CatmullRom 的非法 curveType 保留原模块历史系数，模块临时状态采用 thread_local。以上仍属于未全量验收模块。

Interpolant 及 Discrete/Linear/Cubic/QuaternionLinear/Bezier 插值器保留搜索缓存、区间回调、结果复用、数组类型、共享别名及 Cubic 端点规则；普通数组使用 JSValue 保留空位/undefined。sampleSize 为 double，构造分别执行 Array 与 TypedArray 长度规则。上游 i !== stride 对负数、非整数或非有限 stride 的非终止循环，在实际 copy/interpolate 阶段显式抛 JSRangeError；Quaternion 的非四倍步长同样明确拒绝。任意动态 constructor/数组子类及完整运行时对象图接入仍未完成。

区间回调与 interpolate_ 的 t0/t1 保留为 JSValue，到实际算术运算时才转换；getSettings_ 支持虚调用。Cubic 在转换回调前快照 stride，Wrap 端点的 + 保留字符串拼接；Bezier 先读取控制点再计算，Quaternion 保留重复转换及其顺序。自定义 valueOf/访问器修改数组、settings 或 valueSize 的行为以回调 trace 差分验证。

earcut 的节点、洞桥接、z-order、自交修复、deviation 与 flatten 按固定源码实现。多个 NaN-x 孔洞的比较器不一致排序已改用固定 V8 TimSort 的原生适配；原失败复现 `test/unit/utils/parity/earcut-nan-boundary.mjs` 已通过，并纳入 earcut 差分回归。dim=0 的上游非终止循环在 C++ 明确抛 JSRangeError；flatten 缺失坐标的 undefined 映为 NaN。ISC 许可保留在 `src/extras/lib/earcut.cpp`。

新增基本几何体 Circle、Ring、Sphere、Cylinder/Cone、Torus/TorusKnot、Lathe、Capsule，以及 Polyhedron 与四种规则多面体、Shape、Extrude、Tube、Edges、Wireframe。参数保留原始 double；分段按各类原算法取整或保留小数，不能统一强制转成整数。参数几何的 JSON 在读取属性之前返回，克隆保留上游参数的共享/复制规则。Tube 的 Frenet 分段支持上游小数边界；非终止或缺失索引按实际执行路径显式报错。Extrude 保留孔洞、倒角层、沿路径挤出、自定义 UVGenerator、材质组和共享 options；顶点临时数组使用 JSValue 区分 undefined 与 NaN。Extrude.fromJSON 目前接收持久化 JSON，无法表达原版直接 fromJSON(toJSON()) 时复用活 UVGenerator、或原地把路径 JSON 替换为 Curve。几何参数和自定义回调的完整动态类型仍待验收。

TextureSource/Texture 与 Data、Array、3D、Cube、Depth、Framebuffer、Compressed、External 等 CPU 纹理对象保留 source 身份、double 版本、严格 true 更新、mipmap 容器复制、矩阵/UV 包装、更新范围、layer 集合与 dispose。Texture.copy 深复制 userData；Depth/Framebuffer 等派生 copy 的 source 替换时机单独对照。Material、Scene 环境、Mesh 的纹理/图片资源共享 JSON context；动画片段与 ShapeGeometry 的 shape 也接入场景资源缓存。CPU 纹理类存在不代表所有格式已经接入 GPU。

传统材质已包含 Material、MeshBasic/Lambert/Phong/Standard/Physical/Toon/Matcap/Depth/Distance/Normal、LineBasic/Dashed、Points、Sprite、Shadow、Shader/RawShader。实时字段注册统一 copy、参数更新与序列化，Physical 功能开关按跨零边界递增 version，reflectivity 映射 ior；Shader uniforms 保留 clone 回调、浅复制与纹理资源缓存语义。普通未声明属性保存在唯一的 extraProperties 图。原生包装可访问活颜色/向量/矩阵和各类纹理字段；needsUpdate 仍是只写属性，setValues 因读取到 undefined 而忽略它。

材质包装尚未覆盖全部方法、EventDispatcher 继承及任意属性类型替换/删除。嵌入式颜色与向量不能直接替换身份，clippingPlanes 的动态数组访问仍待实现；userData 的 JsonValue 包装只支持当前 JSON 值域，不能代替完整 JSValue 对象图。矩阵 elements 可按索引修改但仍保持固定长度。GPU 仍只支持已列出的 MeshBasicMaterial 路径，其余材质明确报错。

灯光 CPU 类包括 Ambient、Directional、Point、Spot、Hemisphere、RectArea、LightProbe、IESSpot、Projector 及对应 Shadow，SphericalHarmonics3 按原算法实现。shadow 的 camera/map/mapPass、复制与 dispose 按引用和回调顺序处理；copy 不会复制源 map/mapPass 等上游未复制字段。固定版本 PointLightShadow.updateMatrices 继承通用实现，对缺少 target 的 PointLight 会在写入相机位置后抛错；保留此版本行为，未混入旧版六面阴影算法。GPU 灯光和阴影尚未接入。

Bone、Skeleton、SkinnedMesh、InstancedMesh、InstancedBufferAttribute 已实现逆矩阵、pose、骨骼纹理、CPU 蒙皮、实例矩阵/颜色/morph、对象包围体与射线检测。bones 容器切片而 boneInverses 共享句柄；实例 count 和 meshPerAttribute 保留 JSValue（包含 undefined），避免默认值或字符串转换被 C++ 整数提前改变。动画绑定支持 .bones[name] 和命名骨骼轨道；原生对象图追踪骨骼、实例属性、灯光目标和阴影资源。CPU 包围体路径优先对象自身包围体，回调清空资源时按实际步骤抛错。InstancedMesh 的基础 Vulkan 路径已接入，完整蒙皮 GPU 路径仍明确拒绝。

Line、LineLoop、LineSegments、Points、Sprite 按上游继承 Object3D，保留几何/材质共享、copy/clone、morph 字段、线距离、阈值拾取和 Sprite 相机/中心/旋转规则。Sprite 的共享几何采用 InterleavedBufferAttribute；Box3 和 Frustum 已识别这些对象。LOD 保留活 levels 数组、只读绑定、距离/迟滞、相机 zoom、clone/JSON 与逐级可见性；渲染投影阶段只在 autoUpdate 严格为 true 时更新。Sprite.center 仍为内嵌 Vector2，替换身份、几何 morph 数组及任意动态方法的全量适配尚未完成。线、点和 Sprite 的 GPU 路径仍待实现。

RenderTarget/RenderTarget3D 的 CPU 资源生命周期保留 MRT 源共享、clone/copy 分离 Source/image 而共享像素数据、depthTexture 所有权、resize/dispose 顺序、只读数组的部分失败及 options getter 读取时机。copy 在 clone 回调前保存赋值目标数组，随后字段操作重新读取当前 textures；回调替换数组不会被折叠成一次局部变量操作。选项中的布尔/数值字段仍受 C++ 类型域限制，texture.renderTarget 为非拥有原生指针，完整运行时图、任意动态选项和值替换仍待迁移。Vulkan 渲染目标尚未接入。

AnimationUtils、KeyframeTrack、六种轨道及 AnimationClip 已实现基础算法、数组类型、切线设置、关键帧裁剪/优化、克隆、片段查找与 JSON。自定义访问器/转换/切片及回调替换轨道容器的读取顺序通过直接运行上游验证。NaN 时间数组排序及 getter/valueOf 调用次序已按固定 V8 算法差分；动态 constructor/species/iterator、替换静态 parse/toJSON、任意 JS 类型的 duration/frame/fps 尚未完成。

Uniform、UniformsGroup 已使用管理对象实现 value 克隆、嵌套数组、动态方法、事件与共享引用；ShaderMaterial.uniformsGroups 可直接接收组句柄。UniformsUtils 按上游目录导出 cloneUniforms、mergeUniforms、cloneUniformsGroups 和 getUnlitUniformColorSpace，材质复用相同实现。UniformsLib 的 16 组默认定义由固定源码语法树生成，逐项构建独立数学对象、数组和字段，保留模块内共享与后续可变属性；原生线程对应独立运行时域。GPU UBO、完整着色器库与材质管线仍待实现。

JSValue 新增独立的 constructor/construct(args, newTarget) 和可构造性检查，区分只能构造的类与显式声明可普通调用的构造器；保留构造原型、返回值、描述符、接收者、捕获、异常与 GC。Object/Array 内建接入该入口。Array.map 保留长度快照、holes、继承索引、回调 thisArg 与第三参数、构造器读取及部分失败；完整 Symbol/species、super、Proxy/bind、跨 realm 和基础值专用包装原型仍待实现，现有所有原生类的 constructor/clone 也需逐项迁移。

InterleavedBuffer/InstancedInterleavedBuffer 使用活管理字段保留数组/视图身份、stride/count、version、updateRanges、严格 true 更新、copyAt 重叠、上传回调及 clone/toJSON 元数据缓存。NativeArray 将原生 View/ArrayBuffer 与管理对象连接，提供 buffer/byteOffset/byteLength/constructor/slice/subarray/set 等该路径所需行为；原生存储父字段纳入所有权图，避免把仅在不可达环中持有的数组误判为外部根。Symbol.iterator/species、detach/resize、SharedArrayBuffer、Float16/BigInt 数组与完整 TypedArray 方法仍待实现。

BufferAttribute 与 InterleavedBufferAttribute 分别实现 C++ 的 VertexAttribute 协议，避免把交错属性伪装成 BufferAttribute。Geometry 的 attributes、morphAttributes 与 getAttribute 返回共享协议句柄，index 仍是 BufferAttribute。交错属性的 data/array/offset/itemSize/normalized 使用活管理字段，保留 getter、转换、矩阵变换、clone/JSON 元数据共享及去交错行为；CPU 几何、拾取、包围体与蒙皮读取已迁移。原生矩阵参数在调用期间实时读取，返回或异常后，回调保留的矩阵包装转为安全快照。交错顶点 GPU 上传仍明确拒绝，完整动态属性与构造边界仍需验收。

资源序列化仍有系统边界：JsonValue 不能完整保留活对象图。Geometry 与 detached InterleavedBufferAttribute 的结构快照已保留 -0、NaN 和 Infinity，不提前调用 JSON.stringify 或嵌套 toJSON；groups 按上游单独经过 stringify 归一化。Texture.userData getter 当前提前执行；Object3D 子对象与缓存提取、动画资源转 JsonValue、原生几何数组的共享身份仍需迁移为权威 JSValue 图，然后仅在最终 JSON 输出时生成快照。现有数值及 JSON 内容差分不能证明这些对象身份/回调时序完全等价。

PropertyMixer 实现类型对应的 Float64/普通数组缓冲、交替累积、加法混合、原始状态保存/恢复及绑定回调。PropertyBindingInterface 以共享数组句柄传递 getValue/setValue；PropertyBinding 已增加路径解析、节点查找、延迟绑定/失败状态、四类属性读写、材质/矩阵版本标记与 Composite。默认派发表及其内层数组共享身份，键转换、属性写回与版本标记分别保持上游求值时机。权重使用 JSValue 保留转换顺序，原生 valueSize/index 的任意动态类型仍未覆盖。

AnimationAction、AnimationMixer 与 AnimationObjectGroup 已按上游实现播放、暂停、循环、倒放、fade/warp、混合、共享属性绑定、缓存交换、统计、批量对象绑定和事件重入。三者的权威状态保存在受管理的 JSValue 对象图，C++ 类是保留身份的句柄；公开赋值字段由 JSMember 代理，复制和移动句柄时重新绑定代理。Mixer 的控制插值器共享模块结果数组。AnimationRuntime 将真实轨道、片段、插值器、PropertyBinding/PropertyMixer 接入活属性与共享方法，普通场景节点、材质、morph 和子节点可直接参与动画。

插值器的原生 evaluate 在内部调用 interpolate_、copySampleValue_、intervalChanged_ 时读取当前包装对象的方法，Cubic 同样动态调用 getSettings_；默认方法仍走 C++ virtual，保留派生实现。运行时接收者是显式登记的 JS 根，分发使用无捕获函数指针，回调内修改方法、抛错或收集垃圾均有差分覆盖。三类动画句柄支持传为 JSValue 参数，构造函数名称、长度、原型与属性描述符按上游建立；动画类仍需逐项接入新增构造入口，不能据此宣称全部类已支持构造。uncacheRoot 对基础值缓存保留 for-in 枚举和继承 getter 的原始接收者；完整 String/Number/Boolean 专用原型仍未实现。

新增动画路径保留固定上游的异常行为：Group 绑定缺少 rootNode/path 时 uncacheClip 会抛 TypeError；部分 uncacheRoot 后重新 play 旧 action 的路径也会因保留的绑定缓存索引抛 TypeError。Group 构造使用普通 arguments-like 对象模拟 slice.call 的 length 和索引访问，未实现完整 Arguments exotic。原生动画字段目前不能删除或改为访问器/只读描述符；原生 tracks 等容器不能通过复制普通 JS 数组伪装共享身份，不能表达的替换显式拒绝。Object3D.animations 的原生 vector 与 geometry.animations 支持活索引/length 访问，跨对象整数组替换仍需统一共享存储。任意 C++ 回调闭包内隐藏的 JSValue 无法自动枚举；可回收的 JS 回调必须使用显式管理的捕获。完整类构造、原型及任意动态字段类型仍待覆盖。

NativeObject 将现有场景、数学值、材质、纹理与数组接入 JSValue 属性访问；PropertyBinding 通过相同的 get/has/set/call 操作连接原生属性和普通动态对象。children、材质数组及 morph influence 使用共享容器，替换容器后旧绑定仍保留旧数组。renderOrder 使用 double。Raycaster.intersectObjects 以 SharedArray 值句柄接收对象数组，回调替换调用者变量不改变正在遍历的数组；同一数组的删除与缩短仍即时可见。std::vector 可用于初始化该句柄，但转换会复制容器，后续动态修改应使用 SharedArray。原生宿主具有按身份和类型查找的弱缓存，受管理的内部图边以及当前原生字段的引用追踪；索引访问、Reflect.set、属性定义和删除分别适配，TypedArray 无效索引赋值与 defineProperty 的不同结果不能合并。

原生所有权追踪以 shared_ptr 控制块识别 owner，以真实字段地址去重 JS 根和原生强引用；已注册的外部 owner 及其子资源会保留现有包装对象与动态属性，释放外部 owner 后可回收由 JS 边形成的环。已登记动画、场景 children、Mesh 材料/几何、几何属性、纹理/Source 等当前适配使用的原生图。纯 C++ shared_ptr 强环无法由 JS 回收器自动拆除，会保守保留环及出口，直到原生代码打破强环。回收依赖完整登记真实字段；未登记的原生类型、任意 opaque C++ 闭包和全部宿主对象仍需逐项接入，不能宣称任意原生图都已可回收。

原生桥接仍未覆盖全部类/字段与任意 JavaScript 类型：数值/对象容器的元素域、数组访问器、Euler 任意 order、内嵌成员替换、完整类原型和原生几何属性数组图均需继续迁移。Geometry.morphAttributes 目前仍为快照；morphTargetDictionary 仅支持原生数值索引，不能支持的描述符显式拒绝。原生 clip/track vector 的扩容空项目前是 null，不具备稀疏数组 holes 语义；删除现有索引显式拒绝，不能报告成功却保留原值。直接 C++ 数组操作尚不观察包装上的 preventExtensions。NativeArray 已统一当前属性、纹理图片、骨骼矩阵、轨道和插值器的 View/ArrayBuffer 身份及真实原生所有权，未接入的类型仍需逐项登记。PropertyBinding 的非终止数组长度、自定义 replace/RegExp 宿主等动态边界未全量支持。新增桥接不能代替完整运行时一致性验收。

Quaternion、QuaternionLinearInterpolant 和 PropertyMixer 的 sin/acos 使用纯 C++ 的 V8 fdlibm 算法适配：固定来源为 Node v26.5.0，源文件 SHA-256 与许可记录在 upstream.json 和 src/math/JSNumberMath.cpp。这是为保留插值末位及由严格比较触发的回调次数；其他数学函数尚未统一为该运行库，不能据此宣称所有计算逐位等价。没有引入 V8 引擎或 JS 运行时。

数组数值排序使用独立 C++ TimSort，固定 Node v26.5.0 / V8 14.6.202.34-node.24 的比较调用顺序。AnimationUtils、earcut、ShapePath、Raycaster 和 Vulkan 渲染列表/上传范围接入此算法。NaN 比较结果按零处理，同时保留算法对非传递比较器的实际调用与结果；这不是对其他 JS 引擎排序顺序的保证。通用 helper 处理稠密原生值；holes、undefined、属性访问和写回描述符仍由调用层负责。源哈希及完整许可证记录在 upstream.json、src/core/ArraySort.LICENSE。

渲染项持有对象、材质、几何和原 Group 的共享引用，回调移除对象或清空分组后当前帧引用仍有效。对象回调按 before、实际命令录制、after 执行；回调中的 clear 与 readPixels 支持当前绘制状态。活动渲染回调中递归调用 render 或 setSize 目前显式报错，完整渲染状态栈尚未移植。

普通顶点属性、颜色和索引已有按属性身份/version 的 Vulkan 缓冲缓存。修改数组但未设置 needsUpdate 时保留旧 GPU 内容；版本更新应用有效 updateRanges 并清空，首次上传保留 ranges，上传回调后记录 version。缓存快照不分配库对象 ID；已录制 draw 的缓冲在后续上传时保留原内容。geometry/attribute dispose 注销监听并释放缓存，弱引用过期在下一次 render 或 renderer 析构回收。上传回调中的 render/setSize 也由全程 guard 显式拒绝；完整回调重入仍待实现。

InstancedMesh 使用真实 Vulkan instanceCount 绘制，通过 storage buffer 读取实例矩阵和 RGB 颜色，并按实例位置计算雾深度。矩阵/颜色复用属性缓存、局部上传和 dispose；同一帧先更新几何，再更新实例矩阵及回调之后的当前颜色。绑定按属性身份缓存，保留同一属性修改 divisor 后仍使用旧布局的规则；矩阵四列分别转换 meshPerAttribute，转换回调内 dispose 仍保留当前 draw 的资源。geometry.instanceMatrix/instanceColor 优先于对象字段。GPU 实例数量执行整数转换，统计保留原逻辑数量，严格数值零和字符串零分别处理。

实例 morph 使用 morphTexture 的版本缓存、gl_InstanceIndex 对应纹理行及按目标数量更新的几何缓存；矩阵 divisor 不改变 morph 行号。未上传的纹理及纹理范围外的权重读取为零，着色器显式检查范围，避免 Vulkan 越界读取。该路径仅支持当前基础材质、16 分量矩阵与 3 分量实例颜色，交错或任意 instanced 顶点属性、完整 WebGL 错误队列/绑定状态、光照和蒙皮仍待实现。超出上传属性存储或 Vulkan 设备范围的读取显式拒绝，完整图像差分与性能尚未验收。

线、点和 Sprite 进入持有 Object3D 的渲染列表，按可见性、视锥、材质组和提交前 layers 过滤；实际提交时，在 before 回调后明确报告 GPU 未实现。尚未支持的这些图元暂不执行投影阶段的 GPU 属性上传，其上传回调时序仍待接入。

当前顶点缓存转换为 float32，索引转换为 uint32；雾深度和 morph influence 派生数据仍按 draw 生成。排序已采用 Vector4 clip-z；零数量几何保留属性/index 上传、对象/材质回调与零数量 draw，负裁剪范围才提前退出。TypedArray 快照独立复制字节，避免 CPU 别名修改提前影响 GPU 缓存。属性局部上传按 [WebGL2 bufferSubData](https://registry.khronos.org/webgl/specs/latest/2.0/#3.7.3) 的独立字节/元素偏移转换，零 count 上传剩余元素；合并修改原范围对象，先更新 GPU 再清空范围并调用回调。范围越界仍显式报错，未适配 WebGL 错误队列；含 NaN 的范围排序固定为上述 V8 调用顺序。完整原生存储类型、device-local staging 与 usage 策略仍待实现。

同一属性的数组替换为相同字节数的其他类型时，保留首次上传的 type/bytesPerElement 和独立原始字节快照；版本更新后按当前源数组字节写入旧缓存，局部更新按首次缓存元素大小覆盖转换后的 GPU 组件。几何更新快照属性名，上传每项前重新读取当前字段，保留较早上传回调替换/删除后续属性的时机；新增属性到下次更新才上传。显式 null 属性按上游读取时抛 TypeError。完整受管理几何 attributes 对象图仍待迁移。

WebGLInfo、WebGLBufferRenderer 与 WebGLIndexedBufferRenderer 已按原模块移植为受管理 C++ 对象。Info 保留捕获的 render 身份、动态常量/访问器、统计数值和 reset 顺序；字面量字段使用自有可写/可枚举/可配置数据属性。两个绘制模块保留动态调用接收者、模式/索引缓存、多次绘制和异常/重入顺序，闭包内部状态隔离 Object 原型。GL 参数是原生管理调用接口，未引入 OpenGL 或浏览器；尚未代替 VulkanRenderer 的命令提交与统计实现。

CPU 派生雾和 morph 数据仅处理当前 draw 实际引用的顶点；drawRange 外的 NaN、尾部残余组件和未引用索引不会提前中止合法绘制，实际 GPU 存储越界仍显式拒绝。线框按原数组生成边索引，保留尾部缺失索引写零与 Uint16/32 转换，再按生成后索引数量裁剪。渲染统计使用上游逻辑 double drawCount，独立于 Vulkan 调用必须使用的整数数量。Mesh CPU 拾取读取 morph influence 空位或缺失值时保留 undefined 转 NaN，JSON 输出空位为 null，clone 保留空位。Vulkan 非有限形变仍显式拒绝；缩短 influence 数组所需的 WebGL program uniform 尾部状态保留尚未移植，非零绘制时明确报错，零数量绘制仍保留上传、回调与统计。

CPU 库禁止浮点融合与重排，以保留 JavaScript binary64 的逐步求值顺序。数值差分仍使用明确容差，不能声称不同数学运行库或 GPU 着色器输出逐位相同。

## 完整工作范围

Clock/Timer 保留启动、停止、时间缩放、重置与可见性回调顺序；默认时间源是 steady_clock，可注入 NativeTimeSource，窗口可见性由 NativeVisibilitySource 适配。ArrayCamera、StereoCamera、FrustumArray、InstancedBufferGeometry 与 ClippingGroup 已补 CPU 行为；相机 clone 保留上游忽略 recursive 参数的规则。StereoCamera 共享临时眼矩阵，Matrix4.invert 按固定上游表达式顺序计算，包括正负零。多视图、实例几何和裁剪组 GPU 路径仍待接入。

新增 13 个 helpers，保留几何/颜色更新、源对象矩阵共享、clone 异常和 dispose 顺序。Matrix4 显式 alias 保留原 source.matrixWorld 的身份，即使 helper.source 字段后来被替换；NativeObject 的所有权图追踪该原 owner。CameraHelper.pointMap、颜色值及骨骼/相机/平面数组接入管理对象。入口的任意动态类型、方法替换及 JS import 时的全局 ID 初始化顺序尚未全部覆盖；纯 shared_ptr 强环仍须由 C++ 显式断开。线段辅助对象的实际 GPU 绘制仍未实现。

examples/jsm/helpers 的 VertexNormalsHelper 与 VertexTangentsHelper 使用独立 addons 库，保留原版固定容量、法线/切线变换、三次数值转换、重入临时向量、继承 copy/clone 及 geometry→material dispose 顺序；支持普通、交错、Float16 和 normalized 属性。NativeObjectExtension 将 object/size 的活包装、真实 shared_ptr owning slot 与 JSValue root 接入核心 GC，核心不依赖 addons 类或构造器，也不修改 Object3D ABI。仅 parent 原生拥有 helper 时仍保留源包装的扩展字段；size/source/geometry/material 与包装之间的不可达交叉环可回收。Normals flag 支持 bool 写回，任意赋值经 ToBoolean，未保留该字段的任意 JS 类型；object 仍限于已支持的原生 Object3D。字段删除/描述符替换与全部动态方法仍需验收，两个对象未标记完整完成。

Cache、LoaderUtils、LoadingManager、Loader 和三类解析器已补入。LoadingManager 的 URL NFC 使用 Unicode 17.0 官方表、Hangul 算法和固定哈希生成器；许可与来源记在 upstream.json。Loader.loadAsync 使用原生 Promise，保留动态 load/getter、同步进度回调、成功/错误与 thenable 顺序；C++ 子类仅 override virtual load 尚不会自动替换管理对象上的 load 方法，需要补齐桥接。AnimationLoader、BufferGeometryLoader、MaterialLoader 的 load 明确报告待实现的 FileLoader 传输；parse 仍继承原生几何/材质的静态字段与 JSON 存储边界，不能视为完整 I/O 加载器。

JSPromise/JSJobs 实现同步 executor、异步 thenable/反应队列、then/catch/finally、resolve/reject/withResolvers/try 和任意拒绝值。内建构造入口在读取 newTarget.prototype 前执行自身参数验证，并保留各内建的原型回退；Promise 内部记录隔离可修改的 Object/Array 原型，withResolvers 使用自有数据属性。队列是所属线程的管理根，由 drainMicrotasks 显式执行检查点；未实现聚合方法、Symbol/species、跨 realm、未处理拒绝通知与自动宿主驱动。queueMicrotask 回调抛错后仍执行其余工作，最终重新抛出首个错误，尚无完整平台错误上报。

NativeEventLoop 提供所属线程的原生任务、计时器、取消、注册接收者和显式微任务检查点。工作线程只经 Sender 投递 NativeEventData，托管回调和捕获保留在所属线程；入口检查点属于原生宿主策略，每个任务结束另执行检查点。pending/wait 同时观察线程共享的微任务，等待不执行回调。异常按入口、任务、出口阶段返回；计时器零延迟为独立 FIFO 任务。尚未实现完整 DOM task source/WebIDL/嵌套及后台限时规则、网络 I/O 或自动驱动，Node 差分只验证正常零延迟调度。

取消/注销先移除待执行记录，再从容器退休托管根，避免原生析构回调重入时消费已取消任务或修改正在 erase 的容器；根释放后不再访问循环状态。销毁先关闭循环，保留状态直到回调/接收者根析构完成，所属线程 API 在关闭阶段显式拒绝重入。原生任务也不能在独立 drainMicrotasks 检查点内部执行；等待以唤醒版本检测读取注入时钟后、真正进入等待前的通知，避免旧时间导致多余等待。

JSWeakValue 为原生身份缓存提供不保留 heap/GC root 的弱句柄；lock 成功才产生正常强根，基础值（包含 undefined、负零）保持值语义。对象访问先验证单调线程身份，再访问弱 heap 和不复用的记录 ID；避免线程/内存地址复用误命中。它不等价于完整 JS WeakRef，也不提供 WeakMap 的 ephemeron、WeakRef 的任务保活或自动 GC 策略。

JSWeakMap 单独实现原生弱键表：键为不拥有对象的记录 ID，值为条件图边，只有 map 和 key 都可达才激活 value。每次标记按 key ID 唤醒等待值，与 JS/native 两类工作队列共同求固定点；原生析构后重新建立完整标记图，最终才剪除弱键。弱表首次 set 才分配，普通记录不创建哈希表；方法在读取或复制托管参数前验证线程/heap。管理构造器、原型方法、品牌检查与任意值存储已接入；Symbol 键、iterator 构造初始化、Symbol.toStringTag 和较新的便利方法仍待实现。WebGLProperties 使用该表保留动态方法查找、闭包重读、get/update/dispose 的转换与重入顺序，尚未接入 VulkanRenderer 全部资源缓存。

原生模块根通过 retainForHeapLifetime 保留当前 heap 内的对象与可达图，本身不增加 heap 的共享所有权。WeakMap 和 GLBufferAttribute 的构造器在同一 heap 的 GC 之间保持身份和已修改原型；仅含管理图边时，最后一个应用根释放后模块根随 heap 销毁。原生 payload 中的 owning JSValue 仍可能保留 heap，完整 native 图的运行时关闭策略尚待实现。弱缓存用于定位仍存在的模块对象，不替代模块存活规则。其余使用静态强句柄的模块尚需逐项迁移和验收。

Array.sort/unshift 已接入运行时：sort 保留 holes、undefined、UTF-16 默认比较、固定 V8 比较器调用顺序、长度快照及写回/删除的部分失败；unshift 按原方向逐项读取和移动，再写入参数及 length。比较器修改属性、原型或长度时保留动态行为。原生算法不执行 JS 源码；基础值的完整包装原型及 Symbol/Proxy 等边界仍未完成。

GLBufferAttribute 使用独立的管理对象表示 opaque GPU buffer，保留构造字段、任意值、共享身份、严格 true 更新和动态方法。它不继承 CPU BufferAttribute，也不虚构 array 存储。WebGLAttributes 保留 WeakMap 属性身份、交错 data 共享、首次上传类型/大小、版本比较、范围原地合并、回调及移除顺序；注入的 GL 调用接口由 C++ 管理。普通 BufferAttribute 的该上传算法已通过 VulkanBufferCommands 接入实际 VkBuffer。原生 Float16Array、Symbol.hasInstance、GLBufferAttribute 原生资源导入及完整交错绘制仍待完成。

BufferAttribute 原生 handle 与数据 Storage 分开拥有；唯一管理 wrapper 只强持 Storage 和弱持 handle。应用释放原生对象后 weak_ptr 立即失效；仍存活的 wrapper 可重建同 id、同数据、同管理身份及原 Typed/Float16/Instanced 类型的 handle。数组、name、itemSize/count/normalized 仍受现有 C++ 类型适配限制，任意动态替换与完整构造器/子类语义仍待补齐。原生 EventDispatcher/dispose 属于当前 C++ 扩展，监听器随原 handle 退役，不自动转移到重建 handle，也尚未与管理事件接口完整互通。std::function 的不透明 C++ 捕获不自动追踪，管理环需要显式 JSValue captures。

VulkanBufferCommands 只实现属性管理器需要的六个原生命令和两种 buffer target，不创建 OpenGL 上下文。真实 branded TypedArray 的视图偏移、长度和元素宽度用于上传；公开可遮蔽字段仍按上层原版读取。每次有效写立即提交 Vulkan memory，后续转换抛错不会回滚前次写。上层唯一 WeakMap 缓存负责类型/大小/version/ranges/callback；派生 float/uint32 绘制缓冲只按完成写入的 revision 转换，实际数据从 Vulkan memory 读回，保留已录制绘制的旧资源。当前仍存在额外原始上传和转换缓冲、CPU 读回及转换成本；直接绑定全部原生格式尚待实现。 上传回调删除当前缓存时，Vulkan 仍显式报错；上游缺失顶点缓存的跳过绑定行为尚待接入。原始 allocation 单列 attributeUploadBuffers，旧 attributeBuffers 继续统计派生绘制缓冲。设备销毁前 shutdown 退役全部原始存储；缓存退役前摘除 listener/迭代器，避免动态 getter 的 dispose 重入。

WebGLRenderList/WebGLRenderLists 保留 opaque/transmissive/transparent 数组身份、池化 RenderItem、push 的 reversedDepth、分类读取、稳定排序、finish 清引用、场景弱键与嵌套深度缓存。公开数组替换不改变已捕获数组；默认排序函数属于模块根，任意自有字段和原型修改跨同 heap 的 GC 保留。传统 webgl 模块返回的普通函数保留 call/new/newTarget；类方法和箭头比较器仍不可构造。当前实现 CPU 排序与缓存，完整 VulkanRenderer 投影、排序及 GPU 提交接入仍待完成。

原生 AbortController/AbortSignal 为加载器提供一次性取消、组合信号、监听器移除、取消算法和异常值保留；完整 DOM EventTarget、任意 iterable、平台任务调度及真实网络取消仍待接入。内部适配不执行 JavaScript 源码，不引入浏览器。

`utils/port-audit.mjs` 枚举锁定版本的全部 src 与 examples/jsm 模块，输出 `docs/port-status.json` 与 `docs/api-inventory.json`。已有文件只构成存在证据；API 清单来自语法树，动态/继承/运行时成员的分析边界显式记录。这些清单用于跟踪范围，不代替一致性验收。

## 实施顺序

1. 固定上游与依赖基准，建立核心、官方扩展、第三方依赖和 API 证据清单；建立保留类型、身份、键顺序及回调顺序的差分测试。
2. 实现库所需 JavaScript 语义基础，修正已有数学、核心、事件、场景、相机、几何、材质和拾取实现；原版接口优先。
3. 补齐 CPU 对象、曲线、几何体、纹理、灯光、动画、蒙皮、加载/导出与异步生命周期。
4. 接入原生窗口、输入、网络、图像、音频与平台服务；三平台分别验证。CSS/SVG/XR 等依赖宿主的功能逐项标记实现与验证情况，不通过浏览器代执行。
5. 保留传统 WebGLRenderer 上层逻辑，实现 Vulkan 身份缓存、version/updateRanges/needsUpdate、上传回调、dispose，然后补齐纹理、渲染目标、排序、状态、回调重入、材质、光照、阴影及后处理。
6. 分别保留现代 WebGPURenderer/common 的行为与默认值，复用 Vulkan 设备后端，补齐 nodes、TSL、节点材质、计算和存储资源。GLSL 经 glslang、WGSL 经 Tint 编译到 SPIR-V；VulkanRenderer 保留为传统兼容入口。
7. 完成官方扩展、构建安装、示例与文档；全部 API/行为与三平台证据通过后才标记完成。

当前正在实施第 1、2 阶段、第 3 阶段的曲线/几何体/纹理/灯光/材质/骨骼、线/点/Sprite、LOD、动画、辅助对象与加载基础，以及第 5 阶段的属性和纹理资源缓存、基础实例化、CPU 渲染目标与 uniform/交错缓冲资源。各阶段仍未完成，不能因本轮测试通过而改变全部模块的完成状态。

仍需持续补齐并验证已有 CPU 模块的动态类型/对象图边界、加载器、辅助对象、节点与官方扩展，以及已移植灯光/材质/线/点/Sprite/实例化/蒙皮对象的完整 GPU 路径。

Vulkan 2D DataTexture 已接入 RGBA/RG/Red × UnsignedByte/Float/HalfFloat，支持 MeshBasicMaterial.map/alphaMap、uv0..3、矩阵、wrap/filter/flipY/sRGB/预乘和自动/手工 mipmap。关闭 mipmapsAutoUpdate 时保留已分配各级；局部级别及矩形更新通过 GPU 复制保留其余旧内容。Texture/Source 依身份与版本缓存，共享 source 的同 draw 双绑定更新可见；上传回调前快照材质 uniform。每次上传创建不可变 image/sampler 并同步提交，旧绘制持有旧资源至 fence，性能尚需优化。纹理 updateRanges 目前限制为 RGBA 单行；普通宿主图片、压缩/数组/3D/深度纹理、显式纹理复制/生成接口及其他材质贴图仍未接入。

Vulkan 仍需窗口 Surface/Swapchain 与呈现、其余 GPU 资源生命周期、完整光照/PBR、阴影、裁剪与模板、自定义着色器、渲染目标、后处理、完整实例属性支持、蒙皮、计算管线与同步管理。当前离屏渲染覆盖基础网格、基础实例化、属性缓存及上述 DataTexture 路径。

## 验证

CPU 单元测试通过库目标链接实际实现。Node 差分测试直接执行固定 checkout 中的 three.js，不依赖手写预期值。GPU 测试调用真正 Vulkan 图形管线并读回像素；CPU 差分测试通过不能证明全部渲染等价。Validation Layers 需要运行时可用；必须单独记录是否真正启用。

```powershell
node utils/generate-constants.mjs <three.js目录>
node utils/port-audit.mjs <three.js目录>
cmake -S . -B build/native -DTHREEJS_SOURCE_DIR=<three.js目录>
cmake --build build/native --config Debug --parallel
ctest --test-dir build/native -C Debug --output-on-failure
```

源代码和构建配置不保存机器的绝对工具路径。Vulkan SDK 未安装到默认目录时显式传入 THREECPP_VULKAN_INCLUDE_DIR、THREECPP_GLSLANG_VALIDATOR。
