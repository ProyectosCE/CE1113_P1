cmake_minimum_required(VERSION 3.16)
project(aurabot_api VERSION 1.0.0 LANGUAGES C)

set(AURABOT_RUNTIME_DIRECTORY_OVERRIDE "" CACHE STRING
    "Directorio de ejecución alternativo para pruebas")

add_library(aurabot SHARED src/aurabot.c)
set_target_properties(aurabot PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_include_directories(aurabot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(aurabot PRIVATE -Wall -Wextra -Werror)
if(AURABOT_RUNTIME_DIRECTORY_OVERRIDE)
    target_compile_definitions(aurabot PRIVATE
        AURABOT_RUNTIME_DIRECTORY="${AURABOT_RUNTIME_DIRECTORY_OVERRIDE}")
endif()

add_executable(aurabotctl tools/aurabotctl.c)
target_include_directories(aurabotctl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(aurabotctl PRIVATE aurabot)
target_compile_options(aurabotctl PRIVATE -Wall -Wextra -Werror)

install(TARGETS aurabot LIBRARY DESTINATION lib)
install(TARGETS aurabotctl RUNTIME DESTINATION bin)
install(FILES include/aurabot.h include/aurabot_protocol.h DESTINATION include)
