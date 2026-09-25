cmake_minimum_required(VERSION 3.10)

project(aurabot_hardware_test C)

set(AURABOT_CMAKE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

add_subdirectory("${AURABOT_CMAKE_ROOT}/common" "${CMAKE_CURRENT_BINARY_DIR}/common")

add_subdirectory("${AURABOT_CMAKE_ROOT}/libpwm" "${CMAKE_CURRENT_BINARY_DIR}/libpwm")

add_subdirectory("${AURABOT_CMAKE_ROOT}/libgpio" "${CMAKE_CURRENT_BINARY_DIR}/libgpio")

# Optional sysfs directory for testing without physical hardware.
if(DEFINED GPIO_SYSFS_TEST_ROOT AND
   NOT GPIO_SYSFS_TEST_ROOT STREQUAL "")

    target_compile_definitions(
        gpio PRIVATE
        "GPIO_SYSFS_PATH=\"${GPIO_SYSFS_TEST_ROOT}\""
    )
endif()

add_executable(test_ir_led test_ir_led.c)

target_link_libraries(test_ir_led PRIVATE gpio)