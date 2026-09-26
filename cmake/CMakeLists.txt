cmake_minimum_required (VERSION 3.10)

project(aurabot VERSION 1.0.0 LANGUAGES C)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_INSTALL_RPATH "/lib;/usr/lib")

add_subdirectory(common)
add_subdirectory(libpwm)
add_subdirectory(libgpio)
add_subdirectory(libaudio)
add_subdirectory(libaurabot_hw)
add_subdirectory(src)

# Empaquetado con CPack

set(CPACK_SOURCE_GENERATOR "TGZ")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}")
set (CPACK_SOURCE_IGNORE_FILES "/build/;${CPACK_SOURCE_IGNORE_FILES}")


include(CPack)