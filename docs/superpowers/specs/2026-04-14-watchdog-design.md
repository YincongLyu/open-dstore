# Watchdog 监控框架设计文档

日期：2026-04-14
状态：Draft

## 一、背景与目标

dstore 存储引擎包含多个关键后台线程（WAL 刷盘、Checkpoint、Buffer 刷脏、Undo 回收等），这些线程若发生卡死（死锁、死循环、IO 长时间阻塞），会导致存储引擎停止推进，最终引起业务超时。

**目标**：
1. 监控关键后台线程是否处于活跃状态
2. 若检测到线程疑似卡死（超过配置的超时阈值未上报心跳），自动打印告警日志
3. 提供诊断接口，上层 MySQL Server 可主动查询各后台线程的健康状态

**第一版范围**：仅实现 Thread heartbeat 监控，后续迭代扩展 Memory 和 Disk I/O 监控。

## 二、总体架构

### 2.1 部署层级

WatchDogMgr 作为全局单例部署在 StorageInstance 级别，监控所有 PDB 的后台线程。

### 2.2 核心组件

| 组件 | 职责 |
|------|------|
| WatchDogMgr | 全局单例，管理所有监控项，运行检测线程，提供诊断接口 |
| WatchDogEntry | 单个线程的监控记录，持有心跳时间戳、超时阈值、状态等 |
| WatchDogDiagnose | 诊断信息结构，向上层 MySQL Server 暴露 |

### 2.3 类结构

```
using EntryId = uint64;
constexpr EntryId INVALID_ENTRY_ID = 0;

WatchDogMgr (全局单例)
├── m_entries: unordered_map<EntryId, WatchDogEntry*>
├── m_mutex: mutex (保护 m_entries)
├── m_thread: unique_ptr<thread> (监控线程)
├── m_running: atomic<bool>
├── m_checkIntervalMs: uint32 (默认 1000ms)
└── m_selfEntry: WatchDogEntry* (自注册心跳)

WatchDogEntry
├── m_name: const char* (线程名)
├── m_pdbId: PdbId
├── m_timeoutMs: uint32 (超时阈值，注册时指定)
├── m_lastHeartbeat: atomic<uint64> (时间戳，微秒)
├── m_timeoutCount: atomic<uint32> (连续超时计数)
├── m_status: atomic<WatchDogStatus> (ALIVE/WARNING/HUNG)
├── m_onTimeout: function<void()> (可选回调)
└── m_registered: atomic<bool> (是否已注册)

WatchDogDiagnose
├── threadName: const char*
├── pdbId: PdbId
├── lastHeartbeatTime: uint64 (微秒)
├── timeoutThreshold: uint32 (毫秒)
├── isHung: bool
├── hungDurationMs: uint64 (若卡死，持续时长)
└── timeoutCount: uint32

enum WatchDogStatus {
    ALIVE = 0,
    WARNING = 1,    // 超时计数达到阈值前
    HUNG = 2        // 触发告警
};
```

## 三、核心流程

### 3.1 后台线程生命周期

```
Thread Init:
    WatchDogMgr::Register(entry)  → 返回 entryId

Thread Main Loop:
    while (!needStop) {
        WatchDogMgr::FeedTask(entryId)  // 心跳上报
        ... do actual work ...
    }

Thread Stop:
    WatchDogMgr::Unregister(entryId)  // 反注册
    entry 销毁
```

**关键点**：
- Register/Unregister 用于生命周期管理
- FeedTask 用于心跳上报，线程自主控制上报时机
- 线程可在实际工作循环开始前先 Register，时机灵活

### 3.2 WatchDogMgr 检测流程

检测线程每 `m_checkIntervalMs` (默认 1s) 执行一次：

```
for each entry in m_entries:
    now = GetCurrentTimeInMicrosecond()
    elapsedMs = (now - entry.m_lastHeartbeat) / 1000
    
    if elapsedMs > entry.m_timeoutMs:
        entry.m_timeoutCount++
        if entry.m_timeoutCount >= 4:
            TriggerWarning(entry)  // 打印 DSTORE_ERROR，调用回调
            entry.m_status = HUNG
        else:
            entry.m_status = WARNING
    else:
        entry.m_timeoutCount = 0
        entry.m_status = ALIVE
```

### 3.3 WatchDog 线程自我保护

