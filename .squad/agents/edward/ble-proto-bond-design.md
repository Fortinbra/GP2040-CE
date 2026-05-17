# BLE Bond Storage in GP2040-CE Protobuf Config — Design Document

**Author:** Edward Elric (Senior BLE/BTstack Engineer)  
**Date:** 2025-01-28  
**Status:** Proposal

---

## 1. Root Cause Finding

> Before getting to design, there is a likely root cause for the existing bond-loss bug.

**Flash region collision.** GP2040-CE's `FlashPROM` occupies the last 32 KB of flash:

```
EEPROM_ADDRESS_START  = 0x101F8000
EEPROM_SIZE_BYTES     = 0x8000 (32 KB)
Flash end (2 MB Pico) = 0x10200000
```

BTstack's `pico_flash_bank_instance()` also maps to the **top of flash** (two 4 KB sectors, i.e., 0x101FF000–0x10200000 on a 2 MB device). Both subsystems call `flash_range_erase()` on overlapping pages. Every time GP2040-CE saves config it obliterates BTstack's TLV bank, and every time BTstack writes a bond it may corrupt the protobuf blob. This is almost certainly the root cause of bonds not surviving power cycles.

---

## 2. Recommended Approach: Option B — Custom `le_device_db` backed by protobuf

### Why not Option A (custom `hal_flash_bank_t`)?

Option A keeps BTstack's binary TLV blob format and only redirects flash I/O through GP2040-CE's storage layer. This fixes the flash conflict but:

- Bonds remain an opaque binary blob — not inspectable or manageable via the web configurator.
- You still have to serialise/deserialise a BTstack-internal struct that is not part of the public API.
- The `hal_flash_bank_t` interface is write-once-per-erase, requiring careful alignment — a lot of plumbing for no functional gain.

### Why Option B wins

| Criterion | Option A | Option B |
|-----------|----------|----------|
| Fix flash collision | ✅ | ✅ |
| Bonds survive power cycle reliably | ✅ | ✅ |
| First-class config (web UI, export/import) | ❌ | ✅ |
| Human-readable / inspectable | ❌ | ✅ |
| Code size | ~150 LOC | ~350 LOC |
| Complexity | Low | Medium |
| Depends on BTstack internals | Yes | No |

Option B implements the public `le_device_db.h` interface directly. BTstack's Security Manager calls only these public functions — it does not care how the backend stores data. By storing bonds as a protobuf sub-message inside the existing `Config` root, all existing infrastructure (load, save, web API, factory reset) works for free.

---

## 3. Protobuf Schema Additions

Add to **`proto/config.proto`**:

```protobuf
// One BLE paired-device bond entry.
// Sized for LE Secure Connections (SC) bonds; CSRK fields are
// only used when ENABLE_LE_SIGNED_WRITE is set in btstack_config.h.
message BLEBondEntry
{
    optional bool    valid           = 1  [default = false];  // slot occupied?
    optional bytes   addr            = 2  [(nanopb).max_size = 6];
    optional uint32  addrType        = 3;
    optional bytes   irk             = 4  [(nanopb).max_size = 16];

    // LTK / Encryption info
    optional bytes   ltk             = 5  [(nanopb).max_size = 16];
    optional uint32  ediv            = 6;
    optional bytes   rand            = 7  [(nanopb).max_size = 8];
    optional uint32  keySize         = 8;
    optional bool    authenticated   = 9  [default = false];
    optional bool    authorized      = 10 [default = false];
    optional bool    secureConnection = 11 [default = false];

    // CSRK (optional — only set when ENABLE_LE_SIGNED_WRITE)
    optional bytes   localCsrk       = 12 [(nanopb).max_size = 16];
    optional uint32  localCounter    = 13;
    optional bytes   remoteCsrk      = 14 [(nanopb).max_size = 16];
    optional uint32  remoteCounter   = 15;
}

// BLE subsystem config — bonds and local state.
message BLEConfig
{
    // Up to 4 paired devices (matches NVM_NUM_DEVICE_DB_ENTRIES default).
    repeated BLEBondEntry bonds = 1 [(nanopb).max_count = 4];

    // Sequence counter — monotonically incrementing, used for LRU eviction.
    optional uint32 seqCounter = 2 [default = 0];
}
```

