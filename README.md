# SnapPaste

SnapPaste 是一个 Windows 平台的轻量级截图工具，使用 Qt5 Widgets 和 C++17 开发。项目目标是提供快速截图、复制、保存、贴图、简单标注和截图历史管理等能力。

## 功能概览

- 区域截图：通过托盘菜单或全局热键进入截图选区。
- 快速动作：选区完成后可复制、贴图、保存、编辑或 OCR。
- 贴图窗口：支持从截图或剪贴板创建置顶贴图，并可移动、缩放、旋转、翻转、调整透明度、点击穿透、复制和保存。
- 标注编辑：支持基础标注工具，包括画笔、箭头、矩形、马赛克和文字。
- 截图历史：保存截图记录和缩略图，便于回看已保存内容。
- 设置管理：支持保存目录、图片格式、主题模式和热键配置。
- 深浅色主题：可跟随系统，也可切换为浅色或深色主题。
- Windows OCR：在可用的 Windows SDK / WinRT OCR 环境下，可识别选区文字并复制到剪贴板。

## 默认快捷键

| 操作 | 快捷键 |
| --- | --- |
| 开始截图 | `F1` |
| 从剪贴板或最近截图贴图 | `F3` |
| 隐藏所有贴图 | `Ctrl+Alt+H` |
| 选区复制 | `Enter` |
| 选区贴图 | `F3` |
| 选区保存 | `Ctrl+S` |
| 选区编辑 | `Space` |
| 选区 OCR | `O` |
| 取消截图 | `Esc` |

## 环境要求

- Windows
- CMake 3.16 或更高版本
- 支持 C++17 的 MSVC 工具链
- Qt 5，需包含 `Widgets` 和 `Sql` 模块
- Windows SDK，项目会链接 `user32`、`gdi32`、`shell32`、`shcore`、`dwmapi`、`ole32`、`oleaut32`、`uiautomationcore`、`d3d11`、`dxgi`

## 构建

```powershell
cmake -B build
cmake --build build
```

启用单元测试：

```powershell
cmake -B build -DSNAPPASTE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -V
```

运行单个 QtTest 测试函数：

```powershell
.\build\tests\SnapPasteUnitTests.exe captureSelectionHistoryKeepsRecentUniqueRegions
```

## 项目结构

```text
src/
  app/                    应用入口、启动流程和依赖组装
  domain/                 领域接口与核心业务逻辑
  infrastructure/         配置、文件、SQLite、图片存储、剪贴板等基础设施
  platform/windows/       Windows 平台实现，包括热键、截图、DPI、窗口交互
  presentation/           Qt Widgets UI、ViewModel、托盘、截图浮层、贴图窗口、编辑器
  shared/                 通用类型、事件总线、Result 类型和工具
tests/
  unit/                   QtTest 单元测试
resources/
  icons/                  图标资源
  themes/                 QSS 主题
  qrc/                    Qt 资源文件
```

## 架构说明

项目采用分层结构：

```text
presentation -> domain -> infrastructure -> platform/windows
```

主要约定：

- 组合根在 `src/app/AppContext.*`，负责创建并连接应用依赖。
- 入口为 `src/app/main.cpp`，启动前会配置 Windows 高 DPI。
- 截图流程由 `CaptureOverlay` 选择区域，经 `CaptureViewModel` 异步调用截图服务完成。
- 领域服务通过接口隔离平台和基础设施实现。
- 跨 ViewModel 事件通过 `EventHub` 分发。
- 领域服务统一使用 `Result<T>` 表达成功或失败。

## 数据与配置

SnapPaste 会在用户本地应用数据目录下保存配置、数据库、截图文件和缩略图。配置由 `JsonSettingsRepository` 管理，历史记录和贴图状态由 SQLite 仓储管理。

## 测试

测试集中在 `tests/unit/SnapPasteTests.cpp`，使用 QtTest。测试中包含内存测试替身，也包含少量基于临时目录和 SQLite 的轻量集成测试。

## 当前状态

项目版本为 `0.1.0`。当前实现面向 Windows，不包含跨平台抽象层；`platform/windows/` 是唯一平台实现目录。
