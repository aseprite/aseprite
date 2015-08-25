// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/resources_listbox.h"

#include "app/res/resource.h"
#include "app/res/resources_loader.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

class ResourceListItem : public ListItem {
public:
  ResourceListItem(Resource* resource)
    : ListItem(resource->name()), m_resource(resource) {
  }

  Resource* resource() const {
    return m_resource;
  }

protected:
  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case kMouseLeaveMessage:
      case kMouseEnterMessage:
        invalidate();
        break;
    }
    return ListItem::onProcessMessage(msg);
  }

  void onPaint(PaintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    Graphics* g = ev.getGraphics();
    gfx::Rect bounds = getClientBounds();
    gfx::Color bgcolor, fgcolor;

    if (isSelected()) {
      bgcolor = theme->colors.listitemSelectedFace();
      fgcolor = theme->colors.listitemSelectedText();
    }
    else {
      bgcolor = theme->colors.listitemNormalFace();
      fgcolor = theme->colors.listitemNormalText();
    }

    g->fillRect(bgcolor, bounds);

    static_cast<ResourcesListBox*>(getParent())->
      paintResource(g, bounds, m_resource);

    // for (int i=0; i<m_palette->size(); ++i) {
    //   doc::color_t c = m_resource->getEntry(i);

    //   g->fillRect(gfx::rgba(
    //       doc::rgba_getr(c),
    //       doc::rgba_getg(c),
    //       doc::rgba_getb(c)), box);

    //   box.x += box.w;
    // }

    g->drawString(getText(), fgcolor, gfx::ColorNone,
      gfx::Point(
        bounds.x + guiscale()*2,
        bounds.y + bounds.h/2 - g->measureUIString(getText()).h/2));
  }

  void onPreferredSize(PreferredSizeEvent& ev) override {
    ev.setPreferredSize(
      static_cast<ResourcesListBox*>(getParent())->
        preferredResourceSize(m_resource));
  }

private:
  base::UniquePtr<Resource> m_resource;
};

class ResourcesListBox::LoadingItem : public ListItem {
public:
  LoadingItem()
    : ListItem("Loading")
    , m_state(0) {
  }

  void makeProgress() {
    std::string text = "Loading ";

    switch ((++m_state) % 4) {
      case 0: text += "/"; break;
      case 1: text += "-"; break;
      case 2: text += "\\"; break;
      case 3: text += "|"; break;
    }

    setText(text);
  }

private:
  int m_state;
};

ResourcesListBox::ResourcesListBox(ResourcesLoader* resourcesLoader)
  : m_resourcesLoader(resourcesLoader)
  , m_resourcesTimer(100)
  , m_loadingItem(NULL)
{
  m_resourcesTimer.Tick.connect(Bind<void>(&ResourcesListBox::onTick, this));
}

Resource* ResourcesListBox::selectedResource()
{
  if (ResourceListItem* listItem = dynamic_cast<ResourceListItem*>(getSelectedChild()))
    return listItem->resource();
  else
    return NULL;
}

void ResourcesListBox::paintResource(Graphics* g, const gfx::Rect& bounds, Resource* resource)
{
  onPaintResource(g, bounds, resource);
}

gfx::Size ResourcesListBox::preferredResourceSize(Resource* resource)
{
  gfx::Size pref(0, 0);
  onResourcePreferredSize(resource, pref);
  return pref;
}

bool ResourcesListBox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      m_resourcesTimer.start();
      break;
    }

  }
  return ListBox::onProcessMessage(msg);
}

void ResourcesListBox::onChange()
{
  Resource* resource = selectedResource();
  if (resource)
    onResourceChange(resource);
}

void ResourcesListBox::onResourceChange(Resource* resource)
{
  // Do nothing
}

void ResourcesListBox::onPaintResource(Graphics* g, const gfx::Rect& bounds, Resource* resource)
{
  // Do nothing
}

void ResourcesListBox::onTick()
{
  if (m_resourcesLoader == NULL) {
    stop();
    return;
  }

  if (!m_loadingItem) {
    m_loadingItem = new LoadingItem;
    addChild(m_loadingItem);
  }
  m_loadingItem->makeProgress();

  base::UniquePtr<Resource> resource;
  std::string name;

  if (!m_resourcesLoader->next(resource)) {
    if (m_resourcesLoader->isDone()) {
      stop();

      PRINTF("Done\n");
    }
    return;
  }

  base::UniquePtr<ResourceListItem> listItem(new ResourceListItem(resource));
  insertChild(getItemsCount()-1, listItem);
  layout();

  View* view = View::getView(this);
  if (view)
    view->updateView();

  resource.release();
  listItem.release();
}

void ResourcesListBox::stop()
{
  if (m_loadingItem) {
    removeChild(m_loadingItem);
    delete m_loadingItem;
    m_loadingItem = NULL;

    invalidate();
  }

  m_resourcesTimer.stop();
}

} // namespace app
