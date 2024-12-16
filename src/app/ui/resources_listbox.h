// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_RESOURCES_LISTBOX_H_INCLUDED
#define APP_UI_RESOURCES_LISTBOX_H_INCLUDED
#pragma once

#include "app/res/resources_loader.h"
#include "obs/signal.h"
#include "ui/listbox.h"
#include "ui/listitem.h"
#include "ui/timer.h"

#include <memory>

namespace app {
class ResourceListItem;

class ResourceListItem : public ui::ListItem {
public:
  ResourceListItem(Resource* resource);

  Resource* resource() const { return m_resource.get(); }

protected:
  bool onProcessMessage(ui::Message* msg) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;

private:
  std::unique_ptr<Resource> m_resource;
};

class ResourcesListBox : public ui::ListBox {
public:
  friend class ResourceListItem;

  ResourcesListBox(ResourcesLoader* resourcesLoader);

  Resource* selectedResource();

  void markToReload();
  void reload();

  obs::signal<void()> FinishLoading;

protected:
  virtual bool onProcessMessage(ui::Message* msg) override;
  virtual void onChange() override;
  virtual ResourceListItem* onCreateResourceItem(Resource* resource);

  // abstract
  virtual void onResourceChange(Resource* resource) = 0;
  virtual void onPaintResource(ui::Graphics* g, gfx::Rect& bounds, Resource* resource) = 0;
  virtual void onResourceSizeHint(Resource* resource, gfx::Size& size) = 0;

private:
  void deleteAllChildren();
  void paintResource(ui::Graphics* g, gfx::Rect& bounds, Resource* resource);
  gfx::Size resourceSizeHint(Resource* resource);

  void onTick();
  void stop();

  std::unique_ptr<ResourcesLoader> m_resourcesLoader;
  ui::Timer m_resourcesTimer;
  bool m_reloadOnOpen = false;

  class LoadingItem;
  LoadingItem* m_loadingItem = nullptr;
};

} // namespace app

#endif
