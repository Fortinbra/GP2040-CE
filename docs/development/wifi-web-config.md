# WiFi Web Configuration — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning  
**SDK version:** 2.2.0+  
**Depends on:** RP2040/RP2350 boards with CYW43439 wireless chip (Pico W, Pico 2 W, Pimoroni Pico Lipo 2 XL W)

---

## Overview

This document describes planned **WiFi-based web configuration** for GP2040-CE. The goal is to allow users to access the GP2040-CE web configurator from any device on the same network — or directly via an Access Point — **while the controller is actively functioning as a gamepad**, without needing to reboot into a special USB web config mode.

**Current state:** Web config is only accessible via USB RNDIS (the controller appears as a USB network device at `192.168.7.1`). This requires the controller to be physically connected to the configuring device via USB.

**What this adds:**
- Web config accessible via WiFi from any phone, tablet, or PC
- Two connectivity modes: **Access Point** (controller creates its own WiFi network) and **Station** (controller joins an existing WiFi network)
- Web config available simultaneously with normal gamepad operation — no mode switching required
- Wireless-only controllers (battery-powered, Bluetooth gamepad mode) gain a configuration path that doesn't require a USB cable

---

## Motivation

### The Problem with USB-Only Config

Currently, reconfiguring a wireless GP2040-CE controller requires:
1. Plugging in a USB cable
2. Rebooting into web config mode (or using the RNDIS interface)
3. Making changes
4. Rebooting back into gamepad mode

This is especially painful for:
- Controllers installed inside arcade sticks where the USB port is hard to reach
- Battery-powered controllers used wirelessly with Bluetooth
- Tournament setups where controllers need quick mid-session reconfiguration

### WiFi Config Solves This

With WiFi web config, reconfiguration becomes:
1. Connect phone/laptop to the controller's WiFi AP (or access the controller's IP on the existing network)
2. Open browser, navigate to the config UI
3. Done — no cable, no reboot, no mode switch

---

## Technical Architecture

### CYW43 WiFi Capability

The CYW43439 on Pico W/Pico 2 W supports both:
- **Access Point (AP) mode:** The controller creates a WiFi network (SSID: `GP2040-CE-XXXXXX` where X = last 3 bytes of MAC). Clients connect directly to the controller.
- **Station (STA) mode:** The controller connects to an existing WiFi network. The controller gets a DHCP address and is reachable at that IP.

Both modes use the same lwIP stack already present in the firmware for RNDIS. The web server (`httpd` in `lib/httpd/`) already serves the full configuration UI.

### Coexistence with RNDIS

The firmware currently uses RNDIS over USB to expose the web config at `192.168.7.1`. This uses `pico_lwip` with a USB netif.

WiFi adds a second netif to the same lwIP instance. lwIP natively supports multiple network interfaces — the USB RNDIS netif and the WiFi CYW43 netif can coexist, with the `httpd` server listening on all interfaces (`IP_ADDR_ANY`).

**Important constraint:** CYW43 WiFi and Bluetooth are on the same chip but share the radio. Simultaneous WiFi + Bluetooth operation is supported by the CYW43439 firmware via time-division multiplexing, but throughput is reduced on both. For a web config use case (low traffic, bursty), this tradeoff is acceptable.

### Coexistence with BLE / BT Classic

When WiFi is active alongside BLE or BT Classic:
- Both share the CYW43 radio via the chipset's built-in coexistence firmware
- The game loop continues uninterrupted — WiFi httpd traffic is handled by lwIP callbacks within `cyw43_arch_poll()`
- BLE/BT connections experience minor additional latency during WiFi bursts (web page loads); this is negligible for a config-only interface

### Web Server

The existing `lib/httpd/` web server serves the compiled React app and JSON API endpoints. No changes to the web server itself are needed — it already handles the full config API.

The server must be configured to listen on the CYW43 WiFi netif IP in addition to (or instead of) the RNDIS IP. This is a single `httpd_init()` configuration change.

---

## Connectivity Modes

### Mode 1: Access Point (Recommended Default)

The controller creates its own WiFi network.

