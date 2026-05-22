# References the in-tree Pimoroni board header at configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h.
# PICO_BOARD_HEADER_DIRS is already pointed at this config folder by the root CMakeLists.txt.
set(PICO_BOARD pimoroni_pico_lipo2xl_w)
set(PICO_PLATFORM rp2350-arm-s)

# RP2350B has 48 GPIOs; required for GP43 (battery ADC) to be addressable.
set(PICO_NUM_GPIOS 48)

# Relocate the FlashPROM EEPROM region to the top of the 16 MB flash.
# The legacy default at 0x101F8000 (~2 MB - 32 KB into flash) is erased by the
# RP2350 boot/partition path on boards that declare > 4 MB of flash.
# See docs/development/flashprom-large-flash-support.md.
#   XIP_BASE (0x10000000) + 16 MB - 32 KB = 0x10FF8000
add_compile_definitions(EEPROM_ADDRESS_START=0x10FF8000U)
