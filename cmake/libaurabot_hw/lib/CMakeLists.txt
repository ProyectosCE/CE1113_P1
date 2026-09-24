set(LIB_NAME aurabot_hw)

set(SRC_FILES aurabot_hw.c motor.c sensor.c led.c audio.c)
set(INC_FILES ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_hw.h ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_hw_types.h ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_motor.h ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_sensor.h ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_led.h ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_audio.h)

add_library(${LIB_NAME} SHARED ${SRC_FILES} ${INC_FILES})

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include PRIVATE ${CMAKE_SOURCE_DIR}/libgpio/include)
target_link_libraries(${LIB_NAME} PRIVATE gpio)

install(TARGETS ${LIB_NAME} DESTINATION lib)
install(FILES ${INC_FILES} DESTINATION include)