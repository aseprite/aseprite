// Aseprite Document Library
// Copyright (c) 2019 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gtest/gtest.h"

#include "doc/algorithm/polygon.h"

struct scanSegment {
  int x1;
  int x2;
  int y;

  scanSegment(int x1, int y, int x2) :
    x1(x1),
    x2(x2),
    y(y)
  {}
};

struct ScanLineResult {
  std::vector<scanSegment> scanLines;
};

void captureHscanSegment (int x1, int y, int x2, void* scanDataResults) {
  ScanLineResult* results = (ScanLineResult*) scanDataResults;
  results->scanLines.push_back(scanSegment(x1, y, x2));
}

// polygon() function TESTS:
// =========================

TEST(Polygon, SinglePoint1)
{
  //  P0
  int points[2] = { 2 , 3
  };
  int n = 1;
  ScanLineResult results;
  doc::algorithm::polygon(n, &points[0], &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 1);
  if (results.scanLines.size() == 1) {
    EXPECT_EQ(results.scanLines[0].x1, 2);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 3);
  }
}

TEST(Polygon, SinglePoint2)
{
  //  P0=P1
  int points[4] = { 2 , 3 ,
    2 , 3
  };
  int n = 2;
  ScanLineResult results;
  doc::algorithm::polygon(n, &points[0], &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 1);
  if (results.scanLines.size() == 1) {
    EXPECT_EQ(results.scanLines[0].x1, 2);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 3);
  }
}

TEST(Polygon, SinglePoint3)
{
  //  P0=P1=P2
  int points[6] = { 2 , 3 ,
    2 , 3 , 2 , 3
  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, &points[0], &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 1);
  if (results.scanLines.size() == 1) {
    EXPECT_EQ(results.scanLines[0].x1, 2);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 3);
  }
}

TEST(Polygon, HorizontalLine1Test)
{
  //  P0-----P1
  int points[4] = { 0 , 0 ,
                    2 , 0
                  };
  int n = 2;
  ScanLineResult results;
  doc::algorithm::polygon(n, &points[0], &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 1);
  if (results.scanLines.size() == 1) {
    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 0);
  }
}

TEST(Polygon, HorizontalLine2Test)
{
  //  P0-----P2-----P1
  int points[6] = { 0 , 0 ,
                    2 , 0 ,
                    5 , 0
                  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 1);
  if (results.scanLines.size() == 1) {
    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 5);
    EXPECT_EQ(results.scanLines[0].y, 0);
  }
}

TEST(Polygon, VerticalLine1Test)
{
  //  P0
  //   |
  //   |
  //  P1
  int points[4] = { 0 , 0 ,
                    0 , 2 ,
                  };
  int n = 2;
  ScanLineResult results;
  doc::algorithm::polygon(n, &points[0], &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 3);
  if (results.scanLines.size() == 3) {
    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 0);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 0);
    EXPECT_EQ(results.scanLines[1].x2, 0);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 0);
    EXPECT_EQ(results.scanLines[2].x2, 0);
    EXPECT_EQ(results.scanLines[2].y, 2);
  }
}

TEST(Polygon, VerticalLine2Test)
{
  //  P0
  //   |
  //   |
  //  P2
  //   |
  //   |
  //  P1
  int points[6] = { 0 , 0 ,
                    0 , 2 ,
                    0 , 5 ,
                  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 6);
  if (results.scanLines.size() == 6) {
    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 0);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 0);
    EXPECT_EQ(results.scanLines[1].x2, 0);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 0);
    EXPECT_EQ(results.scanLines[2].x2, 0);
    EXPECT_EQ(results.scanLines[2].y, 2);

    EXPECT_EQ(results.scanLines[3].x1, 0);
    EXPECT_EQ(results.scanLines[3].x2, 0);
    EXPECT_EQ(results.scanLines[3].y, 3);

    EXPECT_EQ(results.scanLines[4].x1, 0);
    EXPECT_EQ(results.scanLines[4].x2, 0);
    EXPECT_EQ(results.scanLines[4].y, 4);

    EXPECT_EQ(results.scanLines[5].x1, 0);
    EXPECT_EQ(results.scanLines[5].x2, 0);
    EXPECT_EQ(results.scanLines[5].y, 5);
  }
}

