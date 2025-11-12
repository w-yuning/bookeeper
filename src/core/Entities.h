// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace core {

// 领域模型的基础数据结构，仅包含数值字段与序列化接口，便于在核心逻辑与存储之间复用。

enum class BillType { Income, Expense };

// Category 描述用户自定义的账目分类信息。
struct Category {
  QString id;
  QString name;
  QString type;  // "income" or "expense"

  QJsonObject toJson() const;
  static Category fromJson(const QJsonObject &obj);
};

// Bill 表示一次收支记录，包含金额、分类、类型等信息。
struct Bill {
  QString id;
  double amount = 0.0;
  QString categoryId;
  QString note;
  QDateTime timestamp;
  BillType type = BillType::Expense;

  QJsonObject toJson() const;
  static Bill fromJson(const QJsonObject &obj);
};

// Reminder 保存用户自定义提醒的文本与触发时间。
struct Reminder {
  QString id;
  QString message;
  QDateTime remindAt;
  bool enabled = true;

  QJsonObject toJson() const;
  static Reminder fromJson(const QJsonObject &obj);
};

// Comment 用于社交动态下的评论内容。
struct Comment {
  QString id;
  QString authorId;
  QString content;
  QDateTime createdAt;

  QJsonObject toJson() const;
  static Comment fromJson(const QJsonObject &obj);
};

// SocialPost 表示一条用户动态，可附带评论列表。
struct SocialPost {
  QString id;
  QString authorId;
  QString content;
  QString visibility;  // "public" or "friends"
  QDateTime createdAt;
  QVector<Comment> comments;

  QJsonObject toJson() const;
  static SocialPost fromJson(const QJsonObject &obj);
};

// UserProfile 存储用户的基本资料与社交关系。
struct UserProfile {
  QString id;
  QString username;
  QString email;
  QString passwordHash;
  bool notificationsEnabled = true;
  QString privacyLevel = "friends";
  QStringList friendIds;

  QJsonObject toJson() const;
  static UserProfile fromJson(const QJsonObject &obj);
};

// UserData 汇总单个用户的所有业务数据片段。
struct UserData {
  UserProfile profile;
  QVector<Category> categories;
  QVector<Bill> bills;
  QVector<Reminder> reminders;
  QVector<SocialPost> posts;
};

}  // namespace core
