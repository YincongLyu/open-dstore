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

#include "framework/dstore_watchdog_mgr.h"
#include "common/log/dstore_log.h"
#include "common/memory/dstore_mctx.h"
#include "securec.h"
#include <chrono>
#include <cstring>

namespace DSTORE {

static WatchDogMgr *g_watchDogMgr = nullptr;

WatchDogMgr *WatchDogMgr::GetInstance()
{
    return g_watchDogMgr;
}

WatchDogMgr::WatchDogMgr()
    : m_initialized(false),
      m_running(false),
      m_enabled(true),
      m_healingEnabled(false),
      m_stopping(false),
      m_monitorThread(nullptr),
      m_entryCount(0),
      m_selfHealthTimestamp(0),
      m_selfHealthTimeoutMs(SELF_HEALTH_TIMEOUT_MS),
      m_policy(WatchDogPolicy::LOG_ONLY),
      m_checkIntervalMs(WATCHDOG_DEFAULT_CHECK_INTERVAL_MS),
      m_memoryContext(nullptr),
      m_nextEntryId(1)
{
    m_entries.reserve(WATCHDOG_MAX_ENTRIES);
}

WatchDogMgr::~WatchDogMgr()
{
    Destroy();
}

RetStatus WatchDogMgr::Init()
{
    if (m_initialized) {
        return DSTORE_SUCC;
    }

    m_memoryContext = DstoreAllocSetContextCreate(
        g_storageInstance->GetMemoryMgr()->GetRoot(),
        "WatchDogMgr",
        ALLOCSET_DEFAULT_SIZES);

    if (STORAGE_VAR_NULL(m_memoryContext)) {
        ErrLog(DSTORE_ERROR, MODULE_FRAMEWORK, ErrMsg("Failed to create memory context for WatchDogMgr."));
        return DSTORE_FAIL;
    }

    LWLockInitialize(&m_registryLock, LWLOCK_GROUP_FRAMEWORK);

    m_initialized = true;
    return DSTORE_SUCC;
}

void WatchDogMgr::Destroy()
{
    if (!m_initialized) {
        return;
    }

    Stop();

    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        for (auto entry : m_entries) {
            if (entry != nullptr) {
                entry->Destroy();
                DstorePfreeExt(entry);
            }
        }
        m_entries.clear();
        m_entryCount = 0;
    }

    if (m_memoryContext != nullptr) {
        DstoreMemoryContextDelete(m_memoryContext);
        m_memoryContext = nullptr;
    }

    m_initialized = false;
}

RetStatus WatchDogMgr::Start()
{
    if (m_running) {
        return DSTORE_SUCC;
    }

    if (!m_enabled) {
        ErrLog(DSTORE_LOG, MODULE_FRAMEWORK, ErrMsg("Watchdog is disabled, not starting."));
        return DSTORE_SUCC;
    }

    m_stopping = false;
    m_running = true;
    m_selfHealthTimestamp = GetCurrentTimestamp();

    m_monitorThread = new std::thread(&WatchDogMgr::MonitorThreadMain, this);
    if (m_monitorThread == nullptr) {
        ErrLog(DSTORE_ERROR, MODULE_FRAMEWORK, ErrMsg("Failed to create watchdog monitor thread."));
        m_running = false;
        return DSTORE_FAIL;
    }

    ErrLog(DSTORE_LOG, MODULE_FRAMEWORK, ErrMsg("Watchdog monitor thread started."));
    return DSTORE_SUCC;
}

void WatchDogMgr::Stop()
{
    if (!m_running) {
        return;
    }

    m_stopping = true;
    m_running = false;

    if (m_monitorThread != nullptr) {
        m_monitorThread->join();
        delete m_monitorThread;
        m_monitorThread = nullptr;
    }

    ErrLog(DSTORE_LOG, MODULE_FRAMEWORK, ErrMsg("Watchdog monitor thread stopped."));
}

void WatchDogMgr::Shutdown()
{
    Stop();
    Destroy();
}

RetStatus WatchDogMgr::Register(WatchDogEntry *entry)
{
    if (entry == nullptr) {
        return DSTORE_FAIL;
    }

    std::lock_guard<std::mutex> lock(m_registryMutex);

    if (m_entryCount >= WATCHDOG_MAX_ENTRIES) {
        ErrLog(DSTORE_WARNING, MODULE_FRAMEWORK, 
               ErrMsg("Watchdog registry full, cannot register entry %s.", entry->GetThreadName()));
        return DSTORE_FAIL;
    }

    entry->Reset();
    m_entries.push_back(entry);
    m_entryCount++;

    ErrLog(DSTORE_DEBUG1, MODULE_FRAMEWORK, 
           ErrMsg("Watchdog entry registered: %s.", entry->GetThreadName()));
    return DSTORE_SUCC;
}

void WatchDogMgr::Unregister(WatchDogEntry *entry)
{
    if (entry == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_registryMutex);

    auto it = std::find(m_entries.begin(), m_entries.end(), entry);
    if (it != m_entries.end()) {
        entry->MarkUnregistered();
        m_entries.erase(it);
        m_entryCount--;

        ErrLog(DSTORE_DEBUG1, MODULE_FRAMEWORK, 
               ErrMsg("Watchdog entry unregistered: %s.", entry->GetThreadName()));
    }
}

