add_library(pwm SHARED lib/libpwm.c)

target_include_directories(pwm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS pwm LIBRARY DESTINATION lib)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/libpwm.h" DESTINATION include)