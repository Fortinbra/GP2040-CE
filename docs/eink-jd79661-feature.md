# Feature: Adafruit 2.13" Quad-Color eInk Display (JD79661 / SRAM)

**Branch:** `copilot/eink-jd79661-display`  
**Target Display:** Adafruit 2.13" 250×122 Quad-Color eInk/ePaper Display w/ SRAM  
**Controller IC:** JD79661  
**Interface:** SPI (display) + SPI (SRAM, Microchip 23K256 or equivalent)

---

## 1. Display Overview

| Property | Value |
|---|---|
| Resolution | 250 × 122 pixels |
| Colors | 4 (black, white, red, yellow — quad-color) |
| Controller | JD79661 |
| Primary interface | SPI (up to ~20 MHz) |
| SRAM | Separate SPI device (same bus, separate CS) |
| Control pins | MOSI, SCK, CS (display), CS (SRAM), DC, RST, BUSY |
| Refresh type | Full refresh (~15 s for color) / partial fast refresh (black+white only) |
| Supply voltage | 3.3 V compatible |

The SRAM chip (Microchip 23K256 / 23LC1024 or equivalent) provides 256 Kbit–1 Mbit of serial RAM connected on the **same SPI bus** as the display, with a dedicated Chip Select line. It is used to buffer the full-frame pixel data off the RP2040's limited SRAM.

---

## 2. Current SPI Bus Architecture

### 2.1 PeripheralSPI (`lib/PicoPeripherals/peripheral_spi.h`)

The existing driver wraps both RP2040 SPI hardware blocks (SPI0 / SPI1).

**What it provides:**
- `setConfig(block, tx, rx, sck, cs)` — configure pins from `PeripheralOptions`
- `beginTransaction(speedMHz, bitOrder, SPIMode)` — set clock/mode per-transfer
- `transfer(tx, rx, count)` — bulk DMA-backed transfer (2 KB DMA buffers per direction)
- `transfer(uint8_t)` / `transfer16(uint16_t)` — single byte/word
- `select(cs)` / `deselect()` — software CS management

**What it lacks for this display:**
| Gap | Details |
|---|---|
| Single CS pin | `setConfig` accepts only **one** CS pin per block; the JD79661 + SRAM share a bus but need **two independent CS lines** |
| No DC (Data/Command) pin | The JD79661 requires a dedicated Data/Command GPIO to distinguish command vs. data bytes; this pin does not exist in the current peripheral model |
| No RST pin | Hard-reset of the display controller requires a dedicated GPIO; not modelled |
| No BUSY polling pin | The JD79661 asserts BUSY during refresh; the host must wait before sending new commands; no polling support exists |
| Speed stored in `PeripheralOptions` | `SPIOptions` has no speed field — speed is set per-transaction but there is no persistent speed stored in config |
| No multi-CS arbitration | `select()` keeps one CS low at a time, but there is no coordination layer for two devices sharing the same bus (display vs. SRAM) |

### 2.2 PeripheralManager (`src/peripheralmanager.cpp`)

- Reads `PeripheralOptions.blockSPI0 / blockSPI1` from proto at boot and calls `setConfig`.
- Provides `getSPI(block)` and `isSPIEnabled(block)`.
- No concept of scanning / detecting SPI devices (no equivalent of `scanForI2CDevice` for SPI).

### 2.3 Proto Configuration (`proto/config.proto` lines 131–149)

```protobuf
message SPIOptions {
    optional bool    enabled = 1;
    optional int32   rx      = 2;
    optional int32   cs      = 3;   // single CS only
    optional int32   sck     = 4;
    optional int32   tx      = 5;
    // No: speed, DC pin, RST pin, BUSY pin, secondary CS
}
```

`DisplayOptions` (lines 246–288) contains layout and cosmetic display settings but has **no display-type selector** and **no SPI-specific fields**.

---

## 3. Current Display Driver Architecture

### 3.1 GPGFX / DisplayAddon

| Component | File | Role |
|---|---|---|
| `DisplayAddon` | `src/addons/display.cpp` | GPAddon; calls `GPGFX::getAvailableDisplay()` then `init()` |
| `GPGFX` | `src/display/GPGFX.cpp` | Factory: creates driver, routes drawing calls |
| `GPGFX_DisplayBase` | `headers/interfaces/i2c/displaybase.h` | Abstract base with `isSPI()` / `isI2C()` virtuals + all draw methods |
| `GPGFX_TinySSD1306` | `src/interfaces/i2c/ssd1306/tiny_ssd1306.cpp` | Only concrete driver; I2C SSD1306 OLED |

