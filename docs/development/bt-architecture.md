# GP2040-CE Bluetooth Architecture

GP2040-CE implements BLE HID gamepad output using the Raspberry Pi Pico SDK's BTstack integration layer on top of the CYW43439 wireless chip. When `INPUT_MODE_BLE` is selected at boot, TinyUSB is bypassed entirely and `BLEHIDManager` drives all radio and report delivery from the main Core 0 loop.

The diagrams below cover where each component lives (block diagram), how a gamepad report travels from button press to radio transmission (report-send sequence), and how pairing and bonded reconnect differ (connection lifecycle sequence).

---

## Block Diagram — Component Layers

```mermaid
graph TD
    subgraph HW["🔧 Hardware"]
        RP2350B["RP2350B\nCore 0 / Core 1"]
        CYW43["CYW43439\nBT + WiFi"]
        GP43["GP43 / ADC 3\nBattery Voltage Divider"]
        RP2350B <-->|"gSPI (GPIO 23-25,29)"| CYW43
        RP2350B -->|"ADC read"| GP43
    end

    subgraph SDK["📦 Pico SDK / CYW43 Driver"]
        CYW43Arch["pico_cyw43_arch_none\ncyw43_arch_init() / cyw43_arch_poll()"]
        AsyncCtx["async_context_threadsafe_background\nPeriodic alarm IRQ on Core 0"]
        BTTransport["pico_btstack_cyw43\nbtstack_run_loop_async_context"]
        CYW43Arch --> AsyncCtx
        AsyncCtx --> BTTransport
    end

    subgraph BTSTACK["🔵 BTstack"]
        HCI["HCI\nHost Controller Interface"]
        L2CAP["L2CAP\nLogical Link Control"]
        SM["SM\nSecurity Manager\n(LE Secure Connections + Bonding)"]
        ATT["ATT\nAttribute Protocol"]
        GATT["GATT Server\natt_server_init()"]
        HIDS["hids_device\nHID over GATT service\n(Input Report + Report Map)"]
        BattSvc["battery_service_server\nGATT UUID 0x180F\nCharacteristic 0x2A19"]
        DevInfo["device_information_service_server\nGATT UUID 0x180A"]
        HCI --> L2CAP
        L2CAP --> SM
        L2CAP --> ATT
        ATT --> GATT
        GATT --> HIDS
        GATT --> BattSvc
        GATT --> DevInfo
    end

    subgraph BLE["🎮 GP2040-CE BLE Layer"]
        BLEMgr["BLEHIDManager\n(singleton)"]
        PowerState["BLEPowerState\nADVERTISING → ACTIVE → IDLE"]
        HCIHandler["_hciPacketHandler\nBTSTACK_EVENT_STATE\nHCI_EVENT_LE_META\nHCI_EVENT_DISCONNECTION_COMPLETE\nHCI_EVENT_HIDS_META"]
        SMHandler["_smPacketHandler\nSM_EVENT_JUST_WORKS_REQUEST\nSM_EVENT_IDENTITY_RESOLVING_*\nSM_EVENT_PAIRING_COMPLETE"]
        ATTCb["_attReadCallback\n_attWriteCallback"]
        BondDB["le_device_db_proto\nProtobuf bond store\n(up to 4 slots, LRU eviction)"]
        BLEMgr --> PowerState
        BLEMgr --> HCIHandler
        BLEMgr --> SMHandler
        BLEMgr --> ATTCb
        BLEMgr --> BondDB
    end

    subgraph CORE["⚙️ GP2040-CE Core"]
        GP2040Loop["GP2040::run()\nCore 0 main loop\n(wirelessOnly path skips TinyUSB)"]
        GamepadObj["Gamepad\nread() → process() → GamepadState\n32 buttons · hat · 4 axes"]
        OutMgr["OutputManager::dispatch()\nGamepadState → 9-byte BLE report"]
        StoreMgr["StorageManager\nProtobuf Config\n(getConfig().bleConfig.bonds[])"]
        FlashPROM["FlashPROM\nFlash-backed NVM\nConfig persistence"]
        GP2040Loop --> GamepadObj
        GamepadObj --> OutMgr
        OutMgr --> BLEMgr
        BondDB --> StoreMgr
        StoreMgr --> FlashPROM
    end

    %% Cross-layer connections
    BTTransport --> HCI
    CYW43 -.->|"BT RF"| RF[("📡 BT RF\n(host device)")]
    HCIHandler -->|"HCI events"| HIDS
    HCIHandler -->|"SM events"| SM
    BattSvc -.->|"ADC read\n(30s interval)"| GP43
```

