# Review 2 — Riza Hawkeye
**Commit reviewed:** `688582e4` (branch: `docs/copilot-instructions`)
**Date:** 2026-03-28
**Files reviewed:**
- `docs/development/rp2350-support.md`
- `docs/development/dependency-updates.md`
- `.github/copilot-instructions.md`

---

## VERDICT: REJECTED

Three issues found. Two are factual errors; one is a stale self-contradicting note. All are in files other than `rp2350-support.md`, which is clean.

**Hughes: locked out (round 1). Edward: locked out (round 2). Escalate to Mustang for round 3.**

---

## Issues by File

### `docs/development/dependency-updates.md`

**Issue 1 — Wrong file named for SDK version update (§ "Pico SDK → Where to update")**
> "**File:** `pico_sdk_import.cmake`"
> ```cmake
> set(sdkVersion 2.2.0)  # Change this to the new version
> ```

`set(sdkVersion 2.2.0)` is on **line 7 of `CMakeLists.txt`**, not in `pico_sdk_import.cmake`.
`pico_sdk_import.cmake` does not contain this variable — it uses `PICO_SDK_FETCH_FROM_GIT_TAG` for git-fetch scenarios and is otherwise a pass-through locator file.
A developer following these instructions would edit the wrong file.

**Fix required:** Change `**File:** \`pico_sdk_import.cmake\`` to `**File:** \`CMakeLists.txt\``.

---

**Issue 2 — Stale self-contradicting note (line 29)**
> "Note: `copilot-instructions.md` currently references SDK 2.1.1 as a default — this is a stale value. The authoritative minimum is 2.2.0 as enforced by the build system and CI."

Edward updated `copilot-instructions.md` to 2.2.0 in this same commit. The note describes a problem that no longer exists. A reader of the doc on this branch will find 2.2.0 in `copilot-instructions.md`, making this note factually wrong and confusing.

**Fix required:** Remove this note entirely, or replace with a brief forward reference confirming both files are in sync at 2.2.0.

---

### `.github/copilot-instructions.md`

**Issue 3 — Wrong year in "Last updated" timestamp (final line)**
> "**Last updated:** 2025-03-28"

The commit is dated 2026-03-28. `docs/development/rp2350-support.md` (in the same commit) correctly shows `2026-03-28`. The copilot-instructions date is off by one year.

**Fix required:** Change `2025-03-28` → `2026-03-28`.

---

### `docs/development/rp2350-support.md`

**No issues.** SDK version (2.2.0), GPIO counts, board config names (`Pico2`, `FlatboxRev8`, `SparkFunProMicroRP2350`), FATAL_ERROR reference, CI pin, and Pico 2 W gap explanation are all accurate and consistent with the build system. ✓

---

## Summary

| File | Status | Issues |
|------|--------|--------|
| `docs/development/rp2350-support.md` | ✅ Clean | — |
| `docs/development/dependency-updates.md` | ❌ Fail | Wrong file in SDK update instructions; stale internal note |
| `.github/copilot-instructions.md` | ❌ Fail | Wrong year in last-updated date |

**Assigned for round 3:** Mustang
