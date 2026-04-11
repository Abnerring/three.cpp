# three.cpp
C++ Three.js Alternative with Vulkan

## 项目目标
将 three.js 移植为 C++ 版本，使用 Vulkan 作为渲染后端，保持与 three.js 相同的文件结构和 API 设计。

## 目录结构（与 three.js 一致）
```
three.cpp/
├── include/          # 头文件
│   ├── common/        # 公共基础
│   ├── core/          # 核心类
│   ├── math/          # 数学库
│   ├── cameras/       # 相机
│   ├── geometries/    # 几何体
│   ├── materials/     # 材质
│   ├── lights/        # 光源
│   ├── textures/      # 纹理
│   ├── renderers/     # 渲染器 (Vulkan)
│   ├── objects/       # 3D对象
│   └── loaders/       # 加载器
├── src/              # 源文件
├── shaders/          # GLSL/Vulkan SPIR-V 着色器
├── CMakeLists.txt    # CMake 构建配置
└── examples/         # 示例代码
```

---

## 分阶段任务计划

### 阶段一：基础设施

| 任务 | 描述 | 优先级 |
|------|------|--------|
| 1.1 项目框架 | CMake 配置、命名空间规划、基础头文件 | ⭐⭐⭐ |
| 1.2 Vulkan 初始化 | Instance、Device、Queue、Surface 创建 | ⭐⭐⭐ |
| 1.3 内存管理 | Vulkan Memory Allocator (VMA) 集成 | ⭐⭐⭐ |
| 1.4 命令系统 | Command Pool、Command Buffer 管理 | ⭐⭐⭐ |
| 1.5 同步机制 | Semaphore、Fence、Timeline Semaphore | ⭐⭐ |

### 阶段二：数学库

| 任务 | 状态 | 描述 |
|------|------|------|
| 2.1 Vector2 | ✅ 已完成 | 二维向量 |
| 2.2 Vector3 | ✅ 已完成 | 三维向量 |
| 2.4 Vector4 | ✅ 已完成 | 四维向量 |
| 2.5 Matrix3 | ✅ 已完成 | 3x3 矩阵 |
| 2.6 Matrix4 | ✅ 已完成 | 4x4 矩阵 |
| 2.7 Quaternion | ✅ 已完成 | 四元数 |
| 2.8 Euler | ✅ 已完成 | 欧拉角 |
| 2.9 Color | ✅ 已完成 | 颜色类 |
| 2.10 Plane/Frustum/Sphere/Box | 🔲 待完成 | 几何数学 |
| 2.11 Ray | 🔲 待完成 | 射线 |

### 阶段三：核心系统

| 任务 | 描述 | 优先级 |
|------|------|--------|
| 3.1 Object3D | 3D对象基类（变换、层级关系） | ⭐⭐⭐ |
| 3.2 Scene | 场景管理 | ⭐⭐⭐ |
| 3.3 Camera | 相机基类 + Perspective/Orthographic | ⭐⭐⭐ |
| 3.4 BufferGeometry | 几何体顶点数据管理 | ⭐⭐⭐ |
| 3.5 BufferAttribute | 顶点属性 | ⭐⭐⭐ |
| 3.6 Material | 材质基类 | ⭐⭐⭐ |
| 3.7 EventDispatcher | 事件系统 | ⭐⭐ |

### 阶段四：Vulkan 渲染管线

| 任务 | 描述 | 优先级 |
|------|------|--------|
| 4.1 Pipeline | Graphics Pipeline / Compute Pipeline | ⭐⭐⭐ |
| 4.2 Shader Module | SPIR-V 编译加载 | ⭐⭐⭐ |
| 4.3 Render Pass | Framebuffer、Render Pass | ⭐⭐⭐ |
| 4.4 Descriptor Sets | 资源绑定（UBO、Sampler） | ⭐⭐⭐ |
| 4.5 Swapchain | 交换链管理 | ⭐⭐⭐ |
| 4.6 Vertex Input | 顶点缓冲、索引缓冲 | ⭐⭐⭐ |
| 4.7 Uniform Buffers | 矩阵 Uniform 上传 | ⭐⭐⭐ |

### 阶段五：几何体与材质

| 任务 | 描述 |
|------|------|
| 5.1 BoxGeometry | 立方体 |
| 5.2 SphereGeometry | 球体 |
| 5.3 PlaneGeometry | 平面 |
| 5.4 CylinderGeometry | 圆柱体 |
| 5.5 MeshBasicMaterial | 基础材质 |
| 5.6 MeshStandardMaterial | PBR 标准材质 |
| 5.7 ShaderMaterial | 自定义着色器材质 |

### 阶段六：纹理与光照

| 任务 | 描述 |
|------|------|
| 6.1 Texture | 2D 纹理 |
| 6.2 CubeTexture | 立方体纹理 |
| 6.3 Sampler | 采样器配置 |
| 6.4 AmbientLight | 环境光 |
| 6.5 DirectionalLight | 方向光 |
| 6.6 PointLight | 点光源 |
| 6.7 SpotLight | 聚光灯 |

### 阶段七：高级特性

