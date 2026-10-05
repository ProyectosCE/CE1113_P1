cmake_minimum_required(VERSION 3.16)
project(aurabot VERSION 1.0.0 LANGUAGES C)

option(AURABOT_FAKE_HARDWARE "Construir con hardware simulado" OFF)
set(AURABOT_RUNTIME_DIRECTORY_OVERRIDE "" CACHE STRING
    "Directorio de ejecución alternativo para pruebas")

find_package(Threads REQUIRED)
find_path(AURABOT_API_INCLUDE_DIR aurabot.h REQUIRED)

set(AURABOT_SOURCES
    src/main.c
    src/fsm.c
    src/autonomous.c
    src/map.c
    src/ipc_server.c
)

if(AURABOT_FAKE_HARDWARE)
    list(APPEND AURABOT_SOURCES tests/fake_hardware.c)
else()
    find_path(PWM_INCLUDE_DIR libpwm.h REQUIRED)
    find_library(PWM_LIBRARY pwm REQUIRED)
    find_path(GPIO_INCLUDE_DIR libgpio.h REQUIRED)
    find_library(GPIO_LIBRARY gpio REQUIRED)
    find_path(SENSORS_INCLUDE_DIR libsensors.h REQUIRED)
    find_library(SENSORS_LIBRARY sensors REQUIRED)
    find_path(LEDS_INCLUDE_DIR libleds.h REQUIRED)
    find_library(LEDS_LIBRARY leds REQUIRED)
    list(APPEND AURABOT_SOURCES src/hardware.c)
endif()

add_executable(aurabot ${AURABOT_SOURCES})
target_include_directories(aurabot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${AURABOT_API_INCLUDE_DIR}
)
target_link_libraries(aurabot PRIVATE Threads::Threads)
target_compile_definitions(aurabot PRIVATE _GNU_SOURCE)
target_compile_options(aurabot PRIVATE -Wall -Wextra -Werror)
if(AURABOT_RUNTIME_DIRECTORY_OVERRIDE)
    target_compile_definitions(aurabot PRIVATE
        AURABOT_RUNTIME_DIRECTORY="${AURABOT_RUNTIME_DIRECTORY_OVERRIDE}")
endif()

if(NOT AURABOT_FAKE_HARDWARE)
    target_include_directories(aurabot PRIVATE
        ${PWM_INCLUDE_DIR}
        ${GPIO_INCLUDE_DIR}
        ${SENSORS_INCLUDE_DIR}
        ${LEDS_INCLUDE_DIR}
    )
    target_link_libraries(aurabot PRIVATE
        ${PWM_LIBRARY}
        ${GPIO_LIBRARY}
        ${SENSORS_LIBRARY}
        ${LEDS_LIBRARY}
    )
endif()

install(TARGETS aurabot DESTINATION bin)

include(CTest)
if(BUILD_TESTING AND AURABOT_FAKE_HARDWARE)
    add_executable(test_core
        tests/test_core.c
        tests/fake_hardware.c
        src/fsm.c
        src/autonomous.c
        src/map.c
    )
    target_include_directories(test_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${AURABOT_API_INCLUDE_DIR}
    )
    target_link_libraries(test_core PRIVATE Threads::Threads)
    target_compile_options(test_core PRIVATE -Wall -Wextra -Werror)
    add_test(NAME aurabot_core_fsm COMMAND test_core)

    add_executable(test_map tests/test_map.c src/map.c)
    target_include_directories(test_map PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${AURABOT_API_INCLUDE_DIR}
    )
    target_compile_options(test_map PRIVATE -Wall -Wextra -Werror)
    add_test(NAME aurabot_map COMMAND test_map)
endif()
