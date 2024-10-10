cmake_minimum_required(VERSION 3.12)

set(NUMBERS "Audio")

if (NOT DEFINED APPLICATION_VERSION)
  set(APPLICATION_VERSION "\"LOCAL_DEBUG_BUILD\"")
endif()

foreach(MY_TARGET IN LISTS NUMBERS)
    message("BUILDING ${MY_TARGET}")
    add_executable(${MY_TARGET}
        main.c
        usb_descriptors.c
        usb_microphone.c
        commandline.c
        embedded_cli.c
        debug_pulse.c
        usb_sync.c
    )
    target_compile_definitions(${MY_TARGET} PRIVATE PRODUCT_DESCRIPTION="Zip_${MY_TARGET}" APPLICATION_VERSION=${APPLICATION_VERSION})
    target_include_directories(${MY_TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(${MY_TARGET} PRIVATE tinyusb_device tinyusb_board pico_pdm_microphone hardware_flash pico_unique_id)
    pico_add_extra_outputs(${MY_TARGET})
endforeach()
