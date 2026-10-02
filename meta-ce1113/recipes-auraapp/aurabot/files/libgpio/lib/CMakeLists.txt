include_directories (${CMAKE_SOURCE_DIR}/libgpio/include)
set (LIB_NAME gpio )
set ( INC_FILE ${CMAKE_SOURCE_DIR}/libgpio/include/libgpio.h)

add_library(${LIB_NAME} SHARED libgpio.c ${INC_FILE})
set_target_properties(${LIB_NAME} PROPERTIES VERSION 1.0.0 SOVERSION 1)

find_package(Threads REQUIRED)


target_link_libraries(${LIB_NAME} PRIVATE Threads::Threads)


install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${INC_FILE} DESTINATION include)
