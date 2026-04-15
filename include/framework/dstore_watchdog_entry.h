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

#ifndef DSTORE_WATCHDOG_ENTRY_H
#define DSTORE_WATCHDOG_ENTRY_H

#include <atomic>
#include <cstdint>
#include "common/dstore_common_utils.h"
#include "common/dstore_datatype.h"
#include "common/memory/dstore_mctx.h"

namespace DSTORE {

enum class WatchDogStatus : uint8 {
    HEALTHY,
    WARN,
    UNHEALTHY,
    UNREGISTERED
};

enum class WatchDogThreadCategory : uint8 {
    WAL_FLUSH,
    WAL_FILE_RECYCLE,
    CHECKPOINT_PROGRESS,
    BUFFER_DIRTY_PAGE_FLUSH,
    UNDO_RECYCLE_DISPATCH,
    BTREE_RECYCLE_PRUNE,
    MAX_CATEGORY
};

constexpr uint32_t WATCHDOG_DEFAULT_TIMEOUT_MS = 30000;
constexpr uint32_t WATCHDOG_DEFAULT_CHECK_INTERVAL_MS = 5000;
constexpr uint32_t WATCHDOG_MAX_ENTRIES = 64;
constexpr uint32_t WATCHDOG_MISS_THRESHOLD = 3;
constexpr uint32_t WATCHDOG_THREAD_NAME_MAX_LEN = 32;

struct WatchDogEntryId {
    uint64_t id;
    WatchDogThreadCategory category;
    PdbId scopeId;
    
    WatchDogEntryId() : id(0), category(WatchDogThreadCategory::MAX_CATEGORY), scopeId(INVALID_PDB_ID) {}
    WatchDogEntryId(uint64_t idVal, WatchDogThreadCategory cat, PdbId scope) 
        : id(idVal), category(cat), scopeId(scope) {}
    
    bool operator==(const WatchDogEntryId &other) const {
        return id == other.id && category == other.category && scopeId == other.scopeId;
    }
};

class WatchDogEntry {
public:
    WatchDogEntry();
    ~WatchDogEntry() = default;
    DISALLOW_COPY_AND_MOVE(WatchDogEntry);

    RetStatus Init(const WatchDogEntryId &entryId, const char *threadName, uint32_t timeoutMs);
    void Destroy();

    void Feed();
    void Reset();
    void MarkUnregistered();

    WatchDogStatus GetStatus() const;
    WatchDogEntryId GetEntryId() const;
    const char *GetThreadName() const;
    uint32_t GetTimeoutMs() const;
    TimestampTz GetLastHeartbeatTime() const;
    uint32_t GetConsecutiveMissCount() const;
    bool IsRegistered() const;

    WatchDogStatus EvaluateHealth(TimestampTz currentTime);

private:
    WatchDogEntryId m_entryId;
    char m_threadName[WATCHDOG_THREAD_NAME_MAX_LEN];
    uint32_t m_timeoutMs;
    std::atomic<TimestampTz> m_lastHeartbeatTime;
    std::atomic<uint32_t> m_consecutiveMissCount;
    std::atomic<WatchDogStatus> m_status;
    std::atomic<bool> m_registered;
    TimestampTz m_registerTime;
};

} // namespace DSTORE

#endif
