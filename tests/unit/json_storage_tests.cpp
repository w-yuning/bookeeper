#include <gtest/gtest.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

#include "core/Entities.h"
#include "core/JsonStorage.h"

using namespace core;

/* 测试 JsonStorage 的保存与读取完整性，确保序列化/反序列化字段不丢失。共1个测试样例 */

// 中文注释：验证 JsonStorage 的保存与读取完整性，确保序列化/反序列化字段不丢失。
TEST(JsonStorageTest, SaveAndLoadUserDataRoundTrip) {
  // 1) 为测试创建独立的数据目录，避免污染真实数据
  const QString envPath = QDir::tempPath() + "/bk_storage_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  JsonStorage storage;

  // 2) 构造一份完整的 UserData 尽可能覆盖所有字段
  UserData data;
  data.profile.id = "user-1";
  data.profile.username = "alice";
  data.profile.email = "alice@example.com";
  data.profile.passwordHash = "hashed";
  data.profile.notificationsEnabled = true;
  data.profile.privacyLevel = "friends";
  // 友列表显式追加，避免某些 Qt 版本对初始化列表赋值的二义性
  data.profile.friendIds.clear();
  data.profile.friendIds << QStringLiteral("friend-1") << QStringLiteral("friend-2");

  Category cat;
  cat.id = "cat-1";
  cat.name = "餐饮";
  cat.type = "expense";
  data.categories.push_back(cat);

  Bill bill;
  bill.id = "bill-1";
  bill.amount = 88.8;
  bill.categoryId = cat.id;
  bill.note = "晚餐";
  bill.timestamp = QDateTime::fromSecsSinceEpoch(1700000000);
  bill.type = BillType::Expense;
  data.bills.push_back(bill);

  Reminder reminder;
  reminder.id = "rem-1";
  reminder.message = "交房租";
  reminder.remindAt = QDateTime::fromSecsSinceEpoch(1700001000);
  reminder.enabled = true;
  data.reminders.push_back(reminder);

  SocialPost post;
  post.id = "post-1";
  post.authorId = data.profile.id;
  post.content = "公开动态";
  post.visibility = "public";
  post.createdAt = QDateTime::fromSecsSinceEpoch(1700002000);
  Comment comment;
  comment.id = "cmt-1";
  comment.authorId = "friend-1";
  comment.content = "赞";
  comment.createdAt = QDateTime::fromSecsSinceEpoch(1700003000);
  post.comments.push_back(comment);
  data.posts.push_back(post);

  // 3) 保存并重新读取 虽然只调用一个函数 但内部会依次调用其他字段的序列化函数
  ASSERT_TRUE(storage.saveUser(data));

  UserData loaded;
  ASSERT_TRUE(storage.loadUser(data.profile.id, loaded));

  // 4) 字段逐项比对，确保序列化链路正确
  EXPECT_EQ(loaded.profile.id, data.profile.id);
  EXPECT_EQ(loaded.profile.username, data.profile.username);
  EXPECT_EQ(loaded.profile.email, data.profile.email);
  EXPECT_EQ(loaded.profile.passwordHash, data.profile.passwordHash);
  EXPECT_EQ(loaded.profile.privacyLevel, data.profile.privacyLevel);
  EXPECT_EQ(loaded.profile.friendIds, data.profile.friendIds);

  ASSERT_EQ(loaded.categories.size(), 1);
  EXPECT_EQ(loaded.categories[0].id, cat.id);
  EXPECT_EQ(loaded.categories[0].type, cat.type);

  ASSERT_EQ(loaded.bills.size(), 1);
  EXPECT_EQ(loaded.bills[0].id, bill.id);
  EXPECT_DOUBLE_EQ(loaded.bills[0].amount, bill.amount);
  EXPECT_EQ(loaded.bills[0].type, bill.type);

  ASSERT_EQ(loaded.reminders.size(), 1);
  EXPECT_EQ(loaded.reminders[0].id, reminder.id);
  EXPECT_EQ(loaded.reminders[0].remindAt, reminder.remindAt);
  EXPECT_TRUE(loaded.reminders[0].enabled);

  ASSERT_EQ(loaded.posts.size(), 1);
  EXPECT_EQ(loaded.posts[0].id, post.id);
  EXPECT_EQ(loaded.posts[0].visibility, post.visibility);
  ASSERT_EQ(loaded.posts[0].comments.size(), 1);
  EXPECT_EQ(loaded.posts[0].comments[0].id, comment.id);

  // 5) 清理临时目录
  QDir(envPath).removeRecursively();
}
