# SnapPaste

SnapPaste 是一个 Windows 平台的轻量级截图工具，使用 Qt5 Widgets 和 C++17 开发。提供快速截图、标注编辑、贴图、OCR 文字识别和截图历史管理等能力。

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

### 全局热键

| 操作 | 快捷键 |
| --- | --- |
| 开始截图 | `F1` |
| 从剪贴板或最近截图贴图 | `F3` |
| 隐藏所有贴图 | `Ctrl+Alt+H` |
| 重复上次截图 | `F4` |

### 截图浮层

| 操作 | 快捷键 |
| --- | --- |
| 选区复制 | `Enter` |
| 选区贴图 | `F3` |
| 选区保存 | `Ctrl+S` |
| 选区编辑 | `Space` |
| 选区 OCR | `O` |
| 移动到/切换候选区域 | `Tab` / `方向键` |
| 截图全屏 | `F` |
| 取消截图 | `Esc` |

### 贴图窗口

| 操作 | 快捷键 |
| --- | --- |
| 关闭贴图 | `Esc` |
| 复制图片 | `Ctrl+C` |
| 保存图片 | `Ctrl+S` |
| 旋转 90° | `R` |
| 水平翻转 | `H` |
| 垂直翻转 | `V` |
| 切换置顶 | `A` |
| 切换点击穿透 | `T` |
| 实际大小 1:1 | `Ctrl+0` |
| 适应屏幕 | `Ctrl+9` |
| 撤销变换 | `Ctrl+Z` |
| 拖拽图片到其他窗口 | `Ctrl+拖拽` |

## 环境要求

- Windows
- CMake 3.16 或更高版本
- 支持 C++17 的 MSVC 工具链
- Qt 5，需包含 `Widgets`、`Sql`、`LinguistTools` 模块
- Windows SDK，项目会链接 `user32`、`gdi32`、`shell32`、`shcore`、`dwmapi`、`ole32`、`oleaut32`、`uiautomationcore`、`d3d11`、`dxgi`、`windowsapp`
- （可选）WinRT SDK 头文件 `winrt/Windows.Media.Ocr.h`，用于 OCR 功能

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
  app/                    应用入口、启动流程和依赖组装（组合根、工厂）
  domain/                 领域接口与核心业务逻辑
    capture/              截图工作流、选区历史
    editor/               标注数据模型
    history/              历史仓储接口
    hotkeys/              热键服务接口
    ocr/                  OCR 服务接口
    pin/                  贴图服务、仓储接口
    settings/             设置服务、仓储接口
  infrastructure/         基础设施实现
    clipboard/            剪贴板图片提供
    config/               JSON 配置仓储
    filesystem/           应用路径管理
    image/                本地图片存储
    logging/              日志
    ocr/                  WinRT WindowsOcrService 实现
    persistence/          SQLite 连接、迁移、历史仓储、贴图仓储
  platform/
    IPlatformService.h    平台抽象接口
    windows/
      capture/            DXGI 截图（主）、GDI 截图（回退）、像素采样、区域检测
      hotkey/             Win32 全局热键注册
      window/             窗口交互（点击穿透）
      WindowsPlatformService  平台服务聚合
  presentation/           Qt Widgets UI
    capture_actions/      选区完成后的动作栏
    capture_overlay/      全屏截图浮层（选区、拖动、缩放、放大镜、候选区域）
    editor/               标注编辑器（画笔、箭头、矩形、文字、马赛克、模糊）
    history/              历史浏览
    icons/                图标提供
    main_window/          主窗口
    ocr/                  OCR 结果窗口
    pin_window/           贴图窗口（拖拽、缩放、旋转、工具栏）
    settings/             设置面板
    toast/                Windows 通知
    tray/                 系统托盘
    viewmodels/           CaptureViewModel、HistoryViewModel、PinViewModel、SettingsViewModel
  shared/
    events/               EventHub 事件总线
    result/               Result<T> 成功/失败类型
    screen/               屏幕分段工具
    types/                数据结构（AppSettings、CaptureRecord、Hotkey、ScreenCaptureTypes）
    utils/                时间提供