### 3.2 Display Type Enum (`headers/display/GPGFX_types.h`)

```cpp
typedef enum {
    DISPLAY_TYPE_NONE,
    DISPLAY_TYPE_SSD1306,
    DISPLAY_TYPE_COUNT       // Only 1 real display type today
} GPGFX_DisplayType;
```

### 3.3 SPI Detection Stub (`src/display/GPGFX.cpp` lines 77–79)

```cpp
if (driver->isSPI()) {
    // NYI: check if SPI display exists
}
```

This stub exists but is completely unimplemented.

### 3.4 `GPGFX_DisplayTypeOptions` Struct

```cpp
typedef struct {
    GPGFX_DisplayType   displayType;
    PeripheralI2C*      i2c;
    PeripheralSPI*      spi;      // present, but never populated
    uint16_t            size;
    uint16_t            address;  // I2C address; meaningless for SPI
    uint8_t             orientation;
    bool                inverted;
    GPGFX_DisplayFont   font;
    uint8_t             contrast;
} GPGFX_DisplayTypeOptions;
```

There are no fields for: DC pin, RST pin, BUSY pin, SRAM CS pin, SPI speed, or display-side CS pin (separate from the bus CS).

### 3.5 OneBitDisplay Library (`lib/OneBitDisplay/`)

The bundled `OneBitDisplay` library has an `obdSPIInit()` function that accepts DC, CS, RST, MOSI, CLK, and bitbang/speed parameters. **GP2040-CE currently only calls `obdI2CInit()`**. The library does not support the JD79661 natively, but it does support SPI wiring to arbitrary display types.

---

## 4. Color Depth & Frame Buffer Gap

The current display stack is entirely **1-bit monochrome**:

- `GPGFX_DisplayMetrics::depth` is always `1` (bits per pixel).
- The SSD1306 frame buffer is at most 128×64 / 8 = 1,024 bytes.
- All draw primitives (`drawPixel`, `drawText`, etc.) use `uint32_t color` but only the low bit is used.

The JD79661 **quad-color** display requires 2 bits per pixel (black / white / red / yellow), giving:

```
250 × 122 × 2 bits = 7,625 bytes (~7.5 KB) per frame
```

The RP2040 has 264 KB SRAM, so an on-chip buffer is technically feasible, but:
- The existing frame-buffer size assumptions are hard-coded for 1-bit.
- The render/flush pipeline sends the entire buffer synchronously; for eInk, full color refresh takes ~15 seconds — a non-blocking (async) update model is needed.
- Black-and-white-only partial refresh is ~1–2 seconds, still too slow for synchronous blocking.

---

## 5. Web Config Gaps

### 5.1 `DisplayOptions` proto message

No field exists for:
- `displayType` — which controller is in use (SSD1306 vs. JD79661 vs. future)
- SPI pin overrides (DC, RST, BUSY, CS_display, CS_sram)
- SPI clock speed
- Color depth / color mode
- Refresh mode (full color vs. fast B&W partial)

### 5.2 `PeripheralOptions.SPIOptions` proto message

No field for clock speed (`speed`) analogous to `I2COptions.speed`.

### 5.3 DisplayConfig.jsx (`www/src/Pages/DisplayConfig.jsx`)

- The web UI checks `getAvailablePeripherals('i2c')` and only renders the display section when an I2C peripheral is configured.
- There is **no SPI display path** in the frontend.
- All display config controls (size, flip, invert, contrast, splash, etc.) assume an OLED; none handle eInk-specific settings such as refresh mode or color palette.

---

## 6. Summary of Gaps