TEST(Polygon, Triangle1Test)
{
  //  P0-----P2
  //   |     /
  //   |    /
  //   |   /
  //   |  /
  //   | /
  //  P1
  int points[6] = { 0 , 0 ,
                    0 , 2 ,
                    2 , 0 ,
                  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 3);
  if (results.scanLines.size() == 3) {

    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 0);
    EXPECT_EQ(results.scanLines[1].x2, 1);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 0);
    EXPECT_EQ(results.scanLines[2].x2, 0);
    EXPECT_EQ(results.scanLines[2].y, 2);
  }
}

TEST(Polygon, Triangle2Test)
{
  /*
          P1
         / \
        /   \
       /     \
      P0-----P2
  */
  int points[6] = { 0 , 4 ,
                    2 , 0 ,
                    4 , 4 ,
                  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 5);
  if (results.scanLines.size() == 5) {
    EXPECT_EQ(results.scanLines[0].x1, 2);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 2);
    EXPECT_EQ(results.scanLines[1].x2, 3);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 1);
    EXPECT_EQ(results.scanLines[2].x2, 3);
    EXPECT_EQ(results.scanLines[2].y, 2);

    EXPECT_EQ(results.scanLines[3].x1, 1);
    EXPECT_EQ(results.scanLines[3].x2, 4);
    EXPECT_EQ(results.scanLines[3].y, 3);

    EXPECT_EQ(results.scanLines[4].x1, 0);
    EXPECT_EQ(results.scanLines[4].x2, 4);
    EXPECT_EQ(results.scanLines[4].y, 4);
  }
}

TEST(Polygon, Triangle3Test)
{
  /*
          P2
         / \
        /   \
       /     \
      P0-----P1
  */
  int points[6] = { 0 , 4 ,
                    4 , 4 ,
                    2 , 0 ,
                  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 5);
  if (results.scanLines.size() == 5) {
    EXPECT_EQ(results.scanLines[0].x1, 2);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 1);
    EXPECT_EQ(results.scanLines[1].x2, 2);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 1);
    EXPECT_EQ(results.scanLines[2].x2, 3);
    EXPECT_EQ(results.scanLines[2].y, 2);

    EXPECT_EQ(results.scanLines[3].x1, 0);
    EXPECT_EQ(results.scanLines[3].x2, 3);
    EXPECT_EQ(results.scanLines[3].y, 3);

    EXPECT_EQ(results.scanLines[4].x1, 0);
    EXPECT_EQ(results.scanLines[4].x2, 4);
    EXPECT_EQ(results.scanLines[4].y, 4);
  }
}

TEST(Polygon, Triangle4Test)
{
  /*
          P2
         / \
        /   \
       /     \
      P1-----P0
  */
  int points[6] = { 4 , 4 ,
                    0 , 4 ,
                    2 , 0 ,
                  };
  int n = 3;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 5);
  if (results.scanLines.size() == 5) {
    EXPECT_EQ(results.scanLines[0].x1, 2);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 2);
    EXPECT_EQ(results.scanLines[1].x2, 3);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 1);
    EXPECT_EQ(results.scanLines[2].x2, 3);
    EXPECT_EQ(results.scanLines[2].y, 2);

    EXPECT_EQ(results.scanLines[3].x1, 1);
    EXPECT_EQ(results.scanLines[3].x2, 4);
    EXPECT_EQ(results.scanLines[3].y, 3);

    EXPECT_EQ(results.scanLines[4].x1, 0);
    EXPECT_EQ(results.scanLines[4].x2, 4);
    EXPECT_EQ(results.scanLines[4].y, 4);
  }
}

