include_directories(${CMAKE_SOURCE_DIR}/libaurabot_hw/include)
include_directories(${CMAKE_SOURCE_DIR}/libgpio/include)

add_executable(aurabot main.c)

target_link_libraries(aurabot aurabot_hw gpio)

install(TARGETS aurabot DESTINATION bin)
