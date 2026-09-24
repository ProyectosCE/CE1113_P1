set(LIB_NAME aurabot_hw)
set(INC_FILE ${CMAKE_SOURCE_DIR}/libaurabot_hw/include/aurabot_hw.h)

add_library(${LIB_NAME} SHARED aurabot_hw.c ${INC_FILE})

target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include PRIVATE ${CMAKE_SOURCE_DIR}/libgpio/include)
target_link_libraries(${LIB_NAME} PRIVATE gpio)

install(TARGETS ${LIB_NAME} DESTINATION lib)
install(FILES ${INC_FILE} DESTINATION include)