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

#ifndef APP_TEST_CONTEXT_H_INCLUDED
#define APP_TEST_CONTEXT_H_INCLUDED
#pragma once

#include "app/context.h"
#include "app/document.h"
#include "app/document_location.h"
#include "doc/settings.h"
#include "raster/layer.h"
#include "raster/sprite.h"

namespace app {

  class TestContext : public app::Context,
                      public doc::Settings {
  public:
    TestContext() : app::Context(NULL) {
      setSettings(this);
    }

    // doc::Settings impl
    size_t undoSizeLimit() const {
      return 8;
    }

  protected:
    void onGetActiveLocation(DocumentLocation* location) const override {
      Document* doc = activeDocument();
      if (!doc)
        return;

      location->document(doc);
      location->sprite(doc->sprite());
      location->layer(doc->sprite()->folder()->getFirstLayer());
      location->frame(FrameNumber(0));
    }
  };

} // namespace app

#endif
