/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_DOCUMENT_RANGE_H_INCLUDED
#define APP_DOCUMENT_RANGE_H_INCLUDED
#pragma once

#include "raster/frame_number.h"
#include "raster/layer_index.h"

#include <algorithm>

namespace app {
  using namespace raster;

  class DocumentRange {
  public:
    enum Type { kNone, kCels, kFrames, kLayers };

    DocumentRange() : m_type(kNone) { }

    Type type() const { return m_type; }
    bool enabled() const { return m_type != kNone; }
    LayerIndex layerBegin() const  { return std::min(m_layerBegin, m_layerEnd); }
    LayerIndex layerEnd() const    { return std::max(m_layerBegin, m_layerEnd); }
    FrameNumber frameBegin() const { return std::min(m_frameBegin, m_frameEnd); }
    FrameNumber frameEnd() const   { return std::max(m_frameBegin, m_frameEnd); }

    int layers() const { return layerEnd() - layerBegin() + 1; }
    FrameNumber frames() const { return (frameEnd() - frameBegin()).next(); }
    void setLayers(int layers);
    void setFrames(FrameNumber frames);
    void displace(int layerDelta, int frameDelta);

    bool inRange(LayerIndex layer) const;
    bool inRange(FrameNumber frame) const;
    bool inRange(LayerIndex layer, FrameNumber frame) const;

    void startRange(LayerIndex layer, FrameNumber frame, Type type);
    void endRange(LayerIndex layer, FrameNumber frame);
    void disableRange();

    bool operator==(const DocumentRange& o) const {
      return m_type == o.m_type &&
        layerBegin() == o.layerBegin() && layerEnd() == o.layerEnd() &&
        frameBegin() == o.frameBegin() && frameEnd() == o.frameEnd();
    }

  private:
    Type m_type;
    LayerIndex m_layerBegin;
    LayerIndex m_layerEnd;
    FrameNumber m_frameBegin;
    FrameNumber m_frameEnd;
  };

} // namespace app

#endif
