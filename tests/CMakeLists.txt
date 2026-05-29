find_package(Qt5 REQUIRED COMPONENTS Test)

add_executable(SnapPasteUnitTests
    unit/SnapPasteTests.cpp
)

target_link_libraries(SnapPasteUnitTests PRIVATE SnapPasteCore Qt5::Test)
target_include_directories(SnapPasteUnitTests PRIVATE ${PROJECT_SOURCE_DIR})

add_test(NAME SnapPasteUnitTests COMMAND SnapPasteUnitTests)

add_executable(SnapPasteIntegrationTests
    integration/SnapPasteIntegrationTests.cpp
)

target_link_libraries(SnapPasteIntegrationTests PRIVATE SnapPasteCore Qt5::Test)
target_include_directories(SnapPasteIntegrationTests PRIVATE ${PROJECT_SOURCE_DIR})

add_test(NAME SnapPasteIntegrationTests COMMAND SnapPasteIntegrationTests)
