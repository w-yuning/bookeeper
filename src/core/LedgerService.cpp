// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "LedgerService.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QPair>
#include <QUuid>
#include <QVector>
#include <algorithm>

namespace core {

// 初始化服务时确定数据目录。
LedgerService::LedgerService() : storage_(JsonStorage::defaultDataDir()) {}

// 注册流程包括唯一性校验、默认分类初始化与写入存储。
bool LedgerService::registerUser(const QString &username, const QString &email,
                                 const QString &password, QString &outUserId,
                                 QString &errorMessage) {
  const auto existing = storage_.listUsers();
  for (const auto &profile : existing) {
    if (profile.username.compare(username, Qt::CaseInsensitive) == 0) {
      errorMessage = "用户名已存在";
      return false;
    }
    if (profile.email.compare(email, Qt::CaseInsensitive) == 0) {
      errorMessage = "邮箱已注册";
      return false;
    }
  }

  UserData data;
  data.profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  data.profile.username = username;
  data.profile.email = email;
  data.profile.passwordHash = hashPassword(password);
  data.profile.notificationsEnabled = true;
  data.profile.privacyLevel = "friends";

  const QVector<QPair<QString, QString>> defaults = {{"日常支出", "expense"},
                                                     {"餐饮", "expense"},
                                                     {"交通", "expense"},
                                                     {"工资", "income"},
                                                     {"其他", "expense"}};
  for (const auto &item : defaults) {
    Category category;
    category.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    category.name = item.first;
    category.type = item.second;
    data.categories.push_back(category);
  }

  if (!storage_.saveUser(data)) {
    errorMessage = "无法保存用户数据";
    return false;
  }

  outUserId = data.profile.id;
  return true;
}

// 懒加载遍历查找匹配用户名或邮箱，验证哈希后返回用户。
std::optional<UserProfile>
LedgerService::authenticate(const QString &usernameOrEmail,
                            const QString &password, QString &errorMessage) {
  const auto profiles = storage_.listUsers();
  for (const auto &profile : profiles) {
    if (profile.username.compare(usernameOrEmail, Qt::CaseInsensitive) == 0 ||
        profile.email.compare(usernameOrEmail, Qt::CaseInsensitive) == 0) {
      if (verifyPassword(password, profile.passwordHash)) {
        return profile;
      }
      errorMessage = "密码错误";
      return std::nullopt;
    }
  }
  errorMessage = "用户不存在";
  return std::nullopt;
}

// 简单设置更新直接落库。
bool LedgerService::updateSettings(const QString &userId,
                                   bool notificationsEnabled,
                                   const QString &privacyLevel,
                                   QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }
  data.profile.notificationsEnabled = notificationsEnabled;
  data.profile.privacyLevel = privacyLevel;
  return saveUser(data);
}

// 好友互加需同时更新双方的关系表。
bool LedgerService::addFriend(const QString &userId,
                              const QString &friendHandle,
                              QString &errorMessage) {
  const auto friendProfile = findUserByHandle(friendHandle);
  if (!friendProfile.has_value()) {
    errorMessage = "未找到对应用户";
    return false;
  }
  if (friendProfile->id == userId) {
    errorMessage = "不能添加自己为好友";
    return false;
  }

  UserData userData;
  if (!loadUser(userId, userData)) {
    errorMessage = "读取用户失败";
    return false;
  }
  UserData friendData;
  if (!loadUser(friendProfile->id, friendData)) {
    errorMessage = "读取好友失败";
    return false;
  }

  if (!userData.profile.friendIds.contains(friendProfile->id)) {
    userData.profile.friendIds.append(friendProfile->id);
  }
  if (!friendData.profile.friendIds.contains(userId)) {
    friendData.profile.friendIds.append(userId);
  }

  return saveUser(userData) && saveUser(friendData);
}

// 分类查询读取整个用户数据再返回拷贝。
QVector<Category> LedgerService::categories(const QString &userId) const {
  UserData data;
  if (!loadUser(userId, data)) {
    return {};
  }
  return data.categories;
}

// 分类新增或更新，若引用被使用则禁止删除。
bool LedgerService::upsertCategory(const QString &userId,
                                   const Category &category,
                                   QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }

  Category updated = category;
  if (updated.id.isEmpty()) {
    updated.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  }

  bool found = false;
  for (auto &existing : data.categories) {
    if (existing.id == updated.id) {
      existing = updated;
      found = true;
      break;
    }
  }
  if (!found) {
    data.categories.push_back(updated);
  }
  return saveUser(data);
}

