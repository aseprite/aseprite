// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "doc/algorithm/shrink_bounds.h"
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
  , m_cel(nullptr)
  , m_src(nullptr)
  , m_dst(nullptr)
  , m_mask(nullptr)
  , m_previewMask(nullptr)
  , m_progressDelegate(NULL)
{
  m_row = 0;
  m_targetOrig = TARGET_ALL_CHANNELS;
  m_target = TARGET_ALL_CHANNELS;

  int x, y;
  Image* image = m_site.image(&x, &y);
  if (!image)
    throw NoImageException();

  init(m_site.cel());
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
  m_mask = (document->isMaskVisible() ? document->mask(): nullptr);
  updateBounds(m_mask);
}

void FilterManagerImpl::beginForPreview()
{
  Document* document = static_cast<app::Document*>(m_site.document());

  if (document->isMaskVisible())
    m_previewMask.reset(new Mask(*document->mask()));
  else {
    m_previewMask.reset(new Mask());
    m_previewMask->replace(m_site.sprite()->bounds());
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

  if (!updateBounds(m_mask)) {
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

  if (m_mask && m_mask->bitmap()) {
    int x = m_bounds.x - m_mask->bounds().x;
    int y = m_bounds.y - m_mask->bounds().y + m_row;
    if ((x >= m_bounds.w) ||
        (y >= m_bounds.h))
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
    gfx::Rect output;
    if (algorithm::shrink_bounds2(m_src.get(), m_dst.get(),
                                  m_bounds, output)) {
      // Patch "m_cel"
      transaction.execute(
        new cmd::PatchCel(
          m_cel, m_dst.get(),
          gfx::Region(output),
          position()));
    }
  }
}

void FilterManagerImpl::applyToTarget()
{
  bool cancelled = false;

  ImagesCollector images((m_target & TARGET_ALL_LAYERS ?
                          m_site.sprite()->root():
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
      applyToCel(transaction, it->cel());
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
          m_bounds.x,
          m_bounds.y+m_row-1)),
      gfx::Size(
        editor->projection().applyX(m_bounds.w),
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
  return m_src->getPixelAddress(m_bounds.x, m_bounds.y+m_row);
}

void* FilterManagerImpl::getDestinationAddress()
{
  return m_dst->getPixelAddress(m_bounds.x, m_bounds.y+m_row);
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

void FilterManagerImpl::init(Cel* cel)
{
  ASSERT(cel);
  if (!updateBounds(static_cast<app::Document*>(m_site.document())->mask()))
    throw InvalidAreaException();

  m_cel = cel;
  m_src.reset(
    crop_image(
      cel->image(),
      gfx::Rect(m_site.sprite()->bounds()).offset(-cel->position()), 0));
  m_dst.reset(Image::createCopy(m_src.get()));

  m_row = -1;
  m_mask = nullptr;
  m_previewMask.reset(nullptr);

  m_target = m_targetOrig;

  // The alpha channel of the background layer can't be modified
  if (cel->layer()->isBackground())
    m_target &= ~TARGET_ALPHA_CHANNEL;
}

void FilterManagerImpl::applyToCel(Transaction& transaction, Cel* cel)
{
  init(cel);
  apply(transaction);
}

bool FilterManagerImpl::updateBounds(doc::Mask* mask)
{
  gfx::Rect bounds;
  if (mask && mask->bitmap() && !mask->bounds().isEmpty()) {
    bounds = mask->bounds();
    bounds &= m_site.sprite()->bounds();
  }
  else {
    bounds = m_site.sprite()->bounds();
  }
  m_bounds = bounds;
  return !m_bounds.isEmpty();
}

} // namespace app
