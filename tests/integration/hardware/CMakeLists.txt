
cmake_minimum_required(VERSION 3.10)

project(aurabot_hardware_test C)

# Compile the real libgpio source without altering the main CMake project.
set(GPIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/libgpio")

add_library(gpio SHARED "${GPIO_ROOT}/lib/libgpio.c")

target_include_directories(gpio PUBLIC "${GPIO_ROOT}/include")

add_executable(test_ir_led test_ir_led.c)

target_link_libraries(test_ir_led PRIVATE gpio)
