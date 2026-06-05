<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh-CN">
<context>
    <name>AppErrors</name>
    <message>
        <location filename="../../src/app/AppStartup.cpp" line="14"/>
        <source>Failed to load theme resource.</source>
        <translation>加载主题资源失败</translation>
    </message>
    <message>
        <location filename="../../src/domain/capture/CaptureWorkflow.cpp" line="22"/>
        <source>Selection is empty. Please select a region to capture.</source>
        <translation>选区为空，请选择要截图的区域</translation>
    </message>
    <message>
        <location filename="../../src/domain/capture/CaptureWorkflow.cpp" line="25"/>
        <source>Selection too small. Please select a larger area.</source>
        <translation>选区太小，请选择更大的区域</translation>
    </message>
    <message>
        <location filename="../../src/domain/capture/CaptureWorkflow.cpp" line="54"/>
        <source>No image data to save. Try capturing again.</source>
        <translation>没有要保存的图像数据，请重新截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/viewmodels/HistoryViewModel.cpp" line="98"/>
        <source>Invalid capture id.</source>
        <translation>无效的截图 ID</translation>
    </message>
    <message>
        <location filename="../../src/domain/pin/PinnedImageService.cpp" line="21"/>
        <source>Cannot pin an empty image.</source>
        <translation>无法贴图空图像</translation>
    </message>
    <message>
        <location filename="../../src/domain/pin/PinnedImageService.cpp" line="38"/>
        <source>Failed to load image file.</source>
        <translation>加载图片文件失败</translation>
    </message>
    <message>
        <location filename="../../src/domain/pin/PinnedImageService.cpp" line="62"/>
        <location filename="../../src/domain/pin/PinnedImageService.cpp" line="76"/>
        <source>Invalid pinned item id.</source>
        <translation>无效的贴图项 ID</translation>
    </message>
    <message>
        <location filename="../../src/domain/settings/SettingsService.cpp" line="19"/>
        <location filename="../../src/presentation/viewmodels/SettingsViewModel.cpp" line="75"/>
        <source>Save directory cannot be empty.</source>
        <translation>保存目录不能为空</translation>
    </message>
    <message>
        <location filename="../../src/domain/settings/SettingsService.cpp" line="22"/>
        <location filename="../../src/presentation/viewmodels/SettingsViewModel.cpp" line="78"/>
        <source>Unsupported image format.</source>
        <translation>不支持的图片格式</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/clipboard/ClipboardImageProvider.cpp" line="19"/>
        <source>Cannot access clipboard.</source>
        <translation>无法访问剪贴板</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/clipboard/ClipboardImageProvider.cpp" line="24"/>
        <source>Nothing on clipboard — copy an image or text first, then press Paste.</source>
        <translation>剪贴板为空，请先复制图片或文字，再按粘贴</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/clipboard/ClipboardImageProvider.cpp" line="50"/>
        <source>Clipboard content is not supported. Try copying an image or text first.</source>
        <translation>不支持的剪贴板内容，请先复制图片或文字</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/clipboard/ClipboardImageProvider.cpp" line="56"/>
        <source>Clipboard HTML is empty.</source>
        <translation>剪贴板 HTML 为空</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/clipboard/ClipboardImageProvider.cpp" line="77"/>
        <source>Clipboard text is empty.</source>
        <translation>剪贴板文本为空</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/clipboard/ClipboardImageProvider.cpp" line="106"/>
        <source>Clipboard text is not a color.</source>
        <translation>剪贴板文本不是颜色值</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/config/JsonSettingsRepository.cpp" line="73"/>
        <source>Failed to open settings file.</source>
        <translation>打开设置文件失败</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/config/JsonSettingsRepository.cpp" line="78"/>
        <source>Settings file is invalid.</source>
        <translation>设置文件无效</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/config/JsonSettingsRepository.cpp" line="139"/>
        <source>Failed to write settings file.</source>
        <translation>写入设置文件失败</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/config/JsonSettingsRepository.cpp" line="143"/>
        <source>Failed to write settings file: </source>
        <translation>写入设置文件失败：</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/config/JsonSettingsRepository.cpp" line="146"/>
        <source>Failed to atomically save settings file: </source>
        <translation>自动保存设置文件失败：</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/image/LocalImageStorage.cpp" line="22"/>
        <source>Image is empty.</source>
        <translation>图像为空</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/image/LocalImageStorage.cpp" line="28"/>
        <source>Failed to create capture directory.</source>
        <translation>创建截图目录失败</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/image/LocalImageStorage.cpp" line="36"/>
        <source>Failed to save capture image.</source>
        <translation>保存截图图像失败</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/persistence/SqliteConnection.cpp" line="39"/>
        <source>Failed to open SQLite database.</source>
        <translation>打开数据库失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="58"/>
        <source>Failed to create DXGI factory.</source>
        <translation>创建 DXGI 工厂失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="105"/>
        <source>No matching DXGI output was found.</source>
        <translation>未找到匹配的 DXGI 输出</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="230"/>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="361"/>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="19"/>
        <source>Capture region is invalid.</source>
        <translation>截图区域无效</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="253"/>
        <source>Capture region is outside the selected output.</source>
        <translation>截图区域超出所选输出</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="259"/>
        <source>DXGI desktop duplication is not available.</source>
        <translation>DXGI 桌面复制不可用</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="266"/>
        <source>Failed to acquire a DXGI desktop frame.</source>
        <translation>获取 DXGI 桌面帧失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="274"/>
        <source>DXGI desktop frame is not a D3D11 texture.</source>
        <translation>DXGI 桌面帧不是 D3D11 纹理</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="296"/>
        <source>Failed to allocate DXGI readback texture.</source>
        <translation>分配 DXGI 回读纹理失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="313"/>
        <source>Failed to map DXGI readback texture.</source>
        <translation>映射 DXGI 回读纹理失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="333"/>
        <source>DXGI returned a black frame.</source>
        <translation>DXGI 返回了黑色帧</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="347"/>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="117"/>
        <source>No primary screen is available.</source>
        <translation>无可用主屏幕</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/DxgiScreenCaptureService.cpp" line="536"/>
        <source>Failed to capture screen with DXGI.</source>
        <translation type="unfinished">DXGI 截图失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="24"/>
        <source>Failed to access the screen device context.</source>
        <translation>访问屏幕设备上下文失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="30"/>
        <source>Failed to create a capture device context.</source>
        <translation>创建截图设备上下文失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="37"/>
        <source>Failed to allocate a capture bitmap.</source>
        <translation>分配截图位图失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="45"/>
        <source>Failed to select bitmap into capture DC.</source>
        <translation>选择位图到截图 DC 失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="62"/>
        <source>Failed to copy the selected screen region.</source>
        <translation>复制所选屏幕区域失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="87"/>
        <source>Failed to read the captured screen pixels.</source>
        <translation>读取截图像素失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="122"/>
        <source>Failed to capture primary screen.</source>
        <translation>截取主屏幕失败</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="176"/>
        <source>Capture region does not intersect any screen.</source>
        <translation>截图区域未与任何屏幕相交</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="192"/>
        <source>No screen is available for capture.</source>
        <translation>无可用屏幕进行截图</translation>
    </message>
    <message>
        <location filename="../../src/platform/windows/capture/GdiScreenCaptureService.cpp" line="197"/>
        <source>Failed to capture selected region.</source>
        <translation>截取所选区域失败</translation>
    </message>