| # | Layer | Gap | Severity |
|---|---|---|---|
| G1 | `PeripheralSPI` | Only one CS pin per SPI block; SRAM needs second CS | **Blocking** |
| G2 | `PeripheralSPI` / `SPIOptions` | No DC, RST, BUSY pin support | **Blocking** |
| G3 | `GPGFX_DisplayType` | JD79661 type not defined | **Blocking** |
| G4 | `GPGFX_DisplayTypeOptions` | No fields for DC / RST / BUSY / SRAM CS pins | **Blocking** |
| G5 | `GPGFX::detectDisplay()` | SPI detection stub NYI | **Blocking** |
| G6 | Display driver | No JD79661 driver class | **Blocking** |
| G7 | `GPGFX_DisplayModes` map | No 250×122 entry; depth hard-coded to 1-bit | **Blocking** |
| G8 | Frame buffer | 1-bit assumption throughout; needs 2-bit support | **Blocking** |
| G9 | Refresh model | Synchronous flush unsuitable for slow eInk refresh | **Major** |
| G10 | SRAM driver | No 23K256/23LC1024 SPI SRAM abstraction | **Major** |
| G11 | `DisplayOptions` proto | No `displayType`, SPI pin, or refresh-mode fields | **Major** |
| G12 | `SPIOptions` proto | No `speed` field | **Minor** |
| G13 | `DisplayConfig.jsx` | Frontend gated on I2C only; no SPI display path | **Major** |
| G14 | `displaybase.h` | Base class inherits from `I2CDeviceBase` — wrong hierarchy for SPI | **Minor** (refactor) |
| G15 | `GPGFX_DisplayTypeOptions` | `address` field is I2C-only; size is a legacy enum not covering 250×122 | **Minor** |

---

## 7. Implementation Plan

### Phase 1 — Infrastructure (no display yet)

1. **`proto/config.proto`**  
   - Add `speed` field to `SPIOptions`.  
   - Add `displayType` enum/field to `DisplayOptions` (values: `NONE`, `SSD1306`, `JD79661`).  
   - Add `SPIDisplayPinOptions` sub-message inside `DisplayOptions` for: `dc`, `rst`, `busy`, `cs_display`, `cs_sram`.

2. **`lib/PicoPeripherals/peripheral_spi.h` / `.cpp`**  
   - Support a configurable clock speed stored alongside the block config (mirrors I2C `speed` field).  
   - Add `select(cs1, cs2)` overload or expose raw `select(int pin)` to callers so the display and SRAM can each manage their own CS.

3. **`headers/display/GPGFX_types.h`**  
   - Add `DISPLAY_TYPE_JD79661` to `GPGFX_DisplayType` enum (before `DISPLAY_TYPE_COUNT`).  
   - Add `SIZE_250x122` to `GPGFX_DisplaySize` enum.  
   - Extend `GPGFX_DisplayTypeOptions` with: `int8_t dc_pin`, `int8_t rst_pin`, `int8_t busy_pin`, `int8_t cs_sram_pin`, `uint32_t spi_speed`.

### Phase 2 — SRAM Abstraction

4. **New file: `headers/interfaces/spi/sram/spi_sram.h`** (+ `.cpp`)  
   - Driver for Microchip 23K256 / 23LC1024 serial SRAM.  
   - Methods: `init(PeripheralSPI*, cs_pin)`, `read(addr, buf, len)`, `write(addr, buf, len)`.  
   - The JD79661 driver will use this to stage image data without holding it all in RP2040 SRAM.

### Phase 3 — JD79661 Display Driver

5. **New file: `headers/interfaces/spi/jd79661/jd79661.h`** (+ `.cpp`)  
   - Subclass of `GPGFX_DisplayBase` (override `isSPI()` → `true`).  
   - `init(GPGFX_DisplayTypeOptions)` — hardware reset sequence, LUT upload, resolution set.  
   - `clear()` / `drawPixel()` — write to SRAM back-buffer (2 bpp, two planes: black-plane + color-plane).  
   - `drawBuffer()` — transfer SRAM contents to display over SPI, trigger refresh.  
   - `setPower(bool)` — deep-sleep / wake sequence.  
   - Refresh modes:  
     - **Full color refresh** (~15 s): send both color planes, use JD79661 full-LUT.  
     - **Fast B&W partial refresh** (~1–2 s): send only black plane, use partial-LUT.

6. **`src/display/GPGFX.cpp`**  
   - Add `{DISPLAY_TYPE_JD79661, {{SIZE_250x122, {250, 122, 2}}}}` to `GPGFX_DisplayModes`.  
   - Instantiate `GPGFX_JD79661` in `GPGFX::init()` switch.  
   - Implement `detectDisplay()` SPI path: verify SPI block is enabled, attempt hardware reset + read device ID register.

### Phase 4 — Async Refresh Model

7. **`src/display/GPGFX.cpp` / `DisplayAddon`**  
   - Add `GPGFX::renderAsync()` that starts a refresh without blocking.  
   - Add `GPGFX::isRefreshing()` that polls the BUSY pin via the driver.  
   - Modify `DisplayAddon::process()` to skip re-renders while a refresh is in progress (BUSY high).  
   - Since eInk cannot update continuously at 60 fps, consider a "dirty flag" so a new render only triggers if display content has changed.

