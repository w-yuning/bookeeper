// Copyright (c) 2025 Yuning Wang. All rights reserved.
//
// Bookkeeper - Personal Finance Management System
// Software Engineering Lab 3, Nanjing University
// Student ID: 231220063

#pragma once

#include "Entities.h"

#include <QDir>
#include <QReadWriteLock>

namespace core {

// JsonStorage 将每个用户的数据写入独立 JSON 文件，并提供线程安全封装。
class JsonStorage {
 public:
  explicit JsonStorage(const QDir &baseDir = defaultDataDir());

  // 返回默认的数据目录，位于 AppData 下的 bookeeper_data。
  static QDir defaultDataDir();

  // 将用户完整数据写入文件。
  bool saveUser(const UserData &data) const;
  // 从文件中读取指定用户数据。
  bool loadUser(const QString &userId, UserData &outData) const;
  // 枚举所有用户的基础档案。
  QVector<UserProfile> listUsers() const;
  // 删除指定用户的持久化文件。
  bool removeUser(const QString &userId) const;

 private:
  // 根据用户 ID 拼接数据文件路径。
  QString userFilePath(const QString &userId) const;
  // 辅助函数：将数据结构编码为 JSON。
  static QJsonObject serialize(const UserData &data);
  // 辅助函数：从 JSON 解码为数据结构。
  static UserData deserialize(const QJsonObject &obj);

  QDir dataDir_;
  mutable QReadWriteLock lock_;
};

}  // namespace core
