// le_device_db_proto.cpp — BTstack le_device_db interface backed by GP2040-CE
// protobuf config.  Replaces le_device_db_tlv.c (which mapped to the same flash
// region as FlashPROM and caused mutual corruption on every config save).
//
// Storage layout: up to 4 bond slots in Config.bleConfig.bonds[].
// LRU eviction via per-entry seqNr + global seqCounter.

#include "ble/le_device_db.h"
#include "ble/le_device_db_tlv.h"  // for le_device_db_tlv_configure stub
#include "storagemanager.h"

#include <string.h>

#define MAX_BONDS 4

static inline BLEConfig& cfg()
{
    return Storage::getInstance().getConfig().bleConfig;
}

// ─── init / counts ────────────────────────────────────────────────────────────

void le_device_db_init(void)
{
    // Config is loaded by StorageManager before BLE starts — nothing to do.
}

void le_device_db_set_local_bd_addr(bd_addr_t bd_addr)
{
    (void)bd_addr;
    // Not stored; local BD address is managed by the CYW43 controller.
}

int le_device_db_count(void)
{
    int count = 0;
    BLEConfig& c = cfg();
    for (pb_size_t i = 0; i < c.bonds_count; i++) {
        if (c.bonds[i].valid) count++;
    }
    return count;
}

int le_device_db_max_count(void)
{
    return MAX_BONDS;
}

// ─── helpers ──────────────────────────────────────────────────────────────────

static int _find_lru(const BLEConfig& c)
{
    int lru_idx = 0;
    uint32_t lru_seq = c.bonds[0].seqNr;
    for (int i = 1; i < MAX_BONDS; i++) {
        if (c.bonds[i].seqNr < lru_seq) {
            lru_seq = c.bonds[i].seqNr;
            lru_idx = i;
        }
    }
    return lru_idx;
}

static void _fill_slot(BLEBondEntry& e, int addr_type, bd_addr_t addr,
                       sm_key_t irk, uint32_t seq_nr)
{
    memset(&e, 0, sizeof(BLEBondEntry));
    e.valid    = true;
    e.addrType = (uint32_t)addr_type;
    e.seqNr    = seq_nr;

    memcpy(e.addr.bytes, addr, 6);
    e.addr.size = 6;

    if (irk) {
        memcpy(e.irk.bytes, irk, 16);
        e.irk.size = 16;
    }
}

// ─── add / remove ─────────────────────────────────────────────────────────────

int le_device_db_add(int addr_type, bd_addr_t addr, sm_key_t irk)
{
    BLEConfig& c    = cfg();
    uint32_t nextSeq = c.seqCounter + 1;
    c.seqCounter     = nextSeq;

    // 1. Update existing entry with same address.
    for (int i = 0; i < MAX_BONDS; i++) {
        BLEBondEntry& e = c.bonds[i];
        if (e.valid &&
            (int)e.addrType == addr_type &&
            memcmp(e.addr.bytes, addr, 6) == 0)
        {
            e.seqNr = nextSeq;
            if (irk) {
                memcpy(e.irk.bytes, irk, 16);
                e.irk.size = 16;
            }
            Storage::getInstance().save();
            return i;
        }
    }

    // 2. Use first empty slot.
    for (int i = 0; i < MAX_BONDS; i++) {
        if (!c.bonds[i].valid) {
            _fill_slot(c.bonds[i], addr_type, addr, irk, nextSeq);
            if ((pb_size_t)(i + 1) > c.bonds_count) {
                c.bonds_count = (pb_size_t)(i + 1);
            }
            Storage::getInstance().save();
            return i;
        }
    }

    // 3. Evict LRU slot.
    int lru = _find_lru(c);
    _fill_slot(c.bonds[lru], addr_type, addr, irk, nextSeq);
    Storage::getInstance().save();
    return lru;
}

void le_device_db_remove(int index)
{
    if (index < 0 || index >= MAX_BONDS) return;
    memset(&cfg().bonds[index], 0, sizeof(BLEBondEntry));
    cfg().bonds[index].valid = false;
    Storage::getInstance().save();
}

