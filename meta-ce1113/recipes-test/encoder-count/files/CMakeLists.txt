cmake_minimum_required(VERSION 3.16)
project(encoder_count VERSION 1.0.0 LANGUAGES C)

find_path(GPIO_INCLUDE_DIR libgpio.h REQUIRED)
find_library(GPIO_LIBRARY gpio REQUIRED)

add_executable(encoder_count encoder_count.c)
target_include_directories(encoder_count PRIVATE ${GPIO_INCLUDE_DIR})
target_link_libraries(encoder_count PRIVATE ${GPIO_LIBRARY})
target_compile_definitions(encoder_count PRIVATE _POSIX_C_SOURCE=200809L)
target_compile_options(encoder_count PRIVATE -Wall -Wextra -Werror)

install(TARGETS encoder_count DESTINATION bin)
