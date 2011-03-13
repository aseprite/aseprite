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

#include "commands/filters/filter_manager_impl.h"

#include "core/cfg.h"
#include "filters/filter.h"
#include "gui/manager.h"
#include "gui/rect.h"
#include "gui/region.h"
#include "gui/view.h"
#include "gui/widget.h"
#include "modules/editors.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/images_collector.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo_history.h"
#include "widgets/editor.h"

#include <cstdlib>
#include <cstring>

using namespace std;

FilterManagerImpl::FilterManagerImpl(Sprite* sprite, Filter* filter)
  : m_sprite(sprite)
  , m_filter(filter)
  , m_progressDelegate(NULL)
{
  int offset_x, offset_y;

  m_src = NULL;
  m_dst = NULL;
  m_row = 0;
  m_offset_x = 0;
  m_offset_y = 0;
  m_mask = NULL;
  m_preview_mask = NULL;
  m_mask_address = NULL;
  m_targetOrig = TARGET_ALL_CHANNELS;
  m_target = TARGET_ALL_CHANNELS;

  Image* image = m_sprite->getCurrentImage(&offset_x, &offset_y);
  if (image == NULL)
    throw NoImageException();

  init(sprite->getCurrentLayer(), image, offset_x, offset_y);
}

FilterManagerImpl::~FilterManagerImpl()
{
  if (m_preview_mask)
    mask_free(m_preview_mask);

  if (m_dst)
    image_free(m_dst);
}

void FilterManagerImpl::setProgressDelegate(IProgressDelegate* progressDelegate)
{
  m_progressDelegate = progressDelegate;
}

void FilterManagerImpl::setTarget(int target)
{
  m_targetOrig = target;
  m_target = target;

  /* the alpha channel of the background layer can't be modified */
  if (m_sprite->getCurrentLayer() &&
      m_sprite->getCurrentLayer()->is_background())
    m_target &= ~TARGET_ALPHA_CHANNEL;
}

void FilterManagerImpl::begin()
{
  m_row = 0;
  m_mask = m_sprite->getMask();

  updateMask(m_mask, m_src);
}

void FilterManagerImpl::beginForPreview()
{
  if (m_preview_mask) {
    mask_free(m_preview_mask);
    m_preview_mask = NULL;
  }

  if ((m_sprite->getMask()) && (m_sprite->getMask()->bitmap))
    m_preview_mask = mask_new_copy(m_sprite->getMask());
  else {
    m_preview_mask = mask_new();
    mask_replace(m_preview_mask,
		 m_offset_x, m_offset_y,
		 m_src->w, m_src->h);
  }

  m_row = 0;
  m_mask = m_preview_mask;

  {
    Editor* editor = current_editor;
    gfx::Rect vp = View::getView(editor)->getViewportBounds();
    int x1, y1, x2, y2;
    int x, y, w, h;

    editor->screen_to_editor(vp.x, vp.y, &x1, &y1);
    editor->screen_to_editor(vp.x+vp.w-1, vp.y+vp.h-1, &x2, &y2);

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= m_sprite->getWidth()) x2 = m_sprite->getWidth()-1;
    if (y2 >= m_sprite->getHeight()) y2 = m_sprite->getHeight()-1;

    x = x1;
    y = y1;
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;

    if ((w < 1) || (h < 1)) {
      mask_free(m_preview_mask);
      m_preview_mask = NULL;
      m_row = -1;
      return;
    }

    mask_intersect(m_preview_mask, x, y, w, h);
  }

  if (!updateMask(m_mask, m_src)) {
    mask_free(m_preview_mask);
    m_preview_mask = NULL;
    m_row = -1;
    return;
  }
}

bool FilterManagerImpl::applyStep()
{
  if ((m_row >= 0) && (m_row < m_h)) {
    if ((m_mask) && (m_mask->bitmap)) {
      m_d = div(m_x-m_mask->x+m_offset_x, 8);
      m_mask_address = ((ase_uint8 **)m_mask->bitmap->line)[m_row+m_y-m_mask->y+m_offset_y]+m_d.quot;
    }
    else
      m_mask_address = NULL;

    switch (m_sprite->getImgType()) {
      case IMAGE_RGB:       m_filter->applyToRgba(this); break;
      case IMAGE_GRAYSCALE: m_filter->applyToGrayscale(this); break;
      case IMAGE_INDEXED:   m_filter->applyToIndexed(this); break;
    }
    ++m_row;

    return true;
  }
  else {
    return false;
  }
}

void FilterManagerImpl::apply()
{
  bool cancelled = false;

  begin();
  while (!cancelled && applyStep()) {
    if (m_progressDelegate) {
      // Report progress.
      m_progressDelegate->reportProgress(m_progressBase + m_progressWidth * (m_row+1) / m_h);

      // Does the user cancelled the whole process?
      cancelled = m_progressDelegate->isCancelled();
    }
  }

  if (!cancelled) {
    // Undo stuff
    if (m_sprite->getUndo()->isEnabled()) {
      m_sprite->getUndo()->setLabel(m_filter->getName());
      m_sprite->getUndo()->undo_image(m_src, m_x, m_y, m_w, m_h);
    }

    // Copy "dst" to "src"
    image_copy(m_src, m_dst, 0, 0);
  }
}

