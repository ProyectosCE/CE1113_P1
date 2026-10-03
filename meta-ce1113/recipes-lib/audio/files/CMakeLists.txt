cmake_minimum_required(VERSION 3.16)
project(aurabot_audio VERSION 1.0.0 LANGUAGES C)

add_library(audio SHARED src/libaudio.c)
set_target_properties(audio PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_include_directories(audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(TARGETS audio LIBRARY DESTINATION lib)
install(FILES include/libaudio.h DESTINATION include)
