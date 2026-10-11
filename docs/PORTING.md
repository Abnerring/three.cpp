# three.js → three.cpp 移植约束

目标是完整复现 three.js 逻辑，渲染后端替换为 Vulkan。首个能运行的示例不代表目标完成。

参考版本记录在 `upstream.json`：three.js r187dev，commit `11738a9d55e502fb484cefffcea3a66dc44e9fe9`。参考源码改变时必须显式更新基准、差分结果和版本记录。

## 一致性要求

- 文件按 upstream `src` 分类；`include` 放声明，`src` 放实现。
- 保持类名、方法名、默认值、返回自身语义、算法、计算顺序和边界行为。
- JavaScript Number 对应 IEEE 754 binary64；Float32Array 和整数 TypedArray 保留独立存储类型与转换规则。
- 矩阵保持列主序、右手坐标和相机负 Z 朝向。Vulkan 的 Y 轴及深度范围转换只发生在渲染后端。
- Object3D 的 rotation/quaternion 双向同步、矩阵更新标记、父子更新顺序必须对照源码。
- copy/clone 区分值复制与共享几何、材质、纹理等资源引用；序列化资源按 UUID 去重。
- 材质颜色使用 three.js 的工作颜色空间；输出转换、混合、雾与透明排序按源码验证。
- 未实现的渲染功能必须显式报错，不能接受属性后静默输出错误结果。
- 每项移植必须对照 upstream 单元测试，并补充实际运行 three.js 与 C++ 的差分验证；存在头文件不能视为完成。

## C++ 适配

当前代码保留 `x()`、`setX()` 等既有访问形式；数值语义以 three.js 为准。`Object3D::children` 用 shared_ptr 持有子节点，parent 为非拥有指针。对象不允许直接复制，以 copy/clone 保持身份、监听器和旋转回调的有效性。需要更新版本的属性使用 C++ 赋值代理。

当前 `JsonValue` 覆盖 JSON 数据，不支持 JavaScript 的函数、undefined、循环引用或任意动态原型。堆上共享对象的 `toNonIndexed()` 无索引分支返回自身；栈对象使用此分支会遇到 C++ shared_from_this 的所有权约束。C++ 关键字 `static` 属性对应 `isStatic`，JSON 仍为 `static`。

TypedArray 数值读取越界时用 NaN 表示 JavaScript undefined 参与数值运算的结果；越界单项写入忽略，跨 itemSize 的组件读取按原数组地址计算。count、索引和 drawRange 当前使用 C++ 非负整数，不能表示任意 JavaScript 浮点下标、负下标、非整除 count 或 undefined 本身。网格拾取要求有效非负整数索引。数学构造参数采用值语义，不保留传入向量的 JavaScript 引用身份。

渲染项持有对象、材质、几何和原 Group 的共享引用，回调移除对象或清空分组后当前帧引用仍有效。对象回调按 before、实际命令录制、after 执行；回调中的 clear 与 readPixels 支持当前绘制状态。活动渲染回调中递归调用 render 或 setSize 目前显式报错，完整渲染状态栈尚未移植。

CPU 库禁止浮点融合与重排，以保留 JavaScript binary64 的逐步求值顺序。数值差分仍使用明确容差，不能声称不同数学运行库或 GPU 着色器输出逐位相同。

## 完整工作范围

`tools/port-audit.mjs` 枚举锁定版本的全部 src 模块，输出 `docs/port-status.json`。它把已有文件标为 partial-unverified，尚未写入的文件标为 missing，renderer 文件标为 vulkan-port-pending。这些标记用于跟踪范围，不代替验收。

仍需持续移植和验证数学与几何查询、其余几何体、纹理、加载器、灯光、全部材质、线与点、实例化、蒙皮、动画、曲线、辅助对象、节点与扩展接口，以及对应运行时能力。

Vulkan 仍需窗口 Surface/Swapchain 与呈现、GPU 资源缓存和 needsUpdate 更新、纹理与描述符、完整光照/PBR、阴影、裁剪与模板、自定义着色器、渲染目标、后处理、实例化、蒙皮、计算管线与同步管理。当前离屏渲染只覆盖基础网格，是完整 Vulkan 实现的第一段。

## 验证

CPU 单元测试通过库目标链接实际实现。Node 差分测试直接执行固定 checkout 中的 three.js，不依赖手写预期值。GPU 测试调用真正 Vulkan 图形管线并读回像素；CPU 差分测试通过不能证明全部渲染等价。Validation Layers 需要运行时可用；必须单独记录是否真正启用。

```powershell
node tools/generate-constants.mjs <three.js目录>
node tools/port-audit.mjs <three.js目录>
cmake -S . -B build/native -DTHREEJS_SOURCE_DIR=<three.js目录>
cmake --build build/native --config Debug --parallel
ctest --test-dir build/native -C Debug --output-on-failure
```

源代码和构建配置不保存机器的绝对工具路径。Vulkan SDK 未安装到默认目录时显式传入 THREECPP_VULKAN_INCLUDE_DIR、THREECPP_GLSLANG_VALIDATOR。
