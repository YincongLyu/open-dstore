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
#include "ut_buffer/ut_buffer_watchdog.h"
#include "framework/dstore_watchdog_mgr.h"
#include "framework/dstore_instance.h"

using namespace DSTORE;

void BufferWatchdogTest::SetUp()
{
    DSTORETEST::SetUp();
}

void BufferWatchdogTest::TearDown()
{
    DSTORETEST::TearDown();
}

TEST_F(BufferWatchdogTest, CheckpointWatchdog_level0)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    if (mgr == nullptr) {
        return;
    }

    WatchDogEntry entry;
    WatchDogEntryId entryId(3, WatchDogThreadCategory::CHECKPOINT_PROGRESS, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "CHECKPOINT", 30000), DSTORE_SUCC);

    EXPECT_EQ(mgr->Register(&entry), DSTORE_SUCC);
    entry.Feed();
    mgr->Unregister(&entry);
}

TEST_F(BufferWatchdogTest, DirtyPageFlushWatchdog_level0)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    if (mgr == nullptr) {
        return;
    }

    WatchDogEntry entry;
    WatchDogEntryId entryId(4, WatchDogThreadCategory::BUFFER_DIRTY_PAGE_FLUSH, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "DIRTY_PAGE_FLUSH", 30000), DSTORE_SUCC);

    EXPECT_EQ(mgr->Register(&entry), DSTORE_SUCC);
    entry.Feed();
    mgr->Unregister(&entry);
}

TEST_F(BufferWatchdogTest, CheckpointLifecycle_level1)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    if (mgr == nullptr) {
        return;
    }

    WatchDogEntry entry;
    WatchDogEntryId entryId(3, WatchDogThreadCategory::CHECKPOINT_PROGRESS, g_defaultPdbId);
    EXPECT_EQ(entry.Init(entryId, "CHECKPOINT", 30000), DSTORE_SUCC);

    EXPECT_EQ(mgr->Register(&entry), DSTORE_SUCC);
    for (int i = 0; i < 5; i++) {
        entry.Feed();
    }
    mgr->Unregister(&entry);
}