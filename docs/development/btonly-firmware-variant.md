# Bluetooth-Only Firmware Variant — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning (Testing/Experimental)  
**SDK version:** 2.2.0+  
**Depends on:**
- [BLE HID Support](ble-hid-support.md) — wireless gamepad output
- [WiFi Web Configuration](wifi-web-config.md) — wireless web config access  
**Hardware:** CYW43-equipped boards only (Pico W, Pico 2 W, Pimoroni Pico Lipo 2 XL W)

---

## Overview

This document describes a **Bluetooth-only firmware variant** of GP2040-CE — a separate build configuration that removes USB entirely and delivers the controller experience over BLE, with web configuration served over WiFi.

This is an **experimental build for testing purposes** — it is not intended as a replacement for the standard USB-capable firmware, but rather as a proving ground for fully wireless controller operation. Lessons learned here will inform the production wireless feature set.

**What this variant provides:**
- BLE HID gamepad output (no USB cable required for gameplay)
- WiFi web configuration — no USB RNDIS, no physical connection needed after initial setup
- A provisioning flow that works entirely without USB: AP mode on first boot, station mode once credentials are configured
- Automatic fallback to AP mode if WiFi credentials fail

**What is removed vs the standard firmware:**
- TinyUSB stack — not initialized, no USB descriptors, no USB HID modes
- RNDIS web config — replaced by WiFi web config
- All USB-dependent output modes (XInput, PS4, Switch, keyboard, etc.)

---

## Motivation

The standard GP2040-CE firmware will always support USB as the primary output mode. But for fully wireless controllers — particularly battery-powered builds inside fight sticks or portable devices — the USB stack is dead weight. Removing it simplifies the firmware, reduces flash and RAM usage, and eliminates any boot-time USB enumeration delay.

More practically: this variant is needed to **validate the BLE HID and WiFi web config features end-to-end** without the USB stack as a fallback. If the controller can be fully configured and operated wirelessly with no USB cable ever needed, the wireless features are mature enough for integration into the standard firmware.

---

## Provisioning Flow

The central challenge of a no-USB firmware is initial configuration: the user has no USB RNDIS interface to set WiFi credentials. The solution is a **provisioning state machine** modeled on the IoT device pattern:

```
┌─────────────────────────────────────────────────────────┐
│                    BOOT                                   │
│                      │                                    │
│          WiFi credentials stored?                         │
│                ┌─────┴──────┐                            │
│               NO            YES                          │
│                │              │                           │
│          ┌─────▼─────┐  ┌────▼──────────┐               │
│          │  AP MODE   │  │ STA MODE      │               │
│          │ (provision)│  │ (connect...)  │               │
│          └─────┬─────┘  └────┬──────────┘               │
│                │              │                           │
│         User opens            │                           │
│         192.168.4.1    ┌──────┴──────┐                   │
│         configures     │             │                    │
│         WiFi creds  SUCCESS       FAILED                 │
│                │       │             │                    │
│                │  ┌────▼──────┐  ┌──▼────────┐          │
│                │  │ STA MODE  │  │  AP MODE  │           │
│           Save │  │ connected │  │ (fallback)│           │
│           → reboot  │ BLE on    │  │ BLE on    │          │
│                     └───────────┘  └───────────┘          │
└─────────────────────────────────────────────────────────┘
```

### State 1: Provisioning (AP Mode)

Triggered when: no WiFi credentials are stored (first boot or after credential wipe).

- CYW43 starts as a WiFi Access Point
- SSID: `GP2040-CE-XXXXXX` (last 3 MAC bytes, no password — open network for ease of initial setup)
- IP: `192.168.4.1`
- `httpd` serves the full web config UI at `http://192.168.4.1/`
- BLE HID is **active** — the controller functions as a gamepad even in AP mode
- OLED shows: `Setup: GP2040-CE-XXXXXX` / `192.168.4.1`
- User connects their phone/PC to the AP, opens the browser, enters WiFi network credentials (and optionally a static IP), and saves
- On save: controller reboots into STA mode

### State 2: Station Mode (Normal Operation)

Triggered when: WiFi credentials are stored and connection succeeds.

