cmake_minimum_required(VERSION 3.10)

project(aurabot_hardware_test C)

set(AURABOT_CMAKE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

add_subdirectory("${AURABOT_CMAKE_ROOT}/common" "${CMAKE_CURRENT_BINARY_DIR}/common")

add_subdirectory("${AURABOT_CMAKE_ROOT}/libpwm" "${CMAKE_CURRENT_BINARY_DIR}/libpwm")

add_subdirectory("${AURABOT_CMAKE_ROOT}/libgpio" "${CMAKE_CURRENT_BINARY_DIR}/libgpio")

# Optional GPIO sysfs simulation.
if(DEFINED GPIO_SYSFS_TEST_ROOT AND NOT GPIO_SYSFS_TEST_ROOT STREQUAL "")
    target_compile_definitions(gpio PRIVATE "GPIO_SYSFS_PATH=\"${GPIO_SYSFS_TEST_ROOT}\"")
endif()

# Optional PWM sysfs simulation.
if(DEFINED PWM_SYSFS_TEST_ROOT AND NOT PWM_SYSFS_TEST_ROOT STREQUAL "")
    target_compile_definitions(pwm PRIVATE "PWM_SYSFS_ROOT=\"${PWM_SYSFS_TEST_ROOT}\"")
endif()

add_executable(test_ir_led test_ir_led.c)

target_link_libraries(test_ir_led PRIVATE gpio)

add_executable(test_pwm test_pwm.c)

target_link_libraries(test_pwm PRIVATE gpio pwm)

if(DEFINED PWM_SYSFS_TEST_ROOT AND NOT PWM_SYSFS_TEST_ROOT STREQUAL "")
    target_compile_definitions(test_pwm PRIVATE "PWM_TEST_SYSFS_ROOT=\"${PWM_SYSFS_TEST_ROOT}\"")
endif()