</context>
<context>
    <name>EditorWindow</name>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="473"/>
        <source>Rect (R)</source>
        <translation>矩形 (R)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="473"/>
        <source>Rectangle</source>
        <translation>矩形</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="474"/>
        <source>Ellipse (E)</source>
        <translation>椭圆 (E)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="91"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="474"/>
        <source>Ellipse</source>
        <translation>椭圆</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="475"/>
        <source>Arrow (A)</source>
        <translation>箭头 (A)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="86"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="475"/>
        <source>Arrow</source>
        <translation>箭头</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="476"/>
        <source>Line (L)</source>
        <translation>直线 (L)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="87"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="476"/>
        <source>Line</source>
        <translation>直线</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="477"/>
        <source>Pen (P)</source>
        <translation>画笔 (P)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="88"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="477"/>
        <source>Pen</source>
        <translation>画笔</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="478"/>
        <source>Text (T)</source>
        <translation>文字 (T)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="89"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="478"/>
        <source>Text</source>
        <translation>文字</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="479"/>
        <source>Hi (H)</source>
        <translation>高亮 (H)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="479"/>
        <source>Highlight</source>
        <translation>高亮</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="480"/>
        <source>Num (N)</source>
        <translation>序号 (N)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="480"/>
        <source>Numbered</source>
        <translation>序号</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="481"/>
        <source>Mosaic (M)</source>
        <translation>马赛克 (M)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="90"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="481"/>
        <source>Mosaic</source>
        <translation>马赛克</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="482"/>
        <source>Eraser (X)</source>
        <translation>橡皮擦 (X)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="93"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="482"/>
        <source>Eraser</source>
        <translation>橡皮擦</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="483"/>
        <source>Select (V)</source>
        <translation>选择 (V)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="84"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="483"/>
        <source>Select</source>
        <translation>选择</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="484"/>
        <source>Crop (C)</source>
        <translation>裁剪 (C)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="95"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="484"/>
        <source>Crop</source>
        <translation>裁剪</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="734"/>
        <source>Outline</source>
        <translation>描边</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="734"/>
        <source>Toggle text outline</source>
        <translation>切换文字描边</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="735"/>
        <source>Fill</source>
        <translation>填充</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="735"/>
        <source>Toggle fill for shapes</source>
        <translation>切换形状填充</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="736"/>
        <source>Blur</source>
        <translation>模糊</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="736"/>
        <source>Toggle mosaic blur mode</source>
        <translation>切换马赛克模糊模式</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="737"/>
        <source>Grid</source>
        <translation>网格</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="737"/>
        <source>Toggle alignment grid</source>
        <translation>切换对齐网格</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="510"/>
        <source>Undo</source>
        <translation>撤销</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="85"/>
        <source>Rect</source>
        <translation type="unfinished">矩形</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="92"/>
        <source>Hi</source>
        <translation type="unfinished">高亮</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="94"/>
        <source>Num</source>
        <translation type="unfinished">编号</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="510"/>
        <source>Undo (Ctrl+Z)</source>
        <translation>撤销 (Ctrl+Z)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="511"/>
        <source>Redo</source>
        <translation>重做</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="511"/>
        <source>Redo (Ctrl+Y)</source>
        <translation>重做 (Ctrl+Y)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="512"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="512"/>
        <source>Copy (Ctrl+Shift+C)</source>
        <translation>复制 (Ctrl+Shift+C)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="513"/>
        <source>Pin</source>
        <translation>贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="513"/>
        <source>Pin (F3)</source>
        <translation>贴图 (F3)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="514"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="514"/>
        <source>Save (Ctrl+S)</source>
        <translation>保存 (Ctrl+S)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="515"/>
        <source>Export...</source>
        <translation>导出...</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="515"/>
        <source>Export (Ctrl+Shift+S)</source>
        <translation>导出 (Ctrl+Shift+S)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="816"/>
        <source>Tri</source>
        <translation type="unfinished">三角</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="817"/>
        <source>Circle</source>
        <translation type="unfinished">圆形</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="818"/>
        <source>Square</source>
        <translation type="unfinished">方形</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1122"/>
        <source>Free</source>
        <translation type="unfinished">自由</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1123"/>
        <source>1:1</source>
        <translation type="unfinished">1:1</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1124"/>
        <source>16:9</source>
        <translation type="unfinished">16:9</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1125"/>
        <source>4:3</source>
        <translation type="unfinished">4:3</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1126"/>
        <source>3:2</source>
        <translation type="unfinished">3:2</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../../src/app/main.cpp" line="74"/>
        <source>Program already running</source>
        <translation>程序已在运行</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="174"/>
        <source>No image is available for OCR.</source>
        <translation>没有可用于 OCR 识别的图像</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="178"/>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="197"/>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="230"/>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="232"/>
        <source>OCR cancelled.</source>
        <translation>OCR 已取消</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="194"/>
        <source>OCR is not available for the current Windows language profile.</source>
        <translation>当前 Windows 语言配置不支持 OCR 识别</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="259"/>
        <source>No text was recognized in the selected region.</source>
        <translation>所选区域未识别到文字</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="265"/>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="268"/>
        <source>OCR failed while processing the selected region.</source>
        <translation>OCR 识别所选区域时失败</translation>
    </message>
    <message>
        <location filename="../../src/infrastructure/ocr/WindowsOcrService.cpp" line="272"/>
        <source>OCR is not available in this build.</source>
        <translation>此版本不支持 OCR 识别</translation>
    </message>