- CYW43 connects to the configured WiFi network
- Assigned IP (or static IP) displayed on OLED and in web UI
- `httpd` serves web config at the WiFi IP and `gp2040-ce.local`
- BLE HID active — controller functions as a gamepad
- This is the normal running state

### State 3: Station Fallback (AP Mode)

Triggered when: WiFi credentials are stored but the network cannot be reached (wrong password, router offline, different location) after a timeout.

- Controller attempts STA connection for **15 seconds** on boot
- If connection fails: switches to AP mode (same as provisioning state, but with a "WiFi Failed" indicator)
- User can reconnect to the AP and update credentials
- BLE HID remains active throughout — the controller still works as a gamepad
- OLED shows: `WiFi Failed` / `AP: GP2040-CE-XXXXXX`

---

## Build Configuration

### CMake Build Variant

The BT-only firmware is a separate CMake target / build configuration, not a runtime option. It is produced by passing a build flag:

```cmake
cmake -B build_btonly -S . \
  -DGP2040_BOARDCONFIG=PimoroniPicoLipo2XLW \
  -DPICO_BOARD=pico2_w \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DGP2040_BTONLY=1
```

The `GP2040_BTONLY=1` flag activates a separate set of compile definitions:

```cmake
if(GP2040_BTONLY AND PICO_CYW43_SUPPORTED)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        ENABLE_BLUETOOTH=1
        ENABLE_BLE=1
        ENABLE_WIFI_CONFIG=1
        GP2040_BTONLY=1
        # Disable USB stack
        PICO_USB_ENABLED=0
        # Use lwIP through CYW43 arch (not RNDIS)
        CYW43_LWIP=1
    )
    # Link CYW43 arch with lwIP (WiFi uses the lwIP stack, not RNDIS)
    target_link_libraries(${PROJECT_NAME}
        pico_cyw43_arch_lwip_poll
        pico_btstack_cyw43
        pico_btstack_ble
        pico_btstack_run_loop_async_context
        pico_lwip_mdns
    )
    # Do NOT link: tinyusb, pico_stdio_usb, rndis, lwip-port (RNDIS netif)
endif()
```

### Key Differences from Standard Build

| Component | Standard Firmware | BT-Only Variant |
|---|---|---|
| USB stack (TinyUSB) | ✅ Required | ❌ Removed |
| USB HID modes | ✅ All 17 modes | ❌ Removed |
| RNDIS web config | ✅ `192.168.7.1` | ❌ Removed |
| BLE HID output | Optional add-on | ✅ Only output mode |
| WiFi web config | Optional add-on | ✅ Required |
| lwIP stack | Via RNDIS | Via CYW43 WiFi |
| AP provisioning | ❌ Not needed | ✅ First-boot flow |
| Boot time | USB enumeration delay | Faster (no USB) |
| Flash usage | ~Full | Smaller (no TinyUSB) |

---

## Implementation Plan

### Prerequisites

Both prerequisite features must be implemented before this variant is buildable:
1. **BLE HID** (`feature/ble-hid`) — provides wireless gamepad output
2. **WiFi Web Config** (`feature/wifi-web-config`) — provides wireless web config

### Phase 1 — Build Infrastructure

- Add `GP2040_BTONLY` CMake flag and compile definition guards
- Create `src/main_btonly.cpp` (or `#ifdef GP2040_BTONLY` blocks in `gp2040.cpp`) that:
  - Skips `tud_init()` entirely
  - Initializes WiFi provisioning state machine instead of RNDIS
  - Uses `BLEHIDManager` as the sole output driver
- Ensure `drivermanager.cpp` handles `INPUT_MODE_BLE` as the default (and only) mode when `GP2040_BTONLY`

### Phase 2 — WiFi Provisioning State Machine

- `headers/WiFiProvisioner.h` / `src/WiFiProvisioner.cpp`
- States: `PROVISIONING_AP`, `CONNECTING_STA`, `CONNECTED_STA`, `FALLBACK_AP`
- Transitions on: credentials saved, connect success, connect timeout (15s)
- AP mode SSID derived from CYW43 MAC address

```cpp
class WiFiProvisioner {
public:
    enum State { PROVISIONING_AP, CONNECTING_STA, CONNECTED_STA, FALLBACK_AP };
    void init();
    void process();
    State getState() const;
    const char* getDisplayIp() const;
};
```