void FilterManagerImpl::applyToTarget()
{
  bool cancelled = false;

  ImagesCollector images(m_sprite,
			 (m_target & TARGET_ALL_LAYERS) == TARGET_ALL_LAYERS,
			 (m_target & TARGET_ALL_FRAMES) == TARGET_ALL_FRAMES,
			 true);	// we will write in each image
  if (images.empty())
    return;

  // Initialize writting operation
  SpriteReader reader(m_sprite);
  SpriteWriter writer(reader);

  // Open group of undo operations
  if (images.size() > 1) {
    if (m_sprite->getUndo()->isEnabled())
      m_sprite->getUndo()->undo_open();
  }
  
  m_progressBase = 0.0f;
  m_progressWidth = 1.0f / images.size();

  // For each target image
  for (ImagesCollector::ItemsIterator it = images.begin();
       it != images.end() && !cancelled;
       ++it) {
    applyToImage(it->layer(), it->image(), it->cel()->x, it->cel()->y);

    // Is there a delegate to know if the process was cancelled by the user?
    if (m_progressDelegate)
      cancelled = m_progressDelegate->isCancelled();

    // Make progress
    m_progressBase += m_progressWidth;
  }

  // Close group of undo operations
  if (images.size() > 1) {
    if (m_sprite->getUndo()->isEnabled())
      m_sprite->getUndo()->undo_close();
  }
}

void FilterManagerImpl::flush()
{
  if (m_row >= 0) {
    JRegion reg1, reg2;
    struct jrect rect;
    Editor* editor = current_editor;

    reg1 = jregion_new(NULL, 0);

    editor->editor_to_screen(m_x+m_offset_x,
			     m_y+m_offset_y+m_row-1,
			     &rect.x1, &rect.y1);
    rect.x2 = rect.x1 + (m_w << editor->editor_get_zoom());
    rect.y2 = rect.y1 + (1 << editor->editor_get_zoom());

    reg2 = jregion_new(&rect, 1);
    jregion_union(reg1, reg1, reg2);
    jregion_free(reg2);

    reg2 = jwidget_get_drawable_region(editor, JI_GDR_CUTTOPWINDOWS);
    jregion_intersect(reg1, reg1, reg2);
    jregion_free(reg2);

    editor->invalidateRegion(reg1);
    jregion_free(reg1);
  }
}

const void* FilterManagerImpl::getSourceAddress()
{
  switch (m_sprite->getImgType()) {
    case IMAGE_RGB:       return ((ase_uint32**)m_src->line)[m_row+m_y]+m_x;
    case IMAGE_GRAYSCALE: return ((ase_uint16**)m_src->line)[m_row+m_y]+m_x;
    case IMAGE_INDEXED:   return ((ase_uint8**)m_src->line)[m_row+m_y]+m_x;
  }
  return NULL;
}

void* FilterManagerImpl::getDestinationAddress()
{
  switch (m_sprite->getImgType()) {
    case IMAGE_RGB:       return ((ase_uint32**)m_dst->line)[m_row+m_y]+m_x;
    case IMAGE_GRAYSCALE: return ((ase_uint16**)m_dst->line)[m_row+m_y]+m_x;
    case IMAGE_INDEXED:   return ((ase_uint8**)m_dst->line)[m_row+m_y]+m_x;
  }
  return NULL;
}

bool FilterManagerImpl::skipPixel()
{
  bool skip = false;

  if (m_mask_address) {
    if (!((*m_mask_address) & (1<<m_d.rem)))
      skip = true;

    // Move to the next pixel in the mask.
    _image_bitmap_next_bit(m_d, m_mask_address);
  }

  return skip;
}

Palette* FilterManagerImpl::getPalette()
{
  return m_sprite->getCurrentPalette();
}

RgbMap* FilterManagerImpl::getRgbMap()
{
  return m_sprite->getRgbMap();
}

void FilterManagerImpl::init(const Layer* layer, Image* image, int offset_x, int offset_y)
{
  m_offset_x = offset_x;
  m_offset_y = offset_y;

  if (!updateMask(m_sprite->getMask(), image))
    throw InvalidAreaException();

  if (m_preview_mask) {
    mask_free(m_preview_mask);
    m_preview_mask = NULL;
  }

  if (m_dst) {
    image_free(m_dst);
    m_dst = NULL;
  }

  m_src = image;
  m_dst = image_crop(image, 0, 0, image->w, image->h, 0);
  m_row = -1;
  m_mask = NULL;
  m_preview_mask = NULL;
  m_mask_address = NULL;

  m_target = m_targetOrig;

  /* the alpha channel of the background layer can't be modified */
  if (layer->is_background())
    m_target &= ~TARGET_ALPHA_CHANNEL;
}

void FilterManagerImpl::applyToImage(Layer* layer, Image* image, int x, int y)
{
  init(layer, image, x, y);
  apply();
}

bool FilterManagerImpl::updateMask(Mask* mask, const Image* image)
{
  int x, y, w, h;

  if ((mask) && (mask->bitmap)) {
    x = mask->x - m_offset_x;
    y = mask->y - m_offset_y;
    w = mask->w;
    h = mask->h;

    if (x < 0) {
      w += x;
      x = 0;
    }

    if (y < 0) {
      h += y;
      y = 0;
    }

    if (x+w-1 >= image->w-1)
      w = image->w-x;

    if (y+h-1 >= image->h-1)
      h = image->h-y;
  }
  else {
    x = 0;
    y = 0;
    w = image->w;
    h = image->h;
  }

  if ((w < 1) || (h < 1)) {
    m_x = 0;
    m_y = 0;
    m_w = 0;
    m_h = 0;
    return false;
  }
  else {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    return true;
  }
}
