include_directories (${CMAKE_SOURCE_DIR}/libdriver/include)
set (LIB_NAME driver )
set ( INC_FILE ${CMAKE_SOURCE_DIR}/libdriver/include/libdriver.h)



add_library(driver SHARED libdriver.c ${INC_FILE})


target_include_directories(driver PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/libgpio/include
)

target_link_libraries(driver PUBLIC gpio)


install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${INC_FILE} DESTINATION include)