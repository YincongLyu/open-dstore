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
    WatchDogDiagnoseIterator()
        : m_records(nullptr),
          m_maxRecords(0),
          m_currentIdx(0),
          m_recordCount(0),
          m_memoryContext(nullptr)
    {
    }
    ~WatchDogDiagnoseIterator() = default;
    DISALLOW_COPY_AND_MOVE(WatchDogDiagnoseIterator);

    RetStatus Init(size_t maxRecords)
    {
        if (maxRecords == 0) {
            return DSTORE_FAIL;
        }

        Destroy();
        m_memoryContext = DstoreAllocSetContextCreate(g_dstoreCurrentMemoryContext,
            "WatchDogDiagnoseIterator", ALLOCSET_SMALL_SIZES);
        if (STORAGE_VAR_NULL(m_memoryContext)) {
            return DSTORE_FAIL;
        }

        m_records = static_cast<HealthSnapshot *>(
            DstoreMemoryContextAllocZero(m_memoryContext, maxRecords * sizeof(HealthSnapshot)));
        if (m_records == nullptr) {
            DstoreMemoryContextDelete(m_memoryContext);
            m_memoryContext = nullptr;
            return DSTORE_FAIL;
        }

        m_maxRecords = maxRecords;
        m_currentIdx = 0;
        m_recordCount = 0;
        return DSTORE_SUCC;
    }

    void Destroy()
    {
        DstorePfreeExt(m_records);
        if (m_memoryContext != nullptr) {
            DstoreMemoryContextDelete(m_memoryContext);
            m_memoryContext = nullptr;
        }
        m_maxRecords = 0;
        m_currentIdx = 0;
        m_recordCount = 0;
    }

    bool HasNext() const
    {
        return m_currentIdx < m_recordCount;
    }

    HealthSnapshot *GetNext()
    {
        if (!HasNext()) {
            return nullptr;
        }
        return &m_records[m_currentIdx++];
    }

    HealthSnapshot *GetCurrent()
    {
        if (m_currentIdx == 0 || m_currentIdx > m_recordCount) {
            return nullptr;
        }
        return &m_records[m_currentIdx - 1];
    }

    size_t GetRecordCount() const
    {
        return m_recordCount;
    }

    RetStatus AddRecord(const HealthSnapshot &snapshot)
    {
        if (m_records == nullptr || m_recordCount >= m_maxRecords) {
            return DSTORE_FAIL;
        }
        m_records[m_recordCount++] = snapshot;
        return DSTORE_SUCC;
    }

private:
    HealthSnapshot *m_records;
    size_t m_maxRecords;
    size_t m_currentIdx;
    size_t m_recordCount;
    DstoreMemoryContext m_memoryContext;
};

} // namespace DSTORE

#endif