// 删除分类前需要确认没有账单引用该分类。
bool LedgerService::removeCategory(const QString &userId,
                                   const QString &categoryId,
                                   QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }

  const bool inUse =
      std::any_of(data.bills.begin(), data.bills.end(), [&](const Bill &bill) {
        return bill.categoryId == categoryId;
      });
  if (inUse) {
    errorMessage = "分类被账单使用，无法删除";
    return false;
  }

  data.categories.erase(
      std::remove_if(data.categories.begin(), data.categories.end(),
                     [&](const Category &cat) { return cat.id == categoryId; }),
      data.categories.end());
  return saveUser(data);
}

// 账单增删改流程与分类类似，需校验分类存在。
QVector<Bill> LedgerService::bills(const QString &userId) const {
  UserData data;
  if (!loadUser(userId, data)) {
    return {};
  }
  return data.bills;
}

// 新建或编辑账单，同时补全缺失的 ID 与时间戳。
bool LedgerService::upsertBill(const QString &userId, const Bill &bill,
                               QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }

  Bill updated = bill;
  if (updated.id.isEmpty()) {
    updated.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  }
  if (!updated.timestamp.isValid()) {
    updated.timestamp = QDateTime::currentDateTime();
  }

  const bool categoryExists = std::any_of(
      data.categories.begin(), data.categories.end(),
      [&](const Category &cat) { return cat.id == updated.categoryId; });
  if (!categoryExists) {
    errorMessage = "分类不存在";
    return false;
  }

  bool found = false;
  for (auto &existing : data.bills) {
    if (existing.id == updated.id) {
      existing = updated;
      found = true;
      break;
    }
  }
  if (!found) {
    data.bills.push_back(updated);
  }
  return saveUser(data);
}

// 删除指定账单，若未找到则返回错误提示。
bool LedgerService::removeBill(const QString &userId, const QString &billId,
                               QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }

  const auto before = data.bills.size();
  data.bills.erase(
      std::remove_if(data.bills.begin(), data.bills.end(),
                     [&](const Bill &bill) { return bill.id == billId; }),
      data.bills.end());
  if (before == data.bills.size()) {
    errorMessage = "未找到账单";
    return false;
  }
  return saveUser(data);
}

// 汇总接口合并分类与账单，方便 UI 可视化。
QVector<CategorySummary>
LedgerService::summarizeByCategory(const QString &userId) const {
  UserData data;
  if (!loadUser(userId, data)) {
    return {};
  }
  QVector<CategorySummary> summaries;
  for (const auto &category : data.categories) {
    CategorySummary summary;
    summary.categoryId = category.id;
    summary.name = category.name;
    summaries.push_back(summary);
  }
  for (const auto &bill : data.bills) {
    auto it = std::find_if(summaries.begin(), summaries.end(),
                           [&](const CategorySummary &summary) {
                             return summary.categoryId == bill.categoryId;
                           });
    if (it != summaries.end()) {
      if (bill.type == BillType::Income) {
        it->income += bill.amount;
      } else {
        it->expense += bill.amount;
      }
    }
  }
  return summaries;
}

// 统计总收入/总支出时直接遍历账单。
double LedgerService::totalIncome(const QString &userId) const {
  double total = 0.0;
  for (const auto &bill : bills(userId)) {
    if (bill.type == BillType::Income) {
      total += bill.amount;
    }
  }
  return total;
}

// 统计总支出。
double LedgerService::totalExpense(const QString &userId) const {
  double total = 0.0;
  for (const auto &bill : bills(userId)) {
    if (bill.type == BillType::Expense) {
      total += bill.amount;
    }
  }
  return total;
}

// 提醒相关接口在保存时确保时间有效。
QVector<Reminder> LedgerService::reminders(const QString &userId) const {
  UserData data;
  if (!loadUser(userId, data)) {
    return {};
  }
  return data.reminders;
}

// 新建或更新提醒，自动补齐缺失的 ID 与时间。
bool LedgerService::upsertReminder(const QString &userId,
                                   const Reminder &reminder,
                                   QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }

  Reminder updated = reminder;
  if (updated.id.isEmpty()) {
    updated.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  }
  if (!updated.remindAt.isValid()) {
    updated.remindAt = QDateTime::currentDateTime();
  }

  bool found = false;
  for (auto &existing : data.reminders) {
    if (existing.id == updated.id) {
      existing = updated;
      found = true;
      break;
    }
  }
  if (!found) {
    data.reminders.push_back(updated);
  }
  return saveUser(data);
}

// 删除提醒，若未找到则反馈错误。
bool LedgerService::removeReminder(const QString &userId,
                                   const QString &reminderId,
                                   QString &errorMessage) {
  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }
  const auto before = data.reminders.size();
  data.reminders.erase(std::remove_if(data.reminders.begin(),
                                      data.reminders.end(),
                                      [&](const Reminder &reminder) {
                                        return reminder.id == reminderId;
                                      }),
                       data.reminders.end());
  if (before == data.reminders.size()) {
    errorMessage = "未找到提醒";
    return false;
  }
  return saveUser(data);
}

