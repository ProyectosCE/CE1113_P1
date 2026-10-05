cmake_minimum_required(VERSION 3.16)
project(prueba_encoder VERSION 1.0.0 LANGUAGES C)

find_path(GPIO_INCLUDE_DIR libgpio.h REQUIRED)
find_library(GPIO_LIBRARY gpio REQUIRED)

add_executable(prueba_encoder prueba_encoder.c)
target_include_directories(prueba_encoder PRIVATE ${GPIO_INCLUDE_DIR})
target_link_libraries(prueba_encoder PRIVATE ${GPIO_LIBRARY})
target_compile_definitions(prueba_encoder PRIVATE _POSIX_C_SOURCE=200809L)
target_compile_options(prueba_encoder PRIVATE -Wall -Wextra -Werror)

install(TARGETS prueba_encoder DESTINATION bin)
