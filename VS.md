# Visual Studio 开发 three.cpp

## 打开项目

1. 打开 Visual Studio 2022
2. 文件 → 打开 → 文件夹
3. 选择项目根目录

VS 会自动识别 `CMakeLists.txt` 并配置 IntelliSense。

## 选择配置

右下角选择：
- **配置**: `x64-Debug` 或 `x64-Release`
- **解决方案平台**: `x64`

## 编译运行

按 **F5** 或点击"本地 Windows 调试器"按钮。

## 添加新文件

### 1. 创建源文件

在 `src/math/` 下新建 `.cpp` 文件，例如 `MyClass.cpp`。

### 2. 添加到 CMakeLists.txt

打开 `CMakeLists.txt`，在 `add_executable` 中添加：

```cmake
add_executable(THREECPP 
    src/main.cpp
    src/math/Vector2.cpp
    src/math/Vector3.cpp
    src/math/Vector4.cpp
    src/math/MathUtils.cpp
    src/math/Matrix3.cpp
    src/math/Matrix4.cpp
    src/math/Quaternion.cpp
    src/math/Euler.cpp
    src/math/Color.cpp
    # 新增文件
    src/math/MyClass.cpp
)
```

保存后 VS 会自动重新配置 CMake。

## 调试

- **断点**: 点击行号左侧
- **监视**: 右键变量 → 监视
- **调用堆栈**: 调试 → 窗口 → 调用堆栈

## CMake 设置

VS 通过 CMakeLists.txt 管理项目，可修改以下配置：

```cmake
set(CMAKE_CXX_STANDARD 20)           # C++ 标准
set(CMAKE_BUILD_TYPE Debug)           # 构建类型: Debug/Release
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY)  # 输出目录
```

## 常见问题

### 找不到头文件

确保 `include_directories(include)` 在 CMakeLists.txt 中，且 `#include` 路径正确：

```cpp
#include "math/Vector3.h"  // 正确
#include "../include/math/Vector3.h"  // 不需要
```

### IntelliSense 报错但能编译

尝试重新加载 CMake：右键 `CMakeLists.txt` → CMake 项目设置 → 重新加载。

### 清理缓存

删除 `build/` 目录，VS 会重新生成。

### 使用 MSVC 编译器

VS 默认使用 MSVC，无需额外配置。若要切换到 Clang：
- 工具 → 获取工具和功能 → 修改 → 勾选 "C++ Clang 生成器"
