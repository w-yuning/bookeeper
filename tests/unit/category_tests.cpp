#include <gtest/gtest.h>

#include <QDir>
#include <QUuid>
#include <algorithm>

#include "core/Entities.h"
#include "core/LedgerService.h"

using namespace core;

/* 测试自定义分类相关功能 共2个测试样例 */

// 用例：新增自定义分类后再次 upsert 覆盖名称，验证只新增一条且名称更新。
TEST(CategoryTests, CreateAndRenameCategory) {
  const QString envPath = QDir::tempPath() + "/bk_category_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("cat", "cat@example.com", "p", userId, err)) << err.toStdString();

  const auto initial = service.categories(userId).size();

  Category c;
  c.id = "c1";
  c.name = "Food";
  c.type = "expense";

  // 测试upsert的增加功能
  ASSERT_TRUE(service.upsertCategory(userId, c, err)) << err.toStdString();

  // 测试upsert的更新功能
  c.name = "Dining";
  ASSERT_TRUE(service.upsertCategory(userId, c, err)) << err.toStdString();

  // 验证只有一条且名称已更新
  const auto categories = service.categories(userId);
  ASSERT_EQ(categories.size(), initial + 1);
  const auto found = std::find_if(categories.begin(), categories.end(), [](const Category &cat) {
    return cat.id == "c1";
  });
  ASSERT_NE(found, categories.end());
  EXPECT_EQ(found->name, "Dining");

  QDir(envPath).removeRecursively();
}

// 用例：分类被账单引用时 removeCategory 返回 false 且分类仍存在。
TEST(CategoryTests, DeleteBlockedWhenBillsUseCategory) {
  const QString envPath = QDir::tempPath() + "/bk_category_block_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("cat2", "cat2@example.com", "p", userId, err)) << err.toStdString();

  const auto initial = service.categories(userId).size();

  Category c;
  c.id = "c-block";
  c.name = "Travel";
  c.type = "expense";
  ASSERT_TRUE(service.upsertCategory(userId, c, err)) << err.toStdString();

  Bill b;
  b.id = "b1";
  b.categoryId = c.id;
  b.amount = 100;
  b.type = BillType::Expense;
  ASSERT_TRUE(service.upsertBill(userId, b, err)) << err.toStdString();

  // 尝试删除被账单使用的分类，应失败
  EXPECT_FALSE(service.removeCategory(userId, c.id, err));
  EXPECT_EQ(err, "分类被账单使用，无法删除");

  // 验证分类仍然存在
  const auto categories = service.categories(userId);
  ASSERT_EQ(categories.size(), initial + 1);
  const auto found = std::find_if(categories.begin(), categories.end(), [&](const Category &cat) {
    return cat.id == c.id;
  });
  ASSERT_NE(found, categories.end());

  QDir(envPath).removeRecursively();
}