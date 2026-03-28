# TinyUSB / BTStack Namespace Conflict Analysis

**Author:** Edward (Firmware Developer)  
**Date:** 2026-03-28  
**Status:** Research complete — grounded in codebase and SDK 2.2.0 headers

---

## 1. Confirmed Symbol Collisions

### 1.1 `hid_report_type_t` — DIRECT TYPEDEF COLLISION (confirmed)

Both libraries define the same typedef name in global C namespace:

**TinyUSB** (`lib/tinyusb/src/class/hid/hid.h:84`):
```c
typedef enum {
    HID_REPORT_TYPE_INVALID = 0,   // ← first value differs
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE
} hid_report_type_t;
```

**BTStack** (`<pico-sdk>/lib/btstack/src/btstack_hid.h:109`):
```c
typedef enum {
    HID_REPORT_TYPE_RESERVED = 0,  // ← first value differs
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE
} hid_report_type_t;
```

**Impact:** Including both `hid.h` (TinyUSB) and `btstack_hid.h` in the same translation unit causes a `redefinition of 'hid_report_type_t'` compile error. This is the primary hard collision.

**GP2040-CE exposure:** `hid_report_type_t` is used throughout the existing driver layer:
- `headers/drivers/hid/HIDDriver.h` — method signatures
- `src/drivers/hid/HIDDriver.cpp` — `get_report()` / `set_report()` implementations
- `src/usbdriver.cpp` — TinyUSB callback dispatch
- Multiple driver implementations (Astro, Egret, Keyboard, MDMini, etc.)

---

### 1.2 `hid_protocol_mode_t` vs `hid_protocol_mode_enum_t` — NO COLLISION (different names)

- **BTStack** defines `hid_protocol_mode_t` (in `btstack_hid.h`) with values `HID_PROTOCOL_MODE_BOOT`, `HID_PROTOCOL_MODE_REPORT`.
- **TinyUSB** defines `hid_protocol_mode_enum_t` (different name) with values `HID_PROTOCOL_BOOT`, `HID_PROTOCOL_REPORT`.

The typedef names differ, so no redefinition error. The enum value names (`HID_PROTOCOL_MODE_BOOT` vs `HID_PROTOCOL_BOOT`) also differ. **No collision.**

---

### 1.3 `HID_USAGE_PAGE_*` constants — NO COLLISION (different design patterns)

- **BTStack** (`btstack_hid.h`) defines 27+ named constants: `HID_USAGE_PAGE_DESKTOP 0x01`, `HID_USAGE_PAGE_KEYBOARD 0x07`, etc.
- **TinyUSB** (`hid.h`) defines a function-like macro `HID_USAGE_PAGE(x)` that generates a HID descriptor item, and `HID_USAGE_PAGE_N(x,n)`. It does NOT define the named page constants.

No macro redefinition collision between the two stacks for usage page names.

---

### 1.4 `HID_KEY_*` constants — NO COLLISION (TinyUSB-only)

`HID_KEY_A`, `HID_KEY_ENTER`, etc. are defined only in TinyUSB `hid.h`. BTStack does not define these. No collision.

---

### 1.5 `PACKED` / alignment macros — NO COLLISION (different names)

- **TinyUSB** uses `TU_ATTR_PACKED` (defined in `lib/tinyusb/src/common/tusb_compiler.h`).
- **BTStack** does not define a `PACKED` macro. It uses `__attribute__((packed))` inline or in its own internal compiler helpers.
- **GP2040-CE** uses `__attribute((packed, aligned(1)))` directly (see `headers/drivers/hid/HIDDescriptors.h:48`).

No collision.

---

### 1.6 `HID_SUBEVENT_*` and `HID_MESSAGE_TYPE_*` — BTStack-only, no collision

BTStack `btstack_defines.h` defines `HID_SUBEVENT_*` and `btstack_hid.h` defines `HID_MESSAGE_TYPE_*`, `HID_HANDSHAKE_PARAM_TYPE_*`, `HID_CONTROL_PARAM_*`. None of these appear in TinyUSB headers. No collision.

---

## 2. CMake Linkage Analysis

### 2.1 No mutual exclusion in SDK CMakeLists

