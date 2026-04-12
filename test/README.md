# three.cpp 测试框架

本项目使用 [Catch2](https://github.com/catchorg/Catch2) 单元测试框架。

## 目录结构

```
test/
├── CMakeLists.txt          # 测试构建配置
├── catch.hpp               # Catch2 头文件
├── TestMain.cpp            # 测试入口
└── math/
    ├── Vector2.tests.cpp
    ├── Vector3.tests.cpp
    ├── Vector4.tests.cpp
    ├── MathUtils.tests.cpp
    ├── Matrix3.tests.cpp
    ├── Matrix4.tests.cpp
    ├── Quaternion.tests.cpp
    ├── Euler.tests.cpp
    └── Color.tests.cpp
```

## 运行测试

### 使用 CMake（推荐）

```bash
cd <项目目录>
cmake -B build
cmake --build build
./build/tests/THREECPPTests    # Linux/macOS
.\build\tests\THREECPPTests.exe  # Windows
```

### 使用 g++ 直接编译

```bash
cd <项目目录>
g++ -std=c++20 -I include -I test test/TestMain.cpp test/math/*.cpp src/math/*.cpp -o test/THREECPPTests
./test/THREECPPTests
```

## 测试命令

```bash
# 运行所有测试
./THREECPPTests

# 运行特定测试
./THREECPPTests "[Vector2]"
./THREECPPTests "[Vector3][MathUtils]"

# 显示详细输出
./THREECPPTests -s

# 生成 XML 报告
./THREECPPTests -r junit -o test-report.xml
```

## 添加新测试

1. 在 `test/math/` 下创建 `ClassName.tests.cpp`
2. 参考现有测试文件格式
3. 在 `CMakeLists.txt` 中添加源文件
