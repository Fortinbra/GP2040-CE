---
name: Pico Build System
description: "Use when configuring, building, compiling, or running CMake for this project. Enforces Ninja as the only generator — never Visual Studio. Covers required environment variables, generator flag, and VS Code task usage for all GP2040-CE builds."
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

When running `cmake` directly (e.g., for a board not covered by the tasks above), **all** of the following must be set:

```powershell
$env:PICO_BOARD         = 'pico'          # or pico_w, pico2_w, etc.
$env:SKIP_WEBBUILD      = 'TRUE'          # omit only when a web build is explicitly needed
$env:GP2040_BOARDCONFIG = 'Pico'          # required for non-default board configs (e.g. PimoroniPicoLipo2XLW)
cmake -G Ninja -B build -S .
```

And to compile:

```powershell
& "$env:USERPROFILE/.pico-sdk/ninja/v1.12.1/ninja.exe" -C build
```

## Clean Configure

For a clean/fresh configure, add `--fresh` to the cmake invocation:

```powershell
cmake -G Ninja -B build -S . --fresh
```

This is equivalent to the "Clean and Configure" VS Code tasks.

## Hard Rules

- **Never** use `cmake -B build -S .` without `-G Ninja` in this repository.
- **Never** invoke `msbuild`, open `.sln` files to build, or use any Visual Studio build path.
- **Never** skip `PICO_BOARD` — the build will silently target the wrong chip.
- If `GP2040_BOARDCONFIG` is not set, CMake defaults to the `Pico` config. Always set it explicitly when targeting any other board.
