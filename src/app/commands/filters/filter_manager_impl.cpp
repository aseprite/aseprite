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
#include "app/cmd/patch_cel.h"
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
  , m_src(nullptr)
  , m_dst(nullptr)
  , m_previewMask(nullptr)
  , m_progressDelegate(NULL)
{
  m_row = 0;
  m_celX = 0;
  m_celY = 0;
  m_mask = NULL;
  m_targetOrig = TARGET_ALL_CHANNELS;
  m_target = TARGET_ALL_CHANNELS;

  int x, y;
  Image* image = m_site.image(&x, &y);
  if (!image)
    throw NoImageException();

  init(m_site.layer(), image, x, y);
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

  updateBounds(m_mask, m_src);
}

void FilterManagerImpl::beginForPreview()
{
  Document* document = static_cast<app::Document*>(m_site.document());

  if (document->isMaskVisible())
    m_previewMask.reset(new Mask(*document->mask()));
  else {
    m_previewMask.reset(new Mask());
    m_previewMask->replace(
      gfx::Rect(
        m_celX, m_celY,
        m_src->width(),
        m_src->height()));
  }

  m_row = 0;
  m_mask = m_previewMask;

  {
    Editor* editor = current_editor;
    Sprite* sprite = m_site.sprite();
    gfx::Rect vp = View::getView(editor)->viewportBounds();
    vp = editor->screenToEditor(vp);
    vp = vp.createIntersection(sprite->bounds());

    if (vp.isEmpty()) {
      m_previewMask.reset(nullptr);
      m_row = -1;
      return;
    }

    m_previewMask->intersect(vp);
  }

  if (!updateBounds(m_mask, m_src)) {
    m_previewMask.reset(nullptr);
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
  if (m_row < 0 || m_row >= m_bounds.h)
    return false;

  if ((m_mask) && (m_mask->bitmap())) {
    int x = m_bounds.x - m_mask->bounds().x + m_celX;
    int y = m_bounds.y - m_mask->bounds().y + m_celY + m_row;

    if ((m_bounds.w - x < 1) ||
        (m_bounds.h - y < 1))
      return false;

    m_maskBits = m_mask->bitmap()
      ->lockBits<BitmapTraits>(Image::ReadLock,
        gfx::Rect(x, y, m_bounds.w - x, m_bounds.h - y));

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
      m_progressDelegate->reportProgress(m_progressBase + m_progressWidth * (m_row+1) / m_bounds.h);

      // Does the user cancelled the whole process?
      cancelled = m_progressDelegate->isCancelled();
    }
  }

  if (!cancelled) {
    // Copy "dst" to "src"
    transaction.execute(
      new cmd::CopyRect(
        m_src, m_dst.get(),
        gfx::Clip(m_bounds.x, m_bounds.y,
                  m_bounds.x, m_bounds.y,
                  m_bounds.w, m_bounds.h)));
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
      applyToImage(
        transaction, it->layer(),
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
          m_bounds.x+m_celX,
          m_bounds.y+m_celY+m_row-1)),
      gfx::Size(
        editor->zoom().apply(m_bounds.w),
        (editor->zoom().scale() >= 1 ? editor->zoom().apply(1):
                                       editor->zoom().remove(1))));

    gfx::Region reg1(rect);
    gfx::Region reg2;
    editor->getDrawableRegion(reg2, Widget::kCutTopWindows);
    reg1.createIntersection(reg1, reg2);

    editor->invalidateRegion(reg1);
  }
}

const void* FilterManagerImpl::getSourceAddress()
{
  return m_src->getPixelAddress(m_bounds.x, m_row+m_bounds.y);
}

void* FilterManagerImpl::getDestinationAddress()
{
  return m_dst->getPixelAddress(m_bounds.x, m_row+m_bounds.y);
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

void FilterManagerImpl::init(const Layer* layer, Image* image, int x, int y)
{
  m_celX = x;
  m_celY = y;

  if (!updateBounds(static_cast<app::Document*>(m_site.document())->mask(), image))
    throw InvalidAreaException();

  m_src = image;
  m_dst.reset(Image::createCopy(m_src));

  m_row = -1;
  m_mask = NULL;
  m_previewMask.reset(nullptr);

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

bool FilterManagerImpl::updateBounds(Mask* mask, const Image* image)
{
  gfx::Rect bounds;

  if (mask && mask->bitmap()) {
    bounds = mask->bounds();
    bounds.offset(-m_celX, -m_celY);
    bounds &= image->bounds();
  }
  else {
    bounds = image->bounds();
  }

  if (bounds.isEmpty()) {
    m_bounds = gfx::Rect(0, 0, 0, 0);
    return false;
  }
  else {
    m_bounds = bounds;
    return true;
  }
}

} // namespace app
