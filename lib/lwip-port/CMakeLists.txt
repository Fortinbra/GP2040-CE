add_library(lwip-port
${PICO_SDK_PATH}/lib/tinyusb/lib/networking/dhserver.c
${PICO_SDK_PATH}/lib/tinyusb/lib/networking/dnserver.c
)
target_include_directories(lwip-port INTERFACE 
.
arch
${PICO_SDK_PATH}/lib/tinyusb/lib/networking
)

# Conditional lwIP linkage:
# - Wireless boards (PICO_CYW43_SUPPORTED): lwIP provided by pico_cyw43_arch_lwip_threadsafe_background
# - Non-wireless boards: use standalone pico_lwip
if(PICO_CYW43_SUPPORTED)
    # CYW43 arch already provides full lwIP — just link stdlib and add lwIP includes
    target_link_libraries(lwip-port 
        pico_stdlib
    )
    # Need lwIP headers for dhserver/dnserver compilation
    target_include_directories(lwip-port PRIVATE
        .
        arch
        ${PICO_SDK_PATH}/lib/lwip/src/include
    )
else()
    # Non-wireless: use standalone lwIP
    target_link_libraries(lwip-port 
        pico_stdlib
        pico_lwip
    )
    target_include_directories(pico_lwip INTERFACE
        .
    )
endif()