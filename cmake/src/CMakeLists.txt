include_directories(${CMAKE_SOURCE_DIR}/libdriver/include)

add_executable(main main.c)



target_link_libraries(main driver)


install (TARGETS main DESTINATION bin)