// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "Entities.h"

#include <QJsonDocument>
#include <QJsonValue>

namespace core {

// 将枚举类型转换为 JSON 中使用的字符串表示。
static QString billTypeToString(BillType type) {
  return type == BillType::Income ? "income" : "expense";
}

// 根据存储的字符串恢复枚举类型，默认视为支出。
static BillType billTypeFromString(const QString &value) {
  return value == "income" ? BillType::Income : BillType::Expense;
}

// Category 序列化，将核心字段写入 JSON 结构。
QJsonObject Category::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["name"] = name;
  obj["type"] = type;
  return obj;
}

// Category 反序列化，容错未知字段。
Category Category::fromJson(const QJsonObject &obj) {
  Category category;
  category.id = obj.value("id").toString();
  category.name = obj.value("name").toString();
  category.type = obj.value("type").toString("expense");
  return category;
}

// Bill 序列化，确保时间戳与类型能恢复。
QJsonObject Bill::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["amount"] = amount;
  obj["categoryId"] = categoryId;
  obj["note"] = note;
  obj["timestamp"] = timestamp.toString(Qt::ISODate);
  obj["type"] = billTypeToString(type);
  return obj;
}

// Bill 反序列化，未提供类型时默认为支出。
Bill Bill::fromJson(const QJsonObject &obj) {
  Bill bill;
  bill.id = obj.value("id").toString();
  bill.amount = obj.value("amount").toDouble();
  bill.categoryId = obj.value("categoryId").toString();
  bill.note = obj.value("note").toString();
  bill.timestamp =
      QDateTime::fromString(obj.value("timestamp").toString(), Qt::ISODate);
  bill.type = billTypeFromString(obj.value("type").toString());
  return bill;
}

// Reminder 序列化。
QJsonObject Reminder::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["message"] = message;
  obj["remindAt"] = remindAt.toString(Qt::ISODate);
  obj["enabled"] = enabled;
  return obj;
}

// Reminder 反序列化。
Reminder Reminder::fromJson(const QJsonObject &obj) {
  Reminder reminder;
  reminder.id = obj.value("id").toString();
  reminder.message = obj.value("message").toString();
  reminder.remindAt =
      QDateTime::fromString(obj.value("remindAt").toString(), Qt::ISODate);
  reminder.enabled = obj.value("enabled").toBool(true);
  return reminder;
}

// Comment 序列化。
QJsonObject Comment::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["authorId"] = authorId;
  obj["content"] = content;
  obj["createdAt"] = createdAt.toString(Qt::ISODate);
  return obj;
}

// Comment 反序列化。
Comment Comment::fromJson(const QJsonObject &obj) {
  Comment comment;
  comment.id = obj.value("id").toString();
  comment.authorId = obj.value("authorId").toString();
  comment.content = obj.value("content").toString();
  comment.createdAt =
      QDateTime::fromString(obj.value("createdAt").toString(), Qt::ISODate);
  return comment;
}

// SocialPost 序列化，附带评论列表。
QJsonObject SocialPost::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["authorId"] = authorId;
  obj["content"] = content;
  obj["visibility"] = visibility;
  obj["createdAt"] = createdAt.toString(Qt::ISODate);
  QJsonArray commentsArray;
  for (const auto &comment : comments) {
    commentsArray.append(comment.toJson());
  }
  obj["comments"] = commentsArray;
  return obj;
}

// SocialPost 反序列化，逐条恢复评论。
SocialPost SocialPost::fromJson(const QJsonObject &obj) {
  SocialPost post;
  post.id = obj.value("id").toString();
  post.authorId = obj.value("authorId").toString();
  post.content = obj.value("content").toString();
  post.visibility = obj.value("visibility").toString("public");
  post.createdAt =
      QDateTime::fromString(obj.value("createdAt").toString(), Qt::ISODate);
  const auto commentsArray = obj.value("comments").toArray();
  for (const auto &value : commentsArray) {
    post.comments.push_back(Comment::fromJson(value.toObject()));
  }
  return post;
}

// UserProfile 序列化。
QJsonObject UserProfile::toJson() const {
  QJsonObject obj;
  obj["id"] = id;
  obj["username"] = username;
  obj["email"] = email;
  obj["passwordHash"] = passwordHash;
  obj["notificationsEnabled"] = notificationsEnabled;
  obj["privacyLevel"] = privacyLevel;
  obj["friendIds"] = QJsonArray::fromStringList(friendIds);
  return obj;
}

// UserProfile 反序列化。
UserProfile UserProfile::fromJson(const QJsonObject &obj) {
  UserProfile profile;
  profile.id = obj.value("id").toString();
  profile.username = obj.value("username").toString();
  profile.email = obj.value("email").toString();
  profile.passwordHash = obj.value("passwordHash").toString();
  profile.notificationsEnabled = obj.value("notificationsEnabled").toBool(true);
  profile.privacyLevel = obj.value("privacyLevel").toString("friends");
  const auto friendArray = obj.value("friendIds").toArray();
  for (const auto &value : friendArray) {
    profile.friendIds.append(value.toString());
  }
  return profile;
}

}  // namespace core
