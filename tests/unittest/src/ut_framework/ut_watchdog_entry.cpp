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

#include <gtest/gtest.h>
#include "ut_framework/ut_watchdog_entry.h"
#include "framework/dstore_watchdog_entry.h"
#include "framework/dstore_instance.h"

using namespace DSTORE;

void WatchDogEntryTest::SetUp()
{
    DSTORETEST::SetUp();
}

void WatchDogEntryTest::TearDown()
{
    DSTORETEST::TearDown();
}

TEST_F(WatchDogEntryTest, InitDestroy_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);
    entry.Destroy();
}

TEST_F(WatchDogEntryTest, FeedReset_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);

    entry.Feed();
    EXPECT_EQ(entry.GetStatus(), WatchDogStatus::HEALTHY);

    entry.Reset();
    EXPECT_EQ(entry.GetStatus(), WatchDogStatus::HEALTHY);

    entry.Destroy();
}

TEST_F(WatchDogEntryTest, StateTransitions_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);

    EXPECT_EQ(entry.GetStatus(), WatchDogStatus::HEALTHY);
    EXPECT_TRUE(entry.IsRegistered());

    entry.MarkUnregistered();
    EXPECT_FALSE(entry.IsRegistered());
    EXPECT_EQ(entry.GetStatus(), WatchDogStatus::UNREGISTERED);

    entry.Destroy();
}

TEST_F(WatchDogEntryTest, GraceWindow_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);

    TimestampTz currentTime = GetCurrentTimestamp();
    WatchDogStatus status = entry.EvaluateHealth(currentTime);
    EXPECT_EQ(status, WatchDogStatus::HEALTHY);

    entry.Destroy();
}

TEST_F(WatchDogEntryTest, MissCounting_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 5000), DSTORE_SUCC);

    entry.Reset();
    EXPECT_EQ(entry.GetConsecutiveMissCount(), 0);

    entry.Feed();
    EXPECT_EQ(entry.GetConsecutiveMissCount(), 0);

    entry.Destroy();
}

TEST_F(WatchDogEntryTest, EntryIdEquality_level0)
{
    WatchDogEntryId id1(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    WatchDogEntryId id2(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    WatchDogEntryId id3(2, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);

    EXPECT_TRUE(id1 == id2);
    EXPECT_FALSE(id1 == id3);
}

TEST_F(WatchDogEntryTest, ThreadName_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);

    EXPECT_STREQ(entry.GetThreadName(), "TestThread");

    entry.Destroy();
}

TEST_F(WatchDogEntryTest, TimeoutThreshold_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 5000), DSTORE_SUCC);

    EXPECT_EQ(entry.GetTimeoutMs(), 5000);

    entry.Destroy();
}

TEST_F(WatchDogEntryTest, LastHeartbeatTime_level0)
{
    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);

    TimestampTz beforeFeed = GetCurrentTimestamp();
    entry.Feed();
    TimestampTz afterFeed = GetCurrentTimestamp();

    TimestampTz lastBeat = entry.GetLastHeartbeatTime();
    EXPECT_GE(lastBeat, beforeFeed);
    EXPECT_LE(lastBeat, afterFeed);

    entry.Destroy();
}