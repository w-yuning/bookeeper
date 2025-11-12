// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#include "JsonStorage.h"

#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUuid>

namespace core {

static const QString kDataFolderName = "bookeeper_data";

// 构造函数会在需要时自动创建数据目录。
JsonStorage::JsonStorage(const QDir &baseDir) : dataDir_(baseDir) {
  if (!dataDir_.exists()) {
    dataDir_.mkpath(".");
  }
}

// AppDataLocation 可能为空，必要时回退到用户主目录。
QDir JsonStorage::defaultDataDir() {
  const auto locations =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(locations.isEmpty() ? QDir::homePath() : locations);
  dir.mkpath(kDataFolderName);
  dir.cd(kDataFolderName);
  return dir;
}

// 写入用户数据时采用写锁，防止并发写入冲突。
bool JsonStorage::saveUser(const UserData &data) const {
  QWriteLocker locker(&lock_);
  QFile file(userFilePath(data.profile.id));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  const auto json = QJsonDocument(serialize(data)).toJson();
  if (file.write(json) != json.size()) {
    return false;
  }
  file.flush();
  return true;
}

// 读取用户数据使用读锁，提高并发读取能力。
bool JsonStorage::loadUser(const QString &userId, UserData &outData) const {
  QReadLocker locker(&lock_);
  QFile file(userFilePath(userId));
  if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
    return false;
  }
  const auto doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject()) {
    return false;
  }
  outData = deserialize(doc.object());
  return true;
}

// 遍历目录下所有 JSON 文件，抽取用户档案列表。
QVector<UserProfile> JsonStorage::listUsers() const {
  QReadLocker locker(&lock_);
  QVector<UserProfile> profiles;
  const auto entries = dataDir_.entryList({"*.json"}, QDir::Files);
  for (const auto &entry : entries) {
    QFile file(dataDir_.filePath(entry));
    if (!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
      continue;
    }
    profiles.push_back(
        UserProfile::fromJson(doc.object().value("profile").toObject()));
  }
  return profiles;
}

// 删除用户文件时使用写锁保证互斥。
bool JsonStorage::removeUser(const QString &userId) const {
  QWriteLocker locker(&lock_);
  return QFile::remove(userFilePath(userId));
}

QString JsonStorage::userFilePath(const QString &userId) const {
  return dataDir_.filePath(userId + ".json");
}

// 按模块序列化所有业务数据。
QJsonObject JsonStorage::serialize(const UserData &data) {
  QJsonObject obj;
  obj["profile"] = data.profile.toJson();

  QJsonArray categories;
  for (const auto &category : data.categories) {
    categories.append(category.toJson());
  }
  obj["categories"] = categories;

  QJsonArray bills;
  for (const auto &bill : data.bills) {
    bills.append(bill.toJson());
  }
  obj["bills"] = bills;

  QJsonArray reminders;
  for (const auto &reminder : data.reminders) {
    reminders.append(reminder.toJson());
  }
  obj["reminders"] = reminders;

  QJsonArray posts;
  for (const auto &post : data.posts) {
    posts.append(post.toJson());
  }
  obj["posts"] = posts;

  return obj;
}

// 从 JSON 逐段恢复业务数据。
UserData JsonStorage::deserialize(const QJsonObject &obj) {
  UserData data;
  data.profile = UserProfile::fromJson(obj.value("profile").toObject());

  const auto categories = obj.value("categories").toArray();
  for (const auto &value : categories) {
    data.categories.push_back(Category::fromJson(value.toObject()));
  }

  const auto bills = obj.value("bills").toArray();
  for (const auto &value : bills) {
    data.bills.push_back(Bill::fromJson(value.toObject()));
  }

  const auto reminders = obj.value("reminders").toArray();
  for (const auto &value : reminders) {
    data.reminders.push_back(Reminder::fromJson(value.toObject()));
  }

  const auto posts = obj.value("posts").toArray();
  for (const auto &value : posts) {
    data.posts.push_back(SocialPost::fromJson(value.toObject()));
  }

  return data;
}

}  // namespace core
