cmake_minimum_required(VERSION 3.10)

project(operaciones-cgi C)

add_executable(operaciones.cgi
	operaciones.cgi.c
	json_response.c
)

target_include_directories(operaciones.cgi PRIVATE
    ${CMAKE_SYSROOT}/usr/include
)

target_link_libraries(operaciones.cgi PRIVATE
    m
)

install(TARGETS operaciones.cgi
    DESTINATION /www/cgi-bin
)
