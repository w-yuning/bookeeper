#include <gtest/gtest.h>

#include <QDir>
#include <QUuid>
#include <algorithm>

#include "core/LedgerService.h"

using namespace core;

/* 测试账单的增删改查及统计功能 共2个测试样例 */

// 用例：新增收入/支出、同 ID 覆盖更新、删除以及总收入/总支出统计。
TEST(LedgerBillTest, CreateUpdateDeleteAndTotals) {
  // 1) 为测试准备独立数据目录
  const QString envPath = QDir::tempPath() + "/bk_bill_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;

  // 2) 注册用户，获取默认分类
  ASSERT_TRUE(service.registerUser("u1", "u1@example.com", "p", userId, err)) << err.toStdString();
  const auto cats = service.categories(userId);
  ASSERT_FALSE(cats.isEmpty());
  const QString catId = cats.first().id;

  // 3) 新增一条收入、一条支出
  Bill income; income.categoryId = catId; income.type = BillType::Income; income.amount = 100.0;
  ASSERT_TRUE(service.upsertBill(userId, income, err)) << err.toStdString();

  // 显式设定 ID，避免 upsert 自动分配不同 ID 导致“新增”而不是“覆盖”。
  Bill expense; expense.id = QStringLiteral("exp-1");
  expense.categoryId = catId; expense.type = BillType::Expense; expense.amount = 30.0;
  ASSERT_TRUE(service.upsertBill(userId, expense, err)) << err.toStdString();

  EXPECT_DOUBLE_EQ(service.totalIncome(userId), 100.0);
  EXPECT_DOUBLE_EQ(service.totalExpense(userId), 30.0);

  // 4) 更新支出金额（upsert 同 ID 覆盖）
  Bill expenseUpdate = expense;
  expenseUpdate.amount = 50.0;
  ASSERT_TRUE(service.upsertBill(userId, expenseUpdate, err)) << err.toStdString();
  EXPECT_DOUBLE_EQ(service.totalExpense(userId), 50.0);

  // 5) 删除支出账单
  ASSERT_TRUE(service.removeBill(userId, expenseUpdate.id, err)) << err.toStdString();
  EXPECT_DOUBLE_EQ(service.totalExpense(userId), 0.0);

  // 6) 删除不存在的账单应返回错误
  EXPECT_FALSE(service.removeBill(userId, "not-exist", err));
  EXPECT_EQ(err, "未找到账单");

  // 7) 清理临时目录
  QDir(envPath).removeRecursively();
}

// 用例：分类不存在时报错；合法账单写入后分类汇总能正确累加收/支。
TEST(LedgerBillTest, UpsertBillValidatesCategoryAndSummaries) {
  const QString envPath = QDir::tempPath() + "/bk_bill_missing_cat_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("u2", "u2@example.com", "p", userId, err)) << err.toStdString();

  // 分类未找到时应失败。
  Bill bad;
  bad.categoryId = "missing";
  bad.amount = 10;
  EXPECT_FALSE(service.upsertBill(userId, bad, err));
  EXPECT_EQ(err, "分类不存在");

  // 先获取默认分类中收入和支出两类ID
  const auto cats = service.categories(userId);
  ASSERT_GE(cats.size(), 2);
  const auto incomeCat = *std::find_if(cats.begin(), cats.end(), [](const Category &c) {
    return c.type == "income";
  });
  const auto expenseCat = *std::find_if(cats.begin(), cats.end(), [](const Category &c) {
    return c.type == "expense";
  });

  // 分别写入一条收入和支出账单
  Bill inc;
  inc.categoryId = incomeCat.id;
  inc.type = BillType::Income;
  inc.amount = 200;
  ASSERT_TRUE(service.upsertBill(userId, inc, err)) << err.toStdString();

  Bill exp;
  exp.categoryId = expenseCat.id;
  exp.type = BillType::Expense;
  exp.amount = 50;
  ASSERT_TRUE(service.upsertBill(userId, exp, err)) << err.toStdString();

  const auto summaries = service.summarizeByCategory(userId);
  const auto incomeSummary = std::find_if(summaries.begin(), summaries.end(), [&](const CategorySummary &s) {
    return s.categoryId == incomeCat.id;
  });
  const auto expenseSummary = std::find_if(summaries.begin(), summaries.end(), [&](const CategorySummary &s) {
    return s.categoryId == expenseCat.id;
  });
  ASSERT_NE(incomeSummary, summaries.end());
  ASSERT_NE(expenseSummary, summaries.end());
  EXPECT_DOUBLE_EQ(incomeSummary->income, 200);
  EXPECT_DOUBLE_EQ(expenseSummary->expense, 50);

  // 再增加一条收入和支出，验证汇总累加
  Bill inc2;
  inc2.categoryId = incomeCat.id;
  inc2.type = BillType::Income;
  inc2.amount = 150;
  ASSERT_TRUE(service.upsertBill(userId, inc2, err)) << err.toStdString();

  Bill exp2;
  exp2.categoryId = expenseCat.id;
  exp2.type = BillType::Expense;
  exp2.amount = 70;
  ASSERT_TRUE(service.upsertBill(userId, exp2, err)) << err.toStdString();

  const auto summaries2 = service.summarizeByCategory(userId);
  const auto incomeSummary2 = std::find_if(summaries2.begin(), summaries2.end(), [&](const CategorySummary &s) {
    return s.categoryId == incomeCat.id;
  });
  const auto expenseSummary2 = std::find_if(summaries2.begin(), summaries2.end(), [&](const CategorySummary &s) {
    return s.categoryId == expenseCat.id;
  });
  ASSERT_NE(incomeSummary2, summaries2.end());
  ASSERT_NE(expenseSummary2, summaries2.end());
  EXPECT_DOUBLE_EQ(incomeSummary2->income, 350);  // 200 + 150
  EXPECT_DOUBLE_EQ(expenseSummary2->expense, 120); // 50 + 70

  QDir(envPath).removeRecursively();
}
