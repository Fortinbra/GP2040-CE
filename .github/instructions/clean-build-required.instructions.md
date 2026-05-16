---
name: Clean Build Required
description: "Use when implementing or finishing any feature, bugfix, enhancement, or other code change in this repository. Work is not complete until the project has been configured and built successfully from a clean slate, with no exceptions."
applyTo: "**"
---
# Clean Build Required

- Any implemented feature, bugfix, enhancement, or other code change must end with a successful clean-slate configure and full build before the work is considered complete.
- No exceptions: if a clean rebuild has not succeeded, the work is not done.
- Prefer the repository's existing CMake and VS Code task workflow over ad hoc build commands when those project tools are available.

## Clean-Slate Standard

- Use a fresh configure/build path, not an incremental-only rebuild.
- Prefer the built-in clean configure task when available, or an equivalent fresh CMake configure for the relevant target.
- Follow the repository's default build conventions unless the task explicitly targets a different board or build mode.
- Default to the Standard Pico configuration unless the task clearly requires a different board.

## Target Selection

- Build the relevant project configuration for the work being done.
- If the task explicitly targets Pico W, Pico 2, Pico 2 W, or another board/config, use that target for the clean rebuild.
- If the change touches the WebUI, generated web assets, or firmware content that depends on the web build, use a clean build path that includes the web build rather than skipping it.

## Completion Rule

- Do not present implementation work as finished if the clean configure/build step was not run.
- Do not treat incremental compile success, partial target success, or diff-only review as a substitute for the clean rebuild requirement.
- If a clean build fails, keep the task in a not-done state and report the failure clearly.

## Output Guidance

- State which clean configuration and build path was used.
- State whether the build was run from a fresh configure state.
- If the build could not be completed, say so directly and do not claim the feature or fix is complete.