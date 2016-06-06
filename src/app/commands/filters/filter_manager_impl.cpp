// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_manager_impl.h"

#include "app/cmd/copy_rect.h"
#include "app/cmd/unlink_cel.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/transaction.h"
#include "app/ui/editor/editor.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/images_collector.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "filters/filter.h"
#include "ui/manager.h"
#include "ui/view.h"
#include "ui/widget.h"

#include <cstdlib>
#include <cstring>
#include <set>

namespace app {

using namespace std;
using namespace ui;

FilterManagerImpl::FilterManagerImpl(Context* context, Filter* filter)
  : m_context(context)
  , m_site(context->activeSite())
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

  Image* image = m_site.image(&offset_x, &offset_y);
  if (image == NULL)
    throw NoImageException();

  init(m_site.layer(), image, offset_x, offset_y);
}

FilterManagerImpl::~FilterManagerImpl()
{
}

app::Document* FilterManagerImpl::document()
{
  return static_cast<app::Document*>(m_site.document());
}

void FilterManagerImpl::setProgressDelegate(IProgressDelegate* progressDelegate)
{
  m_progressDelegate = progressDelegate;
}

PixelFormat FilterManagerImpl::pixelFormat() const
{
  return m_site.sprite()->pixelFormat();
}

void FilterManagerImpl::setTarget(int target)
{
  m_targetOrig = target;
  m_target = target;

  // The alpha channel of the background layer can't be modified.
  if (m_site.layer() &&
      m_site.layer()->isBackground())
    m_target &= ~TARGET_ALPHA_CHANNEL;
}

void FilterManagerImpl::begin()
{
  Document* document = static_cast<app::Document*>(m_site.document());

  m_row = 0;
  m_mask = (document->isMaskVisible() ? document->mask(): NULL);

  updateMask(m_mask, m_src);
}

void FilterManagerImpl::beginForPreview()
{
  Document* document = static_cast<app::Document*>(m_site.document());

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
    Sprite* sprite = m_site.sprite();
    gfx::Rect vp = View::getView(editor)->viewportBounds();
    vp = editor->screenToEditor(vp);
    vp = vp.createIntersection(sprite->bounds());

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

  switch (m_site.sprite()->pixelFormat()) {
    case IMAGE_RGB:       m_filter->applyToRgba(this); break;
    case IMAGE_GRAYSCALE: m_filter->applyToGrayscale(this); break;
    case IMAGE_INDEXED:   m_filter->applyToIndexed(this); break;
  }
  ++m_row;

  return true;
}

void FilterManagerImpl::apply(Transaction& transaction)
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
    // Copy "dst" to "src"
    transaction.execute(new cmd::CopyRect(
        m_src, m_dst, gfx::Clip(m_x, m_y, m_x, m_y, m_w, m_h)));
  }
}

void FilterManagerImpl::applyToTarget()
{
  bool cancelled = false;

  ImagesCollector images((m_target & TARGET_ALL_LAYERS ?
                          m_site.sprite()->folder():
                          m_site.layer()),
                         m_site.frame(),
                         (m_target & TARGET_ALL_FRAMES) == TARGET_ALL_FRAMES,
                         true); // we will write in each image
  if (images.empty())
    return;

  // Initialize writting operation
  ContextReader reader(m_context);
  ContextWriter writer(reader);
  Transaction transaction(writer.context(), m_filter->getName(), ModifyDocument);

  m_progressBase = 0.0f;
  m_progressWidth = 1.0f / images.size();

  std::set<ObjectId> visited;

  // For each target image
  for (auto it = images.begin();
       it != images.end() && !cancelled;
       ++it) {
    Image* image = it->image();

    // Avoid applying the filter two times to the same image
    if (visited.find(image->id()) == visited.end()) {
      visited.insert(image->id());
      applyToImage(transaction, it->layer(),
        image, it->cel()->x(), it->cel()->y());
    }

    // Is there a delegate to know if the process was cancelled by the user?
    if (m_progressDelegate)
      cancelled = m_progressDelegate->isCancelled();

    // Make progress
    m_progressBase += m_progressWidth;
  }

  transaction.commit();
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
        editor->projection().applyX(m_w),
        (editor->projection().scaleY() >= 1 ? editor->projection().applyY(1):
                                              editor->projection().removeY(1))));

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
  return m_site.sprite()->palette(m_site.frame());
}

RgbMap* FilterManagerImpl::getRgbMap()
{
  return m_site.sprite()->rgbMap(m_site.frame());
}

void FilterManagerImpl::init(const Layer* layer, Image* image, int offset_x, int offset_y)
{
  m_offset_x = offset_x;
  m_offset_y = offset_y;

  if (!updateMask(static_cast<app::Document*>(m_site.document())->mask(), image))
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

void FilterManagerImpl::applyToImage(Transaction& transaction, Layer* layer, Image* image, int x, int y)
{
  init(layer, image, x, y);
  apply(transaction);
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
