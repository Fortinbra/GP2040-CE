#ifndef GP2040_PIMORONI_LIPO2XLW_H
#define GP2040_PIMORONI_LIPO2XLW_H

#include "boards/pico2_w.h"

#undef PICO_RP2350A
#define PICO_RP2350A 0

#if PICO_FLASH_SIZE_BYTES != (4 * 1024 * 1024)
#error "Pimoroni Pico Lipo 2 XL W requires the existing 4 MiB flash safety limit"
#endif

#endif