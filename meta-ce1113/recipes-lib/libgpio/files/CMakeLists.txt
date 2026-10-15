cmake_minimum_required(VERSION 3.16)
project(aurabot_gpio VERSION 1.0.0 LANGUAGES C)

add_library(gpio SHARED src/libgpio.c)
set_target_properties(gpio PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_include_directories(gpio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
install(TARGETS gpio LIBRARY DESTINATION lib)
install(FILES include/libgpio.h DESTINATION include)
