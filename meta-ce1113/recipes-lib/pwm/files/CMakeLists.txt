cmake_minimum_required(VERSION 3.16)
project(aurabot_pwm VERSION 1.0.0 LANGUAGES C)

find_package(Threads REQUIRED)
find_path(GPIO_INCLUDE_DIR libgpio.h REQUIRED)
find_library(GPIO_LIBRARY gpio REQUIRED)
add_library(pwm SHARED src/libpwm.c)
set_target_properties(pwm PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_include_directories(pwm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${GPIO_INCLUDE_DIR})
target_link_libraries(pwm PRIVATE ${GPIO_LIBRARY} Threads::Threads)

install(TARGETS pwm LIBRARY DESTINATION lib)
install(FILES include/libpwm.h DESTINATION include)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_pwm
        tests/test_pwm.c
        tests/fake_gpio.c
        src/libpwm.c
    )
    target_include_directories(test_pwm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${GPIO_INCLUDE_DIR}
    )
    target_link_libraries(test_pwm PRIVATE Threads::Threads)
    add_test(NAME pwm_transitions_and_recovery COMMAND test_pwm)
endif()
