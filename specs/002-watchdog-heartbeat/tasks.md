# Tasks: Watchdog Heartbeat Monitoring

**Input**: Design documents from `/specs/002-watchdog-heartbeat/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: Generate module/unit test tasks because the feature specification and constitution require concrete verification for runtime behavior.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Prepare watchdog feature documentation and framework file skeletons

- [ ] T001 Create watchdog framework header skeletons in include/framework/dstore_watchdog_mgr.h, include/framework/dstore_watchdog_entry.h, include/framework/dstore_watchdog_diagnose.h
- [ ] T002 [P] Create watchdog framework source skeletons in src/framework/dstore_watchdog_mgr.cpp and src/framework/dstore_watchdog_entry.cpp
- [ ] T003 [P] Add watchdog feature test file skeletons in tests/unittest/ut_framework/ut_watchdog_mgr.cpp and tests/unittest/ut_framework/ut_watchdog_entry.cpp

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Build the shared watchdog infrastructure required by all user stories

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T004 Define WatchDogStatus, WatchDogEntry, WatchDogDiagnose, and shared constants in include/framework/dstore_watchdog_entry.h and include/framework/dstore_watchdog_diagnose.h
- [ ] T005 Implement WatchDogEntry state transitions, timestamp bookkeeping, and reset logic in src/framework/dstore_watchdog_entry.cpp
- [ ] T006 Implement WatchDogMgr registry, scan loop, self-health handling, and diagnose snapshot assembly in include/framework/dstore_watchdog_mgr.h and src/framework/dstore_watchdog_mgr.cpp
- [ ] T007 Integrate watchdog manager startup/shutdown hooks with framework lifecycle in src/framework/dstore_instance.cpp and src/framework/dstore_thread.cpp
- [ ] T008 Add configuration plumbing for `enable_ai_watchdog`, `enable_ai_watchdog_healing`, and default 5-second interval in src/config/ and include/framework/dstore_watchdog_mgr.h
- [ ] T009 Implement framework unit tests for registry lifecycle, grace window, miss counting, self-health, and empty-diagnose behavior in tests/unittest/ut_framework/ut_watchdog_mgr.cpp and tests/unittest/ut_framework/ut_watchdog_entry.cpp

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Detect Hung Background Work (Priority: P1) 🎯 MVP

**Goal**: Detect overdue heartbeats for the six approved background-thread categories and emit distinct warning logs without killing the process.

**Independent Test**: Start the watchdog and simulate one monitored worker in each category stopping progress; verify that warning state and logs are produced within the configured behavior while healthy workers remain quiet.

### Tests for User Story 1 ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T010 [P] [US1] Add WAL watchdog integration tests in tests/unittest/ut_ha/ut_wal_watchdog.cpp
- [ ] T011 [P] [US1] Add buffer/checkpoint watchdog integration tests in tests/unittest/ut_buffer/ut_buffer_watchdog.cpp
- [ ] T012 [P] [US1] Add undo/index watchdog integration tests in tests/unittest/ut_undo/ut_watchdog_undo.cpp and tests/unittest/ut_index/ut_watchdog_btree.cpp

### Implementation for User Story 1

- [ ] T013 [US1] Wire watchdog register/feed/unregister into WAL flush lifecycle in src/wal/dstore_wal_bgwriter.cpp and src/wal/dstore_wal_logstream.cpp
- [ ] T014 [US1] Wire watchdog register/feed/unregister into WAL file recycle lifecycle in src/wal/dstore_wal_file_manager.cpp
- [ ] T015 [US1] Wire watchdog register/feed/unregister into checkpoint lifecycle in src/buffer/dstore_checkpointer.cpp
- [ ] T016 [US1] Wire watchdog register/feed/unregister into buffer page-writer manager/master/slave loops in src/buffer/dstore_bg_page_writer_mgr.cpp, src/buffer/dstore_bg_page_writer_base.cpp, and src/buffer/dstore_bg_disk_page_writer.cpp
- [ ] T017 [US1] Wire watchdog register/feed/unregister into undo dispatch and worker paths in src/undo/dstore_rollback_trx_task_mgr.cpp and src/undo/dstore_rollback_trx_worker.cpp
- [ ] T018 [US1] Wire watchdog register/feed/unregister into Btree recycle/prune worker path in src/index/dstore_btree_page_recycle.cpp, src/index/dstore_btree_recycle_partition.cpp, and src/index/dstore_btree_prune.cpp
- [ ] T019 [US1] Add warning-log emission paths for overdue peer tasks and watchdog self-health in src/framework/dstore_watchdog_mgr.cpp

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - Query Current Health State (Priority: P2)

**Goal**: Expose a diagnostic snapshot surface that returns the current health state for all active monitored entries and watchdog self-health.

**Independent Test**: Query the diagnose surface with zero entries, healthy entries, and overdue entries; verify returned records match thread names, categories, thresholds, timestamps, and unhealthy durations.

### Tests for User Story 2 ⚠️

- [ ] T020 [P] [US2] Add diagnostic snapshot contract tests in tests/unittest/ut_framework/ut_watchdog_diagnose.cpp
- [ ] T021 [P] [US2] Add upper-layer consumer integration test scaffolding for watchdog diagnose records in tests/unittest/ut_framework/ut_watchdog_sql_surface.cpp

### Implementation for User Story 2

- [ ] T022 [US2] Implement diagnose snapshot model population and sorting in src/framework/dstore_watchdog_mgr.cpp and include/framework/dstore_watchdog_diagnose.h
- [ ] T023 [US2] Add framework-facing diagnose API declarations and accessors in include/framework/dstore_watchdog_mgr.h and interface/framework/dstore_instance_interface.h
- [ ] T024 [US2] Integrate the watchdog diagnose contract for upper-layer consumption in specs/002-watchdog-heartbeat/contracts/watchdog-diagnose-contract.md and any corresponding framework adapter code under src/framework/

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - Use a Safe Default Failure Policy (Priority: P3)

**Goal**: Ensure the default operational policy remains log-only, configuration-gated, and lightweight under the 5-second scan cadence.

**Independent Test**: Run watchdog under default settings with representative monitored threads and verify that it stays below the agreed resource budget, emits warnings without aborting, and only changes behavior when configuration explicitly requests stronger action.

### Tests for User Story 3 ⚠️

- [ ] T025 [P] [US3] Add configuration-behavior tests for log-only vs healing-gated policy in tests/unittest/ut_framework/ut_watchdog_policy.cpp
- [ ] T026 [P] [US3] Add lightweight-overhead verification test scaffolding in tests/unittest/ut_framework/ut_watchdog_perf_budget.cpp

### Implementation for User Story 3

- [ ] T027 [US3] Implement log-only default policy and configuration-gated healing branch in src/framework/dstore_watchdog_mgr.cpp and include/framework/dstore_watchdog_mgr.h
- [ ] T028 [US3] Implement default 5-second wake interval handling and budget-aware bookkeeping in src/framework/dstore_watchdog_mgr.cpp
- [ ] T029 [US3] Add documentation-facing configuration and operational notes in specs/002-watchdog-heartbeat/quickstart.md and specs/002-watchdog-heartbeat/quickstart-cn.md

**Checkpoint**: All user stories should now be independently functional

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Finalize validation, docs alignment, and cross-module quality checks

- [ ] T030 [P] Update planning artifacts if implementation paths diverge in specs/002-watchdog-heartbeat/plan.md, research.md, data-model.md, and contracts/watchdog-diagnose-contract.md
- [ ] T031 Run quickstart validation commands from specs/002-watchdog-heartbeat/quickstart.md and record any required command corrections in specs/002-watchdog-heartbeat/quickstart.md and quickstart-cn.md
- [ ] T032 Run targeted watchdog verification suites via tmp_build for framework, WAL, buffer, undo, and index modules using docs/build-reference.md commands

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - MVP story
- **User Story 2 (P2)**: Depends on the core registry and entry model from Phase 2 and can follow US1 once active entries exist
- **User Story 3 (P3)**: Depends on foundational manager behavior and should follow after US1/US2 make runtime behavior observable

### Within Each User Story

- Tests MUST be written and FAIL before implementation
- Shared watchdog core before module integrations
- Module integrations before diagnostic or policy refinements
- Story complete before moving to next priority

### Parallel Opportunities

- T002 and T003 can run in parallel
- T010, T011, and T012 can run in parallel
- T020 and T021 can run in parallel
- T025 and T026 can run in parallel
- T030 and T031 can run in parallel once all stories finish

---

## Parallel Example: User Story 1

```bash
# Launch US1 test scaffolding together:
Task: "Add WAL watchdog integration tests in tests/unittest/ut_ha/ut_wal_watchdog.cpp"
Task: "Add buffer/checkpoint watchdog integration tests in tests/unittest/ut_buffer/ut_buffer_watchdog.cpp"
Task: "Add undo/index watchdog integration tests in tests/unittest/ut_undo/ut_watchdog_undo.cpp and tests/unittest/ut_index/ut_watchdog_btree.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Test independently → MVP ready
3. Add User Story 2 → Test independently → Diagnose surface ready
4. Add User Story 3 → Test independently → Safe default policy and budget complete

### Suggested MVP Scope

Suggested MVP is **User Story 1 only**: watchdog core plus six-thread heartbeat monitoring and warning behavior.

### Parallel Team Strategy

- Team members should finish Phase 1 and Phase 2 together first, because the shared watchdog types, manager lifecycle, and configuration plumbing block all later work.
- After the foundation is stable, the team can split by user story in parallel: US1 for heartbeat wiring, US2 for diagnose surface, and US3 for policy and budget behavior.
- Within US1, work can also split by subsystem ownership: WAL (`src/wal/`), buffer/checkpoint (`src/buffer/`), undo (`src/undo/`), and index (`src/index/`), while framework owners keep `src/framework/` and `include/framework/` aligned.
- Future TODO: if later phases expand to memory OOM detection or disk I/O stall analysis, add a refreshed team-parallel plan for those larger cross-module investigations.

---

## Notes

- All tasks follow the required checklist format with Task ID, optional `[P]`, optional `[USx]`, and exact file paths.
- Runtime verification is mandatory because the constitution requires evidence-backed completion for engine behavior.
- Keep `.gitignore` and `watchdog-monitor-discussion.md` out of implementation commits unless explicitly requested.