Then add one line to the **`Config`** root message (field 16 is free):

```protobuf
message Config
{
    // ... existing fields 1-15 unchanged ...
    optional BLEConfig bleConfig = 16;
}
```

**Size budget:**
Each `BLEBondEntry` encodes to roughly:
- `valid`(1) + `addr`(8) + `addrType`(3) + `irk`(18) + `ltk`(18) + `ediv`(3) + `rand`(10) + `keySize`(3) + flags(6) ≈ 70 bytes per entry  
- 4 entries × 70 B ≈ 280 B overhead — trivial within the 32 KB FlashPROM.

---

## 4. Implementation Outline

### 4.1 New file: `src/le_device_db_proto.cpp`

```cpp
#include "le_device_db.h"
#include "storagemanager.h"

// In-RAM shadow of the BLEConfig submessage.
// Loaded once at init, written through to StorageManager on every mutation.
static BLEConfig& bleConfig() {
    return Storage.getConfig().bleConfig;
}

void le_device_db_init(void) {
    // Config is already loaded by StorageManager::init() before BLE starts.
    // Nothing to do — in-RAM shadow is the live bleConfig submessage.
}

int le_device_db_count(void) {
    int count = 0;
    for (int i = 0; i < bleConfig().bonds_count; i++) {
        if (bleConfig().bonds[i].valid) count++;
    }
    return count;
}

int le_device_db_max_count(void) {
    return 4;  // NVM_NUM_DEVICE_DB_ENTRIES / BLEBondEntry max_count
}

// Returns index of added/updated entry, or -1 on failure.
int le_device_db_add(int addr_type, bd_addr_t addr, sm_key_t irk) {
    BLEConfig& cfg = bleConfig();
    uint32_t nextSeq = ++cfg.seqCounter;

    // 1. Update existing entry with same address.
    for (int i = 0; i < 4; i++) {
        BLEBondEntry& e = cfg.bonds[i];
        if (e.valid && e.addrType == addr_type &&
            memcmp(e.addr.bytes, addr, 6) == 0) {
            e.seqNr = nextSeq;  // refresh LRU
            memcpy(e.irk.bytes, irk, 16);
            e.irk.size = 16;
            Storage.save();
            return i;
        }
    }

    // 2. Use first empty slot.
    for (int i = 0; i < 4; i++) {
        if (!cfg.bonds[i].valid) {
            _fill_entry(cfg.bonds[i], addr_type, addr, irk, nextSeq);
            if (i >= (int)cfg.bonds_count) cfg.bonds_count = i + 1;
            Storage.save();
            return i;
        }
    }

    // 3. Evict LRU slot.
    int lru = _find_lru(cfg);
    _fill_entry(cfg.bonds[lru], addr_type, addr, irk, nextSeq);
    Storage.save();
    return lru;
}

void le_device_db_remove(int index) {
    if (index < 0 || index >= 4) return;
    memset(&bleConfig().bonds[index], 0, sizeof(BLEBondEntry));
    bleConfig().bonds[index].valid = false;
    Storage.save();
}

void le_device_db_info(int index, int* addr_type, bd_addr_t addr, sm_key_t irk) {
    if (index < 0 || index >= 4 || !bleConfig().bonds[index].valid) return;
    const BLEBondEntry& e = bleConfig().bonds[index];
    if (addr_type) *addr_type = e.addrType;
    if (addr)      memcpy(addr, e.addr.bytes, 6);
    if (irk)       memcpy(irk, e.irk.bytes, 16);
}

void le_device_db_encryption_set(int index, uint16_t ediv, uint8_t rand[8],
                                  sm_key_t ltk, int key_size,
                                  int authenticated, int authorized,
                                  int secure_connection) {
    if (index < 0 || index >= 4) return;
    BLEBondEntry& e = bleConfig().bonds[index];
    e.ediv = ediv;
    memcpy(e.rand.bytes, rand, 8);  e.rand.size = 8;
    memcpy(e.ltk.bytes, ltk, 16);   e.ltk.size = 16;
    e.keySize         = key_size;
    e.authenticated   = authenticated;
    e.authorized      = authorized;
    e.secureConnection = secure_connection;
    Storage.save();
}

void le_device_db_encryption_get(int index, uint16_t* ediv, uint8_t rand[8],
                                  sm_key_t ltk, int* key_size,
                                  int* authenticated, int* authorized,
                                  int* secure_connection) {
    if (index < 0 || index >= 4 || !bleConfig().bonds[index].valid) return;
    const BLEBondEntry& e = bleConfig().bonds[index];
    if (ediv)             *ediv = e.ediv;
    if (rand)              memcpy(rand, e.rand.bytes, 8);
    if (ltk)               memcpy(ltk,  e.ltk.bytes,  16);
    if (key_size)         *key_size = e.keySize;
    if (authenticated)    *authenticated = e.authenticated;
    if (authorized)       *authorized = e.authorized;
    if (secure_connection)*secure_connection = e.secureConnection;
}

void le_device_db_remote_irk_get(int index, sm_key_t irk) {
    if (index < 0 || index >= 4 || !bleConfig().bonds[index].valid) return;
    memcpy(irk, bleConfig().bonds[index].irk.bytes, 16);
}

// CSRK functions — only compiled when ENABLE_LE_SIGNED_WRITE
#ifdef ENABLE_LE_SIGNED_WRITE
void le_device_db_local_csrk_set(int index, sm_key_t csrk) { ... }
void le_device_db_local_csrk_get(int index, sm_key_t csrk) { ... }
void le_device_db_remote_csrk_set(int index, sm_key_t csrk) { ... }
void le_device_db_remote_csrk_get(int index, sm_key_t csrk) { ... }
uint32_t le_device_db_remote_counter_get(int index) { ... }
void le_device_db_remote_counter_set(int index, uint32_t counter) { ... }
uint32_t le_device_db_local_counter_get(int index) { ... }
void le_device_db_local_counter_set(int index, uint32_t counter) { ... }
#endif

void le_device_db_dump(void) {
    // Optional: printf each valid slot for debug builds.
}
```

