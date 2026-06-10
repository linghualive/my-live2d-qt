# QDesktopPet

基于 Qt + Live2D Cubism SDK 的跨平台桌面宠物，支持 macOS、Linux (X11/Wayland)。

通过 OpenGL 渲染 Live2D 模型，窗口背景完全透明，宠物可拖动、跟随鼠标交互。通过系统托盘管理模型切换、设置和退出。

## 功能

- Live2D Cubism SDK 5 原生 OpenGL 渲染（Moc3 格式模型）
- 透明无边框窗口，真正的桌面宠物体验
- 鼠标跟随交互 + 拖动定位
- 系统托盘菜单：模型切换、设置、显示/隐藏
- 可配置：窗口大小、位置、鼠标灵敏度、悬停隐藏
- macOS 开机自启（LaunchAgent）
- macOS 打包为 .app 应用包

## 依赖

| 依赖 | 说明 |
|------|------|
| Qt 6 (推荐) 或 Qt 5 | Core, Gui, Widgets, OpenGLWidgets |
| OpenGL | 系统自带 |
| CMake ≥ 3.16 | 构建系统 |
| Live2D Cubism Core SDK | 已包含在 `third_party/CubismCore/` |
| Live2D Cubism Native Framework | 已包含在 `third_party/CubismNativeFramework/` |

### Linux 额外依赖

**Arch Linux / Manjaro：**
```bash
sudo pacman -S base-devel cmake qt6-base mesa libx11 libxext libxtst libxkbcommon
# Wayland 支持（可选）：
sudo pacman -S wayland pkg-config
```

**Debian / Ubuntu：**
```bash
sudo apt install build-essential cmake \
    qt6-base-dev libqt6opengl6-dev qt6-base-private-dev \
    libgl1-mesa-dev libx11-dev libxext-dev libxtst-dev libxkbcommon-dev
# Wayland 支持（可选）：
sudo apt install libwayland-dev pkg-config
```

**Fedora：**
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel mesa-libGL-devel \
    libX11-devel libXext-devel libXtst-devel libxkbcommon-devel
# Wayland 支持（可选）：
sudo dnf install wayland-devel pkg-config
```

> Linux 上 X11 和 Wayland 支持会自动检测，可以同时编译。

## 构建

### macOS

```bash
cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)

# 生成 QDesktopPet.app，可直接双击运行
open build/QDesktopPet.app

# 可选：打包 Qt 框架到 .app 中（用于分发）
cmake --build build --target deploy

# 可选：安装到 Applications
cp -R build/QDesktopPet.app ~/Applications/
```

### Linux

```bash
cmake -B build
cmake --build build -j$(nproc)

# 运行
./build/QDesktopPet
```

CMake 会自动检测 X11 和 Wayland 库，输出类似：
```
-- Platform detection:
--   macOS:   OFF
--   X11:     ON
--   Wayland: ON
```

如果 X11 检测结果为 OFF，请确认 `libxtst` 已安装（X11 RECORD 扩展用于鼠标追踪）。

### Windows

```bash
# 需要 Visual Studio 2019+ 或 MinGW，以及 Qt 6
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 添加模型

将 Live2D 模型文件夹放到 `Resources/` 目录下（与 `Mao`、`Wanko` 等同级），重新构建即可自动打包。运行后通过托盘菜单切换模型。

也可以在运行时通过托盘菜单 → Preferences → Models → Import 导入模型，模型会复制到 `~/.qdesktoppet/models/`。

## 开发调试

### Arch Linux 开发环境搭建

一键安装所有开发依赖：

```bash
sudo pacman -S base-devel cmake qt6-base mesa libx11 libxext libxtst libxkbcommon gdb
# 可选：Wayland 支持
sudo pacman -S wayland pkg-config
# 可选：IDE
sudo pacman -S qt6-tools    # Qt Creator / Designer
```

Debug 构建 + 运行：

