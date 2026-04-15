/*
 * Copyright (C) 2026 Huawei Technologies Co.,Ltd.
 *
 * dstore is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dstore is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DSTORE_WATCHDOG_MGR_H
#define DSTORE_WATCHDOG_MGR_H

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include "common/dstore_datatype.h"
#include "common/memory/dstore_mctx.h"
#include "lock/dstore_lwlock.h"
#include "framework/dstore_watchdog_entry.h"
#include "framework/dstore_watchdog_diagnose.h"

namespace DSTORE {

enum class WatchDogPolicy : uint8 {
    LOG_ONLY,
    LOG_AND_HEALING
};

class WatchDogMgr {
public:
    static WatchDogMgr *GetInstance();

    WatchDogMgr();
    ~WatchDogMgr();
    DISALLOW_COPY_AND_MOVE(WatchDogMgr);

    RetStatus Init();
    void Destroy();

    RetStatus Start();
    void Stop();
    void Shutdown();

    RetStatus Register(WatchDogEntry *entry);
    void Unregister(WatchDogEntry *entry);

    void FeedSelfHealth();
    void CheckSelfHealth();

    WatchDogPolicy GetPolicy() const;
    void SetPolicy(WatchDogPolicy policy);

    uint32_t GetCheckIntervalMs() const;
    void SetCheckIntervalMs(uint32_t intervalMs);

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    bool IsHealingEnabled() const;
    void SetHealingEnabled(bool enabled);

    RetStatus GetDiagnoseSnapshot(WatchDogDiagnoseIterator &iterator);

    size_t GetActiveEntryCount() const;
    WatchDogEntry *FindEntry(const WatchDogEntryId &entryId);

private:
    void MonitorThreadMain();
    void ScanEntries();
    void EmitWarning(const WarningEvent &event);
    void EmitSelfHealthWarning(TimestampTz overdueDuration);

    RetStatus AllocateEntry(WatchDogEntry *&entry);
    void ReleaseEntry(WatchDogEntry *entry);

    bool m_initialized;
    std::atomic<bool> m_running;
    std::atomic<bool> m_enabled;
    std::atomic<bool> m_healingEnabled;
    std::atomic<bool> m_stopping;
    std::thread *m_monitorThread;

    std::mutex m_registryMutex;
    LWLock m_registryLock;
    std::vector<WatchDogEntry *> m_entries;
    size_t m_entryCount;

    std::atomic<TimestampTz> m_selfHealthTimestamp;
    TimestampTz m_selfHealthTimeoutMs;

    WatchDogPolicy m_policy;
    uint32_t m_checkIntervalMs;

    DstoreMemoryContext m_memoryContext;
    uint64_t m_nextEntryId;

    static constexpr uint32_t SELF_HEALTH_TIMEOUT_MS = 15000;
};

RetStatus InitWatchDogMgr();
void DestroyWatchDogMgr();

WatchDogMgr *GetWatchDogMgr();

} // namespace DSTORE

#endif