// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include "Entities.h"
#include "JsonStorage.h"

#include <optional>

namespace core {

// CategorySummary 用于统计界面展示分类收支汇总。
struct CategorySummary {
  QString categoryId;
  QString name;
  double income = 0.0;
  double expense = 0.0;
};

// LedgerService 处理业务逻辑，协调数据存储与 UI 请求。
class LedgerService {
 public:
  // 注册新用户，自动生成默认分类。
  LedgerService();

  bool registerUser(const QString &username, const QString &email,
                    const QString &password, QString &outUserId,
                    QString &errorMessage);

  // 校验用户名/邮箱与密码，成功返回用户档案。
  std::optional<UserProfile> authenticate(const QString &usernameOrEmail,
                                          const QString &password,
                                          QString &errorMessage);

  // 更新通知与隐私设置。
  bool updateSettings(const QString &userId, bool notificationsEnabled,
                      const QString &privacyLevel, QString &errorMessage);

  // 根据用户名或邮箱建立好友关系（双方互相添加）。
  bool addFriend(const QString &userId, const QString &friendHandle,
                 QString &errorMessage);

  // 分类管理接口。
  QVector<Category> categories(const QString &userId) const;
  bool upsertCategory(const QString &userId, const Category &category,
                      QString &errorMessage);
  bool removeCategory(const QString &userId, const QString &categoryId,
                      QString &errorMessage);

  // 账单管理接口。
  QVector<Bill> bills(const QString &userId) const;
  bool upsertBill(const QString &userId, const Bill &bill,
                  QString &errorMessage);
  bool removeBill(const QString &userId, const QString &billId,
                  QString &errorMessage);

  // 分类统计与全局收支。
  QVector<CategorySummary> summarizeByCategory(const QString &userId) const;
  double totalIncome(const QString &userId) const;
  double totalExpense(const QString &userId) const;

  // 提醒管理与筛选。
  QVector<Reminder> reminders(const QString &userId) const;
  bool upsertReminder(const QString &userId, const Reminder &reminder,
                      QString &errorMessage);
  bool removeReminder(const QString &userId, const QString &reminderId,
                      QString &errorMessage);
  QVector<Reminder> upcomingReminders(const QString &userId,
                                      const QDateTime &from,
                                      const QDateTime &to) const;

  // 动态与评论。
  QVector<SocialPost> timeline(const QString &userId) const;
  bool publishPost(const QString &userId, const QString &content,
                   const QString &visibility, QString &errorMessage);
  bool addComment(const QString &userId, const QString &postOwnerId,
                  const QString &postId, const QString &content,
                  QString &errorMessage);

  // 查询单个用户档案。
  std::optional<UserProfile> profile(const QString &userId) const;

 private:
  // 底层读写封装。
  bool loadUser(const QString &userId, UserData &data) const;
  bool saveUser(const UserData &data) const;
  // 根据用户名或邮箱定位用户。
  std::optional<UserProfile> findUserByHandle(const QString &handle) const;
  // 密码哈希与校验。
  static QString hashPassword(const QString &password);
  static bool verifyPassword(const QString &password, const QString &hash);

  JsonStorage storage_;
};

}  // namespace core
