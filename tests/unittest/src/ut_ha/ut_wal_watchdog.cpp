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
#include "ut_ha/ut_wal_watchdog.h"
#include "framework/dstore_watchdog_mgr.h"
#include "framework/dstore_instance.h"
#include "wal/dstore_wal_bgwriter.h"

using namespace DSTORE;

void WalWatchdogTest::SetUp()
{
    DSTORETEST::SetUp();
    ASSERT_EQ(InitWatchDogMgr(), DSTORE_SUCC);
}

void WalWatchdogTest::TearDown()
{
    DestroyWatchDogMgr();
    DSTORETEST::TearDown();
}

TEST_F(WalWatchdogTest, RegisterWatchdogEntry_level0)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    ASSERT_NE(mgr, nullptr);

    WatchDogEntryId entryId;
    EXPECT_EQ(mgr->Register(WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId, "WAL_FLUSH", 30000, entryId),
        DSTORE_SUCC);
    mgr->Unregister(entryId);
}

TEST_F(WalWatchdogTest, FeedHeartbeat_level0)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    ASSERT_NE(mgr, nullptr);

    WatchDogEntryId entryId;
    EXPECT_EQ(mgr->Register(WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId, "WAL_FLUSH", 30000, entryId),
        DSTORE_SUCC);
    mgr->FeedTask(entryId);
    WatchDogEntry *entry = mgr->FindEntry(entryId);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->GetStatus(), WatchDogStatus::HEALTHY);

    mgr->Unregister(entryId);
}

TEST_F(WalWatchdogTest, WalFlushLifecycle_level1)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    ASSERT_NE(mgr, nullptr);

    WatchDogEntryId entryId;
    EXPECT_EQ(mgr->Register(WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId, "WAL_FLUSH", 30000, entryId),
        DSTORE_SUCC);
    mgr->FeedTask(entryId);
    mgr->Unregister(entryId);
}

TEST_F(WalWatchdogTest, WalRecycleLifecycle_level1)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    ASSERT_NE(mgr, nullptr);

    WatchDogEntryId entryId;
    EXPECT_EQ(mgr->Register(WatchDogThreadCategory::WAL_FILE_RECYCLE, g_defaultPdbId, "WAL_FILE_RECYCLE", 30000,
        entryId), DSTORE_SUCC);
    mgr->FeedTask(entryId);
    mgr->Unregister(entryId);
}
