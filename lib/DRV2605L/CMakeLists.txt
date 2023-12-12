add_library(DRV2605L DRV2605L.cpp)
target_link_libraries(DRV2605L PUBLIC PicoPeripherals)
target_include_directories(DRV2605L INTERFACE .)
target_include_directories(DRV2605L PUBLIC . PicoPeripherals)