WatchDogMgr 将自己注册为监控项（名为 "WatchDogMonitor"），若自身超时则打印特殊告警日志，区分于业务线程超时。

### 3.4 告警行为

触发告警后的行为可配置：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| enable_ai_watchdog | ON | 是否启用 watchdog 监控 |
| enable_ai_watchdog_healing | OFF | 告警后是否 abort 进程 |

告警时：
1. 打印 DSTORE_ERROR 日志，包含线程名、pdbId、卡死持续时间
2. 若 `enable_ai_watchdog_healing=ON`，调用 `entry.m_onTimeout()` 或默认 abort

## 四、接口设计

### 4.1 WatchDogMgr 接口

```cpp
class WatchDogMgr {
public:
    static WatchDogMgr* GetInstance();
    
    // 注册监控项，entry 生命周期由被监控线程持有
    EntryId Register(WatchDogEntry* entry);
    
    // 反注册，线程停止时调用
    void Unregister(EntryId entryId);
    
    // 心跳上报，线程循环中调用
    void FeedTask(EntryId entryId);
    
    // 启动/停止监控线程
    void Start();
    void Stop();
    
    // 诊断接口
    std::vector<WatchDogDiagnose> GetDiagnoseInfo();
    
    // WatchDog 线程自身心跳（内部使用）
    void SelfHeartbeat();
};
```

### 4.2 WatchDogEntry 接口

```cpp
class WatchDogEntry {
public:
    WatchDogEntry(const char* name, PdbId pdbId, uint32 timeoutMs,
                  std::function<void()> onTimeout = nullptr);
    
    const char* GetName() const;
    PdbId GetPdbId() const;
    uint32 GetTimeoutMs() const;
    WatchDogStatus GetStatus() const;
    uint64 GetLastHeartbeat() const;
    uint32 GetTimeoutCount() const;
    
private:
    // 数据成员见 2.3 类结构
};
```

### 4.3 向上诊断接口示例

上层 MySQL Server 可调用：

```cpp
std::vector<WatchDogDiagnose> diagnose = WatchDogMgr::GetInstance()->GetDiagnoseInfo();
for (const auto& info : diagnose) {
    // 可转换为 SQL 结果集返回
}
```

## 五、文件布局

```
include/framework/dstore_watchdog_mgr.h
include/framework/dstore_watchdog_entry.h
include/framework/dstore_watchdog_diagnose.h
src/framework/dstore_watchdog_mgr.cpp
src/framework/dstore_watchdog_entry.cpp
```

与现有 framework 模块结构一致（如 dstore_pdb.h, dstore_thread.h）。

## 六、被监控线程集成示例

以 BgWalWriter 为例：

```cpp
// BgWalWriter.h
class BgWalWriter {
private:
    WatchDogEntry* m_watchdogEntry;
    EntryId m_watchdogEntryId;
};

// BgWalWriter.cpp - Init()
m_watchdogEntry = new WatchDogEntry("BgWalWriter", m_pdbId, 
                                     5000,  // 5秒超时阈值
                                     nullptr);
m_watchdogEntryId = WatchDogMgr::GetInstance()->Register(m_watchdogEntry);

// BgWalWriter.cpp - BgFlushMain()
while (!m_needStop) {
    WatchDogMgr::GetInstance()->FeedTask(m_watchdogEntryId);
    flushedDataLen = m_stream->Flush();
    SleepIfNecessary(flushedDataLen);
}

// BgWalWriter.cpp - Stop()
WatchDogMgr::GetInstance()->Unregister(m_watchdogEntryId);
delete m_watchdogEntry;
m_watchdogEntry = nullptr;
```

## 七、待后续迭代的功能

1. Memory 监控：跟踪内存分配失败次数，预测 OOM
2. Disk I/O 监控：检测磁盘阻塞写
3. 告警历史记录：持久化告警信息，支持查询 `ai_watchdog_warning_retention()`
4. 栈信息收集：告警时尝试获取线程栈（backtrace 或 gdb attach）

## 八、风险与缓解

| 风险 | 缓解措施 |
|------|----------|
| 性能抖动误判 | 连续超时 4 次才告警 |
| WatchDog 线程自身卡死 | 自注册心跳 + 特殊告警日志 |
| abort 误杀进程 | 默认仅打印日志，`enable_ai_watchdog_healing=OFF` |
| 内存开销 | unordered_map + 少量原子变量，约几百字节 per entry |