> **Note:** `async_context_threadsafe_background` fires BTstack event processing from a periodic alarm IRQ on Core 0 — the same core as the main loop. All `BLEHIDManager` state variables shared between the IRQ context and the main thread are declared `volatile` to prevent compiler register-caching.

---

## Sequence Diagram — HID Report Send Flow

```mermaid
sequenceDiagram
    participant GP as GP2040::run()
    participant Gamepad
    participant OutMgr as OutputManager
    participant BLEMgr as BLEHIDManager
    participant BTStack as BTstack (HIDS / ATT / L2CAP)
    participant CYW43 as CYW43439

    GP->>Gamepad: read() + process()
    Note over Gamepad: GamepadState updated<br/>(buttons, hat, axes)

    GP->>OutMgr: dispatch(gamepad)
    Note over OutMgr: Map state → 9-byte report<br/>[buttons×4][hat][x][y][z][rz]

    OutMgr->>BLEMgr: sendReport(report, 9)
    Note over BLEMgr: Check _connected &&<br/>_notificationsEnabled<br/>IDLE? throttle to 50 ms
    BLEMgr->>BLEMgr: memcpy(_pendingReport)<br/>_reportPending = true

    GP->>BLEMgr: process()
    BLEMgr->>BLEMgr: cyw43_arch_poll()
    Note over BLEMgr: _reportPending && connected &&<br/>notificationsEnabled?
    BLEMgr->>BTStack: hids_device_request_can_send_now_event(_conHandle)

    Note over BTStack: ATT layer ready<br/>fires CAN_SEND_NOW event
    BTStack-->>BLEMgr: HIDS_SUBEVENT_CAN_SEND_NOW<br/>(via _hciPacketHandler IRQ)

    BLEMgr->>BTStack: hids_device_send_input_report(_conHandle, _pendingReport, 9)
    Note over BLEMgr: _reportPending = false<br/>Detect payload change for<br/>IDLE→ACTIVE transition

    BTStack->>CYW43: ATT notification<br/>(L2CAP PDU → HCI ACL → SPI)
    CYW43-->>CYW43: BT RF transmission
```

---

## Sequence Diagram — Connection Lifecycle (Fresh Pair vs Bonded Reconnect)

```mermaid
sequenceDiagram
    participant Host as Host (PC / Console)
    participant GAP as GAP / Advertiser
    participant HCI as HCI Layer
    participant SM as Security Manager
    participant BondDB as le_device_db_proto
    participant HIDS as hids_device
    participant BLEMgr as BLEHIDManager

    rect rgb(230, 245, 255)
        Note over Host,BLEMgr: ── FRESH PAIR ──────────────────────────────────────────
        BLEMgr->>GAP: gap_advertisements_enable(1)<br/>(on BTSTACK_EVENT_STATE / HCI_STATE_WORKING)
        GAP-->>Host: ADV_IND (Gamepad appearance, HID/Battery/DIS UUIDs)
        Host->>HCI: CONNECT_REQ
        HCI-->>BLEMgr: HCI_SUBEVENT_LE_CONNECTION_COMPLETE<br/>_connected=true, _conHandle=N
        Host->>SM: SMP Pairing Request (LE Secure Connections)
        SM-->>BLEMgr: SM_EVENT_JUST_WORKS_REQUEST → sm_just_works_confirm()
        SM-->>BLEMgr: SM_EVENT_PAIRING_COMPLETE (status=SUCCESS)<br/>_hasBondedPeers=true
        BondDB->>BondDB: le_device_db_add(addr, IRK, LTK)<br/>→ StorageManager.save()
        HCI-->>BLEMgr: HCI_EVENT_ENCRYPTION_CHANGE (status=SUCCESS)<br/>_notificationsEnabled=true<br/>_powerState=ADVERTISING→...
        Host->>HIDS: GATT Write: Enable Input Report Notifications
        HIDS-->>BLEMgr: HIDS_SUBEVENT_INPUT_REPORT_ENABLE (enable=1)<br/>_notificationsEnabled=true<br/>_powerState=ACTIVE
        Note over BLEMgr: Ready — reports flow
    end

    rect rgb(240, 255, 240)
        Note over Host,BLEMgr: ── BONDED RECONNECT ────────────────────────────────────
        BLEMgr->>GAP: gap_advertisements_enable(1)<br/>(after disconnect / power-on)
        GAP-->>Host: ADV_IND (RPA resolvable private address)
        Host->>HCI: CONNECT_REQ (using cached LTK)
        HCI-->>BLEMgr: HCI_SUBEVENT_LE_CONNECTION_COMPLETE<br/>_connected=true
        SM->>BondDB: IRK resolution lookup
        BondDB-->>SM: Match found
        SM-->>BLEMgr: SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED<br/>_hasBondedPeers=true<br/>⚠️ NOT encrypted yet — do NOT enable notifications here
        SM->>HCI: LTK-based encryption handshake
        HCI-->>BLEMgr: HCI_EVENT_ENCRYPTION_CHANGE (status=SUCCESS)<br/>_notificationsEnabled=true
        Note over Host: Host may skip GATT write on reconnect<br/>(CCCDs are bonded — persisted by BTstack)
        HIDS-->>BLEMgr: HIDS_SUBEVENT_INPUT_REPORT_ENABLE (enable=1)<br/>_powerState=ACTIVE
        Note over BLEMgr: Ready — reports flow
    end
```