void WatchDogMgr::FeedSelfHealth()
{
    m_selfHealthTimestamp = GetCurrentTimestamp();
}

void WatchDogMgr::CheckSelfHealth()
{
    TimestampTz currentTime = GetCurrentTimestamp();
    TimestampTz lastHealthTime = m_selfHealthTimestamp;
    
    uint64_t durationMs = (currentTime - lastHealthTime) * 1000;
    
    if (durationMs > m_selfHealthTimeoutMs) {
        EmitSelfHealthWarning(durationMs);
    }
}

WatchDogPolicy WatchDogMgr::GetPolicy() const
{
    return m_policy;
}

void WatchDogMgr::SetPolicy(WatchDogPolicy policy)
{
    m_policy = policy;
}

uint32_t WatchDogMgr::GetCheckIntervalMs() const
{
    return m_checkIntervalMs;
}

void WatchDogMgr::SetCheckIntervalMs(uint32_t intervalMs)
{
    m_checkIntervalMs = intervalMs;
}

bool WatchDogMgr::IsEnabled() const
{
    return m_enabled;
}

void WatchDogMgr::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool WatchDogMgr::IsHealingEnabled() const
{
    return m_healingEnabled;
}

void WatchDogMgr::SetHealingEnabled(bool enabled)
{
    m_healingEnabled = enabled;
}

RetStatus WatchDogMgr::GetDiagnoseSnapshot(WatchDogDiagnoseIterator &iterator)
{
    std::lock_guard<std::mutex> lock(m_registryMutex);

    for (auto entry : m_entries) {
        if (entry == nullptr || !entry->IsRegistered()) {
            continue;
        }

        HealthSnapshot snapshot;
        snapshot.entryId = entry->GetEntryId().id;
        snapshot.threadCategory = entry->GetEntryId().category;
        snapshot.scopeId = entry->GetEntryId().scopeId;
        snapshot.timeoutThresholdMs = entry->GetTimeoutMs();
        snapshot.lastHeartbeatTime = entry->GetLastHeartbeatTime();
        snapshot.healthState = entry->GetStatus();
        snapshot.isSelfHealth = false;

        errno_t rc = strncpy_s(snapshot.threadName, WATCHDOG_THREAD_NAME_MAX_LEN, 
                                entry->GetThreadName(), strlen(entry->GetThreadName()));
        storage_securec_check(rc, "\0", "\0");

        if (snapshot.healthState == WatchDogStatus::UNHEALTHY) {
            TimestampTz currentTime = GetCurrentTimestamp();
            snapshot.unhealthyDurationMs = (currentTime - snapshot.lastHeartbeatTime) * 1000;
        } else {
            snapshot.unhealthyDurationMs = 0;
        }

        if (STORAGE_FUNC_FAIL(iterator.AddRecord(snapshot))) {
            ErrLog(DSTORE_WARNING, MODULE_FRAMEWORK, 
                   ErrMsg("Failed to add health snapshot for entry %s.", entry->GetThreadName()));
        }
    }

    HealthSnapshot selfSnapshot;
    selfSnapshot.entryId = 0;
    selfSnapshot.threadCategory = WatchDogThreadCategory::MAX_CATEGORY;
    selfSnapshot.scopeId = INVALID_PDB_ID;
    selfSnapshot.timeoutThresholdMs = m_selfHealthTimeoutMs;
    selfSnapshot.lastHeartbeatTime = m_selfHealthTimestamp;
    selfSnapshot.isSelfHealth = true;
    
    TimestampTz currentTime = GetCurrentTimestamp();
    uint64_t selfDuration = (currentTime - m_selfHealthTimestamp) * 1000;
    if (selfDuration > m_selfHealthTimeoutMs) {
        selfSnapshot.healthState = WatchDogStatus::UNHEALTHY;
        selfSnapshot.unhealthyDurationMs = selfDuration;
    } else {
        selfSnapshot.healthState = WatchDogStatus::HEALTHY;
        selfSnapshot.unhealthyDurationMs = 0;
    }

    errno_t rc = strncpy_s(selfSnapshot.threadName, WATCHDOG_THREAD_NAME_MAX_LEN, 
                            "WatchDogMgr", strlen("WatchDogMgr"));
    storage_securec_check(rc, "\0", "\0");

    iterator.AddRecord(selfSnapshot);

    return DSTORE_SUCC;
}

size_t WatchDogMgr::GetActiveEntryCount() const
{
    return m_entryCount;
}

WatchDogEntry *WatchDogMgr::FindEntry(const WatchDogEntryId &entryId)
{
    std::lock_guard<std::mutex> lock(m_registryMutex);

    for (auto entry : m_entries) {
        if (entry != nullptr && entry->GetEntryId() == entryId) {
            return entry;
        }
    }

    return nullptr;
}

void WatchDogMgr::MonitorThreadMain()
{
    while (m_running && !m_stopping) {
        ScanEntries();
        CheckSelfHealth();
        FeedSelfHealth();

        std::chrono::milliseconds sleepDuration(m_checkIntervalMs);
        std::this_thread::sleep_for(sleepDuration);
    }
}

