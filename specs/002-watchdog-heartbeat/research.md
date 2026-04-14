# Research: Watchdog Heartbeat Monitoring

## Decision 1: Global watchdog manager in framework layer

**Decision**: Keep `WatchDogMgr` as a global framework-level manager, with watchdog types under `include/framework/` and `src/framework/`.

**Rationale**: The spec fixes deployment at `StorageInstance` scope, while monitored workers live across WAL, buffer, undo, and index modules. A framework-level manager matches existing thread registration and lifecycle helpers and avoids duplicating monitoring logic per module.

**Alternatives considered**:
- Per-PDB watchdog managers — rejected because the approved design and spec explicitly choose a global manager.
- Module-local watchdog helpers — rejected because they would fragment lifecycle and diagnostic behavior.

## Decision 2: Self-health is scan-cycle progress, not ordinary heartbeat

**Decision**: Represent watchdog self-health with a manager-owned timestamp refreshed after each successful scan cycle, and emit a distinct self-health warning when overdue.

**Rationale**: Production code shows many manager-vs-worker lifecycle loops but no robust production abstraction for a thread monitoring itself as an ordinary peer. Using scan-cycle progress avoids recursive semantics and keeps self-health diagnosable separately.

**Alternatives considered**:
- Register watchdog as a normal monitored entry — rejected because it blurs self-health and peer-health semantics.
- No self-health signal — rejected because it weakens observability and leaves watchdog stalls ambiguous.

## Decision 3: First-release monitored set is exactly six categories

**Decision**: Monitor six categories only: WAL flush, WAL file recycle, checkpoint progress, buffer dirty-page flush, undo recycle dispatch, and Btree recycle/prune.

**Rationale**: The clarified spec fixes this exact first-release set. Each category maps to an identifiable long-running worker loop or manager loop in production code and can be wired into heartbeat reporting without expanding scope.

**Alternatives considered**:
- Monitor only three core categories — rejected because the clarified spec expands beyond the minimum three.
- Add memory/disk monitoring in this release — rejected by constitution and spec scope.

## Decision 4: 5-second default scan cadence

**Decision**: Use a 5-second default watchdog wake interval for the monitor thread.

**Rationale**: Existing dstore management loops are periodic and low-frequency rather than high-frequency polling. A 5-second cadence is lighter than the original 1-second default while remaining comfortably inside the 30-second alerting window when combined with threshold/miss logic.

**Alternatives considered**:
- 1-second cadence — rejected as unnecessarily aggressive for a lightweight first release.
- 10-second cadence — rejected because it increases alert latency without clear first-release benefit.

## Decision 5: Lightweight overhead model and budget

**Decision**: Treat watchdog runtime cost as a low-overhead linear scan over active entries, with a first-release budget of <1% CPU and <1MB extra memory under default configuration.

**Rationale**: Existing framework thread-management structures use array/slot patterns and thread-info snapshots that suggest small fixed metadata plus linear scans over live entries. This supports a conservative but achievable first-release budget without inventing a precise microbenchmark dependency.

**Alternatives considered**:
- Strict micro-budget (<0.1% CPU, <256KB) — rejected as too brittle for first-release acceptance.
- Purely qualitative “low overhead” wording — rejected because the clarified spec now requires measurable completion signals.

## Decision 6: Heartbeat wiring follows existing worker-loop boundaries

**Decision**: Integrate heartbeat reporting into existing worker loops at natural progress points instead of wrapping whole modules in new scheduling abstractions.

**Rationale**: Code-path mapping shows clear long-running loops in `BgWalWriter::BgFlushMain`, `WalFileManager::RecycleWalFileWorkerMain`, `CheckpointMgr::CheckpointerMain`, page-writer run loops, `RollbackTrxTaskMgr::DispatchMain`, and `BtreeRecycleWorker::BtreeRecycleThreadMain`. Using those loops preserves existing ownership and stop/join behavior.

**Alternatives considered**:
- Add a separate proxy thread per module — rejected as extra overhead and lifecycle complexity.
- Hook only init/stop points — rejected because it would not provide real liveness evidence between startup and shutdown.

## Code Mapping Notes

- **WAL flush**: `src/wal/dstore_wal_bgwriter.cpp`, `src/wal/dstore_wal_logstream.cpp`
- **WAL file recycle**: `src/wal/dstore_wal_file_manager.cpp`
- **Checkpoint**: `src/buffer/dstore_checkpointer.cpp`
- **Buffer page writers**: `src/buffer/dstore_bg_page_writer_mgr.cpp`, `src/buffer/dstore_bg_page_writer_base.cpp`, `src/buffer/dstore_bg_disk_page_writer.cpp`
- **Undo dispatch**: `src/undo/dstore_rollback_trx_task_mgr.cpp`, `src/undo/dstore_rollback_trx_worker.cpp`
- **Btree recycle/prune**: `src/index/dstore_btree_page_recycle.cpp`, `src/index/dstore_btree_recycle_partition.cpp`, `src/index/dstore_btree_prune.cpp`
- **Framework lifecycle helpers**: `src/framework/dstore_instance.cpp`, `src/framework/dstore_thread.cpp`, `interface/framework/dstore_instance_interface.h`, `interface/framework/dstore_thread_interface.h`
