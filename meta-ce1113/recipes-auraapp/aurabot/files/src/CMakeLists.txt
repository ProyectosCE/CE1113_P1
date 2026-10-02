include_directories(${CMAKE_SOURCE_DIR}/libaurabot_hw/include)
include_directories(${CMAKE_SOURCE_DIR}/libgpio/include)

add_executable(aurabot main.c)
add_executable(aurabot-audio-server aurabot-audio-server.c)

target_link_libraries(aurabot aurabot_hw gpio)
target_link_libraries(aurabot-audio-server aurabot_hw)

install(TARGETS aurabot aurabot-audio-server DESTINATION bin)