// ─── device info ──────────────────────────────────────────────────────────────

void le_device_db_info(int index, int* addr_type, bd_addr_t addr, sm_key_t irk)
{
    if (index < 0 || index >= MAX_BONDS || !cfg().bonds[index].valid) return;
    const BLEBondEntry& e = cfg().bonds[index];
    if (addr_type) *addr_type = (int)e.addrType;
    if (addr)      memcpy(addr, e.addr.bytes, 6);
    if (irk)       memcpy(irk,  e.irk.bytes,  16);
}

// ─── encryption info ──────────────────────────────────────────────────────────

void le_device_db_encryption_set(int index, uint16_t ediv, uint8_t rand[8],
                                  sm_key_t ltk, int key_size,
                                  int authenticated, int authorized,
                                  int secure_connection)
{
    if (index < 0 || index >= MAX_BONDS) return;
    BLEBondEntry& e = cfg().bonds[index];
    e.ediv = ediv;
    if (rand) {
        memcpy(e.rand.bytes, rand, 8);
        e.rand.size = 8;
    }
    if (ltk) {
        memcpy(e.ltk.bytes, ltk, 16);
        e.ltk.size = 16;
    }
    e.keySize          = (uint32_t)key_size;
    e.authenticated    = (authenticated != 0);
    e.authorized       = (authorized   != 0);
    e.secureConnection = (secure_connection != 0);
    Storage::getInstance().save();
}

void le_device_db_encryption_get(int index, uint16_t* ediv, uint8_t rand[8],
                                  sm_key_t ltk, int* key_size,
                                  int* authenticated, int* authorized,
                                  int* secure_connection)
{
    if (index < 0 || index >= MAX_BONDS || !cfg().bonds[index].valid) return;
    const BLEBondEntry& e = cfg().bonds[index];
    if (ediv)             *ediv             = (uint16_t)e.ediv;
    if (rand)              memcpy(rand, e.rand.bytes, 8);
    if (ltk)               memcpy(ltk,  e.ltk.bytes,  16);
    if (key_size)         *key_size         = (int)e.keySize;
    if (authenticated)    *authenticated    = e.authenticated ? 1 : 0;
    if (authorized)       *authorized       = e.authorized    ? 1 : 0;
    if (secure_connection)*secure_connection = e.secureConnection ? 1 : 0;
}

// ─── CSRK (signed writes — not enabled in this build) ────────────────────────

#ifdef ENABLE_LE_SIGNED_WRITE
void le_device_db_local_csrk_set(int index, sm_key_t csrk)   { (void)index; (void)csrk; }
void le_device_db_local_csrk_get(int index, sm_key_t csrk)   { (void)index; if (csrk) memset(csrk, 0, 16); }
void le_device_db_remote_csrk_set(int index, sm_key_t csrk)  { (void)index; (void)csrk; }
void le_device_db_remote_csrk_get(int index, sm_key_t csrk)  { (void)index; if (csrk) memset(csrk, 0, 16); }
uint32_t le_device_db_remote_counter_get(int index)          { (void)index; return 0; }
void le_device_db_remote_counter_set(int index, uint32_t c)  { (void)index; (void)c; }
uint32_t le_device_db_local_counter_get(int index)           { (void)index; return 0; }
void le_device_db_local_counter_set(int index, uint32_t c)   { (void)index; (void)c; }
#endif

// ─── debug dump ───────────────────────────────────────────────────────────────

void le_device_db_dump(void)
{
    // Intentionally empty in release builds.
}

// ─── TLV configure stub ───────────────────────────────────────────────────────
// btstack_cyw43.c calls le_device_db_tlv_configure() unconditionally during
// CYW43 init.  Our protobuf backend does not need TLV wiring — ignore the call.

void le_device_db_tlv_configure(const btstack_tlv_t* btstack_tlv_impl,
                                 void* btstack_tlv_context)
{
    (void)btstack_tlv_impl;
    (void)btstack_tlv_context;
}
