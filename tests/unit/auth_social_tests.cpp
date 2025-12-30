#include <gtest/gtest.h>

#include <QDir>
#include <QUuid>

#include <algorithm>

#include "core/Entities.h"
#include "core/LedgerService.h"

using namespace core;

/* 测试用户认证和社交相关功能 共7个测试样例 */

// 用例：未注册用户或密码错误返回空结果并给出错误提示。
TEST(AuthSocialTests, AuthenticateFailsForUnknownOrWrongPassword) {
  const QString envPath = QDir::tempPath() + "/bk_auth_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  // 注册一个测试用户
  ASSERT_TRUE(service.registerUser("alice", "alice@example.com", "pw", userId, err)) << err.toStdString();

  // 未注册的用户名
  EXPECT_FALSE(service.authenticate("bob", "pw", err).has_value());
  EXPECT_EQ(err, "用户不存在");

  // 密码错误
  EXPECT_FALSE(service.authenticate("alice", "wrong", err).has_value());
  EXPECT_EQ(err, "密码错误");

  QDir(envPath).removeRecursively();
}

// 用例：正确用户名/密码返回用户档案，校验 ID 一致性。
TEST(AuthSocialTests, AuthenticateSuccessReturnsProfile) {
  const QString envPath = QDir::tempPath() + "/bk_auth_ok_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  // 注册一个测试用户
  ASSERT_TRUE(service.registerUser("okuser", "ok@example.com", "pw", userId, err)) << err.toStdString();

  // 使用正确密码登陆
  const auto profile = service.authenticate("okuser", "pw", err);
  // 校验ID一致性
  ASSERT_TRUE(profile.has_value());
  EXPECT_EQ(profile->id, userId);

  QDir(envPath).removeRecursively();
}

// 用例：addFriend 对双方互相写入 friendIds，实现双向好友。
TEST(AuthSocialTests, FriendAddMutual) {
  const QString envPath = QDir::tempPath() + "/bk_friend_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString aliceId;
  QString bobId;
  QString err;
  // 注册两个用户
  ASSERT_TRUE(service.registerUser("alice", "alice@example.com", "pw", aliceId, err)) << err.toStdString();
  ASSERT_TRUE(service.registerUser("bob", "bob@example.com", "pw", bobId, err)) << err.toStdString();

  // 添加好友
  ASSERT_TRUE(service.addFriend(aliceId, "bob", err)) << err.toStdString();

  // 两个人的都应该有对方的 ID
  const auto aliceProfile = service.profile(aliceId);
  const auto bobProfile = service.profile(bobId);
  ASSERT_TRUE(aliceProfile.has_value());
  ASSERT_TRUE(bobProfile.has_value());
  EXPECT_TRUE(aliceProfile->friendIds.contains(bobId));
  EXPECT_TRUE(bobProfile->friendIds.contains(aliceId));

  QDir(envPath).removeRecursively();
}

// 用例：添加自己或不存在的用户均失败，并返回对应错误文案。
TEST(AuthSocialTests, AddFriendRejectsSelfOrMissingUser) {
  const QString envPath = QDir::tempPath() + "/bk_friend_err_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString aliceId;
  QString err;
  ASSERT_TRUE(service.registerUser("alice2", "alice2@example.com", "pw", aliceId, err)) << err.toStdString();

  EXPECT_FALSE(service.addFriend(aliceId, "alice2", err));
  EXPECT_EQ(err, "不能添加自己为好友");

  EXPECT_FALSE(service.addFriend(aliceId, "ghost", err));
  EXPECT_EQ(err, "未找到对应用户");

  QDir(envPath).removeRecursively();
}

// 用例：公开/好友可见的动态分别对好友/陌生人可见，且好友可成功评论写回。
TEST(AuthSocialTests, PublishTimelineVisibilityAndComment) {
  const QString envPath = QDir::tempPath() + "/bk_social_ok_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString aliceId;
  QString bobId;
  QString charlieId;
  QString err;
  // 三个用户A B C
  ASSERT_TRUE(service.registerUser("alice3", "alice3@example.com", "pw", aliceId, err)) << err.toStdString();
  ASSERT_TRUE(service.registerUser("bob3", "bob3@example.com", "pw", bobId, err)) << err.toStdString();
  ASSERT_TRUE(service.registerUser("charlie3", "charlie3@example.com", "pw", charlieId, err)) << err.toStdString();

  // A和B是好友
  ASSERT_TRUE(service.addFriend(aliceId, "bob3", err)) << err.toStdString();

  // A发表一条公开动态和一条好友可见动态
  ASSERT_TRUE(service.publishPost(aliceId, "公开动态", "public", err)) << err.toStdString();
  ASSERT_TRUE(service.publishPost(aliceId, "好友可见", "friends", err)) << err.toStdString();

  // B获取时间线应能看到两条动态
  const auto bobTimeline = service.timeline(bobId);
  EXPECT_EQ(bobTimeline.size(), 2);

  // C获取时间线应只能一条动态 并且是公开的
  const auto charlieTimeline = service.timeline(charlieId);
  EXPECT_EQ(charlieTimeline.size(), 1);
  EXPECT_EQ(charlieTimeline.first().visibility, "public");

  // 验证 B 能对 A 的动态发表评论
  const auto aliceTimeline = service.timeline(aliceId);
  ASSERT_FALSE(aliceTimeline.isEmpty());
  const auto postId = aliceTimeline.first().id;
  ASSERT_TRUE(service.addComment(bobId, aliceId, postId, "赞", err)) << err.toStdString();

  const auto afterComment = service.timeline(aliceId);
  const auto found = std::find_if(afterComment.begin(), afterComment.end(), [&](const SocialPost &p) {
    return p.id == postId;
  });
  ASSERT_NE(found, afterComment.end());
  ASSERT_EQ(found->comments.size(), 1);
  EXPECT_EQ(found->comments.first().content, "赞");

  QDir(envPath).removeRecursively();
}

// 用例：空内容发帖失败、评论不存在动态失败，返回对应错误。
TEST(AuthSocialTests, SocialOperationsFailForInvalidInput) {
  const QString envPath = QDir::tempPath() + "/bk_social_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("poster", "poster@example.com", "pw", userId, err)) << err.toStdString();

  // 发表空内容应失败
  EXPECT_FALSE(service.publishPost(userId, "   ", "public", err));
  EXPECT_EQ(err, "内容不能为空");

  // 评论不存在的动态应失败
  EXPECT_FALSE(service.addComment(userId, userId, "missing-post", "hi", err));
  EXPECT_EQ(err, "未找到动态");

  QDir(envPath).removeRecursively();
}

// 用例：updateSettings 持久化后，通过 profile 读回能看到更新的通知与隐私设定。
TEST(AuthSocialTests, UpdateSettingsAndProfileReadback) {
  const QString envPath = QDir::tempPath() + "/bk_settings_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString userId;
  QString err;
  ASSERT_TRUE(service.registerUser("settings", "settings@example.com", "pw", userId, err)) << err.toStdString();

  // 更新设置 注意这里我们更新的值与默认值不同
  ASSERT_TRUE(service.updateSettings(userId, false, "private", err)) << err.toStdString();

  const auto profile = service.profile(userId);
  ASSERT_TRUE(profile.has_value());
  EXPECT_FALSE(profile->notificationsEnabled);
  EXPECT_EQ(profile->privacyLevel, "private");

  QDir(envPath).removeRecursively();
}