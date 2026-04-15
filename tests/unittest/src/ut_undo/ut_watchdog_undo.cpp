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
#include "ut_undo/ut_watchdog_undo.h"
#include "framework/dstore_watchdog_mgr.h"
#include "framework/dstore_instance.h"

using namespace DSTORE;

void UndoWatchdogTest::SetUp()
{
    DSTORETEST::SetUp();
    ASSERT_EQ(InitWatchDogMgr(), DSTORE_SUCC);
}

void UndoWatchdogTest::TearDown()
{
    DestroyWatchDogMgr();
    DSTORETEST::TearDown();
}

TEST_F(UndoWatchdogTest, UndoRecycleDispatchWatchdog_level0)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    ASSERT_NE(mgr, nullptr);
    WatchDogEntryId entryId;
    EXPECT_EQ(mgr->Register(WatchDogThreadCategory::UNDO_RECYCLE_DISPATCH, g_defaultPdbId, "UNDO_RECYCLE", 30000,
        entryId), DSTORE_SUCC);
    mgr->FeedTask(entryId);
    mgr->Unregister(entryId);
}

TEST_F(UndoWatchdogTest, UndoLifecycle_level1)
{
    WatchDogMgr *mgr = GetWatchDogMgr();
    ASSERT_NE(mgr, nullptr);
    WatchDogEntryId entryId;
    EXPECT_EQ(mgr->Register(WatchDogThreadCategory::UNDO_RECYCLE_DISPATCH, g_defaultPdbId, "UNDO_RECYCLE", 30000,
        entryId), DSTORE_SUCC);
    for (int i = 0; i < 10; i++) {
        mgr->FeedTask(entryId);
    }
    mgr->Unregister(entryId);
}