TEST(Polygon, Square1Test)
{
  //  P0-----P2
  //   |     |
  //   |     |
  //   |     |
  //  P1-----P2
  int points[8] = { 0 , 0 ,
                    0 , 2 ,
                    2 , 2 ,
                    2 , 0
                  };
  int n = 4;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 3);
  if (results.scanLines.size() == 3) {
    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 2);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 0);
    EXPECT_EQ(results.scanLines[1].x2, 2);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 0);
    EXPECT_EQ(results.scanLines[2].x2, 2);
    EXPECT_EQ(results.scanLines[2].y, 2);
  }
}

TEST(Polygon, Poligon1Test)
{
  //  P0        P2
  //   \      / /
  //    \  P3  /
  //     \    /
  //       P1
  int points[8] = { 0 , 0 ,
                    2 , 3 ,
                    4 , 0 ,
                    2 , 1
                  };
  int n = 4;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 5);
  if (results.scanLines.size() == 5) {
    EXPECT_EQ(results.scanLines[0].x1, 0);
    EXPECT_EQ(results.scanLines[0].x2, 1);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 4);
    EXPECT_EQ(results.scanLines[1].x2, 4);
    EXPECT_EQ(results.scanLines[1].y, 0);

    EXPECT_EQ(results.scanLines[2].x1, 1);
    EXPECT_EQ(results.scanLines[2].x2, 3);
    EXPECT_EQ(results.scanLines[2].y, 1);

    EXPECT_EQ(results.scanLines[3].x1, 1);
    EXPECT_EQ(results.scanLines[3].x2, 3);
    EXPECT_EQ(results.scanLines[3].y, 2);

    EXPECT_EQ(results.scanLines[4].x1, 2);
    EXPECT_EQ(results.scanLines[4].x2, 2);
    EXPECT_EQ(results.scanLines[4].y, 3);
  }
}

TEST(Polygon, Polygon2Test)
{
  /*
                 P3------P4
      P0         |      /
       \         |     /
        \        |    /
         \     / P2  P5
          P1 /        \
                       \
          P9      /P7   \
            \   /    \   \
             P8        \ P6
  */
  int points[20] = { 0 , 1 ,
                     2 , 4 ,
                     4 , 3 ,
                     4 , 0 ,
                     7 , 0 ,

                     6 , 3 ,
                     9 , 7 ,
                     5 , 5 ,
                     3 , 7 ,
                     2 , 5
                   };
  int n = 10;
  ScanLineResult results;
  doc::algorithm::polygon(n, points, (void *) &results, captureHscanSegment);
  EXPECT_EQ(results.scanLines.size(), 13);
  if (results.scanLines.size() == 13) {
    EXPECT_EQ(results.scanLines[0].x1, 4);
    EXPECT_EQ(results.scanLines[0].x2, 7);
    EXPECT_EQ(results.scanLines[0].y, 0);

    EXPECT_EQ(results.scanLines[1].x1, 0);
    EXPECT_EQ(results.scanLines[1].x2, 0);
    EXPECT_EQ(results.scanLines[1].y, 1);

    EXPECT_EQ(results.scanLines[2].x1, 4);
    EXPECT_EQ(results.scanLines[2].x2, 7);
    EXPECT_EQ(results.scanLines[2].y, 1);

    EXPECT_EQ(results.scanLines[3].x1, 0);
    EXPECT_EQ(results.scanLines[3].x2, 1);
    EXPECT_EQ(results.scanLines[3].y, 2);

    EXPECT_EQ(results.scanLines[4].x1, 4);
    EXPECT_EQ(results.scanLines[4].x2, 6);
    EXPECT_EQ(results.scanLines[4].y, 2);

    EXPECT_EQ(results.scanLines[5].x1, 1);
    EXPECT_EQ(results.scanLines[5].x2, 1);
    EXPECT_EQ(results.scanLines[5].y, 3);

    EXPECT_EQ(results.scanLines[6].x1, 3);
    EXPECT_EQ(results.scanLines[6].x2, 6);
    EXPECT_EQ(results.scanLines[6].y, 3);

    EXPECT_EQ(results.scanLines[7].x1, 1);
    EXPECT_EQ(results.scanLines[7].x2, 7);
    EXPECT_EQ(results.scanLines[7].y, 4);

    EXPECT_EQ(results.scanLines[8].x1, 2);
    EXPECT_EQ(results.scanLines[8].x2, 8);
    EXPECT_EQ(results.scanLines[8].y, 5);

    EXPECT_EQ(results.scanLines[9].x1, 2);
    EXPECT_EQ(results.scanLines[9].x2, 4);
    EXPECT_EQ(results.scanLines[9].y, 6);

    EXPECT_EQ(results.scanLines[10].x1, 7);
    EXPECT_EQ(results.scanLines[10].x2, 8);
    EXPECT_EQ(results.scanLines[10].y, 6);

    EXPECT_EQ(results.scanLines[11].x1, 3);
    EXPECT_EQ(results.scanLines[11].x2, 3);
    EXPECT_EQ(results.scanLines[11].y, 7);

    EXPECT_EQ(results.scanLines[12].x1, 9);
    EXPECT_EQ(results.scanLines[12].x2, 9);
    EXPECT_EQ(results.scanLines[12].y, 7);
  }
}

