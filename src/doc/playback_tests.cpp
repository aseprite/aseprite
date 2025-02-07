// Aseprite Document Library
// Copyright (c) 2021-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/playback.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/tags.h"

#include <iostream>
#include <memory>

#define PLAY_TRACE(...) // TRACEARGS

using namespace doc;

namespace std {

std::ostream& operator<<(std::ostream& os, const std::vector<doc::frame_t>& frames)
{
  os << "{ ";
  for (int i = 0; i < int(frames.size()); ++i)
    os << "[" << i << "]=" << frames[i] << (i < frames.size() - 1 ? ", " : " ");
  os << "} ";
  return os;
}

} // namespace std

static std::unique_ptr<Sprite> make_sprite(frame_t nframes, std::vector<Tag*> tags = {})
{
  std::unique_ptr<Sprite> sprite(Sprite::MakeStdSprite(ImageSpec(ColorMode::RGB, 4, 4)));
  sprite->setTotalFrames(nframes);
  for (auto tag : tags)
    sprite->tags().add(tag);
  return sprite;
}

static Tag* make_tag(const char* name, frame_t from, frame_t to, AniDir aniDir, int repeat = 0)
{
  Tag* tag = new Tag(from, to);
  tag->setName(name);
  tag->setAniDir(aniDir);
  tag->setRepeat(repeat);
  return tag;
}

static void expect_frames(Playback& play,
                          const std::vector<frame_t>& expected,
                          frame_t frameDelta = frame_t(+1))
{
  std::vector<frame_t> result;
  result.push_back(play.frame());
  for (int i = 1; i < expected.size(); ++i) {
    PLAY_TRACE("[", i, "]");
    result.push_back(play.nextFrame(frameDelta));
  }

  for (int i = 0; i < expected.size(); ++i) {
    ASSERT_EQ(expected[i], result[i]) << "[ " << i << " ]"
                                      << "\n  expected=" << expected << "\n  result  =" << result;
  }
}

TEST(Playback, OnceFullSprite)
{
  auto sprite = make_sprite(5);
  Playback play(sprite.get(), 2, Playback::Mode::PlayOnce);
  expect_frames(play, { 0, 1, 2, 3, 4, 2, 2, 2, 2, 2 });
  EXPECT_TRUE(play.isStopped());
}

