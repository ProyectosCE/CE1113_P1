cmake_minimum_required(VERSION 3.16)
project(aurabot VERSION 1.0.0 LANGUAGES C)

find_path(PWM_INCLUDE_DIR libpwm.h REQUIRED)
find_library(PWM_LIBRARY pwm REQUIRED)

add_executable(aurabot src/main.c)
target_include_directories(aurabot PRIVATE ${PWM_INCLUDE_DIR})
target_link_libraries(aurabot PRIVATE ${PWM_LIBRARY})

install(TARGETS aurabot DESTINATION bin)
