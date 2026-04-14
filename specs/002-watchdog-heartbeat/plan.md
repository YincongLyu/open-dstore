# Implementation Plan: Watchdog Heartbeat Monitoring

**Branch**: `002-watchdog-heartbeat` | **Date**: 2026-04-14 | **Spec**: `/home/yincong/dstore_github_repo/open-dstore/specs/002-watchdog-heartbeat/spec.md`
**Input**: Feature specification from `/home/yincong/dstore_github_repo/open-dstore/specs/002-watchdog-heartbeat/spec.md`

## Summary

Implement a lightweight watchdog manager thread in dstore framework that monitors six existing background-thread categories through heartbeat entries, exposes current health diagnostics, and defaults to log-only behavior. The implementation will anchor lifecycle ownership in existing thread start/stop paths, use a 5-second scan cadence with per-thread timeout thresholds, and stay within the first-release budget of <1% CPU and <1MB additional memory.

## Technical Context

**Language/Version**: C++17  
**Primary Dependencies**: dstore framework thread/runtime utilities, STL atomics/mutex/thread primitives, existing dstore logging macros and configuration framework  
**Storage**: In-memory watchdog metadata only; no persisted storage in the first release  
**Testing**: gtest-based unit targets via `make run_dstore_framework_unittest`, `make run_dstore_ha_unittest`, `make run_dstore_buffer_unittest`, `make run_dstore_undo_unittest`, and `make run_dstore_index_unittest`  
**Target Platform**: Linux server / WSL-Linux build environment used by dstore  
**Project Type**: C++ storage engine library with internal background-thread infrastructure  
**Performance Goals**: Detect overdue heartbeat conditions with default 5-second scan cadence; emit warnings within the spec-defined 30-second window; keep watchdog overhead under 1% CPU and under 1MB additional memory  
**Constraints**: First increment is thread-heartbeat monitoring only; exactly six monitored categories; log-only by default; explicit lifecycle registration/unregistration; no history retention, memory monitoring, disk monitoring, or stack capture in this phase  
**Scale/Scope**: One global watchdog manager observing six background-thread categories across StorageInstance / per-PDB managers, with linear scan cost over active monitored entries

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Minimal increment identified**: PASS — first release is limited to heartbeat monitoring for six thread categories; memory, disk, retention, stack capture, and stronger healing remain deferred.
- **Runtime observability defined**: PASS — monitored signal is per-thread heartbeat reporting; outputs are warning logs plus a diagnostic query surface returning health snapshots.
- **Lifecycle ownership explicit**: PASS — registration, feed, self-health scan-cycle progress, unregistration, and stop/join paths are mapped to existing thread lifecycle entrypoints.
- **Failure policy safe by default**: PASS — default behavior is log-only; any stronger action remains configuration-gated and out of scope for first-release implementation work.
- **Validation tied to real repo structure**: PASS — framework, WAL, buffer, undo, and index modules plus their mapped test targets are identified from `docs/module-mapping.md` and `docs/build-reference.md`.

## Project Structure

### Documentation (this feature)

```text
specs/002-watchdog-heartbeat/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── watchdog-diagnose-contract.md
└── tasks.md
```

### Source Code (repository root)

```text
include/framework/
├── dstore_watchdog_mgr.h
├── dstore_watchdog_entry.h
└── dstore_watchdog_diagnose.h

src/framework/
├── dstore_watchdog_mgr.cpp
├── dstore_watchdog_entry.cpp
├── dstore_instance.cpp
└── dstore_thread.cpp

src/wal/
├── dstore_wal_bgwriter.cpp
├── dstore_wal_logstream.cpp
└── dstore_wal_file_manager.cpp

src/buffer/
├── dstore_checkpointer.cpp
├── dstore_bg_page_writer_mgr.cpp
├── dstore_bg_page_writer_base.cpp
└── dstore_bg_disk_page_writer.cpp

src/undo/
├── dstore_rollback_trx_task_mgr.cpp
└── dstore_rollback_trx_worker.cpp

src/index/
├── dstore_btree_page_recycle.cpp
├── dstore_btree_recycle_partition.cpp
└── dstore_btree_prune.cpp

tests/
└── [existing module/unit targets selected per affected modules]
```

**Structure Decision**: Use the existing dstore single-project layout and add watchdog framework types under `include/framework/` and `src/framework/`. Integrate heartbeat calls into existing background worker loops in WAL, buffer, undo, and index modules instead of creating new module-local watchdog infrastructure.

## Phase 0: Research Summary

- Existing production code uses manager/worker lifecycle patterns, stop flags, timed waits, and thread registration helpers rather than a reusable watchdog abstraction.
- Watchdog self-health should be modeled as manager-owned scan-cycle progress, not as a normal peer-thread heartbeat entry.
- The six in-scope thread categories map cleanly to concrete worker loops in WAL, buffer, undo, and index code paths, with prune work remaining coupled to the Btree recycle worker path for first-release scope.
- Existing framework/thread info structures indicate low-overhead linear scans and small per-entry metadata are a good fit for the first release budget.

## Phase 1: Design Summary

- `WatchDogMgr` remains a global manager in framework code with one monitor thread, one self-health signal, and a registry of active `WatchDogEntry` objects.
- `WatchDogEntry` is owned by the monitored worker side; `Register`/`Unregister` stay aligned with existing worker init/stop boundaries.
- Heartbeat insertion points live inside long-running worker loops: WAL flush loop, WAL recycle loop, checkpoint loop, page-writer loops, rollback dispatch loop, and Btree recycle worker loop.
- The diagnostic surface is an internal contract for upper-layer consumers, not a persisted storage feature.

## Validation Strategy

- **Framework coverage**: `make run_dstore_framework_unittest` for watchdog manager behavior, self-health handling, and lifecycle bookkeeping.
- **WAL integration coverage**: `make run_dstore_ha_unittest` for WAL writer and WAL file recycle integration.
- **Buffer integration coverage**: `make run_dstore_buffer_unittest` for checkpoint/page-writer integration.
- **Undo integration coverage**: `make run_dstore_undo_unittest` for rollback dispatch integration.
- **Index integration coverage**: `make run_dstore_index_unittest` for Btree recycle/prune integration points.
- **Build validation**: source `buildenv`, build utils if needed, and run targeted incremental rebuild from `tmp_build`.

## Post-Design Constitution Check

- **Minimal increment preserved**: PASS — design still excludes memory/disk monitoring, history retention, and stack capture.
- **Observability preserved**: PASS — warning logs, diagnostic snapshots, and self-health warnings remain explicit.
- **Lifecycle safety preserved**: PASS — every monitored thread category has identified register/feed/unregister boundaries in real worker code.
- **Safe failure preserved**: PASS — no process-killing behavior added in planning artifacts.
- **Evidence-backed verification preserved**: PASS — module-specific test targets and file paths remain concrete.