```bash
# 克隆
git clone https://github.com/lsk-china/my-live2d-qt.git
cd my-live2d-qt

# Debug 构建（带调试符号，不优化）
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# 运行（控制台输出 Qt 调试信息）
QT_LOGGING_RULES="*.debug=true" ./build/QDesktopPet

# GDB 调试
gdb ./build/QDesktopPet

# 修改代码后增量编译，通常只需几秒
cmake --build build -j$(nproc)
```

### 使用 IDE

**Qt Creator：**
直接打开项目根目录的 `CMakeLists.txt`，Qt Creator 会自动配置构建、运行和调试。

**VS Code：**
安装 `CMake Tools` 和 `C/C++` 扩展，打开项目文件夹后选择 Debug 构建变体即可。按 F5 启动调试。

**CLion：**
直接打开项目文件夹，CLion 自动识别 CMake 项目。

### 排查问题

运行不正常时，先跑诊断命令：

```bash
./build/QDesktopPet --diagnose
```

会输出类似：

```
=== QDesktopPet Diagnostics ===

Qt:
  Version:             6.9.0
  Platform:            xcb
  Compiled with:       Qt 6.9.0

OpenGL:
  [OK] Context:           4.6 (Core Profile) Mesa 24.1.2
  Renderer:            AMD Radeon RX 580
  Vendor:              AMD

Display:
  [OK] X11:               running on xcb
  [OK] RECORD ext:        v1.13

Data:
  [OK] Data dir:          /home/user/.qdesktoppet/
  [OK] Models dir:        /home/user/.qdesktoppet/models/ (2 models)
  [OK] Config:            /home/user/.qdesktoppet/config.ini

Build:
  HAS_X11:             yes
  HAS_WAYLAND:         no
```

标记为 `[!!]` 的项就是问题所在。常见问题：

| 诊断结果 | 解决方法 |
|----------|---------|
| `[!!] Context: FAILED` | 安装 GPU 驱动：`sudo pacman -S mesa` |
| `[!!] RECORD ext: NOT available` | 安装 libxtst：`sudo pacman -S libxtst` |
| `[!!] Models dir: (0 models)` | 把 Live2D 模型文件夹放到 `~/.qdesktoppet/models/` |
| `HAS_X11: no` | 重新安装 X11 开发库并重新 cmake |

### 其他调试技巧

```bash
# 查看编译时平台检测结果
cmake -B build 2>&1 | grep "Platform detection" -A 5

# 强制使用 X11（在 Wayland 桌面环境下）
QT_QPA_PLATFORM=xcb ./build/QDesktopPet

# 强制使用 Wayland
QT_QPA_PLATFORM=wayland ./build/QDesktopPet

# 查看 OpenGL 信息
QSG_INFO=1 ./build/QDesktopPet

# 清理重建
rm -rf build && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)
```

### 数据目录

运行时数据存储在 `~/.qdesktoppet/`：

| 文件/目录 | 说明 |
|-----------|------|
| `config.ini` | 用户配置（窗口大小、模型选择等） |
| `models/` | 用户导入的模型 |
| `QDesktopPet.lock` | 单实例锁文件 |
| `model_blacklist.txt` | 不兼容模型列表 |

## 项目结构

```
src/
  core/          配置、模型管理、进程锁
  ui/            PetWindow、托盘、设置对话框
  platform/      平台抽象层（鼠标追踪、输入穿透、自启动）
    macos/       macOS 实现（Cocoa）
    x11/         X11 实现
    wayland/     Wayland 实现
third_party/
  QtLive2d/      Live2D 渲染 Widget
  CubismCore/    Live2D Cubism Core SDK（预编译库）
  CubismNativeFramework/  Live2D 框架源码
Resources/       内置模型资源
```

## 借物表

- 万叶模型来自 [Bilibili](https://www.bilibili.com/video/BV1xq4y1k7QR)
- Live2D Cubism SDK &copy; Live2D Inc.
- 原始项目灵感来自 [QtLive2d](https://github.com/lsk-china/QtLive2d)