| Property | Value |
|---|---|
| SSID | `GP2040-CE-XXXXXX` (last 3 MAC bytes) |
| Password | Configurable (default: `gp2040ce`) |
| IP Address | `192.168.4.1` (CYW43 AP default) |
| DHCP | Provided by controller (lwIP DHCP server) |
| URL | `http://192.168.4.1/` |

**Pros:**
- Works anywhere — no existing network required
- Ideal for tournament/field use
- No credentials to manage beyond the AP password

**Cons:**
- Client device loses its existing WiFi connection while connected to controller AP
- Not suitable if the controller needs to be configured from a device that can't switch networks (e.g., a locked-down work laptop)

### Mode 2: Station (Join Existing Network)

The controller joins the user's existing WiFi network.

| Property | Value |
|---|---|
| SSID | User-configured (stored in `WiFiOptions`) |
| Password | User-configured |
| IP Address | DHCP-assigned by router (displayed in web UI or OLED) |
| URL | `http://<dhcp-ip>/` or `http://gp2040-ce.local/` (mDNS) |

**Pros:**
- No network switching required — phone/PC stays on existing WiFi
- mDNS hostname (`gp2040-ce.local`) avoids needing to know the IP

**Cons:**
- Requires initial configuration of WiFi credentials (chicken-and-egg: first setup still needs USB)
- Controller depends on external network availability
- Less predictable (DHCP IP can change)

### Recommended Approach

Support **both modes**, selectable in web config, with **AP mode as the default**. Station mode is opt-in for users who prefer it. AP mode requires zero network configuration and works everywhere.

---

## Implementation Plan

### Phase 1 — Access Point Mode

**New files:**
- `headers/WiFiManager.h` — WiFi lifecycle singleton (AP/STA, init, poll)
- `src/WiFiManager.cpp` — CYW43 WiFi init, netif setup, httpd binding

**Modified files:**
- `CMakeLists.txt` — add `pico_cyw43_arch_lwip_poll` (or integrate with existing CYW43 arch) when WiFi enabled
- `proto/config.proto` — add `WiFiOptions` message to `Config`
- `src/gp2040.cpp` — add `WiFiManager::getInstance().process()` to main loop
- `src/webconfig.cpp` — add `getWiFiOptions` / `setWiFiOptions` API endpoints
- `www/src/` — add WiFi configuration page (SSID, password, mode, current IP display)

**`WiFiOptions` proto:**
```protobuf
message WiFiOptions {
    bool enabled = 1;
    WifiMode mode = 2;                // AP = 0, Station = 1
    string apSsid = 3;               // AP mode: SSID (default: GP2040-CE-XXXXXX)
    string apPassword = 4;           // AP mode: password (default: gp2040ce)
    string staSsid = 5;              // Station mode: target SSID
    string staPassword = 6;          // Station mode: password
    string staAssignedIp = 7;        // Station mode: last DHCP-assigned IP (read-only display)
}

enum WifiMode {
    WIFI_MODE_AP = 0;
    WIFI_MODE_STATION = 1;
}
```

**Init sequence:**
```
cyw43_arch_init()
  → AP mode:  cyw43_arch_enable_ap_mode(ssid, password, CYW43_AUTH_WPA2_AES_PSK)
              lwip_dhcpserver_init() on netif
  → STA mode: cyw43_arch_enable_sta_mode()
              cyw43_arch_wifi_connect_async(ssid, password, auth)
httpd_init()  (already running for RNDIS; if not, start it)
```

**Main loop integration:**
```cpp
// In gp2040.cpp or OutputManager
WiFiManager::getInstance().process();  // calls cyw43_arch_poll() + httpd poll
```

### Phase 2 — Station Mode + mDNS

Add Station mode support and integrate lwIP's mDNS responder so the controller is discoverable at `gp2040-ce.local` without knowing the DHCP IP.

- `pico_lwip_mdns` provides the mDNS responder
- Register `gp2040-ce` as the mDNS hostname on init
- Display assigned IP and mDNS hostname on OLED (if present)

### Phase 3 — Security

AP mode with WPA2 password is sufficient for most use cases. Additional hardening options:
- Per-device unique AP password (derived from MAC address)
- Optional: HTTP Basic Auth on the web config UI (low priority — config data is not sensitive)