**New header: `headers/le_device_db_proto.h`**

```cpp
#pragma once
// Declares le_device_db_proto_init() for callers that need explicit init.
// The standard le_device_db.h interface is the primary API used by BTstack.
void le_device_db_proto_init(void);  // Optional — noop if config already loaded
```

### 4.2 Modify `src/BLEHIDManager.cpp`

**Remove** the TLV flash bank setup block:

```cpp
// REMOVE these lines from _doInit():
const btstack_tlv_t* tlv_impl = btstack_tlv_flash_bank_init_instance(
    &tlv_context, pico_flash_bank_instance(), NULL);
le_device_db_tlv_configure(tlv_impl, &tlv_context);
```

**Add** (already handled by our implementation — `le_device_db_init()` is called by BTstack SM layer automatically; no explicit call needed in user code).

Also **remove** the `tlv_context` and `btstack_tlv_flash_bank_t` member variables from the class.

### 4.3 Modify `src/config_utils.cpp`

In `initUnsetPropertiesWithDefaults()`, add BLE bond defaults initialisation:

```cpp
// BLE config — zero bonds by default
INIT_UNSET_PROPERTY(config.bleConfig, seqCounter, 0);
// bonds array entries are zero-initialised by nanopb; valid=false is the default
```

### 4.4 Modify `CMakeLists.txt` (or relevant `sources.cmake`)

Add `src/le_device_db_proto.cpp` to the build target and **remove** the link to BTstack's `le_device_db_tlv.c` (if explicitly listed). BTstack's `pico_btstack_ble` already exposes the `le_device_db.h` interface — we just provide the implementation object file.

```cmake
target_sources(gp2040_firmware PRIVATE
    # ... existing sources ...
    src/le_device_db_proto.cpp
)

# Remove or guard this if present:
# src/le_device_db_tlv.c  <-- BTstack's TLV implementation, no longer needed
```

