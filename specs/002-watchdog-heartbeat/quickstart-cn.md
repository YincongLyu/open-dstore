# 快速上手：Watchdog 心跳监控

## 1. 目标

这份 quickstart 不是实现细节文档，而是给后续 `/tasks` 和真实开发使用的“落地操作指南”。目标是让开发者能够按一致流程，把 watchdog 接入到现有后台线程，并完成最小可验证闭环。

## 2. 构建准备

```bash
source buildenv
cd utils && bash build.sh -m debug && cd ..
bash build.sh -m debug -st on -tm ut
```

## 3. 第一批实现入口

第一批代码应优先落在 framework 层：

```text
include/framework/dstore_watchdog_mgr.h
include/framework/dstore_watchdog_entry.h
include/framework/dstore_watchdog_diagnose.h
src/framework/dstore_watchdog_mgr.cpp
src/framework/dstore_watchdog_entry.cpp
```

这一层完成后，再把 Register / Feed / Unregister 埋点逐步接入 6 类线程。

## 4. 一个真实接入案例：WAL Flush 线程

以 `BgWalWriter` 为例，典型接入顺序如下：

### 4.1 在线程初始化时注册

```cpp
m_watchdogEntry = new WatchDogEntry("BgWalWriter", m_pdbId, timeoutMs);
m_watchdogEntryId = WatchDogMgr::GetInstance()->Register(m_watchdogEntry);
```

### 4.2 在主循环中喂心跳

```cpp
while (!m_needStop) {
    WatchDogMgr::GetInstance()->FeedTask(m_watchdogEntryId);
    flushedDataLen = m_stream->Flush();
    SleepIfNecessary(flushedDataLen);
}
```

### 4.3 在线程停止时反注册

```cpp
WatchDogMgr::GetInstance()->Unregister(m_watchdogEntryId);
delete m_watchdogEntry;
m_watchdogEntry = nullptr;
```

这个案例的价值在于：它完整覆盖了注册、运行期 heartbeat、停止时清理三个生命周期点，后续接入 Checkpoint / Buffer / Undo / Btree 时都应保持同样模式。

## 5. 六类线程接入顺序建议

建议按风险和路径清晰度从易到难推进：

1. WAL flush
2. WAL file recycle
3. Checkpoint
4. Buffer dirty-page flush
5. Undo recycle dispatch
6. Btree recycle / prune

原因很简单：前四类线程入口明确、循环边界清楚，更适合作为第一轮闭环；Undo 和 Btree 作为第二轮能降低早期调试复杂度。

## 6. 本地验证示例

完成 framework 核心与至少一类线程接入后，可先执行最小验证：

```bash
cd tmp_build
make run_dstore_framework_unittest
make run_dstore_ha_unittest
```

若接入了更多模块，再继续执行：

```bash
make run_dstore_buffer_unittest
make run_dstore_undo_unittest
make run_dstore_index_unittest
```

## 7. 一个真实诊断闭环案例

假设你已经接入了 `BgWalWriter`：

1. 正常运行时，`BgWalWriter` 周期性喂心跳；
2. 若人为让其停止推进，watchdog 在默认 5s 周期下扫描到超时；
3. 达到连续 miss 阈值后，打印一条针对 `BgWalWriter` 的 warning log；
4. 同时诊断接口中能看到：线程名、所属范围、最近心跳时间、超时阈值、异常持续时间；
5. 若是 watchdog 自己长时间未完成扫描，则输出与普通线程不同的 self-health warning。

这个案例就是第一版最核心的“真实使用感”闭环。

## 8. 实现完成后的手工检查项

- 健康线程不会误报；
- 故意停止推进的线程会在预期时间窗内告警；
- 诊断结果能返回所有活跃 entry；
- watchdog 自健康告警与普通线程告警可区分；
- 默认 5s 周期下，不应出现明显 CPU 热点或异常内存增长。

## 9. 与后续 /tasks 的衔接

这份 quickstart 中的“真实案例”和“接入顺序建议”，后续可以在 `/speckit.tasks` 阶段拆成更细的任务项，例如：

- framework 核心类型实现
- WAL flush 接入
- WAL recycle 接入
- diagnose contract 接口实现
- framework / WAL / buffer / undo / index 分模块验证

也就是说，它现在是开发路线图，后续可以自然回填成更细粒度 tasks。
