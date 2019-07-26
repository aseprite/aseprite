// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_COLOR_CURVE_EDITOR_H_INCLUDED
#define APP_COMMANDS_FILTERS_COLOR_CURVE_EDITOR_H_INCLUDED
#pragma once

#include "filters/color_curve.h"
#include "gfx/point.h"
#include "obs/signal.h"
#include "ui/widget.h"

namespace app {
  using namespace filters;

  class ColorCurveEditor : public ui::Widget {
  public:
    ColorCurveEditor(const ColorCurve& curve, const gfx::Rect& viewBounds);

    const ColorCurve& getCurve() const { return m_curve; }

    obs::signal<void()> CurveEditorChange;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    gfx::Point* getClosestPoint(const gfx::Point& viewPt);
    bool editNodeManually(gfx::Point& viewPt);
    gfx::Point viewToClient(const gfx::Point& viewPt);
    gfx::Point screenToView(const gfx::Point& screenPt);
    gfx::Point clientToView(const gfx::Point& clientPt);
    void addPoint(const gfx::Point& viewPoint);
    void removePoint(const gfx::Point& viewPoint);

    ColorCurve m_curve;
    int m_status;
    gfx::Rect m_viewBounds;
    gfx::Point* m_hotPoint;
    gfx::Point* m_editPoint;
  };

} // namespace app

#endif
