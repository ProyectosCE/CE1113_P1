add_library(sysfs_io STATIC sysfs_io.c)

set_target_properties(sysfs_io PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(sysfs_io PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")