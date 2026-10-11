# 本轮验证

基准：three.js r187dev，commit `11738a9d55e502fb484cefffcea3a66dc44e9fe9`。Windows x64，MSVC 19.51，CMake 4.3.1，Node.js 26.5.0；Vulkan 设备为 NVIDIA GeForce RTX 3070 Laptop GPU。

Debug 和 Release 均成功构建，均通过全部 7 项 CTest。

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

离屏示例实际生成 512×512 图像：7 次 draw、14 个三角形，PNG 已目视检查。文件在 `build/parity/vulkan-basic.ppm` 和 `build/parity/vulkan-basic.png`，生成目录不纳入版本控制。

```powershell
cmake --build build/parity --config Debug --parallel 6
ctest --test-dir build/parity -C Debug --output-on-failure
cmake --build build/parity --config Release --parallel 6
ctest --test-dir build/parity -C Release --output-on-failure
./build/parity/bin/Debug/vulkan_basic.exe ./build/parity/vulkan-basic.ppm
```

完整范围仍未完成。清单枚举 755 个 upstream 模块：41 个已有移植入口、424 个尚缺源码、290 个 renderer 模块等待对应 Vulkan 实现。文件存在和本轮测试通过都不代表模块已完整移植；详见 `PORTING.md` 与 `port-status.json`。
