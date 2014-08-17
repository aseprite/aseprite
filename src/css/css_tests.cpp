// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "css/css.h"

using namespace css;

namespace css {

  std::ostream& operator<<(std::ostream& os, const Value& value)
  {
    os << "(" << value.type();

    if (value.type() == Value::Number)
      os << ", " << value.number() << " [" << value.unit() << "]";
    else if (value.type() == Value::String)
      os << ", " << value.string();

    os << ")";
    return os;
  }
  
} // namespace css

TEST(Css, Style)
{
  Rule background("background");
  Rule text("text");
  Rule border("border");
  Style style("style");

  style[background] = Value("image.png");
  style[text] = Value("hi");
  style[border] = Value(12.0, "px");
  EXPECT_EQ(Value("image.png"), style[background]);
  EXPECT_EQ(Value("hi"), style[text]);
  EXPECT_EQ(Value(12.0, "px"), style[border]);

  style[border].setNumber(13.0);
  EXPECT_EQ(Value(13.0, "px"), style[border]);

  Style style2("style2", &style);
  EXPECT_EQ(&style, style2.base());
  EXPECT_EQ(Value(), style2[background]);
  EXPECT_EQ(Value(), style2[text]);
  EXPECT_EQ(Value(), style2[border]);
}

TEST(Css, QueryIsInSyncWithStyleSheet)
{
  Rule background("background");
  Sheet sheet;
  sheet.addRule(&background);

  Style style("style");
  style[background] = Value("a.png");
  sheet.addStyle(&style);

  Query query = sheet.query(style);
  EXPECT_EQ(Value("a.png"), query[background]);

  style[background] = Value("b.png");
  EXPECT_EQ(Value("b.png"), query[background]);
}

TEST(Css, StatefulStyles)
{
  Rule background("background");
  Rule text("text");
  Rule border("border");
  State hover("hover");
  State focus("focus");
  State active("active");
  Style base("base");
  Style baseHover("base:hover");
  Style baseFocus("base:focus");
  Style baseActive("base:active");
  base[background] = Value("image.png");
  base[text] = Value("textnormal");
  baseHover[text] = Value("texthover");
  baseFocus[border] = Value(12.0);
  baseActive[border] = Value(24.0);

  Sheet sheet;
  sheet.addRule(&background);
  sheet.addRule(&text);
  sheet.addRule(&border);
  sheet.addStyle(&base);
  sheet.addStyle(&baseHover);
  sheet.addStyle(&baseFocus);
  sheet.addStyle(&baseActive);

  Query query = sheet.query(base);
  EXPECT_EQ(Value("image.png"), query[background]);
  EXPECT_EQ(Value("textnormal"), query[text]);

  query = sheet.query(base + hover);
  EXPECT_EQ(Value("image.png"), query[background]);
  EXPECT_EQ(Value("texthover"), query[text]);

  query = sheet.query(base + focus);
  EXPECT_EQ(Value("image.png"), query[background]);
  EXPECT_EQ(Value("textnormal"), query[text]);
  EXPECT_EQ(Value(12.0), query[border]);

  query = sheet.query(base + focus + hover);
  EXPECT_EQ(Value("image.png"), query[background]);
  EXPECT_EQ(Value("texthover"), query[text]);
  EXPECT_EQ(Value(12.0), query[border]);

  query = sheet.query(base + focus + hover + active);
  EXPECT_EQ(Value("image.png"), query[background]);
  EXPECT_EQ(Value("texthover"), query[text]);
  EXPECT_EQ(Value(24.0), query[border]);

  query = sheet.query(base + active + focus + hover); // Different order
  EXPECT_EQ(Value("image.png"), query[background]);
  EXPECT_EQ(Value("texthover"), query[text]);
  EXPECT_EQ(Value(12.0), query[border]);
}

TEST(Css, StyleHierarchy)
{
  Rule bg("bg");
  Rule fg("fg");
  State hover("hover");
  State focus("focus");
  Style base("base");
  Style stylea("stylea", &base);
  Style styleb("styleb", &stylea);
  Style stylec("stylec", &styleb);
  base[bg] = Value(1);
  base[fg] = Value(2);
  stylea[bg] = Value(3);
  styleb[bg] = Value(4);
  styleb[fg] = Value(5);
  stylec[bg] = Value(6);

  Sheet sheet;
  sheet.addRule(&bg);
  sheet.addRule(&fg);
  sheet.addStyle(&base);
  sheet.addStyle(&stylea);
  sheet.addStyle(&styleb);
  sheet.addStyle(&stylec);

  Query query = sheet.query(base);
  EXPECT_EQ(Value(1), query[bg]);
  EXPECT_EQ(Value(2), query[fg]);

  query = sheet.query(stylea);
  EXPECT_EQ(Value(3), query[bg]);
  EXPECT_EQ(Value(2), query[fg]);

  query = sheet.query(styleb);
  EXPECT_EQ(Value(4), query[bg]);
  EXPECT_EQ(Value(5), query[fg]);

  query = sheet.query(stylec);
  EXPECT_EQ(Value(6), query[bg]);
  EXPECT_EQ(Value(5), query[fg]);
}

