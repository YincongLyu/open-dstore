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

#ifndef DSTORE_WATCHDOG_DIAGNOSE_H
#define DSTORE_WATCHDOG_DIAGNOSE_H

#include <cstdint>
#include "common/dstore_datatype.h"
#include "framework/dstore_watchdog_entry.h"

namespace DSTORE {

struct HealthSnapshot {
    uint64_t entryId;
    char threadName[WATCHDOG_THREAD_NAME_MAX_LEN];
    WatchDogThreadCategory threadCategory;
    PdbId scopeId;
    uint32_t timeoutThresholdMs;
    TimestampTz lastHeartbeatTime;
    uint64_t unhealthyDurationMs;
    WatchDogStatus healthState;
    bool isSelfHealth;

    HealthSnapshot() 
        : entryId(0),
          threadCategory(WatchDogThreadCategory::MAX_CATEGORY),
          scopeId(INVALID_PDB_ID),
          timeoutThresholdMs(0),
          lastHeartbeatTime(0),
          unhealthyDurationMs(0),
          healthState(WatchDogStatus::UNREGISTERED),
          isSelfHealth(false)
    {
        threadName[0] = '\0';
    }

    void Clear() {
        entryId = 0;
        threadName[0] = '\0';
        threadCategory = WatchDogThreadCategory::MAX_CATEGORY;
        scopeId = INVALID_PDB_ID;
        timeoutThresholdMs = 0;
        lastHeartbeatTime = 0;
        unhealthyDurationMs = 0;
        healthState = WatchDogStatus::UNREGISTERED;
        isSelfHealth = false;
    }
};

struct WarningEvent {
    enum class WarningType : uint8 {
        PEER_TASK_WARNING,
        SELF_HEALTH_WARNING
    };

    WarningType warningType;
    char threadName[WATCHDOG_THREAD_NAME_MAX_LEN];
    WatchDogThreadCategory threadCategory;
    PdbId scopeId;
    TimestampTz observedAt;
    uint64_t overdueDurationMs;
    uint32_t timeoutThresholdMs;
    uint32_t missCount;

    WarningEvent()
        : warningType(WarningType::PEER_TASK_WARNING),
          threadCategory(WatchDogThreadCategory::MAX_CATEGORY),
          scopeId(INVALID_PDB_ID),
          observedAt(0),
          overdueDurationMs(0),
          timeoutThresholdMs(0),
          missCount(0)
    {
        threadName[0] = '\0';
    }
};

class WatchDogDiagnoseIterator {
public:
    WatchDogDiagnoseIterator();
    ~WatchDogDiagnoseIterator() = default;
    DISALLOW_COPY_AND_MOVE(WatchDogDiagnoseIterator);

    RetStatus Init(size_t maxRecords);
    void Destroy();

    bool HasNext() const;
    HealthSnapshot *GetNext();
    HealthSnapshot *GetCurrent();

    size_t GetRecordCount() const;
    RetStatus AddRecord(const HealthSnapshot &snapshot);

private:
    HealthSnapshot *m_records;
    size_t m_maxRecords;
    size_t m_currentIdx;
    size_t m_recordCount;
    DstoreMemoryContext m_memoryContext;
};

} // namespace DSTORE

#endif