---

## Display Integration

If the board has an OLED/display, show the WiFi connection status:
- AP mode: `WiFi AP: GP2040-CE-XXXXXX` + `192.168.4.1`
- STA mode: `WiFi: Connected` + IP address or `gp2040-ce.local`
- STA mode connecting: `WiFi: Connecting...`
- WiFi disabled: no WiFi status shown

---

## Configuration UI

Add a **WiFi Configuration** page to the web configurator:

```
WiFi Configuration
──────────────────
[✓] Enable WiFi Web Config

Mode:  (●) Access Point  ( ) Join Network

── Access Point Settings ──
SSID:     [GP2040-CE-A1B2C3          ]
Password: [gp2040ce                  ]

── Station Settings (when in Join Network mode) ──
Network SSID:  [                         ]
Password:      [                         ]
Current IP:    192.168.1.42 (read-only)

[Save]
```

The current IP in station mode is useful for bookmarking or connecting from devices that don't support mDNS.

---

## Constraints and Limitations

### CYW43 Radio Coexistence
WiFi and Bluetooth share the CYW43 radio. The chip handles coexistence in firmware, but:
- Web page loads may cause brief BLE/BT audio gaps (not relevant — no audio)
- BLE/BT advertising intervals may be stretched during WiFi activity
- For config-only traffic (low bandwidth, not latency-sensitive), this is acceptable

### lwIP Conflict with RNDIS
`pico_cyw43_arch_lwip_poll` will attempt to add the CYW43 interface to lwIP. RNDIS also uses lwIP. Both can coexist as separate netifs in the same lwIP instance, but initialization order matters:
- RNDIS must init its netif before `httpd_init()` (already the case)
- CYW43 WiFi netif must be added after `cyw43_arch_init()`
- `httpd_init()` must be called once and bind to `IP_ADDR_ANY` to serve both

This requires careful sequencing in the init chain. The existing `pico_cyw43_arch_none` (BT-only) approach in the Bluetooth feature must be replaced with `pico_cyw43_arch_lwip_poll` when WiFi is enabled alongside BT.

### First-Time Station Mode Setup
Station mode requires WiFi credentials before the controller can join a network. Initial credential entry still requires USB web config. A potential workaround: always start in AP mode on first boot (or if STA connection fails after 10 seconds), then fall back to STA once credentials are saved.

### Non-CYW43 Boards
WiFi web config is only available on CYW43-equipped boards. All other boards continue to use USB RNDIS exclusively. CMake guards (`if(PICO_CYW43_SUPPORTED)`) ensure the WiFi code is not compiled for non-wireless boards.

---

## Acceptance Criteria

- [ ] AP mode: controller creates WiFi network visible on phone/PC
- [ ] AP mode: web config UI loads at `http://192.168.4.1/` without USB cable
- [ ] AP mode: settings save correctly while controller is in gamepad mode
- [ ] Gamepad input (USB HID or BLE) continues functioning during WiFi config session
- [ ] RNDIS USB web config still works when WiFi is also enabled
- [ ] Station mode: controller connects to existing WiFi, IP displayed in UI
- [ ] Station mode: web config accessible at DHCP IP and `gp2040-ce.local`
- [ ] Non-CYW43 board builds unaffected (no regression)

---

## Open Questions

1. **AP + STA simultaneously:** CYW43 supports a limited concurrent AP+STA mode. Is this worth pursuing so the controller is reachable on the existing network AND provides its own AP for devices without WiFi? Adds complexity — probably Phase 3.

2. **Security:** Should the AP default to open (no password) for maximum ease of setup, with a password being optional? Or should WPA2 be mandatory? Recommendation: WPA2 with a simple default password, user-changeable.

3. **mDNS hostname collision:** If two GP2040-CE controllers are on the same network, `gp2040-ce.local` would conflict. Use MAC-derived hostnames (`gp2040-a1b2c3.local`) or a user-configurable hostname.

4. **OLED display during config:** Show a "WiFi Config Active" indicator while a client is connected to the web UI, so the user knows the controller is being configured remotely.
