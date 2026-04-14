# 实施计划：Watchdog 心跳监控

**分支**：`002-watchdog-heartbeat` | **日期**：2026-04-14 | **规格**：[spec.md](spec.md)  
**输入**：来自 `/specs/002-watchdog-heartbeat/spec.md` 的特性规格，以及 `docs/superpowers/specs/2026-04-14-watchdog-design.md` 的设计约束

## 一、摘要

为 dstore 存储引擎实现一个内置轻量化 Watchdog 后台线程，用于监控 6 类关键后台线程的心跳活性，并向上层暴露当前健康状态。

第一版目标聚焦于 **Thread heartbeat 监控**，不扩展到 Memory、Disk I/O、告警保留或栈采集。整体方案包括：

1. **全局 WatchDogMgr**：在 `StorageInstance` 级别维护监控线程、注册表、自身健康状态与诊断快照。
2. **统一 WatchDogEntry 模型**：各后台线程按现有生命周期在启动时注册、循环中上报心跳、停止时反注册。
3. **只读诊断接口**：对上层 SQL / MySQL 集成暴露当前监控项状态，用于排障，而不是持久化告警系统。

## 二、技术上下文

**语言/版本**：C++17  
**主要依赖**：dstore framework 线程/运行时工具、STL `atomic`/`mutex`/`thread`、现有 dstore 日志宏与配置框架  
**存储**：仅使用内存态 watchdog 元数据，第一版不落盘  
**测试框架**：Google Test；复用现有 `make run_dstore_framework_unittest`、`run_dstore_ha_unittest`、`run_dstore_buffer_unittest`、`run_dstore_undo_unittest`、`run_dstore_index_unittest`  
**目标平台**：Linux / WSL Linux 开发构建环境  
**项目类型**：C++ 存储引擎库 + 内部后台线程基础设施  
**性能目标**：默认 5s 自唤醒周期；在默认配置下 watchdog CPU 开销 < 1%，额外内存 < 1MB；疑似 hang 告警在 spec 定义的时间窗内产生  
**约束条件**：第一版仅支持 6 类后台线程；默认仅日志告警；必须显式 Register / Feed / Unregister；不做历史保留、内存监控、磁盘监控和栈采集  
**规模/范围**：一个全局 WatchDogMgr，监控跨 `StorageInstance` / per-PDB manager 的 6 类后台线程，扫描成本应为活跃 entry 的线性复杂度

## 三、Constitution Check

*Gate：Phase 0 研究前必须通过；Phase 1 设计后重新复核。*

- **最小增量明确**：通过。第一版只做 heartbeat 检测；Memory / Disk / Retention / Stack Capture / Healing 全部延后。
- **运行时可观测性明确**：通过。已定义监控信号（heartbeat）、日志输出、自监控告警与诊断查询面。
- **生命周期归属明确**：通过。注册、上报、反注册以及 stop/join 边界均锚定到现有线程生命周期入口。
- **失败策略安全**：通过。默认 log-only；更强动作仍为配置控制并不进入本轮实现。
- **验证证据明确**：通过。已映射到 framework / WAL / buffer / undo / index 的真实路径与测试目标。

## 四、项目结构

### 4.1 本特性文档结构

```text
specs/002-watchdog-heartbeat/
├── spec.md
├── plan.md
├── plan-cn.md
├── research.md
├── research-cn.md
├── data-model.md
├── data-model-cn.md
├── quickstart.md
├── quickstart-cn.md
├── contracts/
│   └── watchdog-diagnose-contract.md
└── tasks.md
```

### 4.2 代码结构（仓库根目录）

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
```

**结构决策**：Watchdog 核心类型放在 `framework`，因为它跨多个模块并依赖现有线程注册/销毁基础设施；心跳埋点则落在各模块既有 worker loop 中，而不是新建模块内局部 watchdog。

## 五、架构设计

### 5.1 整体流程架构图

```text
                    ┌──────────────────────────────┐
                    │     StorageInstance          │
                    │  (全局 WatchDogMgr 所在处)   │
                    └──────────────┬───────────────┘
                                   │ Start()
                                   ▼
                    ┌──────────────────────────────┐
                    │         WatchDogMgr          │
                    │  - monitor thread            │
                    │  - entry registry            │
                    │  - self-health timestamp     │
                    │  - diagnose snapshot         │
                    └───────┬─────────┬────────────┘
                            │         │
           Register/Feed/Unregister   │ periodic scan (5s)
                            │         │
     ┌──────────────────────┼─────────┼───────────────────────┐
     ▼                      ▼         ▼                       ▼
┌────────────┐       ┌────────────┐  ┌────────────┐    ┌────────────┐
│ WAL Flush  │       │ Checkpoint │  │ Undo Dispatch│   │ Btree Recycle│
└────────────┘       └────────────┘  └────────────┘    └────────────┘
     ▲                      ▲               ▲                  ▲
     │                      │               │                  │
┌────────────┐       ┌────────────┐         │                  │
│ WAL Recycle│       │ Buffer Flush│        │                  │
└────────────┘       └────────────┘         │                  │
                                            │                  │
                                            └──── Diagnose ────┘
                                                     │
                                                     ▼
                                    上层 MySQL / SQL 诊断查询接口
