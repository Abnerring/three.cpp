# three.cpp 安装与编译指南

## 环境要求

- C++20 兼容的编译器
- CMake 3.10+（可选）

## Windows

### 安装 MSYS2

使用 winget 安装 MSYS2：

```powershell
winget install MSYS2.MSYS2
```

### 安装编译工具

打开 **MSYS2 MinGW 64-bit** 终端，运行：

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
```

### 编译

```powershell
cd <项目目录>
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
g++ -std=c++20 -I include src/main.cpp src/math/*.cpp -o build/THREECPP.exe
```

或使用 CMake：

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

## Linux

### 安装编译工具

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake

# Fedora
sudo dnf install gcc-c++ cmake

# Arch Linux
sudo pacman -S gcc cmake
```

### 编译

```bash
cd <项目目录>
g++ -std=c++20 -I include src/main.cpp src/math/*.cpp -o build/THREECPP
```

或使用 CMake：

```bash
cmake -B build
cmake --build build
```

## macOS

### 安装编译工具

```bash
# 使用 Homebrew
brew install cmake

# 或使用 Xcode 命令行工具
xcode-select --install
```

### 编译

```bash
cd <项目目录>
g++ -std=c++20 -I include src/main.cpp src/math/*.cpp -o build/THREECPP
```

或使用 CMake：

```bash
cmake -B build
cmake --build build
```

## 运行

```bash
# Windows
./build/THREECPP.exe

# Linux/macOS
./build/THREECPP
```

## 预期输出

```
CREAT UUID 0 :xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
seededRandom 0 :0.xxxxxx
random 0 :xx
v1 + v2 = (4, 6)
v1 / v2 = (0.333333, 0.5)
Hello, THREE.CPP!
```

## CMake 构建（推荐）

CMake 可自动处理跨平台编译：

```bash
# 创建构建目录
cmake -B build

# 编译
cmake --build build

# 运行
./build/THREECPP          # Linux/macOS
./build/THREECPP.exe      # Windows
```

## 常见问题

### g++ 找不到

**Windows**: 确保 MSYS2 的 `mingw64/bin` 在 PATH 中：

```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
```

**Linux**: 检查是否安装 `build-essential` 或 `gcc-c++`。

### CMake 生成失败

- 确保 CMake 版本 >= 3.10
- 删除 `build/` 目录后重新生成
- 指定生成器：`cmake -B build -G "Unix Makefiles"`

### C++20 特性不支持

确保编译器支持 C++20：
- GCC 10+
- Clang 10+
- MSVC 2019 16.6+