void WatchDogMgr::ScanEntries()
{
    TimestampTz currentTime = GetCurrentTimestamp();

    std::lock_guard<std::mutex> lock(m_registryMutex);

    for (auto entry : m_entries) {
        if (entry == nullptr || !entry->IsRegistered()) {
            continue;
        }

        WatchDogStatus newStatus = entry->EvaluateHealth(currentTime);

        if (newStatus == WatchDogStatus::UNHEALTHY) {
            WarningEvent event;
            event.warningType = WarningEvent::WarningType::PEER_TASK_WARNING;
            event.threadCategory = entry->GetEntryId().category;
            event.scopeId = entry->GetEntryId().scopeId;
            event.observedAt = currentTime;
            event.timeoutThresholdMs = entry->GetTimeoutMs();
            event.missCount = entry->GetConsecutiveMissCount();

            errno_t rc = strncpy_s(event.threadName, WATCHDOG_THREAD_NAME_MAX_LEN,
                                    entry->GetThreadName(), strlen(entry->GetThreadName()));
            storage_securec_check(rc, "\0", "\0");

            TimestampTz lastBeat = entry->GetLastHeartbeatTime();
            event.overdueDurationMs = (currentTime - lastBeat) * 1000;

            EmitWarning(event);
        }
    }
}

void WatchDogMgr::EmitWarning(const WarningEvent &event)
{
    const char *categoryStr = "UNKNOWN";
    switch (event.threadCategory) {
        case WatchDogThreadCategory::WAL_FLUSH:
            categoryStr = "WAL_FLUSH";
            break;
        case WatchDogThreadCategory::WAL_FILE_RECYCLE:
            categoryStr = "WAL_FILE_RECYCLE";
            break;
        case WatchDogThreadCategory::CHECKPOINT_PROGRESS:
            categoryStr = "CHECKPOINT_PROGRESS";
            break;
        case WatchDogThreadCategory::BUFFER_DIRTY_PAGE_FLUSH:
            categoryStr = "BUFFER_DIRTY_PAGE_FLUSH";
            break;
        case WatchDogThreadCategory::UNDO_RECYCLE_DISPATCH:
            categoryStr = "UNDO_RECYCLE_DISPATCH";
            break;
        case WatchDogThreadCategory::BTREE_RECYCLE_PRUNE:
            categoryStr = "BTREE_RECYCLE_PRUNE";
            break;
        default:
            break;
    }

    ErrLog(DSTORE_WARNING, MODULE_FRAMEWORK,
           ErrMsg("Watchdog warning: thread=%s, category=%s, scope=%u, overdue=%lu ms, threshold=%u ms, miss_count=%u",
                  event.threadName, categoryStr, event.scopeId, 
                  event.overdueDurationMs, event.timeoutThresholdMs, event.missCount));

    if (m_policy == WatchDogPolicy::LOG_AND_HEALING && m_healingEnabled) {
        ErrLog(DSTORE_WARNING, MODULE_FRAMEWORK,
               ErrMsg("Healing policy enabled for watchdog - thread: %s", event.threadName));
    }
}

void WatchDogMgr::EmitSelfHealthWarning(TimestampTz overdueDuration)
{
    ErrLog(DSTORE_WARNING, MODULE_FRAMEWORK,
           ErrMsg("Watchdog self-health warning: overdue=%lu ms, threshold=%u ms",
                  overdueDuration, m_selfHealthTimeoutMs));
}

RetStatus WatchDogMgr::AllocateEntry(WatchDogEntry *&entry)
{
    AutoMemCxtSwitch autoSwitch(m_memoryContext);

    entry = (WatchDogEntry *)DstorePalloc(sizeof(WatchDogEntry));
    if (STORAGE_VAR_NULL(entry)) {
        ErrLog(DSTORE_ERROR, MODULE_FRAMEWORK, ErrMsg("Failed to allocate WatchDogEntry."));
        return DSTORE_FAIL;
    }

    return DSTORE_SUCC;
}

void WatchDogMgr::ReleaseEntry(WatchDogEntry *entry)
{
    if (entry != nullptr) {
        entry->Destroy();
        DstorePfreeExt(entry);
    }
}

RetStatus InitWatchDogMgr()
{
    if (g_watchDogMgr != nullptr) {
        return DSTORE_SUCC;
    }

    g_watchDogMgr = new WatchDogMgr();
    if (g_watchDogMgr == nullptr) {
        ErrLog(DSTORE_ERROR, MODULE_FRAMEWORK, ErrMsg("Failed to allocate WatchDogMgr."));
        return DSTORE_FAIL;
    }

    return g_watchDogMgr->Init();
}

void DestroyWatchDogMgr()
{
    if (g_watchDogMgr != nullptr) {
        g_watchDogMgr->Shutdown();
        delete g_watchDogMgr;
        g_watchDogMgr = nullptr;
    }
}

WatchDogMgr *GetWatchDogMgr()
{
    return WatchDogMgr::GetInstance();
}

} // namespace DSTORE