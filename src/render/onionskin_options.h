// Aseprite Render Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_ONIONSKIN_OPTIONS_H_INCLUDED
#define RENDER_ONIONSKIN_OPTIONS_H_INCLUDED
#pragma once

#include "render/onionskin_position.h"
#include "render/onionskin_type.h"

namespace doc {
  class Layer;
  class Tag;
}

namespace render {

  class OnionskinOptions {
  public:
    OnionskinOptions(OnionskinType type)
      : m_type(type)
      , m_position(OnionskinPosition::BEHIND)
      , m_prevFrames(0)
      , m_nextFrames(0)
      , m_opacityBase(0)
      , m_opacityStep(0)
      , m_loopTag(nullptr)
      , m_layer(nullptr) {
    }

    OnionskinType type() const { return m_type; }
    OnionskinPosition position() const { return m_position; }
    int prevFrames() const { return m_prevFrames; }
    int nextFrames() const { return m_nextFrames; }
    int opacityBase() const { return m_opacityBase; }
    int opacityStep() const { return m_opacityStep; }
    doc::Tag* loopTag() const { return m_loopTag; }
    doc::Layer* layer() const { return m_layer; }

    void type(OnionskinType type) { m_type = type; }
    void position(OnionskinPosition position) { m_position = position; }
    void prevFrames(int prevFrames) { m_prevFrames = prevFrames; }
    void nextFrames(int nextFrames) { m_nextFrames = nextFrames; }
    void opacityBase(int base) { m_opacityBase = base; }
    void opacityStep(int step) { m_opacityStep = step; }
    void loopTag(doc::Tag* loopTag) { m_loopTag = loopTag; }
    void layer(doc::Layer* layer) { m_layer = layer; }

  private:
    OnionskinType m_type;
    OnionskinPosition m_position;
    int m_prevFrames;
    int m_nextFrames;
    int m_opacityBase;
    int m_opacityStep;
    doc::Tag* m_loopTag;
    doc::Layer* m_layer;
  };

} // namespace render

#endif
