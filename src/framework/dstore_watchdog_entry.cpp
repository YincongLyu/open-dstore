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

#include "framework/dstore_watchdog_entry.h"
#include "common/log/dstore_log.h"
#include "securec.h"
#include <cstring>

namespace DSTORE {

WatchDogEntry::WatchDogEntry()
    : m_timeoutMs(WATCHDOG_DEFAULT_TIMEOUT_MS),
      m_lastHeartbeatTime(0),
      m_consecutiveMissCount(0),
      m_status(WatchDogStatus::UNREGISTERED),
      m_registered(false),
      m_registerTime(0)
{
    m_threadName[0] = '\0';
}

RetStatus WatchDogEntry::Init(const WatchDogEntryId &entryId, const char *threadName, uint32_t timeoutMs)
{
    m_entryId = entryId;
    m_timeoutMs = timeoutMs;

    if (threadName != nullptr) {
        errno_t rc = strncpy_s(m_threadName, WATCHDOG_THREAD_NAME_MAX_LEN, 
                                threadName, strlen(threadName));
        storage_securec_check(rc, "\0", "\0");
    } else {
        m_threadName[0] = '\0';
    }

    m_lastHeartbeatTime = GetCurrentTimestamp();
    m_registerTime = m_lastHeartbeatTime;
    m_consecutiveMissCount = 0;
    m_status = WatchDogStatus::HEALTHY;
    m_registered = true;

    return DSTORE_SUCC;
}

void WatchDogEntry::Destroy()
{
    m_registered = false;
    m_status = WatchDogStatus::UNREGISTERED;
    m_threadName[0] = '\0';
    m_lastHeartbeatTime = 0;
    m_consecutiveMissCount = 0;
}

void WatchDogEntry::Feed()
{
    if (!m_registered) {
        return;
    }

    TimestampTz currentTime = GetCurrentTimestamp();
    m_lastHeartbeatTime = currentTime;
    m_consecutiveMissCount = 0;
    m_status = WatchDogStatus::HEALTHY;
}

void WatchDogEntry::Reset()
{
    TimestampTz currentTime = GetCurrentTimestamp();
    m_lastHeartbeatTime = currentTime;
    m_registerTime = currentTime;
    m_consecutiveMissCount = 0;
    m_status = WatchDogStatus::HEALTHY;
    m_registered = true;
}

void WatchDogEntry::MarkUnregistered()
{
    m_registered = false;
    m_status = WatchDogStatus::UNREGISTERED;
}

WatchDogStatus WatchDogEntry::GetStatus() const
{
    return m_status;
}

WatchDogEntryId WatchDogEntry::GetEntryId() const
{
    return m_entryId;
}

const char *WatchDogEntry::GetThreadName() const
{
    return m_threadName;
}

uint32_t WatchDogEntry::GetTimeoutMs() const
{
    return m_timeoutMs;
}

TimestampTz WatchDogEntry::GetLastHeartbeatTime() const
{
    return m_lastHeartbeatTime;
}

uint32_t WatchDogEntry::GetConsecutiveMissCount() const
{
    return m_consecutiveMissCount;
}

bool WatchDogEntry::IsRegistered() const
{
    return m_registered;
}

WatchDogStatus WatchDogEntry::EvaluateHealth(TimestampTz currentTime)
{
    if (!m_registered) {
        return WatchDogStatus::UNREGISTERED;
    }

    TimestampTz lastBeat = m_lastHeartbeatTime;
    uint64_t elapsedMs = (currentTime - lastBeat) * 1000;

    uint64_t gracePeriodMs = m_timeoutMs;
    uint64_t elapsedSinceRegister = (currentTime - m_registerTime) * 1000;
    if (elapsedSinceRegister < gracePeriodMs) {
        return WatchDogStatus::HEALTHY;
    }

    if (elapsedMs < m_timeoutMs) {
        m_consecutiveMissCount = 0;
        m_status = WatchDogStatus::HEALTHY;
        return m_status;
    }

    m_consecutiveMissCount++;

    if (m_consecutiveMissCount < WATCHDOG_MISS_THRESHOLD) {
        m_status = WatchDogStatus::WARNING;
        return m_status;
    }

    m_status = WatchDogStatus::UNHEALTHY;
    return m_status;
}

} // namespace DSTORE