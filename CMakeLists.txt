cmake_minimum_required(VERSION 3.16)

project(SnapPaste VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql)

set(SNAPPASTE_CORE_SOURCES
    src/shared/events/EventHub.h
    src/shared/screen/ScreenSegmentUtil.h

    src/app/Application.cpp
    src/app/AppContext.cpp
    src/app/AppStartup.cpp

    src/domain/capture/CaptureWorkflow.cpp
    src/domain/capture/CaptureSelectionHistory.cpp
    src/domain/editor/AnnotationDocument.cpp
    src/domain/history/HistoryService.cpp
    src/domain/pin/PinnedImageService.cpp
    src/domain/settings/SettingsService.cpp

    src/infrastructure/clipboard/ClipboardImageProvider.cpp
    src/infrastructure/config/JsonSettingsRepository.cpp
    src/infrastructure/filesystem/AppPaths.cpp
    src/infrastructure/image/LocalImageStorage.cpp
    src/infrastructure/logging/Logger.cpp
    src/infrastructure/persistence/SqliteConnection.cpp
    src/infrastructure/persistence/SqliteHistoryRepository.cpp
    src/infrastructure/persistence/SqliteMigrator.cpp
    src/infrastructure/persistence/SqlitePinnedItemRepository.cpp

    src/platform/windows/capture/DxgiScreenCaptureService.cpp
    src/platform/windows/capture/GdiScreenCaptureService.cpp
    src/platform/windows/capture/QtScreenPixelSampler.cpp
    src/platform/windows/capture/WinScreenRegionDetector.cpp
    src/platform/windows/darkmode/DarkModeDetector.cpp
    src/platform/windows/dpi/HighDpiManager.cpp
    src/platform/windows/hotkey/WinHotkeyService.cpp
    src/platform/windows/shell/WindowsShellService.cpp
    src/platform/windows/window/WindowInteractionService.cpp

    src/presentation/capture_actions/CaptureActionBar.cpp
    src/presentation/capture_overlay/CaptureOverlay.cpp
    src/presentation/editor/EditorWindow.cpp
    src/presentation/history/HistoryWidget.cpp
    src/presentation/icons/IconProvider.cpp
    src/presentation/main_window/MainWindow.cpp
    src/presentation/settings/SettingsWidget.cpp
    src/presentation/toast/ToastNotifier.cpp
    src/presentation/tray/TrayController.cpp
    src/presentation/viewmodels/CaptureViewModel.cpp
    src/presentation/viewmodels/HistoryViewModel.cpp
    src/presentation/viewmodels/PinViewModel.cpp
    src/presentation/viewmodels/SettingsViewModel.cpp
    src/infrastructure/ocr/WindowsOcrService.cpp
    src/presentation/ocr/OcrResultWindow.cpp
    src/presentation/pin_window/PinWindow.cpp
)

add_library(SnapPasteCore STATIC ${SNAPPASTE_CORE_SOURCES})

target_include_directories(SnapPasteCore PUBLIC src)

target_link_libraries(SnapPasteCore PUBLIC Qt5::Widgets Qt5::Sql)

add_executable(SnapPaste WIN32
    resources/qrc/resources.qrc
    src/app/main.cpp
)

target_link_libraries(SnapPaste PRIVATE SnapPasteCore)

if (WIN32)
    target_compile_definitions(SnapPasteCore PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(SnapPasteCore PUBLIC user32 gdi32 shell32 shcore dwmapi ole32 oleaut32 uiautomationcore d3d11 dxgi)
endif()

option(SNAPPASTE_BUILD_TESTS "Build SnapPaste tests" OFF)

if (SNAPPASTE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
