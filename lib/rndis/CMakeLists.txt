add_library(rndis
rndis.c
${PICO_SDK_PATH}/lib/tinyusb/lib/networking/rndis_reports.c
)
target_include_directories(rndis PUBLIC 
${CMAKE_CURRENT_LIST_DIR}
${CMAKE_SOURCE_DIR}/headers
${PICO_SDK_PATH}/lib/tinyusb/lib/networking
)

# Conditional lwIP includes for CYW43 boards
if(PICO_CYW43_SUPPORTED)
    target_compile_definitions(rndis PRIVATE
        PICO_CYW43_SUPPORTED=1
    )
    target_include_directories(rndis PRIVATE
        ${PICO_SDK_PATH}/lib/lwip/src/include
        ${CMAKE_CURRENT_LIST_DIR}/../lwip-port
        ${CMAKE_CURRENT_LIST_DIR}/../lwip-port/arch
    )
endif()

target_link_libraries(rndis 
pico_stdlib
tinyusb_device
lwip-port
httpd
)

