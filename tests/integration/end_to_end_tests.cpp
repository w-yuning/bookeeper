#include <gtest/gtest.h>

#include <QDir>
#include <QUuid>

#include "core/LedgerService.h"

using namespace core;

// 集成用例2：注册两人，建立好友关系，查看好友动态并发表评论，验证评论被写回。
TEST(IntegrationFlow, FriendFeedAndComment) {
  const QString envPath = QDir::tempPath() + "/bk_it_friend_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  qputenv("BOOKEEPER_DATA_DIR", envPath.toUtf8());
  QDir(envPath).removeRecursively();

  LedgerService service;
  QString aliceId;
  QString bobId;
  QString err;

  ASSERT_TRUE(service.registerUser("alice_friend", "alice_friend@example.com", "pw", aliceId, err)) << err.toStdString();
  ASSERT_TRUE(service.registerUser("bob_friend", "bob_friend@example.com", "pw", bobId, err)) << err.toStdString();

  // 建立好友关系（双向由 addFriend 完成）。
  ASSERT_TRUE(service.addFriend(aliceId, "bob_friend", err)) << err.toStdString();

  // Alice 发布仅好友可见动态。
  ASSERT_TRUE(service.publishPost(aliceId, "hello from alice", "friends", err)) << err.toStdString();

  // Bob 能看到好友动态。
  const auto bobTimeline = service.timeline(bobId);
  ASSERT_EQ(bobTimeline.size(), 1);
  const auto postId = bobTimeline.first().id;
  EXPECT_EQ(bobTimeline.first().content, "hello from alice");

  // Bob 给好友评论，评论写回 Alice 的数据。
  ASSERT_TRUE(service.addComment(bobId, aliceId, postId, "nice post", err)) << err.toStdString();

  // 模拟 Alice 再次登录后查看自己的时间线，评论应已持久化。
  {
    LedgerService reload;
    auto profile = reload.authenticate("alice_friend", "pw", err);
    ASSERT_TRUE(profile.has_value()) << err.toStdString();

    const auto aliceTimeline = reload.timeline(aliceId);
    ASSERT_EQ(aliceTimeline.size(), 1);
    ASSERT_EQ(aliceTimeline.first().comments.size(), 1);
    EXPECT_EQ(aliceTimeline.first().comments.first().content, "nice post");
  }

  QDir(envPath).removeRecursively();
}