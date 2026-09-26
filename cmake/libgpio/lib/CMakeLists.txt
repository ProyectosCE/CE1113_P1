add_library(gpio SHARED libgpio.c gpio_sysfs.c)

target_include_directories(gpio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(gpio PRIVATE sysfs_io pwm)

install(TARGETS gpio LIBRARY DESTINATION lib)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../include/libgpio.h" DESTINATION include)