TEST(Playback, OnceTag)
{
  //     A
  //   ---->
  // 0 1 2 3 4

  Tag* a = make_tag("A", 1, 3, AniDir::FORWARD);
  auto sprite = make_sprite(5, { a });
  Playback play(sprite.get(), 2, Playback::Mode::PlayOnce, a);
  expect_frames(play, { 1, 2, 3, 2, 2, 2, 2, 2 });
  EXPECT_TRUE(play.isStopped());

  a->setAniDir(AniDir::REVERSE);
  play = Playback(sprite.get(), 2, Playback::Mode::PlayOnce, a);
  expect_frames(play, { 3, 2, 1, 2, 2, 2, 2, 2 });

  a->setAniDir(AniDir::PING_PONG);
  play = Playback(sprite.get(), 0, Playback::Mode::PlayOnce, a);
  expect_frames(play, { 1, 2, 3, 2, 1, 0, 0, 0, 0 });

  a->setAniDir(AniDir::PING_PONG_REVERSE);
  play = Playback(sprite.get(), 0, Playback::Mode::PlayOnce, a);
  expect_frames(play, { 3, 2, 1, 2, 3, 0, 0, 0, 0 });

  // Just check playing the full sprite when there is a tag (the tag must be ignored)
  play = Playback(sprite.get(), 2, Playback::Mode::PlayOnce);
  expect_frames(play, { 0, 1, 2, 3, 4, 2, 2, 2, 2 });
  EXPECT_TRUE(play.isStopped());

  play = Playback(sprite.get(), 2, Playback::Mode::PlayWithoutTagsInLoop);
  expect_frames(play, { 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, LoopSprite)
{
  auto sprite = make_sprite(4);
  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, LoopSpriteStartFromFrame2)
{
  auto sprite = make_sprite(4);
  Playback play(sprite.get(), 2, Playback::Mode::PlayInLoop);
  expect_frames(play, { 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, LoopSpriteReverse)
{
  auto sprite = make_sprite(4);
  Playback play(sprite.get(), 2, Playback::Mode::PlayInLoop);
  expect_frames(play, { 2, 1, 0, 3, 2, 1, 0, 3 }, -1);
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, WithTagRepetitions)
{
  Tag* a = make_tag("A", 1, 2, AniDir::FORWARD, 2);
  auto sprite = make_sprite(4, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 1, 2, 3, 0, 1, 2, 1, 2, 3, 0 });
  EXPECT_FALSE(play.isStopped());

  play = Playback(sprite.get(), 0, Playback::Mode::PlayAll);
  expect_frames(play, { 0, 1, 2, 1, 2, 3, 0, 0, 0 });
  EXPECT_TRUE(play.isStopped());

  Tag* b = make_tag("B", 0, 3, AniDir::PING_PONG, 2);
  sprite = make_sprite(4, { b });
  play = Playback(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 2, 1, 0 });
  EXPECT_FALSE(play.isStopped());

  Tag* c = make_tag("C", 0, 3, AniDir::PING_PONG_REVERSE, 2);
  sprite = make_sprite(4, { c });
  play = Playback(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 3, 2, 1, 0, 1, 2, 3, 3, 2, 1, 0, 1, 2, 3 });
  EXPECT_FALSE(play.isStopped());

  Tag* d = make_tag("D", 0, 3, AniDir::PING_PONG_REVERSE, 2);
  sprite = make_sprite(4, { d });
  play = Playback(sprite.get(), 1, Playback::Mode::PlayInLoop);
  expect_frames(play, { 1, 0, 1, 2, 3, 3, 2, 1, 0, 1, 2, 3, 3, 2, 1, 0, 1, 2, 3 });
  EXPECT_FALSE(play.isStopped());

  Tag* e = make_tag("E", 0, 3, AniDir::PING_PONG_REVERSE, 1);
  sprite = make_sprite(4, { e });
  play = Playback(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0 });
  EXPECT_FALSE(play.isStopped());

  Tag* f = make_tag("F", 0, 3, AniDir::REVERSE, 2);
  sprite = make_sprite(4, { f });
  play = Playback(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, LoopTagInfinite)
{
  //    A
  //   -->
  // 0 1 2 4

  Tag* a = make_tag("A", 1, 2, AniDir::FORWARD, 0);
  auto sprite = make_sprite(4, { a });
  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop, a);
  expect_frames(play, { 0, 1, 2, 1, 2, 1, 2, 1, 2 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, LoopInfiniteReverse)
{
  //    A
  //   -->
  // 0 1 2 4

  Tag* a = make_tag("A", 1, 2, AniDir::FORWARD, 0);
  auto sprite = make_sprite(4, { a });
  Playback play(sprite.get(),
                TagsList(), // Ignore tags
                0,
                Playback::Mode::PlayInLoop,
                nullptr);
  expect_frames(play, { 0, 3, 2, 1, 0, 3, 2, 1, 0 }, -1);
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, LoopTagFinite)
{
  //    A
  //   -->
  // 0 1 2 3

  Tag* a = make_tag("A", 1, 2, AniDir::FORWARD, 2);
  auto sprite = make_sprite(4, { a });
  Playback play(sprite.get(), 2, Playback::Mode::PlayInLoop, a);
  expect_frames(play, { 2, 1, 2, 1, 2, 1, 2 });
  EXPECT_FALSE(play.isStopped());

  // This is not infinite because the tag is not specified in the
  // Playback() ctor.
  play = Playback(sprite.get(), 2, Playback::Mode::PlayInLoop);
  expect_frames(play, { 2, 1, 2, 3, 0, 1, 2, 1, 2, 3, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, SimpleForward)
{
  //  A
  // -->
  // 0 1

  Tag* a = make_tag("A", 0, 1, AniDir::FORWARD, 2);
  auto sprite = make_sprite(2, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, SimpleLoopBug)
{
  //   Loop
  //   -->
  // 0 1 2 3

  Tag* loop = make_tag("Loop", 1, 2, AniDir::FORWARD, 0);
  auto sprite = make_sprite(4, { loop });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 0, 1, 2, 3, 0 });
  EXPECT_FALSE(play.isStopped());

  play = Playback(sprite.get(), 0, Playback::Mode::PlayInLoop, loop);
  expect_frames(play, { 0, 1, 2, 1, 2, 1, 2, 1, 2 });
  EXPECT_FALSE(play.isStopped());

  // Here we detected a bug where the playback kept playing 3,4,5,6,etc.
  play = Playback(sprite.get(), 3, Playback::Mode::PlayInLoop, loop);
  expect_frames(play, { 3, 0, 1, 2, 1, 2, 1, 2, 1, 2 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoSimpleForwards)
{
  //  A
  // -->
  //  B
  // -->
  // 0 1

  Tag* a = make_tag("A", 0, 1, AniDir::FORWARD, 2);
  Tag* b = make_tag("B", 0, 1, AniDir::FORWARD, 2);
  auto sprite = make_sprite(2, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, SimplePingPong2)
{
  //  A
  // <->
  // 0 1

  Tag* a = make_tag("A", 0, 1, AniDir::PING_PONG, 2);
  auto sprite = make_sprite(2, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, SimplePingPong3)
{
  //  A
  // <->
  // 0 1

  Tag* a = make_tag("A", 0, 1, AniDir::PING_PONG, 3);
  auto sprite = make_sprite(2, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, SimplePingPong4)
{
  // A
  // <>
  // 0

  Tag* a = make_tag("A", 0, 0, AniDir::PING_PONG, 1);
  auto sprite = make_sprite(1, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayAll);
  expect_frames(play, { 0, 0 });
  EXPECT_TRUE(play.isStopped());
}

TEST(Playback, SimplePingPong5)
{
  // A
  // <>
  // 0

  Tag* a = make_tag("A", 0, 0, AniDir::PING_PONG, 3);
  auto sprite = make_sprite(1, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayAll);
  expect_frames(play, { 0, 0, 0, 0 });
  EXPECT_TRUE(play.isStopped());
}

TEST(Playback, SimplePingPong6)
{
  // A
  // <>
  // 0

  Tag* a = make_tag("A", 0, 0, AniDir::PING_PONG, 0);
  auto sprite = make_sprite(1, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayAll);
  expect_frames(play, { 0, 0, 0 });
  EXPECT_TRUE(play.isStopped());
}

TEST(Playback, SimplePingPong3Repeats)
{
  //   A
  // <--->
  // 0 1 2

  Tag* a = make_tag("A", 0, 2, AniDir::PING_PONG, 3);
  auto sprite = make_sprite(3, { a });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 1, 0, 1, 2, 0, 1, 2, 1, 0, 1, 2 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TagOneFrame)
{
  // A
  // ->
  // 0  1

  Tag* tagA = make_tag("A", 0, 0, AniDir::FORWARD, 2);
  auto sprite = make_sprite(2, { tagA });

  Playback play(sprite.get(), 1, Playback::Mode::PlayInLoop);
  expect_frames(play, { 1, 0, 0, 1, 0, 0, 1, 0, 0, 1 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, FourTags)
{
  //    A    B     C        D
  //   --> <---- <--->   >------<
  // 0 1 2 3 4 5 6 7 8 9 10 11 12 13

  Tag* a = make_tag("A", 1, 2, AniDir::FORWARD, 1);
  Tag* b = make_tag("B", 3, 5, AniDir::REVERSE, 2);
  Tag* c = make_tag("C", 6, 8, AniDir::PING_PONG, 3);
  Tag* d = make_tag("D", 10, 12, AniDir::PING_PONG_REVERSE, 2);
  auto sprite = make_sprite(14, { a, b, c, d });

  Playback play(sprite.get(), 0, Playback::Mode::PlayWithoutTagsInLoop);
  expect_frames(play, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 0,
                        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0 });
  EXPECT_FALSE(play.isStopped());

  play = Playback(sprite.get(), 0, Playback::Mode::PlayAll);
  expect_frames(play, { 0, 1, 2, 5,  4,  3,  5,  4,  3,  6, 7, 8, 7, 6,
                        7, 8, 9, 12, 11, 10, 11, 12, 13, 0, 0, 0, 0 });
  EXPECT_TRUE(play.isStopped());
}

TEST(Playback, ForwardTagWithInnerPingPong)
{
  //    A
  //   -------->
  //       B
  //     <--->
  // 0 1 2 3 4 5 6

  Tag* tagA = make_tag("A", 1, 5, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 2, 4, AniDir::PING_PONG, 3);
  auto sprite = make_sprite(7, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 4, 3, 2, 3, 4, 5, 1, 2, 3, 4, 3, 2, 3, 4, 5, 6,
                        0, 1, 2, 3, 4, 3, 2, 3, 4, 5, 1, 2, 3, 4, 3, 2, 3, 4, 5, 6 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, ForwardTagWithInnerForwardEndSameFrame)
{
  //    A
  //   ------>
  //       B
  //     ---->
  // 0 1 2 3 4

  Tag* tagA = make_tag("A", 1, 4, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 2, 4, AniDir::FORWARD, 2);
  auto sprite = make_sprite(5, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 4, 2, 3, 4, 1, 2, 3, 4, 2, 3, 4, 0,
                        1, 2, 3, 4, 2, 3, 4, 1, 2, 3, 4, 2, 3, 4, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, ForwardTagWithInnerPingPongEndSameFrame)
{
  //    A
  //   ---->
  //      B
  //   <--->
  // 0 1 2 3

  Tag* tagA = make_tag("A", 1, 3, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 1, 3, AniDir::PING_PONG, 4);
  auto sprite = make_sprite(4, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 2, 1, 2, 3, 2, 1, 1, 2, 3, 2, 1, 2, 3, 2, 1, 0,
                        1, 2, 3, 2, 1, 2, 3, 2, 1, 1, 2, 3, 2, 1, 2, 3, 2, 1, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, ForwardTagWithInnerReverse)
{
  //    A
  //   ------>
  //      B
  //   <----
  // 0 1 2 3 4

  Tag* tagA = make_tag("A", 1, 4, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 1, 3, AniDir::REVERSE, 2);
  auto sprite = make_sprite(5, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 2, 1, 3, 2, 1, 4, 3, 2, 1, 3, 2, 1, 4, 0,
                        3, 2, 1, 3, 2, 1, 4, 3, 2, 1, 3, 2, 1, 4, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, PingPongWithInnerReverse)
{
  //     A
  // <------->
  //     B
  //   <----
  // 0 1 2 3 4

  Tag* tagA = make_tag("A", 0, 4, AniDir::PING_PONG, 2);
  Tag* tagB = make_tag("B", 1, 3, AniDir::REVERSE, 3);
  auto sprite = make_sprite(5, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 2, 1, 3, 2, 1, 3, 2, 1, 4, 1, 2, 3, 1, 2, 3, 1, 2, 3, 0 });
  EXPECT_FALSE(play.isStopped());
}

// OnePingPongInsideOther series
static std::vector<int> goRight(const int a, const int b)
{
  std::vector<int> out;
  if (a > b)
    return out;
  for (int i = a; i <= b; ++i)
    out.push_back(i);
  return out;
}

static std::vector<int> goLeft(const int a, const int b)
{
  std::vector<int> out;
  if (a > b)
    return out;
  for (int i = b; i >= a; --i)
    out.push_back(i);
  return out;
}

static void concat(std::vector<int>& a, const std::vector<int>& b)
{
  for (size_t i = 0; i < b.size(); ++i)
    a.push_back(b[i]);
}

TEST(Playback, OnePingPongInsideOther)
{
  // A repeat = 2  ;  B repeat = 2
  //
  //     A            A            A
  // *-------*    *-------*    *-------*
  //   B              B              B
  // *---*          *---*          *---*
  // 0 1 2 3 4    0 1 2 3 4    0 1 2 3 4
  const int lastFrame = 4;
  std::vector<AniDir> A_AniDirs = { AniDir::PING_PONG, AniDir::PING_PONG_REVERSE };
  std::vector<AniDir> B_AniDirs = { AniDir::PING_PONG, AniDir::PING_PONG_REVERSE };
  std::vector<int> A_Range = { 0, lastFrame };
  std::vector<std::vector<int>> rangeBs = {
    { 0, 2 },
    { 1, 3 },
    { 2, 4 }
  };
  std::vector<std::vector<int>> pingPongSeq1 = {
    { 0, 1, 2, 1, 0 },
    { 2, 1, 0, 1, 2 }
  };
  std::vector<std::vector<int>> pingPongSeq2 = {
    { 1, 2, 3, 2, 1 },
    { 3, 2, 1, 2, 3 }
  };
  std::vector<std::vector<int>> pingPongSeq3 = {
    { 2, 3, 4, 3, 2 },
    { 4, 3, 2, 3, 4 }
  };
  std::vector<int> right012 = { 0, 1, 2 };

  for (auto A_aniDir : A_AniDirs) {
    for (auto B_aniDir : B_AniDirs) {
      for (auto B_Range : rangeBs) {
        std::vector<int> expected;
        std::vector<int> temp;
        //     A            A            A
        // <------->    <------->    <------->
        //   B              B              B
        // *---*          *---*          *---*
        // 0 1 2 3 4    0 1 2 3 4    0 1 2 3 4
        if (A_aniDir == doc::AniDir::PING_PONG) {
          // Start
          temp = goRight(0, B_Range[0] - 1);
          concat(expected, temp);

          // Tag B playback
          if (B_Range[0] == 0)
            concat(expected, B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[0] : right012);
          else if (B_Range[0] == 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[0] : pingPongSeq2[1]);
          else if (B_Range[0] == 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[0] : pingPongSeq3[1]);

          // Reproduce right side of the tag A
          temp = goRight(B_Range[1] + 1, lastFrame);
          concat(expected, temp);
          temp = goLeft(B_Range[1] + 1, lastFrame - 1);
          concat(expected, temp);

          // Tag B playback (only if tag B last frame doesn't match with the tag A last frame
          if (B_Range[1] != A_Range[1]) {
            if (B_Range[1] == lastFrame - 1)
              concat(expected,
                     B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[1] : pingPongSeq2[0]);
            else if (B_Range[1] == lastFrame - 2)
              concat(expected,
                     B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[1] : pingPongSeq1[0]);
          }

          // Reproduce right side of the tag A
          temp = goLeft(0, B_Range[0] - 1);
          concat(expected, temp);
          // Sequence end
        }
        //     A            A            A
        // >-------<    >-------<    >-------<
        //   B              B              B
        // *---*          *---*          *---*
        // 0 1 2 3 4    0 1 2 3 4    0 1 2 3 4
        else {
          // Start
          temp = goRight(0, B_Range[0] - 1);
          concat(expected, temp);

          // Tag B playback
          if (B_Range[0] == 0)
            concat(expected, B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[0] : right012);
          else if (B_Range[0] == 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[0] : pingPongSeq2[1]);
          else if (B_Range[0] == 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[0] : pingPongSeq3[1]);

          // Reproduce right side of the tag A
          temp = goRight(B_Range[1] + 1, lastFrame);
          concat(expected, temp);
          // Sequence end

          // New Start
          temp = goLeft(B_Range[1] + 1, lastFrame);
          concat(expected, temp);

          // Tag B playback
          if (B_Range[1] == lastFrame)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[1] : pingPongSeq3[0]);
          else if (B_Range[1] == lastFrame - 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[1] : pingPongSeq2[0]);
          else if (B_Range[1] == lastFrame - 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[1] : pingPongSeq1[0]);

          // Reproduce left side of the tag A
          temp = goLeft(0, B_Range[0] - 1);
          concat(expected, temp);
          temp = goRight(1, B_Range[0] - 1);
          concat(expected, temp);

          // Tag B playback (only if tag B first frame doesn't match with the tag A first frame
          if (B_Range[0] == 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[0] : pingPongSeq2[1]);
          else if (B_Range[0] == 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[0] : pingPongSeq3[1]);

          // Reproduce right side of the tag A
          temp = goRight(B_Range[1] + 1, lastFrame);
          concat(expected, temp);
          // Sequence end
        }

        // Test
        Tag* tagA = make_tag("A", 0, 4, A_aniDir, 2);
        Tag* tagB = make_tag("B", B_Range[0], B_Range[1], B_aniDir, 2);
        auto sprite = make_sprite(lastFrame + 1, { tagA, tagB });
        Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);

        expect_frames(play, expected);
        EXPECT_FALSE(play.isStopped());
      }
    }
  }
}

TEST(Playback, OnePingPongInsideOther1Repeat)
{
  // A repeat = 1  ;  B repeat = 1
  //
  //     A            A            A
  // *-------*    *-------*    *-------*
  //   B              B              B
  // *---*          *---*          *---*
  // 0 1 2 3 4    0 1 2 3 4    0 1 2 3 4

  const int lastFrame = 4;
  std::vector<AniDir> A_AniDirs = { AniDir::PING_PONG, AniDir::PING_PONG_REVERSE };
  std::vector<AniDir> B_AniDirs = { AniDir::PING_PONG, AniDir::PING_PONG_REVERSE };
  std::vector<int> A_Range = { 0, lastFrame };
  std::vector<std::vector<int>> rangeBs = {
    { 0, 2 },
    { 1, 3 },
    { 2, 4 }
  };
  std::vector<std::vector<int>> pingPongSeq1 = {
    { 0, 1, 2 },
    { 2, 1, 0 }
  };
  std::vector<std::vector<int>> pingPongSeq2 = {
    { 1, 2, 3 },
    { 3, 2, 1 }
  };
  std::vector<std::vector<int>> pingPongSeq3 = {
    { 2, 3, 4 },
    { 4, 3, 2 }
  };

  for (auto A_aniDir : A_AniDirs) {
    for (auto B_aniDir : B_AniDirs) {
      for (auto B_Range : rangeBs) {
        std::vector<int> expected;
        std::vector<int> temp;
        //     A            A            A
        // <------->    <------->    <------->
        //   B              B              B
        // *---*          *---*          *---*
        // 0 1 2 3 4    0 1 2 3 4    0 1 2 3 4
        if (A_aniDir == doc::AniDir::PING_PONG) {
          // Start
          temp = goRight(0, B_Range[0] - 1);
          concat(expected, temp);
          // Tag B playback
          if (B_Range[0] == 0) {
            temp = { 0 };
            concat(expected, B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[0] : temp);
          }
          else if (B_Range[0] == 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[0] : pingPongSeq2[1]);
          else if (B_Range[0] == 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[0] : pingPongSeq3[1]);
          // Reproduce right side of the tag A
          temp = goRight(B_Range[1] + 1, lastFrame);
          concat(expected, temp);
          // Sequence end

          // Fresh sequence start
          temp = goRight(0, B_Range[0] - 1);
          concat(expected, temp);
          // Tag B playback
          if (B_Range[0] == 0)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[0] : pingPongSeq1[1]);
          else if (B_Range[0] == 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[0] : pingPongSeq2[1]);
          else if (B_Range[0] == 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[0] : pingPongSeq3[1]);
          // Reproduce right side of the tag A
          temp = goRight(B_Range[1] + 1, lastFrame);
          concat(expected, temp);
          // Sequence end
        }
        //     A            A            A
        // >-------<    >-------<    >-------<
        //   B              B              B
        // *---*          *---*          *---*
        // 0 1 2 3 4    0 1 2 3 4    0 1 2 3 4
        else {
          // Start
          temp = { 0 };
          // Tag B playback
          if (B_Range[0] == 0 && B_aniDir == doc::AniDir::PING_PONG)
            concat(expected, pingPongSeq1[0]);
          else
            concat(expected, temp);
          // Sequence end

          // Fresh sequence start
          // Reproduce right side of the tag A
          temp = goLeft(B_Range[1] + 1, lastFrame);
          concat(expected, temp);

          // Tag B playback
          if (B_Range[1] == lastFrame)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq3[1] : pingPongSeq3[0]);
          else if (B_Range[1] == lastFrame - 1)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq2[1] : pingPongSeq2[0]);
          else if (B_Range[1] == lastFrame - 2)
            concat(expected,
                   B_aniDir == doc::AniDir::PING_PONG ? pingPongSeq1[1] : pingPongSeq1[0]);

          // Reproduce left side of the tag A
          temp = goLeft(0, B_Range[0] - 1);
          concat(expected, temp);
          // Sequence end
        }

        // Test
        Tag* tagA = make_tag("A", 0, 4, A_aniDir, 1);
        Tag* tagB = make_tag("B", B_Range[0], B_Range[1], B_aniDir, 1);
        auto sprite = make_sprite(lastFrame + 1, { tagA, tagB });
        Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);

        expect_frames(play, expected);
        EXPECT_FALSE(play.isStopped());
      }
    }
  }
}

TEST(Playback, OnePingPongInsideForward)
{
  //     A
  // -------->
  //       B
  //     <--->
  // 0 1 2 3 4

  Tag* tagA = make_tag("A", 0, 4, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 2, 4, AniDir::PING_PONG, 2);
  auto sprite = make_sprite(5, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 4, 3, 2, 0, 1, 2, 3, 4, 3, 2 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, OnePingPongInsideForward2)
{
  //      A
  //   -------->
  //         B
  //       <--->
  // 0 1 2 3 4 5

  Tag* tagA = make_tag("A", 1, 5, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 3, 5, AniDir::PING_PONG, 2);
  auto sprite = make_sprite(6, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 4, 5, 4, 3, 1, 2, 3, 4, 5, 4, 3,
                        0, 1, 2, 3, 4, 5, 4, 3, 1, 2, 3, 4, 5, 4, 3 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, OnePingPongInsidePingPongReverse)
{
  //      A
  //   >-------<
  //         B
  //       <--->
  // 0 1 2 3 4 5

  Tag* tagA = make_tag("A", 1, 5, AniDir::PING_PONG_REVERSE, 2);
  Tag* tagB = make_tag("B", 3, 5, AniDir::PING_PONG, 2);
  auto sprite = make_sprite(6, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 5, 4, 3, 4, 5, 2, 1, 2, 3, 4, 5, 4, 3,
                        0, 5, 4, 3, 4, 5, 2, 1, 2, 3, 4, 5, 4, 3 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, OneReverseInsidePingPongReverse)
{
  //      A
  //   >-------<
  //         B
  //       <----
  // 0 1 2 3 4 5

  Tag* tagA = make_tag("A", 1, 5, AniDir::PING_PONG_REVERSE, 2);
  Tag* tagB = make_tag("B", 3, 5, AniDir::REVERSE, 2);
  auto sprite = make_sprite(6, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 4, 5, 3, 4, 5, 2, 1, 2, 5, 4, 3, 5, 4, 3,
                        0, 3, 4, 5, 3, 4, 5, 2, 1, 2, 5, 4, 3, 5, 4, 3 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, OnePingPongReverseInsideReverse)
{
  //      A
  //   <--------
  //         B
  //       >---<
  // 0 1 2 3 4 5

  Tag* tagA = make_tag("A", 1, 5, AniDir::REVERSE, 2);
  Tag* tagB = make_tag("B", 3, 5, AniDir::PING_PONG_REVERSE, 2);
  auto sprite = make_sprite(6, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 4, 5, 4, 3, 2, 1, 3, 4, 5, 4, 3, 2, 1,
                        0, 3, 4, 5, 4, 3, 2, 1, 3, 4, 5, 4, 3, 2, 1 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascade)
{
  //    A
  //   ---->
  //       B
  //     ---->
  // 0 1 2 3 4

  Tag* tagA = make_tag("A", 1, 3, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 2, 4, AniDir::FORWARD, 2);
  auto sprite = make_sprite(5, { tagA, tagB });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 1, 2, 3, 2, 3, 4, 2, 3, 4, 0,
                        1, 2, 3, 1, 2, 3, 2, 3, 4, 2, 3, 4, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascadeReverse)
{
  //    A
  //   <----
  //       B
  //     <----
  // 0 1 2 3 4

  Tag* a = make_tag("A", 1, 3, AniDir::REVERSE, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::REVERSE, 2);
  auto sprite = make_sprite(5, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 2, 1, 3, 2, 1, 4, 3, 2, 4, 3, 2, 0,
                        3, 2, 1, 3, 2, 1, 4, 3, 2, 4, 3, 2, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascadeReversePingPongReverse1)
{
  //    A
  //   <----
  //       B
  //     >---<
  // 0 1 2 3 4

  Tag* a = make_tag("A", 1, 3, AniDir::REVERSE, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::PING_PONG_REVERSE, 2);
  auto sprite = make_sprite(5, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play,
                { 0, 3, 2, 1, 3, 2, 1, 4, 3, 2, 3, 4, 0, 3, 2, 1, 3, 2, 1, 4, 3, 2, 3, 4, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascadeReversePingPongReverse2)
{
  //    A
  // <------
  //       B
  //     >---<
  // 0 1 2 3 4

  Tag* a = make_tag("A", 0, 3, AniDir::REVERSE, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::PING_PONG_REVERSE, 2);
  auto sprite = make_sprite(5, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 3, 2, 1, 0, 4, 3, 2, 3, 4, 3, 2, 1, 0, 3, 2, 1, 0, 4, 3, 2, 3, 4 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascadeReversePingPongReverse3)
{
  //    A
  // <--------
  //       B
  //     >---<
  // 0 1 2 3 4

  Tag* a = make_tag("A", 0, 4, AniDir::REVERSE, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::PING_PONG_REVERSE, 2);
  auto sprite = make_sprite(5, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play,
                {
                  0, 2, 3, 4, 3, 2, 1, 0, 2, 3, 4, 3, 2, 1, 0, 2, 3, 4, 3, 2, 1, 0,
                });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascadePingPongReverseReverse1)
{
  //    A
  // >-----<
  //       B
  //     <----
  // 0 1 2 3 4

  Tag* a = make_tag("A", 0, 3, AniDir::PING_PONG_REVERSE, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::REVERSE, 2);
  auto sprite = make_sprite(5, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 4, 3, 2, 4, 3, 2, 3, 2, 1, 0, 1, 2, 3, 4, 3, 2, 4, 3, 2 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, TwoLoopsInCascadePingPongReverseReverse2)
{
  //     A
  //   >---<
  //       B
  //     <----
  // 0 1 2 3 4

  Tag* a = make_tag("A", 1, 3, AniDir::PING_PONG_REVERSE, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::REVERSE, 2);
  auto sprite = make_sprite(5, { a, b });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play,
                { 0, 3, 2, 1, 2, 3, 4, 3, 2, 4, 3, 2, 0, 3, 2, 1, 2, 3, 4, 3, 2, 4, 3, 2, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, ThreeLoopsInCascade)
{
  //    A
  //   ---->
  //       B
  //     ---->
  //         C
  //       ---->
  // 0 1 2 3 4 5

  Tag* a = make_tag("A", 1, 3, AniDir::FORWARD, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::FORWARD, 2);
  Tag* c = make_tag("C", 3, 5, AniDir::FORWARD, 2);
  auto sprite = make_sprite(6, { a, b, c });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 1, 2, 3, 2, 3, 4, 2, 3, 4, 3, 4, 5, 3, 4, 5, 0,
                        1, 2, 3, 1, 2, 3, 2, 3, 4, 2, 3, 4, 3, 4, 5, 3, 4, 5, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, ThreeLoopsInCascadeDiffAniDirs)
{
  //    A
  //   ---->
  //       B
  //     <----
  //         C
  //       <--->
  // 0 1 2 3 4 5 6

  Tag* tagA = make_tag("A", 1, 3, AniDir::FORWARD, 2);
  Tag* tagB = make_tag("B", 2, 4, AniDir::REVERSE, 2);
  Tag* tagC = make_tag("C", 3, 5, AniDir::PING_PONG, 2);
  auto sprite = make_sprite(7, { tagA, tagB, tagC });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play, { 0, 1, 2, 3, 1, 2, 3, 4, 3, 2, 4, 3, 2, 3, 4, 5, 4, 3, 6, 0,
                        1, 2, 3, 1, 2, 3, 4, 3, 2, 4, 3, 2, 3, 4, 5, 4, 3, 6, 0 });
  EXPECT_FALSE(play.isStopped());
}

TEST(Playback, InnerCascades)
{
  GTEST_SKIP() << "TODO not yet ready";

  //      A
  //   <--------->
  //       B
  //     <----
  //         C
  //       <--->
  // 0 1 2 3 4 5 6

  Tag* a = make_tag("A", 1, 6, AniDir::PING_PONG, 2);
  Tag* b = make_tag("B", 2, 4, AniDir::REVERSE, 2);
  Tag* c = make_tag("C", 3, 5, AniDir::PING_PONG, 2);
  auto sprite = make_sprite(7, { a, b, c });

  Playback play(sprite.get(), 0, Playback::Mode::PlayInLoop);
  expect_frames(play,
                { 0, 1, 4, 3, 2, 4, 3, 2, 3, 4, 5, 4, 3, 6, 5, 4, 3, 4, 5, 2, 3, 4, 2, 3, 4, 1, 0,
                  1, 4, 3, 2, 4, 3, 2, 3, 4, 5, 4, 3, 6, 5, 4, 3, 4, 5, 2, 3, 4, 2, 3, 4, 1, 0 });
  EXPECT_FALSE(play.isStopped());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