// 根据时间窗口筛选启用状态的提醒。
QVector<Reminder> LedgerService::upcomingReminders(const QString &userId,
                                                   const QDateTime &from,
                                                   const QDateTime &to) const {
  QVector<Reminder> result;
  for (const auto &reminder : reminders(userId)) {
    if (!reminder.enabled) {
      continue;
    }
    if (reminder.remindAt >= from && reminder.remindAt <= to) {
      result.push_back(reminder);
    }
  }
  return result;
}

// 时间线会汇总本人、好友与公开动态，按时间倒序排序。
QVector<SocialPost> LedgerService::timeline(const QString &userId) const {
  UserData viewerData;
  if (!loadUser(userId, viewerData)) {
    return {};
  }

  QVector<SocialPost> posts = viewerData.posts;
  const auto profiles = storage_.listUsers();
  for (const auto &profile : profiles) {
    if (profile.id == userId) {
      continue;
    }

    UserData authorData;
    if (!loadUser(profile.id, authorData)) {
      continue;
    }

    const bool isFriend = viewerData.profile.friendIds.contains(profile.id) &&
                          authorData.profile.friendIds.contains(userId);

    for (const auto &post : authorData.posts) {
      if (post.visibility == "public") {
        posts.push_back(post);
      } else if (post.visibility == "friends" && isFriend) {
        posts.push_back(post);
      }
    }
  }

  std::sort(posts.begin(), posts.end(),
            [](const SocialPost &a, const SocialPost &b) {
              return a.createdAt > b.createdAt;
            });
  return posts;
}

// 发布动态生成新的 UUID 并写入时间戳。
bool LedgerService::publishPost(const QString &userId, const QString &content,
                                const QString &visibility,
                                QString &errorMessage) {
  if (content.trimmed().isEmpty()) {
    errorMessage = "内容不能为空";
    return false;
  }

  UserData data;
  if (!loadUser(userId, data)) {
    errorMessage = "读取用户失败";
    return false;
  }

  SocialPost post;
  post.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  post.authorId = userId;
  post.content = content;
  post.visibility = visibility;
  post.createdAt = QDateTime::currentDateTimeUtc();

  data.posts.push_back(post);
  return saveUser(data);
}

// 评论追加到动态所属用户的数据中。
bool LedgerService::addComment(const QString &userId,
                               const QString &postOwnerId,
                               const QString &postId, const QString &content,
                               QString &errorMessage) {
  if (content.trimmed().isEmpty()) {
    errorMessage = "评论不能为空";
    return false;
  }

  UserData ownerData;
  if (!loadUser(postOwnerId, ownerData)) {
    errorMessage = "读取动态失败";
    return false;
  }

  Comment comment;
  comment.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  comment.authorId = userId;
  comment.content = content;
  comment.createdAt = QDateTime::currentDateTimeUtc();

  bool updated = false;
  for (auto &post : ownerData.posts) {
    if (post.id == postId) {
      post.comments.push_back(comment);
      updated = true;
      break;
    }
  }

  if (!updated) {
    errorMessage = "未找到动态";
    return false;
  }

  return saveUser(ownerData);
}

// 读取用户档案的统一入口。
std::optional<UserProfile> LedgerService::profile(const QString &userId) const {
  UserData data;
  if (!loadUser(userId, data)) {
    return std::nullopt;
  }
  return data.profile;
}

// 封装存储层读取，方便统一错误处理。
bool LedgerService::loadUser(const QString &userId, UserData &data) const {
  return storage_.loadUser(userId, data);
}

// 持久化用户数据的便捷入口。
bool LedgerService::saveUser(const UserData &data) const {
  return storage_.saveUser(data);
}

// 根据用户名或邮箱查找用户档案。
std::optional<UserProfile>
LedgerService::findUserByHandle(const QString &handle) const {
  const auto profiles = storage_.listUsers();
  for (const auto &profile : profiles) {
    if (profile.username.compare(handle, Qt::CaseInsensitive) == 0 ||
        profile.email.compare(handle, Qt::CaseInsensitive) == 0) {
      return profile;
    }
  }
  return std::nullopt;
}

// 采用 SHA-256 生成密码哈希。
QString LedgerService::hashPassword(const QString &password) {
  const auto hash =
      QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
  return QString::fromLatin1(hash.toHex());
}

// 将明文密码与哈希值进行对比。
bool LedgerService::verifyPassword(const QString &password,
                                   const QString &hash) {
  return hashPassword(password) == hash;
}

}  // namespace core