TEST(Css, CompoundStyles)
{
  Rule bg("bg");
  Rule fg("fg");
  State hover("hover");
  State focus("focus");
  Style base("base");
  Style baseHover("base:hover");
  Style baseFocus("base:focus");
  Style sub("sub", &base);
  Style subFocus("sub:focus", &base);
  Style sub2("sub2", &sub);
  Style sub3("sub3", &sub2);
  Style sub3FocusHover("sub3:focus:hover", &sub2);

  base[bg] = Value(1);
  base[fg] = Value(2);
  baseHover[fg] = Value(3);
  baseFocus[bg] = Value(4);

  sub[bg] = Value(5);
  subFocus[fg] = Value(6);

  sub3[bg] = Value(7);
  sub3FocusHover[fg] = Value(8);

  Sheet sheet;
  sheet.addRule(&bg);
  sheet.addRule(&fg);
  sheet.addStyle(&base);
  sheet.addStyle(&baseHover);
  sheet.addStyle(&baseFocus);
  sheet.addStyle(&sub);
  sheet.addStyle(&subFocus);
  sheet.addStyle(&sub2);
  sheet.addStyle(&sub3);
  sheet.addStyle(&sub3FocusHover);

  CompoundStyle compoundBase = sheet.compoundStyle("base");
  EXPECT_EQ(Value(1), compoundBase[bg]);
  EXPECT_EQ(Value(2), compoundBase[fg]);
  EXPECT_EQ(Value(1), compoundBase[hover][bg]);
  EXPECT_EQ(Value(3), compoundBase[hover][fg]);
  EXPECT_EQ(Value(4), compoundBase[focus][bg]);
  EXPECT_EQ(Value(2), compoundBase[focus][fg]);
  EXPECT_EQ(Value(4), compoundBase[hover+focus][bg]);
  EXPECT_EQ(Value(3), compoundBase[hover+focus][fg]);

  CompoundStyle compoundSub = sheet.compoundStyle("sub");
  EXPECT_EQ(Value(5), compoundSub[bg]);
  EXPECT_EQ(Value(2), compoundSub[fg]);
  EXPECT_EQ(Value(5), compoundSub[hover][bg]);
  EXPECT_EQ(Value(3), compoundSub[hover][fg]);
  EXPECT_EQ(Value(4), compoundSub[focus][bg]);
  EXPECT_EQ(Value(6), compoundSub[focus][fg]);
  EXPECT_EQ(Value(4), compoundSub[hover+focus][bg]);
  EXPECT_EQ(Value(6), compoundSub[hover+focus][fg]);

  CompoundStyle compoundSub2 = sheet.compoundStyle("sub2");
  EXPECT_EQ(Value(5), compoundSub2[bg]);
  EXPECT_EQ(Value(2), compoundSub2[fg]);
  EXPECT_EQ(Value(5), compoundSub2[hover][bg]);
  EXPECT_EQ(Value(3), compoundSub2[hover][fg]);
  EXPECT_EQ(Value(4), compoundSub2[focus][bg]);
  EXPECT_EQ(Value(6), compoundSub2[focus][fg]);
  EXPECT_EQ(Value(4), compoundSub2[hover+focus][bg]);
  EXPECT_EQ(Value(6), compoundSub2[hover+focus][fg]);

  CompoundStyle compoundSub3 = sheet.compoundStyle("sub3");
  EXPECT_EQ(Value(7), compoundSub3[bg]);
  EXPECT_EQ(Value(2), compoundSub3[fg]);
  EXPECT_EQ(Value(7), compoundSub3[hover][bg]);
  EXPECT_EQ(Value(3), compoundSub3[hover][fg]);
  EXPECT_EQ(Value(4), compoundSub3[focus][bg]);
  EXPECT_EQ(Value(6), compoundSub3[focus][fg]);
  EXPECT_EQ(Value(4), compoundSub3[hover+focus][bg]);
  EXPECT_EQ(Value(6), compoundSub3[hover+focus][fg]);
  EXPECT_EQ(Value(8), compoundSub3[focus+hover][fg]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