// createUnion() function TESTS:
// =============================
// Function Tests to ensure correct results when:
// testA1 : pairs.size() == 0 / ints == 0
// testA2 : pairs.size() == 0 / ints > 0 (with ints even number)
// testA3 : pairs.size() == 0 / ints == 1
// testA4 : pairs.size() == 1 / ints > 0 (with ints even number)
//
// Pre-condition fulfilled: pairs.size() >= 2 (even elements number)
// testB1 : ints > pairs.size()
// testB2 : ints == 0
// testB3 : ints == 1
// testB4 : ints < 0
//
// Pre-condition fulfilled: pairs.size() >= 2 / ints >= 2 (with ints even number)
// testC1 : x == pairs[i]
// testC2 : x == pairs[i+1]
// testC3 : x == pairs[i]-1
// testC4 : x == pairs[i+1]+1
// testC5 : x < pairs[i]-1
// testC6 : x > pairs[i+1]+1
// testC7 : pairs[i] < x < pairs[i+1]
// testC8 : x == pairs[i+1]+1 && x == pairs[i+2]+1
// testC9 :  special case
// testC10 : special case

TEST(createUnion, testA1)
{
  // testA1 : pairs.size() == 0 / ints == 0
  int ints = 0;
  int x = 2;
  std::vector<int> pairs;
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 2);
  EXPECT_EQ(pairs[1], 2);
  EXPECT_EQ(ints, 2);
}

TEST(createUnion, testA2)
{
  // testA2 : pairs.size() == 0 / ints > 0 (with ints even number)
  int ints = 2;
  int x = 5;
  std::vector<int> pairs;
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), false);
}

TEST(createUnion, testA3)
{
  // testA3 : pairs.size() == 0 / ints == 1
  int ints = 1;
  int x = 5;
  std::vector<int> pairs;
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), false);
}

TEST(createUnion, testA4)
{
  // testA4 : pairs.size() == 1 / ints > 0 (with ints even number)
  int ints = 2;
  int x = 5;
  std::vector<int> pairs;
  pairs.push_back(0);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), false);
}

// Next tests have the following condition fulfilled:
//   pairs.size() >= 2 (even elements number)
// testB1 : ints > pairs.size()
// testB2 : ints == 0
// testB3 : ints == 1
// testB4 : ints < 0
TEST(createUnion, testB1)
{
  // testB1 : ints > pairs.size()
  // pairs.size() >= 2 (even elements number)
  int ints = 3;
  int x = 5;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(0);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), false);
}

TEST(createUnion, testB2)
{
  // testB2 : ints == 0
  // pairs.size() >= 2 (even elements number)
  int ints = 0;
  int x = 5;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(0);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 5);
  EXPECT_EQ(pairs[1], 5);
  EXPECT_EQ(ints, 2);
}

TEST(createUnion, testB3)
{
  // testB3 : ints == 1
  // pairs.size() >= 2 (even elements number)
  int ints = 1;
  int x = 5;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(0);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), false);
}

