find_package(Qt5 REQUIRED COMPONENTS Test)

add_executable(SnapPasteUnitTests
    unit/SnapPasteTests.cpp
)

target_link_libraries(SnapPasteUnitTests PRIVATE SnapPasteCore Qt5::Test)

add_test(NAME SnapPasteUnitTests COMMAND SnapPasteUnitTests)
