cmake_minimum_required(VERSION 3.16)

project(aurabot VERSION 1.0.0 LANGUAGES C)

add_subdirectory(libgpio)
add_subdirectory(libaudio)
add_subdirectory(libaurabot_hw)
add_subdirectory(src)

# Empaquetado con CPack

set(CPACK_SOURCE_GENERATOR "TGZ")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}")
set (CPACK_SOURCE_IGNORE_FILES "/build/;${CPACK_SOURCE_IGNORE_FILES}")


include(CPack)