TEST(createUnion, testB4)
{
  // testB4 : ints < 0
  int ints = -1;
  int x = 5;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(0);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), false);
}

// Next tests have the following condition fulfilled:
//   pairs.size() >= 2 / ints >= 2 (with ints even number)
// testC1 : x == pairs[i]
// testC2 : x == pairs[i+1]
// testC3 : x == pairs[i]-1
// testC4 : x == pairs[i+1]+1
// testC5 : x < pairs[i]-1
// testC6 : x > pairs[i+1]+1
// testC7 : pairs[i] < x < pairs[i+1]
// testC9 :  special case
// testC10 : special case

TEST(createUnion, testC1)
{
  // testC1 : x == pairs[i]
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = 0;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(4);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 1);
  EXPECT_EQ(pairs[2], 4);
  EXPECT_EQ(pairs[3], 5);
  EXPECT_EQ(ints, 4);
}

TEST(createUnion, testC2)
{
  // testC2 : x == pairs[i+1]
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = 1;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(4);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 1);
  EXPECT_EQ(pairs[2], 4);
  EXPECT_EQ(pairs[3], 5);
  EXPECT_EQ(ints, 4);
}

TEST(createUnion, testC3)
{
  // testC3 : x == pairs[i]-1
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = -1;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(4);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], -1);
  EXPECT_EQ(pairs[1], 1);
  EXPECT_EQ(pairs[2], 4);
  EXPECT_EQ(pairs[3], 5);
  EXPECT_EQ(ints, 4);
}

TEST(createUnion, testC4)
{
  // testC4 : x == pairs[i+1]+1
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = 2;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(4);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 2);
  EXPECT_EQ(pairs[2], 4);
  EXPECT_EQ(pairs[3], 5);
  EXPECT_EQ(ints, 4);
}

TEST(createUnion, testC5)
{
  // testC5 : x < pairs[i]-1
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = -2;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(4);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], -2);
  EXPECT_EQ(pairs[1], -2);
  EXPECT_EQ(pairs[2], 0);
  EXPECT_EQ(pairs[3], 1);
  EXPECT_EQ(pairs[4], 4);
  EXPECT_EQ(pairs[5], 5);
  EXPECT_EQ(ints, 6);
}

TEST(createUnion, testC6)
{
  // testC6 : x > pairs[i+1]+1
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = 7;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(4);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 1);
  EXPECT_EQ(pairs[2], 4);
  EXPECT_EQ(pairs[3], 5);
  EXPECT_EQ(pairs[4], 7);
  EXPECT_EQ(pairs[5], 7);
  EXPECT_EQ(ints, 6);
}

TEST(createUnion, testC7)
{
  // testC7 : pairs[i] < x < pairs[i+1]
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 2;
  int x = 3;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(5);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 5);
  EXPECT_EQ(ints, 2);
}

TEST(createUnion, testC8)
{
  // testC8 : x == pairs[i+1]+1 && x == pairs[i+2]+1
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = 3;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(2);
  pairs.push_back(3);
  pairs.push_back(4);

  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 4);
  EXPECT_EQ(ints, 2);
}

TEST(createUnion, testC9)
{
  // testC9 : special case
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 6;
  int x = 1;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(0);
  pairs.push_back(1);
  pairs.push_back(1);
  pairs.push_back(2);
  pairs.push_back(4);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 4);
  EXPECT_EQ(ints, 2);
}

TEST(createUnion, testC10)
{
  // testC10 : special case
  // pairs.size() >= 2 / ints >= 2 (with ints even number)
  int ints = 4;
  int x = 3;
  std::vector<int> pairs;
  pairs.push_back(0);
  pairs.push_back(3);
  pairs.push_back(4);
  pairs.push_back(7);
  pairs.push_back(0);
  pairs.push_back(0);
  pairs.push_back(0);
  pairs.push_back(0);
  EXPECT_EQ(doc::algorithm::createUnion(pairs, x, ints), true);
  EXPECT_EQ(pairs[0], 0);
  EXPECT_EQ(pairs[1], 7);
  EXPECT_EQ(ints, 2);
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();

}