tests/
  unit/                   QtTest 单元测试（CaptureSelectionHistory、CaptureWorkflow、CaptureActionBar、CaptureViewModel、SettingsViewModel）
  integration/            QtTest 集成测试（SQLite 仓储、贴图服务、图片存储、像素采样、PinWindow、CaptureOverlay）
  test_helpers.h          测试替身（FakeClipboardImageProvider、FakeScreenCaptureService 等）
resources/
  icons/                  22 个 SVG 图标
  themes/                 QSS 主题（浅色/深色）
  qrc/                    Qt 资源文件
  translations/           翻译文件（en、zh_CN）
```

## 架构说明

项目采用分层结构，依赖方向严格单向：

```text
presentation -> domain -> infrastructure -> platform/windows
```

### 依赖注入

- 组合根在 `src/app/AppContext.*`，通过 `InfrastructureFactory`、`ServiceFactory`、`ViewModelFactory` 三级工厂创建并组装全部依赖。
- 领域服务通过 C++ 接口（纯虚类）隔离平台和基础设施实现。

### 截图流程

1. 用户通过全局热键或托盘菜单触发截图。
2. `CaptureOverlay` 创建全屏半透明浮层，支持鼠标选区、键盘方向键微调、多显示器候选区域。
3. 选区确认后，浮层隐藏，`CaptureViewModel` 通过 `QThreadPool` 异步调用截图服务。
4. 截图服务优先使用 **DXGI 桌面复制**（DirectX 11），若失败则自动回退到 **GDI**（`BitBlt`）。
5. 截图完成后返回 UI 线程，执行复制/保存/贴图/编辑/OCR 等后续动作。

### 贴图

- `PinWindow` 创建置顶无边框窗口，支持拖拽、缩放（滚轮/拖拽边缘）、旋转、翻转、透明度调整、点击穿透。
- 贴图状态持久化到 SQLite，重启后可恢复。

### OCR

- 使用 Windows SDK **WinRT `Windows.Media.Ocr`** API。
- 后台线程处理，支持预处理（灰度化、锐化、Otsu 二值化、自动放大小字）。
- 多次触发 OCR 时，先前的请求自动失效，仅显示最新的识别结果。

### 事件通信

- 跨 ViewModel 通信通过 `EventHub`（`QObject` 信号总线）分发。
- 异步操作使用 `QPointer` + `QMetaObject::invokeMethod` 确保线程安全。

### 错误处理

- 领域服务统一使用 `Result<T>` 类型表达成功或失败，避免异常抛出。
- 截图服务提供三级容错：DXGI（主）→ 设备重建重试（3 次）→ GDI 回退。

## 数据与配置

SnapPaste 会在用户本地应用数据目录下保存配置、数据库、截图文件和缩略图。配置由 `JsonSettingsRepository` 管理，历史记录和贴图状态由 SQLite 仓储管理。界面语言支持简体中文和英文（遵循系统语言设置）。

## 测试

- **单元测试**：在 `tests/unit/` 中，使用 QtTest。覆盖 `CaptureSelectionHistory`、`CaptureWorkflow`、`CaptureActionBar`、`CaptureViewModel`、`SettingsViewModel` 等核心逻辑。
- **集成测试**：在 `tests/integration/` 中，使用 QtTest。覆盖 SQLite 仓储、贴图服务、图片存储、像素采样、`CaptureOverlay`、`PinWindow` 等。
- **测试替身**：内存实现配置服务、截图服务、剪贴板服务等，便于隔离测试。

## 当前状态

项目版本为 `0.1.0`。当前实现面向 Windows，不包含跨平台抽象层；`platform/windows/` 是唯一平台实现目录。构建系统支持单元测试和集成测试的独立开关。