Neither the TinyUSB CMakeLists (`<sdk>/src/rp2_common/tinyusb/CMakeLists.txt`) nor the BTStack CMakeLists (`<sdk>/src/rp2_common/pico_btstack/CMakeLists.txt`) assert a `FATAL_ERROR` or conditional exclusion when both are requested simultaneously. There is **no SDK-enforced mutual exclusion**.

### 2.2 BTStack availability is gated on `PICO_CYW43_SUPPORTED`

From `<sdk>/src/rp2_common/pico_btstack/CMakeLists.txt`:
```cmake
if (PICO_CYW43_SUPPORTED AND NOT EXISTS ${PICO_BTSTACK_PATH}/${BTSTACK_TEST_PATH})
    message(WARNING "btstack submodule has not been initialized; Pico W BLE support unavailable.")
```

BTStack libraries are only registered if the SDK finds a valid BTStack path (always true in SDK 2.2.0 where it ships as a submodule). The available CMake targets are:
- `pico_btstack_base`
- `pico_btstack_ble`
- `pico_btstack_classic`
- `pico_btstack_flash_bank`
- `pico_btstack_run_loop_async_context`

### 2.3 Simultaneous linkage: possible at link level, problematic at compile level

Both stacks can be added to `target_link_libraries()` in the same CMake target. The linker-level conflict is **not** the primary issue. The problem is header-level: including TinyUSB HID headers and BTStack HID headers in the **same translation unit (`.cpp` file)** will fail to compile due to the `hid_report_type_t` redefinition.

**Practical constraint:** TinyUSB and BTStack can coexist in the same firmware binary, but each `.cpp` file that uses HID functionality must include only one stack's HID headers. Isolation at the translation-unit level is sufficient to compile cleanly.

---

## 3. Known Workarounds

### 3.1 No existing workarounds in GP2040-CE

As of this analysis, there are **zero** btstack references, `#ifdef USE_BT` guards, or isolation patterns anywhere in `src/`, `headers/`, or the main `CMakeLists.txt`. The project has not begun this work.

### 3.2 Translation-unit isolation (standard pattern)

The standard approach for combining TinyUSB and BTStack in a single firmware image is **source file isolation**: driver files that implement BT output are separate `.cpp` files that include BTStack headers but NOT TinyUSB HID headers. Existing TinyUSB-using files remain unchanged.

```cpp
// bt_hid_driver.cpp — includes ONLY btstack headers, NOT tusb.h
#include "btstack_hid.h"
#include "classic/hid_device.h"
// ...BTHIDDriver implementation...
```

### 3.3 Optional: typedef bridging via forward-declaration

If a shared interface layer must refer to `hid_report_type_t` without including either stack's header, define a project-local `gp_hid_report_type_t` enum in a neutral header:

```cpp
// headers/gp_hid_types.h — no stack headers included
typedef enum {
    GP_HID_REPORT_INVALID = 0,
    GP_HID_REPORT_INPUT,
    GP_HID_REPORT_OUTPUT,
    GP_HID_REPORT_FEATURE
} gp_hid_report_type_t;
```

Both `HIDDriver` (USB) and a future `BTDriver` (BT) would cast to/from this type at the boundary. This is the cleanest interface-level approach but requires refactoring the existing HID driver signatures.

### 3.4 `#undef` approach (fragile, not recommended)

In principle: include TinyUSB headers first, then `#undef hid_report_type_t`, then include BTStack headers. This is fragile and non-portable across compiler versions and library updates. **Not recommended.**

---

## 4. Recommended Approach for GP2040-CE

### Compile-time flag: `PICO_CYW43_SUPPORTED` (already provided by SDK)

Rather than a custom `USE_BT=1` flag, the SDK already provides `PICO_CYW43_SUPPORTED` which is `1` only for `pico_w` and `pico2_w` boards. This is the correct gate.

### CMake changes:
```cmake
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_btstack_classic  # for Bluetooth Classic HID
        pico_btstack_base
        pico_btstack_run_loop_async_context
        pico_cyw43_arch_lwip_threadsafe_background
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE GP2040_HAS_BLUETOOTH=1)
endif()
```