</context>
<context>
    <name>SettingsWidget</name>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="165"/>
        <source>Auto (System Default)</source>
        <translation>自动（系统默认）</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="166"/>
        <source>English</source>
        <translation>英语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="167"/>
        <source>Chinese (Simplified)</source>
        <translation>简体中文</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="168"/>
        <source>Chinese (Traditional)</source>
        <translation>繁体中文</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="169"/>
        <source>Japanese</source>
        <translation>日语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="170"/>
        <source>Korean</source>
        <translation>韩语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="171"/>
        <source>French</source>
        <translation>法语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="172"/>
        <source>German</source>
        <translation>德语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="173"/>
        <source>Spanish</source>
        <translation>西班牙语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="174"/>
        <source>Italian</source>
        <translation>意大利语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="175"/>
        <source>Portuguese (Brazil)</source>
        <translation>葡萄牙语（巴西）</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="176"/>
        <source>Russian</source>
        <translation>俄语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="177"/>
        <source>Arabic</source>
        <translation>阿拉伯语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="178"/>
        <source>Dutch</source>
        <translation>荷兰语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="179"/>
        <source>Polish</source>
        <translation>波兰语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="180"/>
        <source>Swedish</source>
        <translation>瑞典语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="181"/>
        <source>Turkish</source>
        <translation>土耳其语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="182"/>
        <source>Czech</source>
        <translation>捷克语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="183"/>
        <source>Danish</source>
        <translation>丹麦语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="184"/>
        <source>Finnish</source>
        <translation>芬兰语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="185"/>
        <source>Greek</source>
        <translation>希腊语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="186"/>
        <source>Hungarian</source>
        <translation>匈牙利语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="187"/>
        <source>Norwegian</source>
        <translation>挪威语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="188"/>
        <source>Thai</source>
        <translation>泰语</translation>
    </message>
</context>
<context>
    <name>snappaste::AnnotationCanvas</name>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="294"/>
        <source>SnapPaste Editor</source>
        <translation>SnapPaste 编辑器</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="301"/>
        <source> - %1 ann</source>
        <translation> - %1 个标注</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="840"/>
        <source>Unsaved Changes</source>
        <translation type="unfinished">未保存的更改</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="841"/>
        <source>Drop image and discard all annotations?</source>
        <translation type="unfinished">替换图片并丢弃所有标注？</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="862"/>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="891"/>
        <source>Edit Text</source>
        <translation>编辑文字</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="862"/>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="891"/>
        <source>Edit text:</source>
        <translation>编辑文字：</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1783"/>
        <source>Keyboard Shortcuts</source>
        <translation>快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1784"/>
        <source>&lt;b&gt;Tools&lt;/b&gt;&lt;br&gt;R - Rectangle&lt;br&gt;E - Ellipse&lt;br&gt;A - Arrow&lt;br&gt;L - Line&lt;br&gt;P - Pen&lt;br&gt;T - Text&lt;br&gt;H - Highlight&lt;br&gt;N - Numbered&lt;br&gt;M - Mosaic&lt;br&gt;V - Select&lt;br&gt;X - Eraser&lt;br&gt;C - Crop&lt;br&gt;&lt;br&gt;&lt;b&gt;Edit&lt;/b&gt;&lt;br&gt;Ctrl+Z - Undo&lt;br&gt;Ctrl+Y - Redo&lt;br&gt;Ctrl+D - Duplicate annotation&lt;br&gt;Ctrl+A - Select last annotation&lt;br&gt;Delete/Backspace - Remove annotation&lt;br&gt;Arrow keys - Nudge annotation (Shift+Arrow = 10px)&lt;br&gt;Ctrl+Shift+Arrow Up/Down - Bring forward / Send backward&lt;br&gt;Double-click - Delete annotation (Text tool: edit text)&lt;br&gt;[ / ] - Decrease/Increase text font size&lt;br&gt;&lt;br&gt;&lt;b&gt;Zoom&lt;/b&gt;&lt;br&gt;Ctrl+Scroll / Ctrl++ / Ctrl+- - Zoom&lt;br&gt;Ctrl+0 - 100%&lt;br&gt;Ctrl+9 - Fit to window&lt;br&gt;&lt;br&gt;&lt;b&gt;File&lt;/b&gt;&lt;br&gt;Ctrl+C - Copy image&lt;br&gt;Ctrl+S - Save&lt;br&gt;Ctrl+Shift+S - Export...&lt;br&gt;F3 - Pin image&lt;br&gt;Escape - Close editor</source>
        <translation>&lt;b&gt;工具&lt;/b&gt;&lt;br&gt;R - 矩形&lt;br&gt;E - 椭圆&lt;br&gt;A - 箭头&lt;br&gt;L - 直线&lt;br&gt;P - 画笔&lt;br&gt;T - 文字&lt;br&gt;H - 高亮&lt;br&gt;N - 序号&lt;br&gt;M - 马赛克&lt;br&gt;V - 选择&lt;br&gt;X - 橡皮擦&lt;br&gt;C - 裁剪&lt;br&gt;&lt;br&gt;&lt;b&gt;编辑&lt;/b&gt;&lt;br&gt;Ctrl+Z - 撤销&lt;br&gt;Ctrl+Y - 重做&lt;br&gt;Ctrl+D - 复制标注&lt;br&gt;Ctrl+A - 选中最后一个标注&lt;br&gt;Delete/Backspace - 删除标注&lt;br&gt;方向键 - 微移标注 (Shift+方向键=10像素)&lt;br&gt;Ctrl+Shift+上/下 - 上移/下移图层&lt;br&gt;双击 - 删除标注（文字工具：编辑文字）&lt;br&gt;[ / ] - 减小/增大字号&lt;br&gt;&lt;br&gt;&lt;b&gt;缩放&lt;/b&gt;&lt;br&gt;Ctrl+滚轮 / Ctrl++ / Ctrl+- - 缩放&lt;br&gt;Ctrl+0 - 100%&lt;br&gt;Ctrl+9 - 适应窗口&lt;br&gt;&lt;br&gt;&lt;b&gt;文件&lt;/b&gt;&lt;br&gt;Ctrl+C - 复制图片&lt;br&gt;Ctrl+S - 保存&lt;br&gt;Ctrl+Shift+S - 导出...&lt;br&gt;F3 - 贴图&lt;br&gt;Esc - 关闭编辑器</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1845"/>
        <source>Copy Image	Ctrl+C</source>
        <translation>复制图片	Ctrl+C</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1846"/>
        <source>Export...</source>
        <translation>导出...</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1853"/>
        <source>Delete Annotation	Del</source>
        <translation>删除标注	Del</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1854"/>
        <source>Duplicate Annotation</source>
        <translation>复制标注</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1856"/>
        <source>Bring Forward	Ctrl+Shift+Up</source>
        <translation>上移一层	Ctrl+Shift+上</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1858"/>
        <source>Send Backward	Ctrl+Shift+Down</source>
        <translation>下移一层	Ctrl+Shift+下</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1861"/>
        <source>Zoom In	Ctrl++</source>
        <translation>放大	Ctrl++</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1862"/>
        <source>Zoom Out	Ctrl+-</source>
        <translation>缩小	Ctrl+-</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1863"/>
        <source>Actual Size (100%)	Ctrl+0</source>
        <translation>实际大小 (100%)	Ctrl+0</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1864"/>
        <source>Fit to Window	Ctrl+9</source>
        <translation>适应窗口	Ctrl+9</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1866"/>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1930"/>
        <source>Clear All Annotations</source>
        <translation>清除所有标注</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1898"/>
        <source>Save As</source>
        <translation>另存为</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1899"/>
        <source>PNG (*.png);;JPEG (*.jpg *.jpeg)</source>
        <translation>PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1931"/>
        <source>Are you sure you want to clear all annotations?</source>
        <translation>确定要清除所有标注吗？</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="1994"/>
        <source>(%1, %2) %3</source>
        <translation type="unfinished">(%1, %2) %3</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="296"/>
        <source> - %1x%2</source>
        <translation> - %1x%2</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/AnnotationCanvas.cpp" line="298"/>
        <source> - %1%</source>
        <translation> - %1%</translation>
    </message>
