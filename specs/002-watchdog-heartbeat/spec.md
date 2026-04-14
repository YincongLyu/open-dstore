# Feature Specification: Watchdog Heartbeat Monitoring

**Feature Branch**: `002-watchdog-heartbeat`  
**Created**: 2026-04-14  
**Status**: Draft  
**Input**: User description: "I think before ./.specify/memory/constitution.md is wonderfull，my desire is in docs/superpowers/specs/2026-04-14-watchdog-design.md, I will check your checklist later, ulw"

## Clarifications

### Session 2026-04-14

- Q: How should the watchdog thread detect itself, and how is that different from detecting other background threads? → A: Watchdog uses a dedicated self-health timestamp refreshed after each successful scan cycle; overdue self-health emits a distinct self-health warning rather than using the ordinary peer-thread heartbeat path.
- Q: Which background threads are in scope for the first release? → A: The first release monitors six categories: WAL flush, WAL file recycle, checkpoint progress, buffer dirty-page flush, undo recycle dispatch, and Btree recycle/prune.
- Q: What should the first-release watchdog wake interval be? → A: The first release uses a 5-second watchdog wake interval as the default self-wakeup cadence.
- Q: What performance and memory budget should the first release meet? → A: Under the default 5-second wake interval, watchdog overhead must stay below 1% CPU and below 1MB of additional memory.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Detect Hung Background Work (Priority: P1)

As a database operator, I want the storage engine to detect when a critical
background task stops reporting liveness so that I can identify hang events
before they turn into prolonged service outages.

**Why this priority**: Early detection is the core value of the feature. Without
it, operators only see indirect symptoms such as stalled throughput or timeouts.

**Independent Test**: Start the engine with monitored background work enabled,
stop liveness reporting for one monitored task, and verify that the engine emits
the expected warning after the configured timeout policy is exceeded.

**Acceptance Scenarios**:

1. **Given** a monitored background task is healthy, **When** it continues to
   report liveness within its allowed threshold, **Then** no hang warning is
   emitted for that task.
2. **Given** a monitored background task stops reporting liveness beyond its
   configured threshold, **When** the missed-heartbeat policy is met,
   **Then** the engine records a warning that identifies the affected task.

---

### User Story 2 - Query Current Health State (Priority: P2)

As an upper-layer service or operator, I want to query the current health view of
monitored background tasks so that I can diagnose which task is delayed and how
long it has been unhealthy.

**Why this priority**: Logging alone is not enough for live diagnosis. A current
health view is needed for troubleshooting and operational tooling.

**Independent Test**: Request the diagnostic view while tasks are healthy and
again after one task exceeds its timeout; verify that the returned records show
current status and last known liveness information for each monitored task.

**Acceptance Scenarios**:

1. **Given** monitored tasks are registered, **When** the diagnostic interface is
   queried, **Then** it returns one record per monitored task with its current
   health state.
2. **Given** a monitored task has exceeded its timeout, **When** the diagnostic
   interface is queried, **Then** the returned record marks that task unhealthy
   and includes its last reported liveness time and unhealthy duration.

---

### User Story 3 - Use a Safe Default Failure Policy (Priority: P3)

As an operator, I want the first release of watchdog monitoring to alert by
default rather than terminate the process so that the feature improves visibility
without creating new outage risk.

**Why this priority**: Safe rollout is required for a new runtime detection
feature. Operators need observability first and stronger responses only when they
choose to enable them.

**Independent Test**: Trigger a timeout under default settings and verify that
the system emits an alert while remaining available for further diagnosis.

**Acceptance Scenarios**:

1. **Given** the default watchdog policy is in effect, **When** a monitored task
   is judged unhealthy, **Then** the system emits an alert and continues running.

### Edge Cases

- A newly registered task receives one full timeout window before it can be
  judged unhealthy if it has not yet begun active work.
- A task that resumes liveness reporting after temporary delay or load jitter is
  returned to a healthy state and its consecutive-miss count is reset.
- A task that stops normally and unregisters is removed from active monitoring
  and no longer appears in the current diagnostic view.
- If no tasks are currently registered, the diagnostic view returns an empty
  result rather than a failure.
- If the monitoring facility itself becomes delayed, it emits a distinct
  self-health warning based on overdue scan-cycle progress rather than
  attributing that failure to an unrelated task or evaluating itself through the
  ordinary peer-thread heartbeat path.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST allow designated storage-engine background tasks to
  be registered for liveness monitoring.
- **FR-002**: The system MUST allow a monitored task to report liveness updates
  repeatedly during its active lifetime.
- **FR-003**: The system MUST stop monitoring a task after that task has been
  explicitly unregistered.
- **FR-004**: The system MUST evaluate each monitored task against a task-specific
  timeout threshold.
- **FR-005**: The system MUST avoid alerting on a single delayed report and MUST
  use a consecutive-miss policy before declaring a task unhealthy.
- **FR-006**: The system MUST emit an operator-visible warning when a monitored
  task is judged unhealthy.
- **FR-007**: Warning output MUST identify the affected task and the scope it
  belongs to.