### Phase 3 — Integration and OLED

- Wire `WiFiProvisioner` into the main loop
- Add OLED status display for each provisioning state
- Show `gp2040-ce.local` or static IP on OLED once connected

---

## User Experience Flow

### First Boot (No Credentials)

1. Board powers on
2. OLED: `GP2040-CE BT-Only` → `Setup: Join WiFi` → `AP: GP2040-CE-A1B2C3`
3. BLE advertising starts — controller is already discoverable and pairable
4. User connects phone to `GP2040-CE-A1B2C3` WiFi (open, no password)
5. Browser opens `http://192.168.4.1/`
6. User navigates to WiFi Configuration, enters home network SSID + password (or static IP)
7. User clicks Save → controller reboots
8. Controller connects to home WiFi
9. OLED: `WiFi: 192.168.1.100` (or assigned IP)
10. Controller is now accessible at that IP for all future config

### Subsequent Boots

1. Board powers on
2. BLE advertising starts (~3s after boot)
3. Controller connects to saved WiFi network
4. OLED: `WiFi: 192.168.1.100`
5. User pairs via BLE from Windows/Android if not already paired
6. Controller works as a gamepad

### When WiFi is Unavailable

1. Board powers on
2. Tries to connect for 15 seconds
3. Falls back to AP mode
4. OLED: `WiFi Failed` / `AP: GP2040-CE-A1B2C3`
5. BLE still works — controller is functional as a gamepad
6. User can connect to AP and fix credentials when convenient

---

## Constraints and Limitations

### Testing Scope Only
This variant is experimental. It is not suitable for production or general distribution until:
- BLE HID stability is validated (connection drops resolved)
- WiFi provisioning flow is tested on Windows, Android, and iOS
- AP → STA → AP fallback is exercised across multiple real-world network conditions

### CYW43 Radio Contention
WiFi (STA or AP) and BLE share the CYW43 radio. During active web config sessions, BLE HID input may have slightly elevated latency. This is acceptable for a configuration session but should be measured.

### No USB Fallback
If the WiFi credentials are wrong AND BLE doesn't connect, the controller has no input or config path until it boots into AP mode. The AP fallback is therefore critical — it must always work regardless of other failures.

### Board Compatibility
Only boards with CYW43 are supported. The build system must reject `GP2040_BTONLY=1` for non-CYW43 boards with a clear CMake error.

---

## Acceptance Criteria

- [ ] `cmake -DGP2040_BTONLY=1` produces a firmware that does not enumerate as a USB device
- [ ] First boot: controller creates open WiFi AP `GP2040-CE-XXXXXX`
- [ ] Web config UI loads at `http://192.168.4.1/` with no USB connected
- [ ] WiFi credentials saved via web config → controller reboots into STA mode
- [ ] In STA mode: web config accessible at configured IP and `gp2040-ce.local`
- [ ] In STA mode: BLE HID gamepad functional simultaneously
- [ ] If STA connection fails after 15s: fallback to AP mode, BLE still active
- [ ] OLED shows correct state in all phases
- [ ] Standard firmware build (no `GP2040_BTONLY`) is completely unaffected

---

## Open Questions

1. **AP network security:** Should the provisioning AP be open (no password) for maximum accessibility on first setup? A QR code printed on the PCB or box could encode the AP SSID for easy phone connection without remembering a password. Recommendation: open for provisioning, since credentials are only needed once and no sensitive data is transmitted before WiFi config is saved.

2. **Credential wipe:** How does the user reset WiFi credentials if they move to a new network and can't reach the controller? Options: (a) hold a button combo on boot to force AP mode regardless of stored credentials, (b) "forget network" button in web config UI. Both should be implemented.

3. **BLE + AP simultaneous:** Some CYW43 firmware versions handle BLE advertising during AP mode better than others. This needs empirical testing — BLE HID might not be stable during WiFi AP operation due to radio contention.

4. **Output mode selection:** In the BT-only variant, `INPUT_MODE_BLE` is the only option. The Settings → Output Mode page should either hide the mode selector or show it as a read-only "Bluetooth LE" label to avoid user confusion.