```

### 5.2 单线程 heartbeat 监控框架

**设计模式**：Manager + Entry Registry + Worker Loop 埋点

```text
后台线程 Init
  │
  ├─ 创建 WatchDogEntry
  ├─ WatchDogMgr::Register(entry)
  │
后台线程 Main Loop
  │
  ├─ WatchDogMgr::FeedTask(entryId)
  ├─ 执行真实业务逻辑
  └─ wait / sleep / flush / dispatch
  │
后台线程 Stop
  │
  ├─ WatchDogMgr::Unregister(entryId)
  └─ 销毁本地 entry
```

**WatchDogMgr 扫描逻辑**：

```text
monitor thread 每 5s 唤醒一次
  │
  ├─ 刷新 self-health timestamp
  ├─ 遍历所有 active entries
  ├─ 比较 now - lastHeartbeat 与 timeoutThreshold
  ├─ 更新 consecutiveMissCount / healthState
  ├─ 达到阈值则发出 warning log
  └─ 生成当前诊断快照
```

### 5.3 六类监控对象与落点

1. **WAL flush**：`BgWalWriter::BgFlushMain()`
2. **WAL file recycle**：`WalFileManager::RecycleWalFileWorkerMain()`
3. **Checkpoint**：`CheckpointMgr::CheckpointerMain()`
4. **Buffer dirty-page flush**：master/slave page writer run loop
5. **Undo recycle dispatch**：`RollbackTrxTaskMgr::DispatchMain()`
6. **Btree recycle / prune**：`BtreeRecycleWorker::BtreeRecycleThreadMain()`

### 5.4 Watchdog 自监控策略

自监控不复用普通 `WatchDogEntry` 路径，而是使用 **scan-cycle progress timestamp**：

- 每次扫描循环完成后刷新一次自健康时间戳；
- 若超时，则打印与普通线程不同的 self-health warning；
- 避免“watchdog 把自己当普通线程再次检测”的递归语义。

### 5.5 失败策略

第一版仅支持：

- `enable_ai_watchdog=ON/OFF`
- `enable_ai_watchdog_healing=OFF`（默认）

也就是说第一版默认只告警，不 abort，不退出进程，不做自动修复。

## 六、实现阶段分解

### Phase 0：研究与路径确认

- 明确 6 类线程的真实循环入口与生命周期边界。
- 确认 framework 层线程注册/销毁辅助接口。
- 确认自监控、自唤醒周期、性能/内存预算已在 spec 中冻结。

### Phase 1：核心设计落地

- 定义 `WatchDogMgr` / `WatchDogEntry` / `WatchDogDiagnose`。
- 定义注册、反注册、心跳上报、自监控、自唤醒扫描逻辑。
- 定义对上诊断 contract。

### Phase 2：模块接入

- WAL flush / recycle 接入
- Checkpoint / buffer page writer 接入
- Undo dispatch 接入
- Btree recycle / prune 接入

### Phase 3：验证与收敛

- framework 单测
- 各模块回归
- 默认 5s 周期的轻量性验证

## 七、验证策略

- **Framework**：`make run_dstore_framework_unittest`
- **WAL / HA**：`make run_dstore_ha_unittest`
- **Buffer**：`make run_dstore_buffer_unittest`
- **Undo**：`make run_dstore_undo_unittest`
- **Index**：`make run_dstore_index_unittest`

构建流程遵循 `docs/build-reference.md`：

```bash
source buildenv
cd utils && bash build.sh -m debug && cd ..
bash build.sh -m debug -st on -tm ut
cd tmp_build
```

## 八、Post-Design Constitution Check

- **Minimal Increment First**：通过。无 scope creep。
- **Built-In Observability**：通过。heartbeat、warning、diagnose 都明确。
- **Lifecycle-Safe Integration**：通过。Register / Feed / Unregister 已绑定真实生命周期。
- **Safe Failure Policy**：通过。仍是 log-only。
- **Evidence-Backed Verification**：通过。真实模块路径与测试目标均已映射。

## 九、C++17 Best Practices Applied

| 改进点 | 本项目做法 |
|--------|------------|
| `enum class` | `WatchDogStatus`、后续策略枚举全部使用 `enum class`，避免裸枚举污染 |
| RAII | 管理线程对象、锁、容器资源按 RAII 语义管理，避免手动清理遗漏 |
| `const` correctness | 诊断查询接口、只读访问器、快照构造过程尽量使用 `const` |
| `std::chrono` 语义 | 时间/周期表达优先使用明确时间单位语义，避免毫秒/微秒混淆 |
| 原子状态 | heartbeat 时间戳、运行状态、miss 计数使用原子或明确同步保护 |
| 最小共享状态 | 仅共享注册表和必要状态，worker 自持 entry 生命周期 |
| 清晰 ownership | `WatchDogMgr` 不拥有 worker 线程本体，只拥有注册表和监控线程 |
| 可测试性 | 核心状态迁移与诊断组装逻辑设计为可单测，而不是只能靠集成验证 |
