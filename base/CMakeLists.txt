cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(base_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(base
    main.c
    usb_descriptors.c
    app/app.c
    app/event_dispatcher.c
    app/mainloop_timer.c
    app/soft_timer.c
    app/sys_timer.c
    app/ws2812.c
    app/misc.c
    app/circ_buffer.c
    app/shell.c
    app/shell_if_usb.c
)

pico_generate_pio_header(base ${CMAKE_CURRENT_LIST_DIR}/app/led.pio)
pico_generate_pio_header(base ${CMAKE_CURRENT_LIST_DIR}/app/ws2812.pio)


target_include_directories(base PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(base 0)
pico_enable_stdio_uart(base 0)

pico_add_extra_outputs(base)

target_link_libraries(base
    pico_stdlib
    cmsis_core
    tinyusb_device
    tinyusb_board
    hardware_pio
    hardware_irq
    hardware_dma
)