- **FR-008**: The system MUST expose a diagnostic view that lists all currently
  monitored tasks and their latest health state.
- **FR-009**: The diagnostic view MUST include each task's last reported liveness
  time, configured timeout threshold, and current unhealthy duration when
  applicable.
- **FR-010**: The first release MUST cover exactly six monitored background task
  categories: WAL flush, WAL file recycle, checkpoint progress, buffer
  dirty-page flush, undo recycle dispatch, and Btree recycle/prune.
- **FR-011**: The default reaction to an unhealthy task MUST be alerting only.
- **FR-012**: If stronger remediation is supported, the system MUST require an
  explicit operator-controlled setting before using it.
- **FR-013**: A newly registered task MUST receive an initial grace period equal
  to its timeout threshold before it can be declared unhealthy.
- **FR-014**: When a previously overdue task resumes liveness reporting, the
  system MUST restore that task to a healthy state and clear its consecutive-miss
  state.
- **FR-015**: If no tasks are currently registered, the diagnostic view MUST
  return an empty result set.
- **FR-016**: The monitoring facility MUST expose its own delayed-health signal
  separately from monitored task warnings.
- **FR-017**: Watchdog self-health MUST be evaluated through a dedicated
  scan-cycle progress timestamp owned by the watchdog thread rather than through
  the ordinary monitored-task heartbeat path used for peer threads.
- **FR-018**: If watchdog self-health becomes overdue, the system MUST emit a
  distinct self-health warning that is distinguishable from ordinary monitored
  task warnings.
- **FR-019**: The first release MUST use a default watchdog self-wakeup interval
  of 5 seconds for scan-cycle execution.
- **FR-020**: Under the default 5-second wake interval, the first release MUST
  target watchdog overhead below 1% CPU and below 1MB of additional memory.

### Operational Observability & Safety *(mandatory for runtime features)*

- **OO-001**: The monitored runtime signal is task liveness reporting from each
  registered background task.
- **OO-002**: Operators must be able to observe unhealthy tasks through warning
  output and a current diagnostic query path.
- **OO-003**: The default safety posture is non-destructive alerting so the
  system remains available for diagnosis after detection.
- **OO-004**: Monitoring lifecycle boundaries include task registration, active
  liveness reporting, normal unregistration, and watcher self-health visibility.
- **OO-005**: Watchdog self-health visibility is based on successful completion
  of its own scan loop, while peer-task visibility is based on externally
  reported liveness updates from each monitored task.
- **OO-006**: The default watchdog wake interval MUST favor lightweight periodic
  scanning over high-frequency polling; the first release default is 5 seconds.
- **OO-007**: Watchdog diagnostics and bookkeeping MUST remain lightweight enough
  that the default configuration stays within the first-release CPU and memory
  budget.

### Key Entities *(include if feature involves data)*

- **Monitored Task**: A background engine task that opts into watchdog tracking,
  with an identity, scope, and timeout policy.
- **Health Snapshot**: The current diagnostic record for a monitored task,
  including its latest liveness evidence and health state.
- **Warning Event**: An operator-visible alert produced when a monitored task is
  considered unhealthy under the configured miss policy.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of registered watchdog-covered tasks appear in the diagnostic
  view while they are active.
- **SC-002**: When a covered task stops reporting liveness and remains overdue,
  the system emits an operator-visible warning within 30 seconds under default
  monitoring settings.
- **SC-003**: Under default settings, 100% of unhealthy-task detections leave the
  process running so operators can continue diagnosis.
- **SC-004**: The diagnostic view identifies the unhealthy task, its scope, and
  its last reported liveness evidence for 100% of simulated timeout cases.
- **SC-005**: Under the default 5-second wake interval, watchdog processing uses
  less than 1% CPU in steady state during representative background-thread
  activity.
- **SC-006**: The first-release watchdog adds less than 1MB of memory overhead,
  including task metadata and diagnostic bookkeeping, under the configured
  monitored-thread set.

## Assumptions

- The first increment is limited to thread-heartbeat monitoring and excludes
  memory monitoring, disk health monitoring, warning retention, and stack
  capture.
- The first release monitored set is fixed to six categories: WAL flush, WAL
  file recycle, checkpoint progress, buffer dirty-page flush, undo recycle
  dispatch, and Btree recycle/prune.
- The first release uses a 5-second watchdog self-wakeup interval as the default
  scan cadence.
- The first-release resource budget is capped at less than 1% CPU and less than
  1MB of additional memory under the default configuration.
- Registration establishes the initial liveness baseline for a monitored task,
  giving that task one full timeout window before first-time overdue evaluation.
- Watchdog self-health is represented by scan-cycle progress owned by the
  watchdog thread itself, not by registering the watchdog as a normal monitored
  peer task.
- The primary consumers are storage-engine operators and upper-layer database
  services that need current health information for diagnosis.
- Existing repository guidance for module mapping, logging, and validation will
  be used during planning and implementation.
- Critical background work already has identifiable task boundaries that can be
  monitored without redefining business ownership.
- Stronger remediation actions, if added later, will be introduced as separate
  follow-up scope rather than silently included in the first release.
