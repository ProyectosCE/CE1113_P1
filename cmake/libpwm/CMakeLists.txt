add_library(pwm SHARED lib/libpwm.c)

target_include_directories(pwm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(pwm PRIVATE sysfs_io)

install(TARGETS pwm LIBRARY DESTINATION lib)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/libpwm.h" DESTINATION include)