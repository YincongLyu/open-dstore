# Quickstart: Watchdog Heartbeat Monitoring

## Goal

Validate the planning assumptions for the first-release watchdog heartbeat feature and prepare the repository for implementation and verification.

## Prerequisites

```bash
source buildenv
cd utils && bash build.sh -m debug && cd ..
```

## Build the project

```bash
bash build.sh -m debug -st on -tm ut
```

## Run targeted verification suites

```bash
cd tmp_build
make run_dstore_framework_unittest
make run_dstore_ha_unittest
make run_dstore_buffer_unittest
make run_dstore_undo_unittest
make run_dstore_index_unittest
```

## Implementation checkpoints

1. Add watchdog framework types under `include/framework/` and `src/framework/`.
2. Wire register/feed/unregister calls into the six approved background-thread categories.
3. Expose the internal diagnostic snapshot surface for upper-layer consumption.
4. Verify default behavior is log-only and self-health warnings are distinct.
5. Re-run the targeted module test suites plus any new framework tests.

## Manual verification expectations

After implementation, confirm the following in a debug build:

- A healthy monitored worker does not emit warnings.
- An overdue monitored worker emits a warning within the spec-defined time window.
- The diagnostic snapshot surface shows current state for all active entries.
- Watchdog self-health warnings are distinguishable from peer-thread warnings.
- Default 5-second cadence remains lightweight under representative background-thread activity.
