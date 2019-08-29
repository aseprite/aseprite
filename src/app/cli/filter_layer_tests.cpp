// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/cli/cli_processor.h"
#include "doc/layer.h"
#include "doc/sprite.h"

using namespace app;
using namespace doc;

class FilterLayers : public ::testing::Test {
public:
  FilterLayers()
    : sprite(ImageSpec(ColorMode::RGB, 32, 32))
  {
    a->setName("a");
    b->setName("b");
    aa->setName("a");
    ab->setName("b");
    ba->setName("a");
    bb->setName("b");

    sprite.root()->addLayer(a);
    sprite.root()->addLayer(b);
    a->addLayer(aa);
    a->addLayer(ab);
    b->addLayer(ba);
    b->addLayer(bb);
  }

  void filter(
    std::vector<std::string> includes,
    std::vector<std::string> excludes)
  {
    CliProcessor::FilterLayers(
      &sprite, std::move(includes), std::move(excludes), sel);
  }

  Sprite sprite;
  LayerGroup* a = new LayerGroup(&sprite);
  LayerGroup* b = new LayerGroup(&sprite);
  LayerImage* aa = new LayerImage(&sprite);
  LayerImage* ab = new LayerImage(&sprite);
  LayerImage* ba = new LayerImage(&sprite);
  LayerImage* bb = new LayerImage(&sprite);
  SelectedLayers sel;
};

TEST_F(FilterLayers, Default)
{
  filter({}, {});
  EXPECT_EQ(6, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(b));
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_TRUE(sel.contains(ba));
  EXPECT_TRUE(sel.contains(bb));
}

TEST_F(FilterLayers, DefaultWithHiddenChild)
{
  aa->setVisible(false);

  filter({}, {});
  EXPECT_EQ(5, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(b));
  EXPECT_FALSE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_TRUE(sel.contains(ba));
  EXPECT_TRUE(sel.contains(bb));
}

TEST_F(FilterLayers, DefaultWithHiddenGroup)
{
  b->setVisible(false);

  filter({}, {});
  EXPECT_EQ(3, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_FALSE(sel.contains(b));
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_FALSE(sel.contains(ba));
  EXPECT_FALSE(sel.contains(bb));
}

TEST_F(FilterLayers, All)
{
  filter({ "*" }, {});
  EXPECT_EQ(6, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(b));
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_TRUE(sel.contains(ba));
  EXPECT_TRUE(sel.contains(bb));
}

TEST_F(FilterLayers, AllWithHiddenChild)
{
  ab->setVisible(false);

  filter({ "*" }, {});
  EXPECT_EQ(6, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(b));
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_TRUE(sel.contains(ba));
  EXPECT_TRUE(sel.contains(bb));
}

TEST_F(FilterLayers, IncludeGroupAmbiguousNameSelectTopLevel)
{
  filter({ "a" }, {});
  EXPECT_EQ(1, sel.size());
  EXPECT_TRUE(sel.contains(a));
}

TEST_F(FilterLayers, IncludeChild)
{
  filter({ "a/a" }, {});
  EXPECT_EQ(1, sel.size());
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_FALSE(sel.contains(a));
}

TEST_F(FilterLayers, IncludeGroupWithHiddenChild)
{
  aa->setVisible(false);

  filter({ "a" }, {});
  EXPECT_EQ(1, sel.size());
  EXPECT_TRUE(sel.contains(a));
}

TEST_F(FilterLayers, IncludeChildrenEvenHiddenOne)
{
  aa->setVisible(false);

  filter({ "a/*" }, {});
  EXPECT_EQ(2, sel.size());
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
}

TEST_F(FilterLayers, IncludeHiddenChild)
{
  aa->setName("aa");
  aa->setVisible(false);

  filter({ "aa" }, {});
  EXPECT_EQ(1, sel.size());
  EXPECT_TRUE(sel.contains(aa));
}

TEST_F(FilterLayers, IncludeHiddenChildWithFullPath)
{
  aa->setVisible(false);

  filter({ "a/a" }, {});
  EXPECT_EQ(1, sel.size());
  EXPECT_TRUE(sel.contains(aa));
}

TEST_F(FilterLayers, ExcludeAll)
{
  filter({}, { "*" });
  EXPECT_TRUE(sel.empty());
}

TEST_F(FilterLayers, ExcludeChild)
{
  filter({}, { "a/a" });
  EXPECT_EQ(5, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(b));
  EXPECT_FALSE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_TRUE(sel.contains(ba));
  EXPECT_TRUE(sel.contains(bb));
}

TEST_F(FilterLayers, ExcludeGroup)
{
  filter({}, { "b" });
  EXPECT_EQ(3, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
  EXPECT_FALSE(sel.contains(b));
  EXPECT_FALSE(sel.contains(ba));
  EXPECT_FALSE(sel.contains(ba));
}

TEST_F(FilterLayers, IncludeOneGroupAndExcludeOtherGroup)
{
  filter({ "a" }, { "b" });
  EXPECT_EQ(1, sel.size());
  EXPECT_TRUE(sel.contains(a));
}

TEST_F(FilterLayers, IncludeAllExcludeOneGroup)
{
  filter({ "*" }, { "b" });
  EXPECT_EQ(3, sel.size());
  EXPECT_TRUE(sel.contains(a));
  EXPECT_TRUE(sel.contains(aa));
  EXPECT_TRUE(sel.contains(ab));
}
