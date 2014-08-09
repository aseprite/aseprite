/* Aseprite
 * Copyright (C) 2014  David Capello
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

#ifndef APP_UI_RESOURCES_LISTBOX_H_INCLUDED
#define APP_UI_RESOURCES_LISTBOX_H_INCLUDED
#pragma once

#include "app/res/resources_loader.h"
#include "base/override.h"
#include "base/unique_ptr.h"
#include "ui/listbox.h"
#include "ui/timer.h"

namespace app {

  class ResourcesListBox : public ui::ListBox {
  public:
    ResourcesListBox(ResourcesLoader* resourcesLoader);

    Resource* selectedResource();

    void paintResource(ui::Graphics* g, const gfx::Rect& bounds, Resource* resource);
    gfx::Size preferredResourceSize(Resource* resource);

  protected:
    virtual bool onProcessMessage(ui::Message* msg) OVERRIDE;
    virtual void onChangeSelectedItem() OVERRIDE;
    virtual void onResourceChange(Resource* resource) = 0;

    // abstract
    virtual void onPaintResource(ui::Graphics* g, const gfx::Rect& bounds, Resource* resource) = 0;
    virtual void onResourcePreferredSize(Resource* resource, gfx::Size& size) = 0;

  private:
    void onTick();
    void stop();

    ResourcesLoader* m_resourcesLoader;
    ui::Timer m_resourcesTimer;

    class LoadingItem;
    LoadingItem* m_loadingItem;
  };

} // namespace app

#endif
