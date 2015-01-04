/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_manager_impl.h"

#include "app/context_access.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/ui/editor/editor.h"
#include "app/undo_transaction.h"
#include "app/undoers/image_area.h"
#include "filters/filter.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/images_collector.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/view.h"
#include "ui/widget.h"

#include <cstdlib>
#include <cstring>

namespace app {

using namespace std;
using namespace ui;
  
FilterManagerImpl::FilterManagerImpl(Context* context, Filter* filter)
  : m_context(context)
  , m_location(context->activeLocation())
  , m_filter(filter)
  , m_dst(NULL)
  , m_preview_mask(NULL)
  , m_progressDelegate(NULL)
{
  int offset_x, offset_y;

  m_src = NULL;
  m_row = 0;
  m_offset_x = 0;
  m_offset_y = 0;
  m_mask = NULL;
  m_targetOrig = TARGET_ALL_CHANNELS;
  m_target = TARGET_ALL_CHANNELS;

  Image* image = m_location.image(&offset_x, &offset_y);
  if (image == NULL)
    throw NoImageException();

  init(m_location.layer(), image, offset_x, offset_y);
}

FilterManagerImpl::~FilterManagerImpl()
{
}

void FilterManagerImpl::setProgressDelegate(IProgressDelegate* progressDelegate)
{
  m_progressDelegate = progressDelegate;
}

PixelFormat FilterManagerImpl::pixelFormat() const
{
  return m_location.sprite()->pixelFormat();
}

void FilterManagerImpl::setTarget(int target)
{
  m_targetOrig = target;
  m_target = target;

  // The alpha channel of the background layer can't be modified.
  if (m_location.layer() &&
      m_location.layer()->isBackground())
    m_target &= ~TARGET_ALPHA_CHANNEL;
}

void FilterManagerImpl::begin()
{
  Document* document = m_location.document();

  m_row = 0;
  m_mask = (document->isMaskVisible() ? document->mask(): NULL);

  updateMask(m_mask, m_src);
}

void FilterManagerImpl::beginForPreview()
{
  Document* document = m_location.document();

  if (document->isMaskVisible())
    m_preview_mask.reset(new Mask(*document->mask()));
  else {
    m_preview_mask.reset(new Mask());
    m_preview_mask->replace(
      gfx::Rect(m_offset_x, m_offset_y,
        m_src->width(),
        m_src->height()));
  }

  m_row = 0;
  m_mask = m_preview_mask;

  {
    Editor* editor = current_editor;
    Sprite* sprite = m_location.sprite();
    gfx::Rect vp = View::getView(editor)->getViewportBounds();
    vp = editor->screenToEditor(vp);
    vp = vp.createIntersect(sprite->bounds());

    if (vp.isEmpty()) {
      m_preview_mask.reset(NULL);
      m_row = -1;
      return;
    }

    m_preview_mask->intersect(vp);
  }

  if (!updateMask(m_mask, m_src)) {
    m_preview_mask.reset(NULL);
    m_row = -1;
    return;
  }
}

void FilterManagerImpl::end()
{
  m_maskBits.unlock();
}

bool FilterManagerImpl::applyStep()
{
  if (m_row < 0 || m_row >= m_h)
    return false;

  if ((m_mask) && (m_mask->bitmap())) {
    int x = m_x - m_mask->bounds().x + m_offset_x;
    int y = m_y - m_mask->bounds().y + m_offset_y + m_row;

    if ((m_w - x < 1) || (m_h - y < 1))
      return false;

    m_maskBits = m_mask->bitmap()
      ->lockBits<BitmapTraits>(Image::ReadLock,
        gfx::Rect(x, y, m_w - x, m_h - y));

    m_maskIterator = m_maskBits.begin();
  }

  switch (m_location.sprite()->pixelFormat()) {
    case IMAGE_RGB:       m_filter->applyToRgba(this); break;
    case IMAGE_GRAYSCALE: m_filter->applyToGrayscale(this); break;
    case IMAGE_INDEXED:   m_filter->applyToIndexed(this); break;
  }
  ++m_row;

  return true;
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
    UndoTransaction undo(m_context, m_filter->getName(), undo::ModifyDocument);

    // Undo stuff
    if (undo.isEnabled())
      undo.pushUndoer(new undoers::ImageArea(undo.getObjects(), m_src, m_x, m_y, m_w, m_h));

    // Copy "dst" to "src"
    copy_image(m_src, m_dst);

    undo.commit();
  }
}

