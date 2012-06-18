// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_BOX_H_INCLUDED
#define UI_BOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class Box : public Widget
  {
  public:
    Box(int align);

  protected:
    // Events
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(PaintEvent& ev) OVERRIDE;

  private:
    void layoutBox(JRect rect);
  };

  class VBox : public Box
  {
  public:
    VBox() : Box(JI_VERTICAL) { }
  };

  class HBox : public Box
  {
  public:
    HBox() : Box(JI_HORIZONTAL) { }
  };

  class BoxFiller : public Box
  {
  public:
    BoxFiller() : Box(JI_HORIZONTAL) {
      this->setExpansive(true);
    }
  };

} // namespace ui

#endif
