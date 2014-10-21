// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "doc/context.h"
#include "doc/document.h"

using namespace doc;

namespace doc {

  std::ostream& operator<<(std::ostream& os, ColorMode mode) {
    return os << (int)mode;
  }
  
} // namespace doc

TEST(Context, AddDocument)
{
  Context ctx;
  Document* doc = ctx.documents().add(32, 28);
  ASSERT_TRUE(doc != NULL);

  EXPECT_EQ(32, doc->width());
  EXPECT_EQ(28, doc->height());
  EXPECT_EQ(ColorMode::RGB, doc->colorMode()); // Default color mode is RGB
}

TEST(Context, DeleteDocuments)
{
  Context ctx;
  Document* doc1 = ctx.documents().add(2, 2);
  Document* doc2 = ctx.documents().add(4, 4);
  EXPECT_EQ(2, ctx.documents().size());

  delete doc1;
  delete doc2;

  EXPECT_EQ(0, ctx.documents().size());
}

TEST(Context, CloseAndDeleteDocuments)
{
  Context ctx;
  EXPECT_EQ(0, ctx.documents().size());

  Document* doc1 = ctx.documents().add(2, 2);
  Document* doc2 = ctx.documents().add(4, 4);
  EXPECT_EQ(2, ctx.documents().size());

  doc1->close();
  EXPECT_EQ(1, ctx.documents().size());

  delete doc1;
  delete doc2;
  EXPECT_EQ(0, ctx.documents().size());
}

TEST(Context, SwitchContext)
{
  Context ctx1, ctx2;
  Document* doc1 = new Document();
  Document* doc2 = new Document();
  doc1->setContext(&ctx1);
  doc2->setContext(&ctx2);
  EXPECT_EQ(&ctx1, doc1->context());
  EXPECT_EQ(&ctx2, doc2->context());

  doc1->setContext(&ctx2);
  doc2->setContext(&ctx1);
  EXPECT_EQ(&ctx2, doc1->context());
  EXPECT_EQ(&ctx1, doc2->context());

  ctx2.documents().remove(doc1);
  ctx1.documents().remove(doc2);
  ctx1.documents().add(doc1);
  ctx2.documents().add(doc2);
  EXPECT_EQ(&ctx1, doc1->context());
  EXPECT_EQ(&ctx2, doc2->context());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