</context>
<context>
    <name>snappaste::Application</name>
    <message>
        <location filename="../../src/app/Application.cpp" line="94"/>
        <source>All pinned images closed.</source>
        <translation>所有贴图已关闭</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="125"/>
        <source>Saved %1 - Click to open</source>
        <translation>已保存 %1 - 点击打开</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="119"/>
        <source>Screenshot copied. Press %1 to pin.</source>
        <translation type="unfinished">截图已复制，按 %1 贴图</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="136"/>
        <source>Pinned image created. Press %1 to repeat.</source>
        <translation type="unfinished">贴图已创建，按 %1 重复截图</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="189"/>
        <source>Open Image</source>
        <translation>打开图片</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="190"/>
        <source>Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*)</source>
        <translation>图片 (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;所有文件 (*)</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="197"/>
        <source>Failed to open image: %1</source>
        <translation>打开图片失败：%1</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="224"/>
        <source>Pinned images hidden.</source>
        <translation>贴图已隐藏</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="235"/>
        <source>Pinned images restored.</source>
        <translation>贴图已恢复</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="250"/>
        <source>Failed to capture image for pinning.</source>
        <translation>截图贴图失败</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="270"/>
        <source>Failed to capture image for editing.</source>
        <translation>截图编辑失败</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="281"/>
        <source>Failed to capture image for OCR.</source>
        <translation>截图 OCR 识别失败</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="290"/>
        <source>OCR processing...</source>
        <translation>OCR 识别中...</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="309"/>
        <source>OCR - %1 characters</source>
        <translation>OCR - %1 个字符</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="405"/>
        <source>Failed to register capture hotkey: %1</source>
        <translation>注册截图快捷键失败：%1</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="407"/>
        <location filename="../../src/app/Application.cpp" line="412"/>
        <location filename="../../src/app/Application.cpp" line="417"/>
        <location filename="../../src/app/Application.cpp" line="422"/>
        <source>SnapPaste</source>
        <translation>SnapPaste</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="410"/>
        <source>Failed to register paste hotkey: %1</source>
        <translation>注册贴图快捷键失败：%1</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="415"/>
        <source>Failed to register hide-pins hotkey: %1</source>
        <translation>注册隐藏贴图快捷键失败：%1</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="420"/>
        <source>Failed to register repeat-capture hotkey: %1</source>
        <translation type="unfinished">注册重复截图热键失败: %1</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="472"/>
        <source>No pinned image is available to copy.</source>
        <translation>没有可复制的贴图</translation>
    </message>
    <message>
        <location filename="../../src/app/Application.cpp" line="480"/>
        <source>Pinned image copied. Press %1 to repeat.</source>
        <translation type="unfinished">贴图已复制，按 %1 重复截图</translation>
    </message>
</context>
<context>
    <name>snappaste::CaptureActionBar</name>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="75"/>
        <source>Copy (Enter)</source>
        <translation>复制 (Enter)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="75"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="76"/>
        <source>Pin (F3)</source>
        <translation>贴图 (F3)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="76"/>
        <source>Pin</source>
        <translation>贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="77"/>
        <source>Save (Ctrl+S)</source>
        <translation>保存 (Ctrl+S)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="77"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="78"/>
        <source>Edit (Space)</source>
        <translation>编辑 (Space)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="78"/>
        <source>Edit</source>
        <translation>编辑</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="79"/>
        <source>OCR (O)</source>
        <translation>文字识别 (O)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="79"/>
        <source>OCR</source>
        <translation>文字识别</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="80"/>
        <source>Cancel (Esc)</source>
        <translation>取消 (Esc)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_actions/CaptureActionBar.cpp" line="80"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
</context>
<context>
    <name>snappaste::CaptureOverlay</name>
    <message>
        <location filename="../../src/presentation/capture_overlay/CaptureOverlay.cpp" line="494"/>
        <source>Tab / Arrow keys to cycle  ·  Enter to capture</source>
        <translation>Tab/方向键切换  ·  Enter 确认截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_overlay/CaptureOverlay.cpp" line="503"/>
        <source>Drag to select area  ·  Double-click to capture full screen</source>
        <translation>拖动选择区域  ·  双击截取全屏</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_overlay/CaptureOverlay.cpp" line="536"/>
        <source>(%1, %2)</source>
        <translation>(%1, %2)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_overlay/CaptureOverlay.cpp" line="972"/>
        <source>%1 x %2</source>
        <translation>%1 × %2</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_overlay/CaptureOverlay.cpp" line="1092"/>
        <source>%1
rgb(%2,%3,%4)
%5,%6</source>
        <translation>%1
rgb(%2,%3,%4)
%5,%6</translation>
    </message>
    <message>
        <location filename="../../src/presentation/capture_overlay/CaptureOverlay.cpp" line="1098"/>
        <source>%1,%2</source>
        <translation>%1,%2</translation>
    </message>
</context>
<context>
    <name>snappaste::CaptureViewModel</name>
    <message>
        <location filename="../../src/presentation/viewmodels/CaptureViewModel.cpp" line="136"/>
        <source>No image is available to copy.</source>
        <translation>没有可复制的图像</translation>
    </message>
</context>
<context>
    <name>snappaste::EditorWindow</name>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="108"/>
        <source>SnapPaste Editor</source>
        <translation>SnapPaste 编辑器</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="124"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="236"/>
        <source>Undo</source>
        <translation>撤销</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="129"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="242"/>
        <source>Redo</source>
        <translation>重做</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="134"/>
        <source>Copy Image</source>
        <translation>复制图片</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="139"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1445"/>
        <source>Copied to clipboard</source>
        <translation>已复制到剪贴板</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="143"/>
        <source>Paste Image</source>
        <translation>粘贴图片</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="149"/>
        <source>Image pasted from clipboard</source>
        <translation>已从剪贴板粘贴图片</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="154"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="159"/>
        <source>Export</source>
        <translation>导出</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="160"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1464"/>
        <source>PNG (*.png);;JPEG (*.jpg *.jpeg)</source>
        <translation>PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="164"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1467"/>
        <source>Saved to %1</source>
        <translation>已保存到 %1</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="166"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1469"/>
        <source>Failed to save image</source>
        <translation>保存图片失败</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="172"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="177"/>
        <source>Save</source>
        <translation>保存</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="183"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1459"/>
        <source>Saved</source>
        <translation>已保存</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="187"/>
        <source>Pin</source>
        <translation>贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="193"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1452"/>
        <source>Image pinned</source>
        <translation>图片已贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="201"/>
        <source>Unsaved Changes</source>
        <translation>未保存的更改</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="202"/>
        <source>You have unsaved annotations. Save before closing?</source>
        <translation>存在未保存的标注，关闭前保存吗？</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="223"/>
        <source>Image: %1 x %2 px</source>
        <translation>图片：%1 x %2 像素</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="236"/>
        <source>Undo (%1)</source>
        <translation>撤销 (%1)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="242"/>
        <source>Redo (%1)</source>
        <translation>重做 (%1)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="327"/>
        <source>Click or drag to select</source>
        <translation type="unfinished">点击或拖动选择</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="328"/>
        <source>Drag to draw a rectangle</source>
        <translation type="unfinished">拖动绘制矩形</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="329"/>
        <source>Drag to draw an arrow</source>
        <translation type="unfinished">拖动绘制箭头</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="330"/>
        <source>Drag to draw a line</source>
        <translation type="unfinished">拖动绘制直线</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="331"/>
        <source>Freehand drawing</source>
        <translation type="unfinished">自由手绘</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="332"/>
        <source>Click to place text</source>
        <translation type="unfinished">点击放置文字</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="333"/>
        <source>Drag to apply mosaic blur</source>
        <translation type="unfinished">拖动应用马赛克</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="334"/>
        <source>Drag to draw an ellipse</source>
        <translation type="unfinished">拖动绘制椭圆</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="335"/>
        <source>Drag to highlight an area</source>
        <translation type="unfinished">拖动高亮区域</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="336"/>
        <source>Click or drag to erase</source>
        <translation type="unfinished">点击或拖动擦除</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="337"/>
        <source>Click to place numbered circle</source>
        <translation type="unfinished">点击放置序号标记</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="338"/>
        <source>Drag crop handles to trim</source>
        <translation type="unfinished">拖动裁剪框裁剪</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="441"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="669"/>
        <source>Custom Color...</source>
        <translation>自定义颜色...</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="444"/>
        <source>Choose Color</source>
        <translation>选择颜色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="542"/>
        <source>Tools</source>
        <translation>工具</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="600"/>
        <source>Color</source>
        <translation>颜色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="608"/>
        <source>Eyedropper</source>
        <translation>取色器</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="627"/>
        <source>Fill Color</source>
        <translation type="unfinished">填充颜色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="672"/>
        <source>Choose Fill Color</source>
        <translation type="unfinished">选择填充颜色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="681"/>
        <source>No Fill</source>
        <translation type="unfinished">无填充</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="711"/>
        <source>Stroke: %1px</source>
        <translation>描边：%1像素</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="802"/>
        <source>Opacity</source>
        <translation>不透明度</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="811"/>
        <source>Arrow</source>
        <translation>箭头</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="844"/>
        <source>Radius</source>
        <translation>圆角半径</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="310"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="853"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="860"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1046"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1049"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1101"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1505"/>
        <source>Off</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="879"/>
        <source>Font family</source>
        <translation type="unfinished">字体</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="908"/>
        <source>B</source>
        <translation type="unfinished">B</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="908"/>
        <source>Bold</source>
        <translation type="unfinished">粗体</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="909"/>
        <source>I</source>
        <translation type="unfinished">I</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="909"/>
        <source>Italic</source>
        <translation type="unfinished">斜体</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="910"/>
        <source>U</source>
        <translation type="unfinished">U</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="910"/>
        <source>Underline</source>
        <translation type="unfinished">下划线</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="931"/>
        <source>Align left</source>
        <translation type="unfinished">左对齐</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="932"/>
        <source>Align center</source>
        <translation type="unfinished">居中对齐</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="933"/>
        <source>Align right</source>
        <translation type="unfinished">右对齐</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="965"/>
        <source>Font</source>
        <translation>字体</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="971"/>
        <source>Decrease font size ( [ )</source>
        <translation>减小字号 ( [ )</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="984"/>
        <source>Font size for Text / Numbered tools</source>
        <translation>文字/序号工具的字号</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="990"/>
        <source>Increase font size ( ] )</source>
        <translation>增大字号 ( ] )</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1034"/>
        <source>BG</source>
        <translation type="unfinished">背景</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1039"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1046"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1049"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1101"/>
        <source>On</source>
        <translation type="unfinished">开</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1040"/>
        <source>Toggle text background</source>
        <translation type="unfinished">切换文字背景</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1054"/>
        <source>Text background color</source>
        <translation type="unfinished">文字背景色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1070"/>
        <source>Choose Text Background Color</source>
        <translation type="unfinished">选择文字背景色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1116"/>
        <source>Aspect Ratio</source>
        <translation type="unfinished">宽高比</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1146"/>
        <source>Crop ratio locked: %1</source>
        <translation type="unfinished">裁剪比例锁定: %1</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1148"/>
        <source>Crop ratio: Free</source>
        <translation type="unfinished">裁剪比例: 自由</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1166"/>
        <source>Zoom</source>
        <translation>缩放</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1172"/>
        <source>Zoom out</source>
        <translation>缩小</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1195"/>
        <source>Zoom in</source>
        <translation>放大</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1201"/>
        <source>Reset zoom to 100% (Ctrl+0)</source>
        <translation type="unfinished">重置缩放 100% (Ctrl+0)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1206"/>
        <source>Fit</source>
        <translation type="unfinished">适应</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1207"/>
        <source>Fit to window (Ctrl+9)</source>
        <translation type="unfinished">适应窗口 (Ctrl+9)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1260"/>
        <source>Adjust</source>
        <translation type="unfinished">调整</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1264"/>
        <source>Bright</source>
        <translation type="unfinished">亮度</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1266"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1270"/>
        <source>+%1</source>
        <translation type="unfinished">+%1</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1268"/>
        <source>Contrast</source>
        <translation type="unfinished">对比度</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1283"/>
        <source>Transform</source>
        <translation type="unfinished">变换</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1306"/>
        <source>CW</source>
        <translation type="unfinished">顺时针</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1306"/>
        <source>Rotate 90 degrees clockwise</source>
        <translation type="unfinished">顺时针旋转 90 度</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1307"/>
        <source>CCW</source>
        <translation type="unfinished">逆时针</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1307"/>
        <source>Rotate 90 degrees counter-clockwise</source>
        <translation type="unfinished">逆时针旋转 90 度</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1308"/>
        <source>180</source>
        <translation type="unfinished">180</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1308"/>
        <source>Rotate 180 degrees</source>
        <translation type="unfinished">旋转 180 度</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1309"/>
        <source>H</source>
        <translation type="unfinished">H</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1309"/>
        <source>Flip horizontal</source>
        <translation type="unfinished">水平翻转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1310"/>
        <source>V</source>
        <translation type="unfinished">V</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1310"/>
        <source>Flip vertical</source>
        <translation type="unfinished">垂直翻转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1330"/>
        <source>Layers</source>
        <translation type="unfinished">图层</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1379"/>
        <source>Delete</source>
        <translation type="unfinished">删除</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1380"/>
        <source>Duplicate</source>
        <translation type="unfinished">复制</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1382"/>
        <source>Move Up</source>
        <translation type="unfinished">上移</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1383"/>
        <source>Move Down</source>
        <translation type="unfinished">下移</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1463"/>
        <source>Save As</source>
        <translation>另存为</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="701"/>
        <source>S</source>
        <translation>小</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="701"/>
        <source>M</source>
        <translation>中</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="701"/>
        <source>L</source>
        <translation>大</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="804"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1216"/>
        <source>%1%</source>
        <translation>%1%</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="310"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="860"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="982"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1002"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1505"/>
        <source>%1px</source>
        <translation>%1px</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="970"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1171"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="989"/>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1194"/>
        <source>+</source>
        <translation>+</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1188"/>
        <source>100%</source>
        <translation>100%</translation>
    </message>
    <message>
        <location filename="../../src/presentation/editor/EditorWindow.cpp" line="1200"/>
        <source>1:1</source>
        <translation>1:1</translation>
    </message>
</context>
<context>
    <name>snappaste::HistoryViewModel</name>
    <message>
        <location filename="../../src/presentation/viewmodels/HistoryViewModel.cpp" line="15"/>
        <location filename="../../src/presentation/viewmodels/HistoryViewModel.cpp" line="33"/>
        <source>Capture History</source>
        <translation>截图历史</translation>
    </message>
    <message>
        <location filename="../../src/presentation/viewmodels/HistoryViewModel.cpp" line="49"/>
        <source>%1
%2x%3</source>
        <translation>%1
%2x%3</translation>
    </message>
</context>
<context>
    <name>snappaste::HistoryWidget</name>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="55"/>
        <source>Search by filename...</source>
        <translation>按文件名搜索...</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="65"/>
        <source>Refresh</source>
        <translation>刷新</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="66"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="198"/>
        <source>Pin</source>
        <translation>贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="67"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="199"/>
        <source>Copy</source>
        <translation>复制</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="68"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="200"/>
        <source>Open</source>
        <translation>打开</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="70"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="203"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="237"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="269"/>
        <source>Delete</source>
        <translation>删除</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="84"/>
        <source>Search:</source>
        <translation>搜索：</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="111"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="121"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="150"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="156"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="190"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="212"/>
        <source>SnapPaste</source>
        <translation>SnapPaste</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="111"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="121"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="150"/>
        <source>No captures selected.</source>
        <translation>未选择截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="125"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="235"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="267"/>
        <source>Are you sure you want to delete this capture?</source>
        <translation>确定要删除此截图吗？</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="126"/>
        <source>Are you sure you want to delete %1 captures?</source>
        <translation>确定要删除 %1 个截图吗？</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="127"/>
        <source>Delete Capture</source>
        <translation>删除截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="156"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="212"/>
        <source>Failed to load image.</source>
        <translation>加载图片失败</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="199"/>
        <source>Copy All</source>
        <translation>全部复制</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="69"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="201"/>
        <source>Show in Explorer</source>
        <translation>在资源管理器中显示</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="203"/>
        <source>Delete All</source>
        <translation>全部删除</translation>
    </message>
    <message>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="236"/>
        <location filename="../../src/presentation/history/HistoryWidget.cpp" line="268"/>
        <source>Delete %1 captures?</source>
        <translation>删除 %1 个截图？</translation>
    </message>
</context>
<context>
    <name>snappaste::HotkeyInput</name>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="50"/>
        <source>Press shortcut...</source>
        <translation>按下快捷键...</translation>
    </message>
</context>
<context>
    <name>snappaste::MainWindow</name>
    <message>
        <location filename="../../src/presentation/main_window/MainWindow.cpp" line="17"/>
        <source>SnapPaste</source>
        <translation>SnapPaste</translation>
    </message>
    <message>
        <location filename="../../src/presentation/main_window/MainWindow.cpp" line="23"/>
        <source>Start Capture</source>
        <translation>开始截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/main_window/MainWindow.cpp" line="31"/>
        <source>Capture</source>
        <translation>截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/main_window/MainWindow.cpp" line="35"/>
        <source>History</source>
        <translation>历史</translation>
    </message>
    <message>
        <location filename="../../src/presentation/main_window/MainWindow.cpp" line="37"/>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
</context>
<context>
    <name>snappaste::OcrResultWindow</name>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="82"/>
        <source>OCR Result</source>
        <translation>OCR 结果</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="93"/>
        <source>Minimize</source>
        <translation>最小化</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="104"/>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="557"/>
        <source>Maximize</source>
        <translation>最大化</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="120"/>
        <source>Close (Esc)</source>
        <translation>关闭 (Esc)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="173"/>
        <source>  TEXT BLOCKS</source>
        <translation>  文本块</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="245"/>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="393"/>
        <source>Copy All</source>
        <translation>全部复制</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="250"/>
        <source>Paste &amp;&amp; Close</source>
        <translation>粘贴并关闭</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="391"/>
        <source>%1 text blocks</source>
        <translation>%1 个文本块</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="395"/>
        <source>%1/%2 selected</source>
        <translation>已选 %1/%2</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="398"/>
        <source>Copy (%1)</source>
        <translation>复制 (%1)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="557"/>
        <source>Restore</source>
        <translation>还原</translation>
    </message>
    <message>
        <location filename="../../src/presentation/ocr/OcrResultWindow.cpp" line="581"/>
        <source>Copied!</source>
        <translation>已复制！</translation>
    </message>
</context>
<context>
    <name>snappaste::PinWindow</name>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="225"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="638"/>
        <source>Copy	Ctrl+C</source>
        <translation>复制	Ctrl+C</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="226"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="639"/>
        <source>Save	Ctrl+S</source>
        <translation>保存	Ctrl+S</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="227"/>
        <source>Save As...	Ctrl+Shift+S</source>
        <translation>另存为...	Ctrl+Shift+S</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="228"/>
        <source>Copy Color</source>
        <translation>复制颜色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="230"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="517"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="641"/>
        <source>Rotate Left</source>
        <translation>左旋转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="231"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="518"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="642"/>
        <source>Rotate Right</source>
        <translation>右旋转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="232"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="519"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="643"/>
        <source>Flip Horizontal</source>
        <translation>水平翻转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="233"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="520"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="644"/>
        <source>Flip Vertical</source>
        <translation>垂直翻转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="234"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="646"/>
        <source>Always on Top	A</source>
        <translation>总在最前	A</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="237"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="521"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="649"/>
        <source>Click Through</source>
        <translation>点击穿透</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="241"/>
        <source>Actual Size (1:1)	Ctrl+0</source>
        <translation>实际大小 (1:1)	Ctrl+0</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="242"/>
        <source>Fit to Screen	Ctrl+9</source>
        <translation>适应屏幕	Ctrl+9</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="244"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="516"/>
        <source>Close</source>
        <translation>关闭</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="249"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="353"/>
        <source>Copied to clipboard</source>
        <translation>已复制到剪贴板</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="253"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="360"/>
        <source>Save As</source>
        <translation>另存为</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="254"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="361"/>
        <source>PNG (*.png);;JPEG (*.jpg *.jpeg)</source>
        <translation>PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg)</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="256"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="363"/>
        <source>Save Error</source>
        <translation type="unfinished">保存错误</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="256"/>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="363"/>
        <source>Failed to save image.</source>
        <translation type="unfinished">保存图片失败</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="265"/>
        <source>Copied %1</source>
        <translation>已复制 %1</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="383"/>
        <source>Undo transform</source>
        <translation>撤销变换</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="401"/>
        <source>Keyboard Shortcuts</source>
        <translation>快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="402"/>
        <source>&lt;b&gt;Pin Window Controls&lt;/b&gt;&lt;br&gt;&lt;br&gt;&lt;b&gt;Transform&lt;/b&gt;&lt;br&gt;R - Rotate 90&amp;deg;&lt;br&gt;Shift+R - Rotate -90&amp;deg;&lt;br&gt;H - Flip horizontally&lt;br&gt;V - Flip vertically&lt;br&gt;Ctrl+Z - Undo transform&lt;br&gt;&lt;br&gt;&lt;b&gt;View&lt;/b&gt;&lt;br&gt;Ctrl+0 - Actual size (1:1)&lt;br&gt;Ctrl+9 - Fit to screen&lt;br&gt;Ctrl+Scroll - Change opacity&lt;br&gt;Scroll - Zoom&lt;br&gt;Shift+Double-click - Toggle thumbnail mode&lt;br&gt;&lt;br&gt;&lt;b&gt;Actions&lt;/b&gt;&lt;br&gt;Ctrl+C - Copy image&lt;br&gt;Ctrl+S - Save&lt;br&gt;Ctrl+Shift+S - Save As...&lt;br&gt;Ctrl+Drag - Drag image out&lt;br&gt;A - Toggle always on top&lt;br&gt;T - Toggle click through&lt;br&gt;Escape - Close pin window</source>
        <translation>&lt;b&gt;贴图窗口控制&lt;/b&gt;&lt;br&gt;&lt;br&gt;&lt;b&gt;变换&lt;/b&gt;&lt;br&gt;R - 顺时针旋转 90&amp;deg;&lt;br&gt;Shift+R - 逆时针旋转 90&amp;deg;&lt;br&gt;H - 水平翻转&lt;br&gt;V - 垂直翻转&lt;br&gt;Ctrl+Z - 撤销变换&lt;br&gt;&lt;br&gt;&lt;b&gt;视图&lt;/b&gt;&lt;br&gt;Ctrl+0 - 实际大小 (1:1)&lt;br&gt;Ctrl+9 - 适应屏幕&lt;br&gt;Ctrl+滚轮 - 调节透明度&lt;br&gt;滚轮 - 缩放&lt;br&gt;Shift+双击 - 切换缩略图模式&lt;br&gt;&lt;br&gt;&lt;b&gt;操作&lt;/b&gt;&lt;br&gt;Ctrl+C - 复制图片&lt;br&gt;Ctrl+S - 保存&lt;br&gt;Ctrl+Shift+S - 另存为...&lt;br&gt;Ctrl+拖动 - 拖出图片&lt;br&gt;A - 切换总在最前&lt;br&gt;T - 切换点击穿透&lt;br&gt;Esc - 关闭贴图窗口</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="522"/>
        <source>Always on Top</source>
        <translation>总在最前</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="731"/>
        <source>%1%</source>
        <translation>%1%</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="653"/>
        <source>Close	Esc</source>
        <translation>关闭	Esc</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="892"/>
        <source>Rotated %1°</source>
        <translation>已旋转 %1°</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="921"/>
        <source>Flipped horizontally</source>
        <translation>已水平翻转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="931"/>
        <source>Flipped vertically</source>
        <translation>已垂直翻转</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="720"/>
        <source>T</source>
        <translation>透</translation>
    </message>
    <message>
        <location filename="../../src/presentation/pin_window/PinWindow.cpp" line="762"/>
        <source>...</source>
        <translation>...</translation>
    </message>
