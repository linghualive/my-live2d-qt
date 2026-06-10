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

### 添加模型

将 Live2D 模型文件夹放到 `Resources/` 目录下（与 `Mao`、`Wanko` 等同级），重新构建即可自动打包。运行后通过托盘菜单切换模型。

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