### Phase 5 — Firmware Config & Peripheral Manager

8. **`headers/addons/display.h` / `src/addons/display.cpp`**  
   - Read new `displayType` and SPI pin fields from `DisplayOptions` proto.  
   - Pass DC / RST / BUSY / SRAM CS pins into `GPGFX_DisplayTypeOptions` before calling `getAvailableDisplay()`.

9. **`src/webconfig.cpp`**  
   - Extend `getDisplayOptions()` / `setDisplayOptions()` to serialize/deserialize new fields.

### Phase 6 — Web Config UI

10. **`www/src/Pages/DisplayConfig.jsx`**  
    - Add display-type selector dropdown (SSD1306, JD79661, …).  
    - When JD79661 is selected, show SPI-specific pin fields (DC, RST, BUSY, CS display, CS SRAM).  
    - Change peripheral gate from `getAvailablePeripherals('i2c')` to also allow `'spi'` for SPI display types.  
    - Add refresh-mode selector (Full Color / Fast B&W).  
    - Hide or repurpose contrast slider (eInk does not support hardware contrast).

11. **`www/src/Locales/en/DisplayConfig.jsx`** (and other locales)  
    - Add i18n keys for new UI controls.

12. **`www/src/Services/WebApi.js`**  
    - Add new fields to `getDisplayOptions` / `setDisplayOptions` payload mapping.

### Phase 7 — Board Config & Documentation

13. **`configs/` (example board config)**  
    - Add sample `BoardConfig.h` fragment showing `SPI_JD79661_*` pin defines.

14. **`docs/eink-jd79661-feature.md`** (this document)  
    - Update with final implementation notes once complete.

---

## 8. File Change Summary

| File | Change |
|---|---|
| `proto/config.proto` | Add `displayType`, SPI pin sub-message to `DisplayOptions`; add `speed` to `SPIOptions` |
| `lib/PicoPeripherals/peripheral_spi.h/.cpp` | Support configurable speed; document multi-CS usage |
| `headers/display/GPGFX_types.h` | New enum values, extend `GPGFX_DisplayTypeOptions` |
| `src/display/GPGFX.cpp` | Add JD79661 to display modes map, implement SPI detection, instantiate driver |
| `headers/interfaces/i2c/displaybase.h` | Decouple base class from `I2CDeviceBase` so SPI drivers do not inherit an I2C base |
| `headers/interfaces/spi/sram/spi_sram.h` + `.cpp` | **NEW** — 23K256/23LC1024 SRAM driver |
| `headers/interfaces/spi/jd79661/jd79661.h` + `.cpp` | **NEW** — JD79661 display driver |
| `headers/addons/display.h` | Add SPI display board-config `#define`s for new pins |
| `src/addons/display.cpp` | Pass SPI pin config into `GPGFX_DisplayTypeOptions` |
| `src/webconfig.cpp` | Extend display options serialization |
| `www/src/Pages/DisplayConfig.jsx` | Display type selector; SPI pin fields; refresh mode |
| `www/src/Locales/en/DisplayConfig.jsx` | i18n keys for new fields |
| `www/src/Services/WebApi.js` | Map new proto fields in API payloads |

---

## 9. Open Questions

1. **Color rendering on existing UI screens** — The button-layout screens, splash images, and status bar are all 1-bit. Should the JD79661 render them in B&W fast-refresh mode, or should full 4-color palette be reserved for a dedicated "eInk" UI layout?

2. **Refresh rate vs. gameplay feedback** — eInk is fundamentally unsuitable for fast feedback (button state, turbo indicators). Should a fast B&W partial-refresh mode be the default for "live" display modes, with full-color used only for splash/static screens?

3. **SRAM requirement** — Is the external SRAM chip strictly required, or should an optional mode exist where the 250×122×2-bit buffer (~7.5 KB) is held in RP2040 SRAM directly? (264 KB total; feasible but tight with other buffers.)

4. **Library strategy** — Should the JD79661 driver be implemented from scratch (to avoid adding a large eInk library dependency), or should an existing OSS driver (e.g., Adafruit EPD, ZinggJM GxEPD2) be vendored into `lib/`? A from-scratch driver against the JD79661 datasheet is likely smallest and most maintainable.

5. **Peripheral config UX** — Currently the user configures SPI bus pins (TX/RX/SCK/CS) in Peripheral Mapping, then display options separately. With an eInk display adding DC/RST/BUSY/SRAM CS, should all display-related SPI pins live in Display Config, or remain split across Peripheral Mapping + Display Config?
