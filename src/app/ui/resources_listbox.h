// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_RESOURCES_LISTBOX_H_INCLUDED
#define APP_UI_RESOURCES_LISTBOX_H_INCLUDED
#pragma once

#include "app/res/resources_loader.h"
#include "base/unique_ptr.h"
#include "ui/listbox.h"
#include "ui/timer.h"

namespace app {

  class ResourcesListBox : public ui::ListBox {
  public:
    ResourcesListBox(ResourcesLoader* resourcesLoader);

    Resource* selectedResource();

    void paintResource(ui::Graphics* g, const gfx::Rect& bounds, Resource* resource);
    gfx::Size resourceSizeHint(Resource* resource);

  protected:
    virtual bool onProcessMessage(ui::Message* msg) override;
    virtual void onChange() override;
    virtual void onResourceChange(Resource* resource) = 0;

    // abstract
    virtual void onPaintResource(ui::Graphics* g, const gfx::Rect& bounds, Resource* resource) = 0;
    virtual void onResourceSizeHint(Resource* resource, gfx::Size& size) = 0;

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