</context>
<context>
    <name>snappaste::SettingsWidget</name>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="139"/>
        <source>Auto-save on capture</source>
        <translation>截图后自动保存</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="145"/>
        <source>Browse</source>
        <translation>浏览</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="147"/>
        <source>Save Settings</source>
        <translation>保存设置</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="148"/>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="291"/>
        <source>Restore Defaults</source>
        <translation>恢复默认</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="151"/>
        <source>System</source>
        <translation>跟随系统</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="151"/>
        <source>Light</source>
        <translation>浅色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="151"/>
        <source>Dark</source>
        <translation>深色</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="155"/>
        <source>Auto (System Default)</source>
        <translation>自动（系统默认）</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="156"/>
        <source>English</source>
        <translation>英语</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="157"/>
        <source>Chinese (Simplified)</source>
        <translation>简体中文</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="199"/>
        <source>Save directory</source>
        <translation>保存目录</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="200"/>
        <source>Image format</source>
        <translation>图片格式</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="201"/>
        <source>Theme</source>
        <translation>主题</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="202"/>
        <source>Language</source>
        <translation>语言</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="203"/>
        <source>OCR language</source>
        <translation>OCR 语言</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="205"/>
        <source>Capture hotkey</source>
        <translation>截图快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="206"/>
        <source>Paste hotkey</source>
        <translation>贴图快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="207"/>
        <source>Hide pins hotkey</source>
        <translation>隐藏贴图快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="208"/>
        <source>Repeat capture hotkey</source>
        <translation>重复截图快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="221"/>
        <source>Choose capture directory</source>
        <translation>选择截图保存目录</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="228"/>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="258"/>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="265"/>
        <source>Validation</source>
        <translation>校验</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="228"/>
        <source>Save directory cannot be empty.</source>
        <translation>保存目录不能为空</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="239"/>
        <source>Capture</source>
        <translation>截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="239"/>
        <source>Paste</source>
        <translation>贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="239"/>
        <source>Hide Pins</source>
        <translation>隐藏贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="239"/>
        <source>Repeat Capture</source>
        <translation>重复截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="247"/>
        <source>Hotkey Conflict</source>
        <translation>快捷键冲突</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="248"/>
        <source>&quot;%1&quot; and &quot;%2&quot; have the same shortcut.</source>
        <translation>“%1”和“%2”使用了相同的快捷键</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="259"/>
        <source>Save directory does not exist and could not be created.</source>
        <translation type="unfinished">保存目录不存在且无法创建</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="266"/>
        <source>Save directory is not writable.</source>
        <translation type="unfinished">保存目录不可写</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="286"/>
        <source>Language Changed</source>
        <translation type="unfinished">语言已更改</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="287"/>
        <source>The language change will take effect after you restart the application.</source>
        <translation type="unfinished">语言更改将在重启应用后生效</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="292"/>
        <source>Reset all settings to their default values?</source>
        <translation>将所有设置恢复为默认值？</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="316"/>
        <source>Saved [OK]</source>
        <translation>保存成功</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="323"/>
        <source>Settings saved successfully</source>
        <translation>设置已保存</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="327"/>
        <source>Error: </source>
        <translation>错误：</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="150"/>
        <source>png</source>
        <translation>PNG</translation>
    </message>
    <message>
        <location filename="../../src/presentation/settings/SettingsWidget.cpp" line="150"/>
        <source>jpg</source>
        <translation>JPEG</translation>
    </message>
</context>
<context>
    <name>snappaste::TrayController</name>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="15"/>
        <source>Capture</source>
        <translation>截图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="16"/>
        <source>Open Image...</source>
        <translation>打开图片...</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="17"/>
        <source>Open SnapPaste</source>
        <translation>打开 SnapPaste</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="18"/>
        <source>Hide Pins</source>
        <translation>隐藏贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="19"/>
        <source>Show Pins</source>
        <translation>显示贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="20"/>
        <source>Close All Pins</source>
        <translation>关闭所有贴图</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="22"/>
        <source>Quit</source>
        <translation>退出</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="25"/>
        <source>SnapPaste</source>
        <translation>SnapPaste</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="35"/>
        <source>Quit SnapPaste</source>
        <translation>退出 SnapPaste</translation>
    </message>
    <message>
        <location filename="../../src/presentation/tray/TrayController.cpp" line="36"/>
        <source>Are you sure you want to quit?
Pinned images will be lost.</source>
        <translation>确定要退出吗？
所有贴图将会丢失。</translation>
    </message>
</context>
</TS>
