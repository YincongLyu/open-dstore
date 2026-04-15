# 数据模型：Watchdog 心跳监控

## 核心实体

### 1. WatchDogMgr

**职责**：全局 watchdog 管理器，负责维护监控线程、注册表、自监控状态和诊断快照输出。

| 字段 | 含义 |
|------|------|
| `entries` | 当前活跃的 `WatchDogEntry` 集合 |
| `running` | 监控线程是否正在运行 |
| `checkInterval` | 默认扫描周期，第一版固定为 5s |
| `selfHealthTimestamp` | 最近一次成功完成扫描循环的时间戳 |
| `defaultPolicy` | 默认失败策略，第一版为 log-only |

**约束**：
- 第一版不能监控超过 6 类线程；
- 自监控与普通线程监控必须分离；
- 即使当前没有活跃 peer entry，也必须返回 watchdog 自健康记录而不是报错。

### 2. WatchDogEntry

**职责**：表示一个被监控后台线程的运行时元数据。

| 字段 | 含义 |
|------|------|
| `entryId` | 注册表内唯一 ID |
| `threadCategory` | 所属线程类别（6 类之一） |
| `threadName` | 用于日志和诊断的线程名 |
| `scopeId` | 所属范围，如 PDB / 全局上下文 |
| `timeoutThreshold` | 当前线程独立的超时阈值 |
| `lastHeartbeatTime` | 最近一次心跳上报时间 |
| `consecutiveMissCount` | 连续 miss 次数 |
| `healthState` | healthy / warn / unhealthy |
| `registered` | 当前是否仍在 registry 中 |

**约束**：
- 必须先 Register 再 Feed；
- Unregister 后不再出现在诊断结果中；
- 新注册线程享有一个完整 timeout window 的 grace period。

### 3. HealthSnapshot

**职责**：向上层诊断接口暴露当前健康状态快照。

| 字段 | 含义 |
|------|------|
| `entryId` | 对应被监控对象的 ID |
| `threadName` | 线程名 |
| `threadCategory` | 线程类别 |
| `scopeId` | 所属范围 |
| `timeoutThreshold` | 超时阈值 |
| `lastHeartbeatTime` | 最近心跳时间 |
| `unhealthyDuration` | 已持续异常的时间 |
| `healthState` | 当前状态 |
| `isSelfHealth` | 是否为 watchdog 自健康记录 |

**约束**：
- 所有活跃 entry 都要能生成 snapshot；
- 自监控 snapshot 必须能和普通线程 snapshot 明确区分。

### 4. WarningEvent

**职责**：watchdog 发出的操作员可见告警事件。

| 字段 | 含义 |
|------|------|
| `warningType` | 普通线程告警 / watchdog 自告警 |
| `threadName` | 线程名 |
| `threadCategory` | 线程类别 |
| `scopeId` | 所属范围 |
| `observedAt` | 告警产生时间 |
| `overdueDuration` | 超时持续时间 |
| `timeoutThreshold` | 触发时的阈值 |
| `missCount` | 连续 miss 次数 |

**约束**：
- 第一版只做日志告警，不做持久化保留；
- 必须清晰区分普通线程告警与 watchdog 自告警。

## 实体关系

```text
WatchDogMgr
  ├── owns ────────> WatchDogEntry (0..N)
  ├── exports ─────> HealthSnapshot (0..N)
  └── emits ───────> WarningEvent (0..N)

WatchDogEntry
  ├── derives -----> HealthSnapshot
  └── may emit ----> WarningEvent

WatchDogMgr self-health
  ├── derives -----> HealthSnapshot(isSelfHealth=true)
  └── may emit ----> WarningEvent(type=self-health)
```

## 状态流转

### WatchDogEntry 状态机

```text
unregistered
   │ Register
   ▼
registered/grace
   │ Feed within threshold
   ▼
healthy
   │ threshold exceeded
   ▼
warn
   │ miss count 达到阈值
   ▼
unhealthy
   │ Feed resumes
   └───────────────> healthy

任意 registered 状态
   │ Unregister
   ▼
unregistered
```

### WatchDogMgr 自健康状态机

```text
healthy
   │ scan-cycle progress overdue
   ▼
overdue-self-health
   │ next scan completes successfully
   ▼
healthy
```

## 第一版数据边界

- 只保留内存态元数据；
- 不做历史 retention；
- 不引入 memory / disk 监控数据结构；
- 不在第一版中引入 stack trace 或 crash dump 相关对象。
