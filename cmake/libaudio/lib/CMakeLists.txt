include_directories (${CMAKE_SOURCE_DIR}/libaudio/include)
set (LIB_NAME audio )
set ( INC_FILE ${CMAKE_SOURCE_DIR}/libaudio/include/libaudio.h)

add_library(${LIB_NAME} SHARED libaudio.c ${INC_FILE})



#target_link_libraries(${LIB_NAME} PRIVATE m)

install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${INC_FILE} DESTINATION include)