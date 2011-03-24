/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "app.h"
#include "base/mutex.h"
#include "base/path.h"
#include "document.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "settings/ui_settings_impl.h"
#include "ui_context.h"
#include "undo/undo_history.h"
#include "widgets/color_bar.h"
#include "widgets/tabs.h"

#include <allegro/file.h>
#include <allegro/system.h>

UIContext* UIContext::m_instance = NULL;

UIContext::UIContext()
  : Context(new UISettingsImpl)
{
  ASSERT(m_instance == NULL);
  m_instance = this;
}

UIContext::~UIContext()
{
  ASSERT(m_instance == this);
  m_instance = NULL;
}

void UIContext::onAddDocument(Document* document)
{
  // base method
  Context::onAddDocument(document);

  // add the tab for this sprite
  app_get_tabsbar()->addTab(get_filename(document->getFilename()), document);

  // Rebuild the list of tabs
  app_rebuild_documents_tabs();
}

void UIContext::onRemoveDocument(Document* document)
{
  // base method
  Context::onRemoveDocument(document);

  // Remove this document from tabs
  app_get_tabsbar()->removeTab(document);

  // Rebuild the tabs
  app_rebuild_documents_tabs();

  // Select other documents in the editors where are this sprite
  editors_hide_document(document);
}

void UIContext::onSetActiveDocument(Document* document)
{
  Context::onSetActiveDocument(document);

  // Select the document in the tabs.
  app_get_tabsbar()->selectTab(document);

  // Change the image-type of color bar.
  app_get_colorbar()->setImgType(app_get_current_image_type());

  // Change the main frame title.
  base::string defaultTitle = PACKAGE " v" VERSION;
  base::string title;
  if (document) {
    // Prepend the document's filename.
    title += base::get_file_name(document->getFilename());
    title += " - ";
  }
  title += defaultTitle;
  set_window_title(title.c_str());
}
