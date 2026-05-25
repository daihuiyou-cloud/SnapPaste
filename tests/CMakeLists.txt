find_package(Qt5 REQUIRED COMPONENTS Test)

add_executable(NanoSnapUnitTests
    unit/NanoSnapTests.cpp
)

target_link_libraries(NanoSnapUnitTests PRIVATE NanoSnapCore Qt5::Test)

add_test(NAME NanoSnapUnitTests COMMAND NanoSnapUnitTests)
