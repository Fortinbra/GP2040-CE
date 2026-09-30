---
name: Pico Build System
description: "Use when configuring, building, compiling, or running CMake for this project. Enforces Ninja, the GP2040_BOARD selector, SDK prerequisites, and clean VS Code task workflows."
applyTo: "**"
---
# Pico Build System

This is a Pico SDK project. The **only** supported build system is **Ninja**. Visual Studio (MSBuild) generators must never be used.

## Generator Rule

- **Always pass `-G Ninja`** when invoking `cmake` directly from the terminal.
- Never omit the generator flag — without it, CMake will default to the Visual Studio generator on Windows and produce a broken build tree.
- If the build directory contains Visual Studio artifacts (`.sln`, `.vcxproj`), it was configured with the wrong generator. Delete it and reconfigure with `-G Ninja`.

## Preferred Build Method

Prefer the built-in VS Code tasks over raw terminal commands:

| Task label | Purpose |
|---|---|
| **Configure CMake (Standard Pico)** | Fresh configure for `pico` board |
| **Configure CMake (Pico W)** | Fresh configure for `pico_w` board |
| **Configure CMake (Pico 2 W)** | Fresh configure for `pico2_w` / `Pico2W` boardconfig |
| **Clean and Configure (Pico W)** | `--fresh` configure for `pico_w` |
| **Clean and Configure (Pico 2 W)** | `--fresh` configure for `pico2_w` |
| **Compile Project** | Ninja build (incremental) |

These tasks set all required environment variables automatically. Use them whenever possible.

## Required Parameters for Manual cmake Invocations

Select one registered target with `GP2040_BOARD`. Use Pico SDK 2.3.1; do not also
supply `PICO_BOARD`, `PICO_PLATFORM`, or `GP2040_BOARDCONFIG`. The resolver derives
hardware selection before SDK import. Clear conflicting inherited legacy values.

```powershell
$env:PICO_SDK_PATH = "$env:USERPROFILE/.pico-sdk/sdk/2.3.1"
$env:SKIP_WEBBUILD = 'TRUE'
cmake -G Ninja -B build -S . --fresh -DGP2040_BOARD=Pico
```

Set `SKIP_WEBBUILD=FALSE` for a web-inclusive build. Wireless builds require
`pycryptodomex` in CMake's Python interpreter. List targets with
`cmake -G Ninja -P modules/ListBoards.cmake`. External target packages use
`GP2040_BOARD_DIRS`; this is a search path, not another board selector.

And to compile:

```powershell
& "$env:USERPROFILE/.pico-sdk/ninja/v1.12.1/ninja.exe" -C build
```

## Clean Configure

For a clean/fresh configure, add `--fresh` to the cmake invocation:

```powershell
cmake -G Ninja -B build -S . --fresh -DGP2040_BOARD=Pico
```

This is equivalent to the "Clean and Configure" VS Code tasks.

Changing targets in a reused tree requires `--fresh`. For full clean validation,
also run Ninja's `-t clean` before the full build, or use
`cmake -G Ninja -P tests/build-board-matrix.cmake`. That script includes a web build
for Pico and capability assertions for the reference targets.

## Hard Rules

- **Never** use `cmake -B build -S .` without `-G Ninja` in this repository.
- **Never** invoke `msbuild`, open `.sln` files to build, or use any Visual Studio build path.
- Use `GP2040_BOARD` for new builds. Only an absent selector defaults to `Pico`;
	supplied unknown targets fail instead of falling back.
- `GP2040_BOARDCONFIG` is a deprecated compatibility alias, not a second selector.
- Preserve the Pimoroni 4 MiB safety limit and existing EEPROM placement.
