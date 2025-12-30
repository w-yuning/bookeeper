#include <gtest/gtest.h>

#include <QDateTime>
#include <QDir>
#include <QJsonObject>

#include "core/Entities.h"
#include "core/JsonStorage.h"

using namespace core;

/* 冒烟测试 不用管 */

// Basic round-trip for Bill JSON serialization.
TEST(EntitiesSmokeTest, BillJsonRoundTrip) {
  Bill bill;
  bill.id = "b1";
  bill.amount = 123.45;
  bill.categoryId = "c1";
  bill.note = "lunch";
  bill.timestamp = QDateTime::fromSecsSinceEpoch(1700000000);
  bill.type = BillType::Income;

  const auto obj = bill.toJson();
  const auto restored = Bill::fromJson(obj);

  EXPECT_EQ(restored.id, bill.id);
  EXPECT_DOUBLE_EQ(restored.amount, bill.amount);
  EXPECT_EQ(restored.categoryId, bill.categoryId);
  EXPECT_EQ(restored.note, bill.note);
  EXPECT_EQ(restored.timestamp, bill.timestamp);
  EXPECT_EQ(restored.type, bill.type);
}

// Ensure BOOKEEPER_DATA_DIR overrides defaultDataDir for test isolation.
TEST(StorageSmokeTest, DataDirEnvironmentOverride) {
  const QString envPath = QDir::tempPath() + "/bk_env_override";
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());

  const QDir dir = JsonStorage::defaultDataDir();
  EXPECT_EQ(dir.absolutePath(), QDir(envPath).absolutePath());

  // Cleanup the temporary directory.
  QDir(envPath).removeRecursively();
}
