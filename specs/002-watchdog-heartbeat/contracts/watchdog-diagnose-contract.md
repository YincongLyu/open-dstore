# Contract: Watchdog Diagnose Snapshot

## Purpose

Define the internal contract for the watchdog diagnostic surface consumed by upper-layer database services.

## Consumer

- Upper-layer MySQL/SQL-engine integration that queries current watchdog health state

## Producer

- `WatchDogMgr` diagnostic snapshot API in framework code

## Contract Shape

Each returned record must contain:

- `thread_name`
- `thread_category`
- `scope_id`
- `timeout_threshold_ms`
- `last_heartbeat_time`
- `health_state`
- `unhealthy_duration_ms`
- `is_self_health`

## Behavioral Rules

- One record is returned for each active monitored entry.
- A distinct record or signal is returned for watchdog self-health when applicable.
- If no entries are registered, the query returns an empty result set.
- The contract is read-only in first release; no acknowledge/reset/retention operations are included.

## Compatibility Notes

- First release scope is limited to six monitored thread categories.
- Memory monitoring, disk monitoring, warning retention, and stack capture are explicitly out of scope.