| 任务 | 描述 |
|------|------|
| 7.1 阴影映射 | Shadow Map |
| 7.2 后期处理 | Post Processing |
| 7.3 渲染到纹理 | Render Target |
| 7.4 帧率控制 | 帧同步 |
| 7.5 调试工具 | Vulkan Validation Layers |

---

## Vulkan 渲染器架构设计

```
VulkanRenderer
├── VulkanInstance          # Vulkan 实例
├── VulkanDevice            # 逻辑设备
├── VulkanPhysicalDevice    # 物理设备
├── VulkanSwapchain         # 交换链
├── VulkanPipelineCache     # Pipeline 缓存
├── VulkanRenderPass       # 渲染通道
├── VulkanDescriptorPool   # 描述符池
├── VulkanCommandPool     # 命令池
├── TextureManager         # 纹理管理
├── GeometryManager        # 几何体管理
└── RenderQueue            # 渲染队列
```

---

## 文件对应关系（three.js → three.cpp）

| three.js | three.cpp |
|----------|-----------|
| `src/core/Object3D.js` | `include/core/Object3D.h` |
| `src/scenes/Scene.js` | `include/core/Scene.h` |
| `src/cameras/Camera.js` | `include/cameras/Camera.h` |
| `src/cameras/PerspectiveCamera.js` | `include/cameras/PerspectiveCamera.h` |
| `src/core/BufferGeometry.js` | `include/core/BufferGeometry.h` |
| `src/materials/Material.js` | `include/materials/Material.h` |
| `src/objects/Mesh.js` | `include/objects/Mesh.h` |
| `src/renderers/WebGLRenderer.js` | `include/renderers/VulkanRenderer.h` |

---

## 建议开发顺序

```
1. 完善数学库 (Vector4, Quaternion, Euler, Color)
        ↓
2. 实现核心类 (Object3D, Scene, Camera)
        ↓
3. 搭建 Vulkan 基础框架
        ↓
4. 实现基本渲染流程 (三角形→屏幕)
        ↓
5. 添加几何体和材质
        ↓
6. 实现光照系统
        ↓
7. 添加高级特性
```

---

## 文档注释规范

本项目使用 **Doxygen** 风格注释。

### 文件头注释

```cpp
/**
 * @file   Vector3.h
 * @brief  三维向量类
 * @author Abnerring
 * @date   2025-2026
 * @note   参考 three.js Vector3 实现
 */
```

### 类注释

```cpp
/**
 * @class Vector3
 * @brief 三维向量，用于表示空间中的点和方向
 * @details 支持向量运算、插值、几何计算等功能
 * @see Vector2 Vector4
 */
class Vector3 {
```

### 方法注释

```cpp
/**
 * @brief 设置向量分量
 * @param x X 分量值
 * @param y Y 分量值
 * @param z Z 分量值
 * @return Vector3& 返回自身引用，支持链式调用
 * @note 不会验证数值有效性
 *
 * @example
 * @code
 * Vector3 v;
 * v.set(1, 2, 3);
 * @endcode
 */
Vector3 &set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z);

/**
 * @brief 向量加法
 * @param v 要加的向量
 * @return Vector3& 计算后的新向量（不修改原向量）
 *
 * @warning 返回新向量，不修改原向量
 */
Vector3 operator+(const Vector3 &v) const;

/**
 * @brief 计算点积
 * @param v 另一个向量
 * @return HIGH_PRECISION 点积结果
 * @retval 0 两向量垂直
 * @retval >0 两向量夹角小于 90°
 * @retval <0 两向量夹角大于 90°
 */
HIGH_PRECISION dot(const Vector3 &v) const;

/**
 * @brief 归一化向量
 * @return Vector3& 归一化后的向量
 * @throws std::runtime_error 如果向量长度为 0
 */
Vector3 &normalize();

/**
 * @brief 线性插值
 * @param v1 起始向量
 * @param v2 结束向量
 * @param alpha 插值因子 [0, 1]
 * @return Vector3& 插值结果
 * @note alpha=0 返回 v1，alpha=1 返回 v2
 */
Vector3 &lerp(const Vector3 &v1, const Vector3 &v2, HIGH_PRECISION alpha);
```

### 常用标签速查

| 标签 | 用途 | 示例 |
|------|------|------|
| `@brief` | 简短描述 | `@brief 三维向量类` |
| `@details` | 详细描述 | `@details 包含所有基础向量运算` |
| `@param` | 参数说明 | `@param x X坐标` |
| `@return` | 返回值说明 | `@return 归一化后的向量` |
| `@retval` | 返回值枚举 | `@retval true 成功` |
| `@throws` | 异常说明 | `@throws std::runtime_error` |
| `@note` | 备注 | `@note 线程不安全` |
| `@warning` | 警告 | `@warning 返回新对象` |
| `@see` | 参见 | `@see Vector2` |
| `@example` | 示例 | `@example` + `@code` 块 |
| `@tparam` | 模板参数 | `@tparam T 类型` |
| `@since` | 起始版本 | `@since v1.0` |
| `@deprecated` | 废弃说明 | `@deprecated 使用 xxx 代替` |
| `@ingroup` | 所属组 | `@ingroup math` |

