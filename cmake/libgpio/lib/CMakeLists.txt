include_directories (${CMAKE_SOURCE_DIR}/libgpio/include)
set (LIB_NAME gpio )
set ( INC_FILE ${CMAKE_SOURCE_DIR}/libgpio/include/libgpio.h)

add_library(${LIB_NAME} SHARED libgpio.c ${INC_FILE})



#target_link_libraries(${LIB_NAME} PRIVATE m)

install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${INC_FILE} DESTINATION include)