add_library(httpd
fs.c
fsdata.c
${PICO_SDK_PATH}/lib/lwip/src/apps/http/httpd.c
)
target_include_directories(httpd INTERFACE 
.
)

# Conditional lwIP linkage:
# - Wireless boards: lwIP provided by pico_cyw43_arch_lwip_threadsafe_background (via lwip-port)
# - Non-wireless boards: link pico_lwip directly
if(PICO_CYW43_SUPPORTED)
    target_link_libraries(httpd
        pico_stdlib
        lwip-port
    )
    # Need lwIP headers for fs.c, fsdata.c, and httpd.c compilation
    target_include_directories(httpd PRIVATE
        ${PICO_SDK_PATH}/lib/lwip/src/include
        ${PICO_SDK_PATH}/lib/lwip/src/include/lwip/apps
        ${CMAKE_CURRENT_LIST_DIR}/../lwip-port
        ${CMAKE_CURRENT_LIST_DIR}/../lwip-port/arch
    )
else()
    target_link_libraries(httpd
        pico_stdlib
        lwip-port
        pico_lwip
    )
endif()