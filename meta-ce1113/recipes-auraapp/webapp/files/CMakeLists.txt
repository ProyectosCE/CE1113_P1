cmake_minimum_required(VERSION 3.10)

project(operaciones-cgi C)
find_path(AURABOT_INCLUDE_DIR aurabot.h REQUIRED)
find_library(AURABOT_LIBRARY aurabot REQUIRED)

add_executable(operaciones.cgi
	operaciones.cgi.c
	json_response.c
)

target_include_directories(operaciones.cgi PRIVATE
    ${CMAKE_SYSROOT}/usr/include
    ${AURABOT_INCLUDE_DIR}
)

target_link_libraries(operaciones.cgi PRIVATE
    m
    ${AURABOT_LIBRARY}
)

install(TARGETS operaciones.cgi
    DESTINATION /www/cgi-bin
)
