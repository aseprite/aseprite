// Aseprite
// Copyright (c) 2001-2018 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef DOC_TEST_CONTEXT_H_INCLUDED
#define DOC_TEST_CONTEXT_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/doc.h"
#include "app/site.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

  template<typename Base>
  class TestContextT : public Base {
  public:
    TestContextT() : m_activeDoc(nullptr) {
    }

  protected:

    void onGetActiveSite(Site* site) const override {
      Doc* doc = m_activeDoc;
      if (!doc)
        return;

      site->document(doc);
      site->sprite(doc->sprite());
      site->layer(doc->sprite()->root()->firstLayer());
      site->frame(0);
    }

    void onAddDocument(Doc* doc) override {
      m_activeDoc = doc;
      this->notifyActiveSiteChanged();
    }

    void onRemoveDocument(Doc* doc) override {
      if (m_activeDoc == doc) {
        m_activeDoc = nullptr;
        this->notifyActiveSiteChanged();
      }
    }

  private:
    Doc* m_activeDoc;
  };

  typedef TestContextT<Context> TestContext;

} // namespace app

#endif
