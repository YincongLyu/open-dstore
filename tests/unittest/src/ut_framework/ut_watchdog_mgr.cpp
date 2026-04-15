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
#include "ut_framework/ut_watchdog_mgr.h"
#include "framework/dstore_watchdog_mgr.h"
#include "framework/dstore_instance.h"
#include "common/memory/dstore_mctx.h"

using namespace DSTORE;

void WatchDogMgrTest::SetUp()
{
    DSTORETEST::SetUp();
}

void WatchDogMgrTest::TearDown()
{
    DSTORETEST::TearDown();
}

TEST_F(WatchDogMgrTest, InitDestroy_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);
    mgr.Destroy();
}

TEST_F(WatchDogMgrTest, StartStop_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);
    EXPECT_EQ(mgr.Start(), DSTORE_SUCC);
    mgr.Stop();
    mgr.Destroy();
}

TEST_F(WatchDogMgrTest, RegisterUnregister_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);

    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);

    EXPECT_EQ(mgr.Register(&entry), DSTORE_SUCC);
    EXPECT_EQ(mgr.GetActiveEntryCount(), 1);

    mgr.Unregister(&entry);
    EXPECT_EQ(mgr.GetActiveEntryCount(), 0);

    mgr.Destroy();
}

TEST_F(WatchDogMgrTest, GetDiagnoseSnapshot_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);

    WatchDogEntry entry;
    WatchDogEntryId entryId(1, WatchDogThreadCategory::WAL_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "TestThread", 30000), DSTORE_SUCC);
    EXPECT_EQ(mgr.Register(&entry), DSTORE_SUCC);

    WatchDogDiagnoseIterator iterator;
    EXPECT_EQ(iterator.Init(64), DSTORE_SUCC);
    EXPECT_EQ(mgr.GetDiagnoseSnapshot(iterator), DSTORE_SUCC);
    EXPECT_GE(iterator.GetRecordCount(), 1);

    iterator.Destroy();
    mgr.Unregister(&entry);
    mgr.Destroy();
}

TEST_F(WatchDogMgrTest, EmptyRegistry_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);

    WatchDogDiagnoseIterator iterator;
    EXPECT_EQ(iterator.Init(64), DSTORE_SUCC);
    EXPECT_EQ(mgr.GetDiagnoseSnapshot(iterator), DSTORE_SUCC);
    EXPECT_GE(iterator.GetRecordCount(), 1);

    iterator.Destroy();
    mgr.Destroy();
}

TEST_F(WatchDogMgrTest, SelfHealth_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);

    mgr.FeedSelfHealth();
    mgr.CheckSelfHealth();

    mgr.Destroy();
}

TEST_F(WatchDogMgrTest, PolicyConfiguration_level0)
{
    WatchDogMgr mgr;
    EXPECT_EQ(mgr.Init(), DSTORE_SUCC);

    EXPECT_EQ(mgr.GetPolicy(), WatchDogPolicy::LOG_ONLY);
    mgr.SetPolicy(WatchDogPolicy::LOG_AND_HEALING);
    EXPECT_EQ(mgr.GetPolicy(), WatchDogPolicy::LOG_AND_HEALING);

    EXPECT_EQ(mgr.GetCheckIntervalMs(), WATCHDOG_DEFAULT_CHECK_INTERVAL_MS);
    mgr.SetCheckIntervalMs(10000);
    EXPECT_EQ(mgr.GetCheckIntervalMs(), 10000);

    EXPECT_TRUE(mgr.IsEnabled());
    mgr.SetEnabled(false);
    EXPECT_FALSE(mgr.IsEnabled());

    EXPECT_FALSE(mgr.IsHealingEnabled());
    mgr.SetHealingEnabled(true);
    EXPECT_TRUE(mgr.IsHealingEnabled());

    mgr.Destroy();
}