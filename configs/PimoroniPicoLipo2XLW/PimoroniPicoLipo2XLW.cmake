# References the in-tree Pimoroni board header at configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h.
# PICO_BOARD_HEADER_DIRS is already pointed at this config folder by the root CMakeLists.txt.
set(PICO_BOARD pimoroni_pico_lipo2xl_w)
set(PICO_PLATFORM rp2350-arm-s)

# RP2350B has 48 GPIOs; required for GP43 (battery ADC) to be addressable.
set(PICO_NUM_GPIOS 48)
