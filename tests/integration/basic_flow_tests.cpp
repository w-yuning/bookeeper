#include <gtest/gtest.h>

#include <QDateTime>
#include <QDir>
#include <QUuid>

#include <algorithm>

#include "core/LedgerService.h"

using namespace core;

// 集成用例1：注册->登录->新增分类/账单/提醒/动态，重建服务再次登录后继续新增并校验数据持久化与统计正确。
TEST(IntegrationFlow, FullLifecycleWithReloginAndPersistence) {
  const QString envPath = QDir::tempPath() + "/bk_flow_full_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  QString userId;
  QString err;
  QString customCatId;

  // 第一次会话：注册、登录、添加分类/账单/提醒/动态。
  {
    LedgerService service;
    ASSERT_TRUE(service.registerUser("flow_user", "flow@example.com", "pw", userId, err)) << err.toStdString();

    auto profile = service.authenticate("flow_user", "pw", err);
    ASSERT_TRUE(profile.has_value()) << err.toStdString();

    Category cat;
    cat.name = "自定义分类";
    cat.type = "expense";
    ASSERT_TRUE(service.upsertCategory(userId, cat, err)) << err.toStdString();

    const auto cats = service.categories(userId);
    const auto foundCat = std::find_if(cats.begin(), cats.end(), [](const Category &c) {
      return c.name == "自定义分类";
    });
    ASSERT_NE(foundCat, cats.end());
    customCatId = foundCat->id;

    Bill bill1;
    bill1.categoryId = customCatId;
    bill1.type = BillType::Expense;
    bill1.amount = 20.0;
    bill1.note = "coffee";
    ASSERT_TRUE(service.upsertBill(userId, bill1, err)) << err.toStdString();

    Reminder r1;
    r1.message = "pay rent";
    r1.remindAt = QDateTime::currentDateTimeUtc().addSecs(120);
    ASSERT_TRUE(service.upsertReminder(userId, r1, err)) << err.toStdString();

    ASSERT_TRUE(service.publishPost(userId, "first post", "public", err)) << err.toStdString();
  }

  // 第二次会话：重新登录后继续写数据并校验累计结果。
  {
    LedgerService service;
    auto profile = service.authenticate("flow_user", "pw", err);
    ASSERT_TRUE(profile.has_value()) << err.toStdString();

    Bill bill2;
    bill2.categoryId = customCatId;
    bill2.type = BillType::Income;
    bill2.amount = 50.0;
    bill2.note = "bonus";
    ASSERT_TRUE(service.upsertBill(userId, bill2, err)) << err.toStdString();

    Reminder r2;
    r2.message = "check mail";
    r2.remindAt = QDateTime::currentDateTimeUtc().addSecs(240);
    ASSERT_TRUE(service.upsertReminder(userId, r2, err)) << err.toStdString();

    ASSERT_TRUE(service.publishPost(userId, "second post", "public", err)) << err.toStdString();

    const auto bills = service.bills(userId);
    ASSERT_EQ(bills.size(), 2);
    EXPECT_DOUBLE_EQ(service.totalExpense(userId), 20.0);
    EXPECT_DOUBLE_EQ(service.totalIncome(userId), 50.0);

    const auto reminders = service.reminders(userId);
    ASSERT_EQ(reminders.size(), 2);

    const auto timeline = service.timeline(userId);
    ASSERT_EQ(timeline.size(), 2);
    bool hasFirst = false, hasSecond = false;
    for (const auto &p : timeline) {
      if (p.content == "first post") hasFirst = true;
      if (p.content == "second post") hasSecond = true;
    }
    EXPECT_TRUE(hasFirst);
    EXPECT_TRUE(hasSecond);
  }

  QDir(envPath).removeRecursively();
}
