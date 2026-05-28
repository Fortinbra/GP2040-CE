# PSRAM Support Evaluation (Pimoroni Pico Lipo 2 XL W and Similar RP2350 Boards)

**Last updated:** 2026-05-28  
**Status:** Research complete; implementation not started  
**Scope:** GP2040-CE firmware architecture, board configuration, and runtime tradeoffs for external PSRAM on RP2350

---

## 1) Executive Summary

External PSRAM on RP2350-class boards is promising for **capacity-oriented, non-latency-critical workloads** in GP2040-CE (for example: larger transient web-config payloads, diagnostic ring buffers, and optional feature caches). It is a poor fit for the **core input path** where deterministic low latency is required.

For GP2040-CE specifically, the biggest blocker is not API surface area; it is **platform integration risk**:

1. Current Pimoroni board support in-tree intentionally uses `PICO_BOARD=pico2_w` with a 4 MB declared flash profile due to a validated persistence regression when 16 MB flash is declared (config save lost across reboot).  
2. PSRAM enablement on RP2350 is tied to QMI/CS1 setup and board-level definitions; this must be introduced without reintroducing the flash persistence regression.

**Recommendation:** proceed with a **phased opt-in PSRAM path** for carefully selected buffers only, behind compile-time + runtime guards, after first establishing a stable board configuration that preserves current FlashPROM behavior.

---

## 2) Current GP2040-CE State (Repository Evidence)

### 2.1 Pimoroni board currently trades capacity for persistence safety

- `configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake` intentionally sets:
  - `PICO_BOARD pico2_w`
  - `PICO_PLATFORM rp2350-arm-s`
  - `PICO_NUM_GPIOS 48`
- The same file documents why 16 MB declaration is deferred: boot/partition path clobbers EEPROM region after reboot under the tested 16 MB declaration.
- `docs/development/flashprom-large-flash-support.md` records:
  - tested EEPROM addresses (`0x101F8000` and `0x10FF8000`) both failing when flash is declared 16 MB,
  - v1 decision to keep 4 MB declared for persistence reliability.

### 2.2 Firmware has memory pressure candidates, mostly outside critical timing path

Examples in-tree:

- Web config uses a 16 KB POST payload buffer and large dynamic JSON documents (`src/webconfig.cpp`).
- Animation/LED subsystems maintain frame buffers and effect state (`headers/animationstation/animationstation.h`, `headers/addons/neopicoleds.h`).
- Driver/auth paths use medium-size buffers (example: 1 KB class buffers in shared protocol code and auth modules).
- Input macro/data limits are explicit and fixed (`headers/addons/input_macro.h`).

None of these currently prove that PSRAM is required, but several are plausible consumers if additional feature growth is expected.

---

## 3) RP2350 + Pico SDK PSRAM Platform Findings

### 3.1 QMI supports two external memory devices (flash + optional second device)

The RP2350 QMI register interface is described as supporting up to two SPI/DSPI/QSPI flash or PSRAM devices (`hardware/regs/qmi.h` in Pico SDK 2.2.0), with CS1 available for the second device.

### 3.2 Board definitions indicate PSRAM-capable Pimoroni RP2350 targets

Pico SDK board definitions for Pimoroni RP2350 variants include board-specific PSRAM CS pin definitions (for example `PIMORONI_PICO_PLUS2_W_PSRAM_CS_PIN 47` in `src/boards/include/boards/pimoroni_pico_plus2_w_rp2350.h`).

### 3.3 Flash APIs already account for CS1/PSRAM interaction risks

Pico SDK `hardware_flash` implementation explicitly preserves/restores QMI CS1 configuration to avoid clobbering app-managed CS1 setup (noted in comments as relevant for PSRAM) and performs cache clean before flash operations (`src/rp2_common/hardware_flash/flash.c`).

### 3.4 Cache coherence requirements are explicit when flash + PSRAM coexist

Pico SDK `hardware_xip_cache` guidance for RP2350 calls out the clean/program/invalidate sequence to keep flash programming coherent without discarding pending PSRAM write data (`src/rp2_common/hardware_xip_cache/include/hardware/xip_cache.h`).

### 3.5 Multicore exclusives have caveats with external PSRAM

Pico SDK spin lock runtime notes that global exclusive monitor handling on RP2350 does not automatically cover every external-PSRAM exclusives pattern; custom MPU/shareable-region treatment may be needed for exotic cases (`src/rp2_common/hardware_sync_spin_lock/sync_spin_lock.c`).

---

## 4) Where PSRAM Is Likely Valuable for GP2040-CE

### 4.1 High-value candidates (recommended)

1. **Web-config transient working memory**
   - JSON parse/serialize scratch and larger import/export payload handling.
   - Keeps feature growth (profiles/macros/theme payloads) from consuming internal SRAM headroom.
