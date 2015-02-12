// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TEST_CONTEXT_H_INCLUDED
#define APP_TEST_CONTEXT_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/document.h"
#include "app/document_location.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

  class TestContext : public app::Context {
  public:
    TestContext() : app::Context(NULL) {
    }

  protected:
    void onGetActiveLocation(DocumentLocation* location) const override {
      Document* doc = activeDocument();
      if (!doc)
        return;

      location->document(doc);
      location->sprite(doc->sprite());
      location->layer(doc->sprite()->folder()->getFirstLayer());
      location->frame(0);
    }
  };

} // namespace app

#endif
