include_directories(${CMAKE_SOURCE_DIR}/libaurabot_hw/include)
include_directories(${CMAKE_SOURCE_DIR}/libgpio/include)
include_directories(${CMAKE_SOURCE_DIR}/libaudio/include)

add_executable(main main.c)



target_link_libraries(main aurabot_hw gpio audio)

install (TARGETS main DESTINATION bin)