Verify that `btstack_tlv_flash_bank.c` and `pico_btstack_flash_bank.c` are not pulled in transitively (check via `cmake --graphviz` or inspect the Pico SDK's `pico_btstack_ble` target).

---

## 5. Migration Path

### Existing users with no stored bonds

No migration needed. `BLEConfig` fields are all `optional` with safe defaults. On first load from an existing protobuf blob that has no `bleConfig` field, nanopb initialises `bonds_count = 0` and all `valid = false`. The user re-pairs normally.

### Existing users with BTstack TLV bonds (if TLV was working)

The TLV bonds are in a separate flash region and will be abandoned on upgrade. Since the TLV bank and FlashPROM overlap (see §1), any "surviving" TLV bonds are actually unreliable. **Intentionally not migrating** is the correct call:

1. On firmware upgrade, all BLE bonds are lost.
2. User re-pairs all devices (one-time action per device).
3. Going forward, bonds persist correctly via protobuf.

Include a release-notes entry: *"BLE bonds must be re-paired after this upgrade due to storage system changes."*

### Factory reset

`StorageManager::ResetSettings()` calls `EEPROM.reset()` which zeros the write cache and commits. This automatically clears `bleConfig` along with all other config. Web configurator "Clear BLE bonds" can call a targeted save: set all `bonds[i].valid = false`, then `Storage.save()`.

---

## 6. Estimated Scope

| Work item | Est. LOC | Notes |
|-----------|----------|-------|
| `src/le_device_db_proto.cpp` | ~200 | Core implementation + CSRK stubs |
| `headers/le_device_db_proto.h` | ~15 | Trivial |
| `proto/config.proto` additions | ~35 | `BLEBondEntry` + `BLEConfig` + field 16 |
| `src/config_utils.cpp` defaults | ~5 | One `INIT_UNSET_PROPERTY` block |
| `src/BLEHIDManager.cpp` removals | ~−15 | Remove TLV init block + members |
| `CMakeLists.txt` | ~5 | Add/remove one source file |
| **Total net new** | **~245 LOC** | |

**Complexity rating: Medium**

The logic itself is straightforward (array of 4 structs with LRU eviction). The main complexity drivers are:

1. Verifying that BTstack's SM layer does not call `le_device_db` functions before `StorageManager::init()` completes — BLE init is already deferred 3 s in `BLEHIDManager::process()`, so this should be safe.
2. Ensuring `Storage.save()` inside `le_device_db_add/remove/encryption_set` does not cause double-save storms during a single pairing handshake. Mitigation: the FlashPROM 50 ms debounce coalesces rapid saves automatically.
3. Confirming that `le_device_db_tlv.c` is not linked — if it is, you'll get duplicate symbol linker errors that will catch the problem immediately.

---

## 7. Open Questions

1. **`set_local_bd_addr` / `le_device_db_set_local_bd_addr`** — BTstack may call this. Add a noop stub or store the local BD address in `BLEConfig` (useful for display in web UI).
2. **Web UI surface** — Do we want a "Manage Paired Devices" panel? If yes, expose `bleConfig.bonds[]` via the existing REST/config JSON endpoint; `addr` and `addrType` are sufficient to show a device list. Deletion is `valid = false` + save.
3. **`NVM_NUM_DEVICE_DB_ENTRIES`** — This define in `btstack_config.h` must match `(nanopb).max_count = 4`. Verify current value; update `btstack_config.h` if needed.
4. **CSRK / signed writes** — GP2040-CE's HID profile currently does not set `ENABLE_LE_SIGNED_WRITE`, so CSRK stubs returning zero/noop are sufficient.

---

## 8. Summary

The recommended path is **Option B**: implement the `le_device_db.h` interface directly, backed by a new `BLEConfig` protobuf sub-message stored inside GP2040-CE's existing `Config` structure. This fixes the flash region collision that is almost certainly causing bond loss, makes bonds first-class managed config, and costs approximately 245 net lines of straightforward C++.
