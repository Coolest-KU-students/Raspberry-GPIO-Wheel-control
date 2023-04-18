cmake_minimum_required(VERSION 3.12)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(PicoSensorControl2 C CXX ASM)
pico_sdk_init()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_executable(adc_console
        adc_console.c
        )

target_link_libraries(adc_console pico_stdlib hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(adc_console)

# add url via pico_set_program_url
# example_auto_set_url(adc_console)

