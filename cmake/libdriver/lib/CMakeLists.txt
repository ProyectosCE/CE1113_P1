include_directories (${CMAKE_SOURCE_DIR}/include)
set (LIB_NAME driver )
set ( INC_FILE ${CMAKE_SOURCE_DIR}/include/libdriver.h)

add_library(${LIB_NAME} SHARED libdriver.c ${INC_FILE})



#target_link_libraries(${LIB_NAME} PRIVATE m)

install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${INC_FILE} DESTINATION include)