2. **Optional diagnostics/telemetry ring buffers**
   - Input timing traces, BLE/USB event history, and debug snapshots.
   - Useful for maintainers and power users without polluting fast-path memory.
3. **Feature-specific caches for optional modules**
   - Precomputed display/LED tables or optional host-processing scratch regions.
   - Good fit when access pattern is bursty and not in every poll-loop critical section.

### 4.2 Medium-value candidates (needs benchmarking first)

1. **Larger user macro/profile authoring workflows**
   - Temporary staging during parse/validation is a fit.
   - Runtime execution state should remain internal SRAM.
2. **BLE/web concurrency buffers**
   - Candidate for less-frequent background data structures.
   - Must verify latency impact under sustained polling + wireless activity.

### 4.3 Poor-fit candidates (not recommended)

1. **Core gamepad state and hot polling path**
2. **ISR-critical structures**
3. **Anything requiring strict sub-millisecond deterministic access**

These should remain in on-chip SRAM.

---

## 5) What It Will Take to Use PSRAM Safely

### 5.1 Board/SDK integration work

1. Introduce a PSRAM-capable RP2350 board path for Pimoroni-class targets while preserving current persistence behavior.
2. Avoid coupling PSRAM rollout to the unresolved 16 MB flash declaration issue.  
3. Ensure CS1/device info handling is explicit and validated during boot for the selected board target.

### 5.2 Firmware architecture work

1. Add a small **PSRAM memory service layer** (allocator wrappers + availability checks).
2. Keep all PSRAM usage **opt-in** by feature/module, not global heap redirection at first.
3. Require **fallback allocation path** to internal SRAM for non-PSRAM boards.
4. Add guardrails so critical modules cannot silently migrate hot-path memory to PSRAM.

### 5.3 Persistence and flash-operation safety

1. Audit all flash write/erase paths for cache-coherency ordering requirements.
2. Validate config save/load behavior under repeated writes + reboot on PSRAM-enabled builds.
3. Treat flash/PSRAM interaction as release-gating for affected boards.

### 5.4 Validation and release criteria

1. Add CI build target(s) for PSRAM-enabled board config.
2. Execute hardware regression suite:
   - save/reboot persistence,
   - web-config stress (large POST/import),
   - BLE + USB runtime coexistence,
   - long-run stability with periodic flash writes.
3. Capture memory and latency deltas versus current baseline.

---

## 6) Side Effects / Drawbacks to Expect

1. **Latency variability risk** if PSRAM is used in hot paths.
2. **Higher integration complexity** (board config, allocator policy, cache/flash interaction).
3. **Test burden increase** because behavior differs by board capabilities.
4. **Potential concurrency pitfalls** for multicore/exclusive access patterns.
5. **Volatile medium**: PSRAM is not persistence storage; power loss drops contents.
6. **Debug complexity**: memory-corruption failures may become harder to reproduce when mixed internal/external allocation is introduced.

---

## 7) Recommended Adoption Strategy (Phased)

### Phase 0 — Prerequisite hardening

- Keep current shipping behavior unchanged.
- Finalize a PSRAM-capable board configuration strategy that does not regress config persistence.

### Phase 1 — Safe pilot (narrow scope)

- Enable PSRAM only for one non-critical subsystem (recommended: web-config transient buffers).
- Add runtime instrumentation and fallback path.
- Validate no measurable impact on input latency in standard modes.

### Phase 2 — Optional feature expansion

- Expand to diagnostics and selected optional caches.
- Continue isolating hot path from PSRAM.
- Ship as opt-in board/feature configuration.

### Phase 3 — Re-evaluate broader usage

- Only after long-run stability and latency evidence.
- Decide whether to keep selective PSRAM use permanently or broaden usage scope.

---

## 8) Decision Guidance

If the near-term goal is core gameplay latency and stability, PSRAM is **not** the first lever to pull.  
If the near-term goal is expanding web-config complexity, diagnostics, or optional rich features on RP2350B boards, PSRAM is a strong enabler **when introduced conservatively**.

**Go/no-go recommendation now:**  
- **Go** for a narrow pilot behind flags.  
- **No-go** for blanket/global-memory migration until board-level persistence and latency validations are complete.

---

## 9) References

### GP2040-CE repository

- `configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake`
- `docs/development/flashprom-large-flash-support.md`
- `src/webconfig.cpp`
- `headers/animationstation/animationstation.h`
- `headers/addons/neopicoleds.h`
- `headers/addons/input_macro.h`

### Pico SDK 2.2.0

- `src/rp2350/hardware_regs/include/hardware/regs/qmi.h`
- `src/boards/include/boards/pimoroni_pico_plus2_w_rp2350.h`
- `src/rp2_common/hardware_flash/include/hardware/flash.h`
- `src/rp2_common/hardware_flash/flash.c`
- `src/rp2_common/hardware_xip_cache/include/hardware/xip_cache.h`
- `src/rp2_common/hardware_sync_spin_lock/sync_spin_lock.c`