### Architecture changes:
1. **New `BTHIDDriver` class** in `src/drivers/bt/BTHIDDriver.cpp` — implements BT Classic HID via BTStack. Includes only BTStack headers.
2. **Existing `HIDDriver` et al. untouched** — no TinyUSB headers are disturbed.
3. **`GPOutputTransport` abstraction** (previously identified in bt-analysis.md) — both USB and BT drivers implement a common interface. The shared method signatures use `gp_hid_report_type_t` (or an `int`-based interface) to avoid typedef collision at the abstraction boundary.
4. **`OutputManager`** fans out button state to USB (TinyUSB) and BT (BTStack) simultaneously — these are separate hardware paths, no runtime conflict.

### Translation-unit firewall rule:
> No `.cpp` file may `#include` both `tusb.h` (or any TinyUSB class header) and `btstack_hid.h` (or `classic/hid_device.h`). BT driver files are isolated in `src/drivers/bt/`.

---

## 5. RP2350 + CYW43 — Confirmed Valid BT Target

### SDK 2.2.0 confirmation (verified in codebase)

`<pico-sdk>/src/boards/include/boards/pico2_w.h`:
```c
pico_board_cmake_set(PICO_PLATFORM, rp2350)
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)
#define PICO_RP2350A 1
```

The `pico2_w` board definition sets `PICO_CYW43_SUPPORTED=1` and targets the `rp2350` platform. Since `pico_btstack` is gated on `PICO_CYW43_SUPPORTED`, **BTStack is fully available on RP2350+CYW43 in SDK 2.2.0.**

### Previous documentation uncertainty is incorrect

Earlier notes in this project marked RP2350+CYW43 BTStack support as "uncertain" or "pending CYW43 wireless stack porting." This is incorrect. The Pico SDK 2.2.0 ships `pico2_w.h` with `PICO_CYW43_SUPPORTED=1`, and the `pico_btstack` cmake target registers all sub-libraries when CYW43 is supported. **RP2350 + CYW43 is a fully supported BTStack target as of SDK 2.2.0.**

### Pimoroni Pico Lipo 2 XL W

The Pimoroni Pico Lipo 2 XL W is a commercially available RP2350A + CYW43439 board. Fortinbra has confirmed a working BT controller project runs on this hardware. This corroborates the SDK-level support and removes any remaining uncertainty about RP2350 BTStack viability.

---

## 6. Summary Table

| Conflict Area | Status | Severity | Workaround |
|---|---|---|---|
| `hid_report_type_t` typedef | **CONFIRMED COLLISION** | High — compile error if both included in same TU | Source file isolation (separate BT driver TUs) |
| `hid_protocol_mode_t` typedef | No collision (different names) | None | — |
| `HID_USAGE_PAGE_*` macros | No collision (TinyUSB uses function-like macro, BTStack uses named constants) | None | — |
| `HID_KEY_*` constants | No collision (TinyUSB-only) | None | — |
| `PACKED` macros | No collision (different names: `TU_ATTR_PACKED` vs none) | None | — |
| CMake mutual exclusion | No SDK-enforced exclusion | N/A | Link both targets, isolate include paths |
| RP2350+CYW43 BTStack support | **CONFIRMED SUPPORTED** (SDK 2.2.0 `pico2_w.h`) | N/A — resolved | Use `PICO_BOARD=pico2_w` |

---

## 7. Files Referenced

| File | Role |
|---|---|
| `lib/tinyusb/src/class/hid/hid.h:84` | TinyUSB `hid_report_type_t` definition |
| `lib/tinyusb/src/common/tusb_compiler.h` | TinyUSB `TU_ATTR_PACKED` definition |
| `headers/tusb_config.h` | GP2040-CE TinyUSB configuration (`CFG_TUD_HID=2`, `CFG_TUH_HID=4`) |
| `headers/drivers/hid/HIDDriver.h` | Uses `hid_report_type_t` in method signatures |
| `<sdk>/lib/btstack/src/btstack_hid.h:109` | BTStack `hid_report_type_t` definition (collision point) |
| `<sdk>/lib/btstack/src/classic/hid_device.h` | BTStack HID device API (`hid_sdp_record_t`, callbacks) |
| `<sdk>/src/rp2_common/pico_btstack/CMakeLists.txt` | BTStack cmake targets (`pico_btstack_classic`, `pico_btstack_ble`) |
| `<sdk>/src/rp2_common/tinyusb/CMakeLists.txt` | TinyUSB cmake targets — no btstack exclusion |
| `<sdk>/src/boards/include/boards/pico2_w.h` | RP2350+CYW43 board definition (`PICO_CYW43_SUPPORTED=1`) |
