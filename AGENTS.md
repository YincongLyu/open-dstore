# dstore - Database Storage Engine

C++17 storage engine with CMake build system, gtest testing. Requires at least GCC 10.3 and specific dependencies.

# OpenCode Guidance

This repository keeps its operational guidance inside the `docs/` directory so OpenCode agents can read it directly.

## Build & Test Reference
- Use `docs/build-reference.md` for prerequisites, `build.sh` options, Docker tips, incremental workflows, and CI expectations.

## Coding Conventions
- Follow the detailed naming, formatting, and include rules in `docs/coding-style.md` and keep the copy-control patterns described there.

## Module Mapping
- Refer to `docs/module-mapping.md` when assigning commit modules or running module-specific unit tests. The same doc lists test targets that cover each subsystem.

## Project and Commit Expectations
- `src/`, `include/`, `interface/`, `tests/`, `utils/`, and `tools/` follow the namespace/prefix conventions in the coding guide. All contributions must prefix paths with `dstore_`.
- Commit messages continue to use the format enforced by `.githooks/commit-msg`: provide `Description`, optional `TicketNo`, and required `Module` (module names are listed in `docs/module-mapping.md`).

## Commit Message Format

Enforced by `.githooks/commit-msg`. Required format:

```
Description: <summary>
TicketNo: <ticket>          (optional)
Module: <module name>       (required)
```

Valid modules: Transaction State Manager, Centralized Lock Manager, Distributed Lock Manager, Heap Manager, Index Manager, Centralized Buffer Manager, Distributed Buffer Manager, Segment-page Storage Manager, XLog Manager, Undo Manager, Column Data Manager, Column Buffer Manager, SCM Cache Manager, Catalog Table Manager, SQL Engine, Tenant Resource Scheduler, CI

## Reference Notes
- Historical `.claude/` material lives in the same docs directory; there is no separate `.claude` support in OpenCode, so keep modifying the `docs/` files and `AGENTS.md` if the guidance changes.

## Active Technologies
- C++17 + dstore framework thread/runtime utilities, STL atomics/mutex/thread primitives, existing dstore logging macros and configuration framework (002-watchdog-heartbeat)
- In-memory watchdog metadata only; no persisted storage in the first release (002-watchdog-heartbeat)

## Recent Changes
- 002-watchdog-heartbeat: Added C++17 + dstore framework thread/runtime utilities, STL atomics/mutex/thread primitives, existing dstore logging macros and configuration framework