void FilterManagerImpl::applyToTarget()
{
  bool cancelled = false;

  ImagesCollector images((m_target & TARGET_ALL_LAYERS ?
                          m_location.sprite()->folder():
                          m_location.layer()),
                         m_location.frame(),
                         (m_target & TARGET_ALL_FRAMES) == TARGET_ALL_FRAMES,
                         true); // we will write in each image
  if (images.empty())
    return;

  // Initialize writting operation
  ContextReader reader(m_context);
  ContextWriter writer(reader);
  UndoTransaction undo(writer.context(), m_filter->getName(), undo::ModifyDocument);

  m_progressBase = 0.0f;
  m_progressWidth = 1.0f / images.size();

  // For each target image
  for (ImagesCollector::ItemsIterator it = images.begin();
       it != images.end() && !cancelled;
       ++it) {
    applyToImage(it->layer(), it->image(), it->cel()->x(), it->cel()->y());

    // Is there a delegate to know if the process was cancelled by the user?
    if (m_progressDelegate)
      cancelled = m_progressDelegate->isCancelled();

    // Make progress
    m_progressBase += m_progressWidth;
  }

  undo.commit();
}

void FilterManagerImpl::flush()
{
  if (m_row >= 0) {
    Editor* editor = current_editor;
    gfx::Rect rect(
      editor->editorToScreen(
        gfx::Point(
          m_x+m_offset_x,
          m_y+m_offset_y+m_row-1)),
      gfx::Size(
        editor->zoom().apply(m_w),
        editor->zoom().apply(1)));

    gfx::Region reg1(rect);
    gfx::Region reg2;
    editor->getDrawableRegion(reg2, Widget::kCutTopWindows);
    reg1.createIntersection(reg1, reg2);

    editor->invalidateRegion(reg1);
  }
}

const void* FilterManagerImpl::getSourceAddress()
{
  return m_src->getPixelAddress(m_x, m_row+m_y);
}

void* FilterManagerImpl::getDestinationAddress()
{
  return m_dst->getPixelAddress(m_x, m_row+m_y);
}

bool FilterManagerImpl::skipPixel()
{
  bool skip = false;

  if ((m_mask) && (m_mask->bitmap())) {
    if (!*m_maskIterator)
      skip = true;

    ++m_maskIterator;
  }

  return skip;
}

Palette* FilterManagerImpl::getPalette()
{
  return m_location.sprite()->palette(m_location.frame());
}

RgbMap* FilterManagerImpl::getRgbMap()
{
  return m_location.sprite()->rgbMap(m_location.frame());
}

void FilterManagerImpl::init(const Layer* layer, Image* image, int offset_x, int offset_y)
{
  m_offset_x = offset_x;
  m_offset_y = offset_y;

  if (!updateMask(m_location.document()->mask(), image))
    throw InvalidAreaException();

  m_src = image;
  m_dst.reset(crop_image(image, 0, 0, image->width(), image->height(), 0));
  m_row = -1;
  m_mask = NULL;
  m_preview_mask.reset(NULL);

  m_target = m_targetOrig;

  /* the alpha channel of the background layer can't be modified */
  if (layer->isBackground())
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

  if (mask && mask->bitmap()) {
    x = mask->bounds().x - m_offset_x;
    y = mask->bounds().y - m_offset_y;
    w = mask->bounds().w;
    h = mask->bounds().h;

    if (x < 0) {
      w += x;
      x = 0;
    }

    if (y < 0) {
      h += y;
      y = 0;
    }

    if (x+w-1 >= image->width()-1)
      w = image->width()-x;

    if (y+h-1 >= image->height()-1)
      h = image->height()-y;
  }
  else {
    x = 0;
    y = 0;
    w = image->width();
    h = image->height();
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

} // namespace app
