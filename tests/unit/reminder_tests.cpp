#include <gtest/gtest.h>

#include <QDir>
#include <QUuid>

#include "core/Entities.h"
#include "core/LedgerService.h"

using namespace core;

/* 测试提醒的增删改查及过滤功能 共4个测试样例 */

// 用例：手动指定 ID 的提醒可写入并删除，基础增删流程正常。
TEST(ReminderTests, UpsertAndRemoveReminder) {
  const QString envPath = QDir::tempPath() + "/bk_reminder_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;

  ASSERT_TRUE(service.registerUser("user1", "user1@example.com", "p", userId, err)) << err.toStdString();

  Reminder r;
  r.id = "r1";
  r.message = "pay rent";
  r.remindAt = QDateTime::fromString("2024-03-01T12:00:00Z", Qt::ISODate);

  ASSERT_TRUE(service.upsertReminder(userId, r, err)) << err.toStdString();

  const auto reminders = service.reminders(userId);
  ASSERT_EQ(reminders.size(), 1);
  EXPECT_EQ(reminders.first().id, "r1");
  EXPECT_EQ(reminders.first().message, "pay rent");

  ASSERT_TRUE(service.removeReminder(userId, "r1", err)) << err.toStdString();
  EXPECT_TRUE(service.reminders(userId).isEmpty());

  QDir(envPath).removeRecursively();
}

// 用例：upcomingReminders 过滤掉未启用或超出时间窗的提醒，仅返回窗口内启用项。
TEST(ReminderTests, UpcomingFiltersDisabledAndOutsideWindow) {
  const QString envPath = QDir::tempPath() + "/bk_reminder_window_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("user2", "user2@example.com", "p", userId, err)) << err.toStdString();

  const auto now = QDateTime::fromString("2024-03-01T10:00:00Z", Qt::ISODate);

  // 在窗口内 但未启用
  Reminder disabled;
  disabled.id = "disabled";
  disabled.message = "ignore";
  disabled.remindAt = now.addSecs(60);
  disabled.enabled = false;

  // 超出窗口
  Reminder outside;
  outside.id = "outside";
  outside.message = "too late";
  outside.remindAt = now.addDays(2);

  // 窗口内且启用
  Reminder inside;
  inside.id = "inside";
  inside.message = "soon";
  inside.remindAt = now.addSecs(120);

  ASSERT_TRUE(service.upsertReminder(userId, disabled, err)) << err.toStdString();
  ASSERT_TRUE(service.upsertReminder(userId, outside, err)) << err.toStdString();
  ASSERT_TRUE(service.upsertReminder(userId, inside, err)) << err.toStdString();

  const auto upcoming = service.upcomingReminders(userId, now, now.addDays(1));
  ASSERT_EQ(upcoming.size(), 1);
  EXPECT_EQ(upcoming.first().id, "inside");

  QDir(envPath).removeRecursively();
}

// 用例：缺省 ID 与时间的提醒在 upsert 时自动填充有效 ID 和时间戳。
TEST(ReminderTests, UpsertAssignsIdAndTimestampWhenMissing) {
  const QString envPath = QDir::tempPath() + "/bk_reminder_auto_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("user3", "user3@example.com", "p", userId, err)) << err.toStdString();

  Reminder r;  // 空 ID 与无效时间应被自动填充。
  ASSERT_TRUE(service.upsertReminder(userId, r, err)) << err.toStdString();

  const auto reminders = service.reminders(userId);
  ASSERT_EQ(reminders.size(), 1);
  EXPECT_FALSE(reminders.first().id.isEmpty());
  EXPECT_TRUE(reminders.first().remindAt.isValid());

  QDir(envPath).removeRecursively();
}

// 用例：删除不存在的提醒返回 false 并给出“未找到提醒”错误。
TEST(ReminderTests, RemoveReminderNotFoundReturnsError) {
  const QString envPath = QDir::tempPath() + "/bk_reminder_notfound_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("user4", "user4@example.com", "p", userId, err)) << err.toStdString();

  EXPECT_FALSE(service.removeReminder(userId, "nope", err));
  EXPECT_EQ(err, "未找到提醒");

  QDir(envPath).removeRecursively();
}