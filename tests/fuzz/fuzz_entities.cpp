// 简单 libFuzzer 入口：对 JSON 解析与实体反序列化做健壮性探测。
// 将输入尝试解析为 JSON 对象，分别喂给各 fromJson 函数，观察是否触发崩溃。

#include <cstdint>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/Entities.h"

using namespace core;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const QByteArray input(reinterpret_cast<const char *>(data), static_cast<int>(size));
  const auto doc = QJsonDocument::fromJson(input);
  if (!doc.isObject()) {
    return 0;
  }
  const QJsonObject obj = doc.object();

  // 逐个实体尝试反序列化，关注潜在崩溃而非语义正确性。
  (void)Category::fromJson(obj);
  (void)Bill::fromJson(obj);
  (void)Reminder::fromJson(obj);
  (void)Comment::fromJson(obj);
  (void)SocialPost::fromJson(obj);
  (void)UserProfile::fromJson(obj);
  return 0;
}
