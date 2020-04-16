// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_ACTIVE_SITE_HANDLER_H_INCLUDED
#define APP_ACTIVE_SITE_HANDLER_H_INCLUDED
#pragma once

#include "app/doc_observer.h"
#include "app/doc_range.h"
#include "doc/frame.h"
#include "doc/object_id.h"
#include "doc/palette_picks.h"

#include <map>

namespace doc {
  class Layer;
}

namespace app {
  class Doc;
  class Site;

  // Pseudo-DocViews to handle active layer/frame in a non-UI context
  // per Doc.
  //
  // TODO we could move code to handle active frame/layer from
  //      Timeline to this class.
  class ActiveSiteHandler : public DocObserver {
  public:
    ActiveSiteHandler();
    virtual ~ActiveSiteHandler();

    void addDoc(Doc* doc);
    void removeDoc(Doc* doc);
    void getActiveSiteForDoc(Doc* doc, Site* site);
    void setActiveLayerInDoc(Doc* doc, doc::Layer* layer);
    void setActiveFrameInDoc(Doc* doc, doc::frame_t frame);
    void setRangeInDoc(Doc* doc, const DocRange& range);
    void setSelectedColorsInDoc(Doc* doc, const doc::PalettePicks& picks);

  private:
    // DocObserver impl
    void onAddLayer(DocEvent& ev) override;
    void onAddFrame(DocEvent& ev) override;
    void onBeforeRemoveLayer(DocEvent& ev) override;
    void onRemoveFrame(DocEvent& ev) override;

    // Active data for a document
    struct Data {
      doc::ObjectId layer;
      doc::frame_t frame;
      DocRange range;
      doc::PalettePicks selectedColors;
    };

    Data& getData(Doc* doc);

    std::map<Doc*, Data> m_data;
  };

} // namespace app

#endif
