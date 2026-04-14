# 研究结论：Watchdog 心跳监控

## R1：WatchDogMgr 放在 framework 层

**Decision**：WatchDogMgr 作为全局 framework 级组件实现，头文件和实现文件放在 `include/framework/` 与 `src/framework/`。

**Rationale**：
- 规格已明确部署层级为 `StorageInstance` 全局单例。
- 被监控对象横跨 WAL、buffer、undo、index 多个模块，若做成模块内局部组件会造成监控逻辑分裂。
- 与现有线程注册、线程可见性和 stop/join 的 framework 工具最容易对接。

**Alternatives considered**：
- per-PDB watchdog manager：被设计稿明确否决。
- 各模块自建 watchdog：无法统一诊断接口和自监控语义。

## R2：自监控采用 scan-cycle progress，而不是普通 heartbeat

**Decision**：watchdog 自身不注册成普通 `WatchDogEntry`，而是维护一个“最近一次成功完成扫描循环”的时间戳。

**Rationale**：
- 当前 dstore 生产代码里普遍是 manager 观察 worker，而没有成熟的“线程把自己当普通对象监控自己”的抽象。
- scan-cycle progress 更容易表达“监控线程还在推进”，也不会出现递归检测语义。

**Alternatives considered**：
- 把 watchdog 也当普通 entry 注册：语义混乱，容易和 peer thread 健康状态混在一起。
- 不做自监控：会让 watchdog 自己卡死时缺乏区分性的诊断信号。

## R3：第一版固定监控 6 类线程

**Decision**：第一版只监控以下 6 类：

1. WAL flush
2. WAL file recycle
3. checkpoint progress
4. buffer dirty-page flush
5. undo recycle dispatch
6. Btree recycle / prune

**Rationale**：
- 这 6 类已被 spec 澄清并冻结。
- 每一类都能映射到真实的长生命周期 worker loop 或 manager loop。
- 再扩大范围会违反当前的最小增量原则。

**Alternatives considered**：
- 只做 3 类核心线程：已被后续 clarify 否决。
- 引入 memory / disk 监控：明确属于后续迭代。

## R4：默认 5s 自唤醒周期

**Decision**：monitor thread 默认 5s 唤醒一次。

**Rationale**：
- 比最初设计中的 1s 更轻量，符合“轻量化后台线程”的定位。
- 结合当前超时与 miss 计数策略，仍然可以满足告警时间窗要求。
- dstore 现有后台管理循环总体偏低频，而不是密集轮询。

**Alternatives considered**：
- 1s：过于频繁，不适合作为第一版默认值。
- 10s：虽然更轻，但告警反应变慢，收益不明显。

## R5：资源预算采用宽松轻量化口径

**Decision**：第一版预算定义为：默认 5s 周期下，watchdog CPU 开销 < 1%，额外内存 < 1MB。

**Rationale**：
- 当前 dstore 的线程元数据与状态导出结构显示，watchdog 更像是“活跃 entry 的线性扫描 + 少量元数据”，而不是高复杂度管理器。
- 用可测但不过分苛刻的预算，更适合作为第一版验收口径。

**Alternatives considered**：
- <0.1% CPU / <256KB：对第一版过于严格。
- 只写“低开销”：不利于 tasks 和验收分解。

## R6：heartbeat 埋点应放在真实 worker loop 中

**Decision**：heartbeat 只放在真实业务循环内部，而不是只在 init/stop 处做一次“伪活性”登记。

**Rationale**：
- Watchdog 的目标是检测“线程是否还在推进”，而不是“线程曾经启动成功”。
- 现有 6 类线程都存在明确的循环边界，可自然埋点。

**Alternatives considered**：
- 只在 init/stop 记录：无法反映运行期间 hang。
- 每模块外包一层代理线程：复杂度和开销都偏高。

## R7：真实代码路径映射

- **WAL flush**：`src/wal/dstore_wal_bgwriter.cpp`，`src/wal/dstore_wal_logstream.cpp`
- **WAL file recycle**：`src/wal/dstore_wal_file_manager.cpp`
- **Checkpoint**：`src/buffer/dstore_checkpointer.cpp`
- **Buffer flush**：`src/buffer/dstore_bg_page_writer_mgr.cpp`、`src/buffer/dstore_bg_page_writer_base.cpp`、`src/buffer/dstore_bg_disk_page_writer.cpp`
- **Undo dispatch**：`src/undo/dstore_rollback_trx_task_mgr.cpp`、`src/undo/dstore_rollback_trx_worker.cpp`
- **Btree recycle / prune**：`src/index/dstore_btree_page_recycle.cpp`、`src/index/dstore_btree_recycle_partition.cpp`、`src/index/dstore_btree_prune.cpp`
- **线程生命周期辅助**：`src/framework/dstore_instance.cpp`、`src/framework/dstore_thread.cpp`、`interface/framework/dstore_instance_interface.h`、`interface/framework/dstore_thread_interface.h`