---

## Power State Reference

| State | Trigger (→ this state) | Behavior |
|---|---|---|
| **ADVERTISING** | Boot / Disconnect (`HCI_EVENT_DISCONNECTION_COMPLETE`) | `gap_advertisements_enable(1)`. No reports sent. Battery service inactive. TinyUSB skipped in wirelessOnly mode. |
| **ACTIVE** | `HIDS_SUBEVENT_INPUT_REPORT_ENABLE` (enable=1) **or** input change detected in `HIDS_SUBEVENT_CAN_SEND_NOW` while IDLE | Reports sent at gamepad frame rate. Battery ADC polled every 30 s via `battery_service_server_set_battery_value()`. |
| **IDLE** | No input change for 30 continuous seconds while ACTIVE | Reports throttled to ≤ 1 per 50 ms to reduce radio activity. Any payload change in `CAN_SEND_NOW` transitions back to ACTIVE. |

> **IDLE → ACTIVE detection:** The `_lastSentReport` buffer is compared against `_pendingReport` inside `HIDS_SUBEVENT_CAN_SEND_NOW` (IRQ context). If they differ, `_lastInputChangeMs` is refreshed and `_powerState` is set to `ACTIVE`. `_lastSentReport` is only written in the IRQ handler so it does not need `volatile`.

---

## GATT Profile Summary

Services declared in `src/ble_hid.gatt` (compiled to `ble_hid.h` at build time via `pico_btstack_make_gatt_header`):

| Service | UUID | Key Characteristics |
|---|---|---|
| GAP | 0x1800 | Device Name (`"GP2040-CE Gamepad"`), Appearance (Gamepad 0x03C4) |
| Battery Service | 0x180F | Battery Level (0x2A19) — GATT notify, read |
| Device Information | 0x180A | Manufacturer, Model, Firmware Revision |
| HID | 0x1812 | Report (Input, NOTIFY), Report Map, Protocol Mode, HID Info, Control Point |
| GATT | 0x1801 | Database Hash |

> The HID Input Report characteristic uses `REPORT_REFERENCE` descriptor `(1, 1)` to communicate Report ID 1. The ATT notification payload is 9 bytes with **no Report ID prefix** — the descriptor carries that association out-of-band.

---

## Bond Storage

`le_device_db_proto.cpp` implements the BTstack `le_device_db` interface backed by `Config.bleConfig.bonds[]` (protobuf, up to 4 slots). LRU eviction is handled via a per-slot `seqNr` counter. The protobuf backend replaces `le_device_db_tlv.c`, which mapped to the same flash region as FlashPROM and caused mutual corruption on every config save. The TLV source file is excluded at compile time via CMake `HEADER_FILE_ONLY`.

---

## Build System Integration

BLE support is gated on `PICO_CYW43_SUPPORTED` in `CMakeLists.txt`:

```cmake
if(PICO_CYW43_SUPPORTED)
    target_sources(... PRIVATE
        src/BLEHIDManager.cpp
        src/OutputManager.cpp
        src/le_device_db_proto.cpp
    )
    target_link_libraries(...
        pico_cyw43_arch_none
        pico_btstack_ble
        pico_btstack_cyw43
    )
    target_compile_definitions(... ENABLE_BLUETOOTH=1)
    pico_btstack_make_gatt_header(... src/ble_hid.gatt)
endif()
```

`ENABLE_BLUETOOTH=1` is the compile-time guard used in `gp2040.cpp`, `OutputManager.cpp`, and all BLE call sites to prevent BLE code from being compiled for boards without CYW43 (e.g. plain Pico, Pico 2).
