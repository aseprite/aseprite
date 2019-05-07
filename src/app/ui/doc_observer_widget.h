// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DOC_OBSERVER_WIDGET_H_INCLUDED
#define APP_UI_DOC_OBSERVER_WIDGET_H_INCLUDED
#pragma once

#include "app/context_observer.h"
#include "app/doc.h"
#include "app/doc_observer.h"
#include "app/docs_observer.h"
#include "app/site.h"
#include "app/ui_context.h"
#include "base/debug.h"

namespace app {

  // A widget that observes the active document.
  template<typename BaseWidget>
  class DocObserverWidget : public BaseWidget
                          , public ContextObserver
                          , public DocsObserver
                          , public DocObserver {
  public:
    template<typename... Args>
    DocObserverWidget(Args&&... args)
      : BaseWidget(std::forward<Args>(args)...)
      , m_doc(nullptr) {
      UIContext::instance()->add_observer(this);
      UIContext::instance()->documents().add_observer(this);
    }

    ~DocObserverWidget() {
      ASSERT(!m_doc);
      UIContext::instance()->documents().remove_observer(this);
      UIContext::instance()->remove_observer(this);
    }

  protected:
    Doc* doc() const { return m_doc; }

    virtual void onDocChange(Doc* doc) {
    }

    // ContextObserver impl
    void onActiveSiteChange(const Site& site) override {
      if (m_doc && site.document() != m_doc) {
        m_doc->remove_observer(this);
        m_doc = nullptr;
      }

      if (site.document() && site.sprite()) {
        if (!m_doc) {
          m_doc = const_cast<Doc*>(site.document());
          m_doc->add_observer(this);

          onDocChange(m_doc);
        }
        else {
          ASSERT(m_doc == site.document());
        }
      }
      else {
        ASSERT(m_doc == nullptr);
      }
    }

    // DocsObservers impl
    void onRemoveDocument(Doc* doc) override {
      if (m_doc &&
          m_doc == doc) {
        m_doc->remove_observer(this);
        m_doc = nullptr;

        onDocChange(nullptr);
      }
    }

    // The DocObserver impl will be in the derived class.

  private:
    Doc* m_doc;
  };

} // namespace app

#endif
