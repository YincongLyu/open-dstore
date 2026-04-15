# Data Model: Watchdog Heartbeat Monitoring

## Entity: WatchDogMgr

**Purpose**: Global manager that owns the watchdog monitor thread, the active registry of monitored entries, self-health state, and the diagnostic snapshot surface.

### Fields

- `entries`: collection of active `WatchDogEntry` objects keyed by entry identity
- `running`: whether the watchdog monitor thread is active
- `checkInterval`: default 5-second scan cadence
- `selfHealthTimestamp`: last successful scan-cycle completion time
- `defaultPolicy`: default reaction policy (log-only in first release)

### Relationships

- Owns zero or more `WatchDogEntry` registrations
- Produces zero or more `HealthSnapshot` records on diagnostic query
- Produces `WarningEvent` records/logs when peer or self-health becomes overdue

### Validation Rules

- Must not monitor more than the six approved thread categories in first release
- Must keep self-health separate from ordinary peer-task heartbeat semantics
- Must support an empty peer-entry registry without error while still exposing watchdog self-health

## Entity: WatchDogEntry

**Purpose**: Runtime metadata for one monitored background worker or manager loop.

### Fields

- `entryId`: unique identity within the watchdog registry
- `threadCategory`: one of the six approved first-release categories
- `threadName`: canonical diagnostic name
- `scopeId`: owning scope such as PDB or global context
- `timeoutThreshold`: per-entry timeout threshold
- `lastHeartbeatTime`: latest peer-reported progress time
- `consecutiveMissCount`: current miss streak
- `healthState`: healthy / warn / unhealthy
- `registered`: whether entry is active in the registry

### Relationships

- Registered into exactly one `WatchDogMgr`
- Produces one current `HealthSnapshot`
- May cause one or more `WarningEvent` emissions over its lifetime

### Validation Rules

- Registration must precede heartbeat reporting
- Unregistration removes the entry from active diagnostics
- New registrations receive one full timeout window before first-time overdue evaluation

## Entity: HealthSnapshot

**Purpose**: Diagnostic record exposed to upper-layer consumers for the current health view.

### Fields

- `entryId`
- `threadName`
- `threadCategory`
- `scopeId`
- `timeoutThreshold`
- `lastHeartbeatTime`
- `unhealthyDuration`
- `healthState`
- `isSelfHealth`: whether this record describes watchdog self-health

### Relationships

- Derived from either one `WatchDogEntry` or watchdog self-health state

### Validation Rules

- Must be present for every active monitored entry
- Must be distinguishable between peer-task health and watchdog self-health

## Entity: WarningEvent

**Purpose**: Operator-visible alert generated when health state crosses into overdue/unhealthy territory.

### Fields

- `warningType`: peer-task warning or watchdog self-health warning
- `threadName`
- `threadCategory`
- `scopeId`
- `observedAt`
- `overdueDuration`
- `timeoutThreshold`
- `missCount`

### Validation Rules

- Must identify the affected task or watchdog self-health distinctly
- First release uses warning/log behavior only; no persisted retention required

## State Transitions

### WatchDogEntry

`unregistered -> registered/grace -> healthy -> warn -> unhealthy`

- `registered/grace -> healthy`: heartbeat received within threshold
- `healthy -> warn`: threshold exceeded but miss streak below unhealthy threshold
- `warn -> unhealthy`: consecutive-miss threshold reached
- `warn/unhealthy -> healthy`: heartbeat resumes and miss streak resets
- `registered/* -> unregistered`: worker stops and unregisters cleanly

### WatchDogMgr self-health

`healthy -> overdue-self-health -> healthy`

- `healthy -> overdue-self-health`: scan-cycle progress timestamp becomes overdue
- `overdue-self-health -> healthy`: a subsequent scan cycle completes successfully
