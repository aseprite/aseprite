// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_ACTIVE_SITE_HANDLER_H_INCLUDED
#define APP_ACTIVE_SITE_HANDLER_H_INCLUDED
#pragma once

#include "app/doc_observer.h"
#include "doc/frame.h"
#include "doc/object_id.h"

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
    };

    Data& getData(Doc* doc);

    std::map<Doc*, Data> m_data;
  };

} // namespace app

#endif
