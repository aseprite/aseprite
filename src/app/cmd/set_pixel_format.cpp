// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_pixel_format.h"

#include "app/cmd/remove_palette.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_palette.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/document.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"
#include "render/task_delegate.h"

namespace app {
namespace cmd {

using namespace doc;

namespace {

class SuperDelegate : public render::TaskDelegate {
public:
  SuperDelegate(int ncels, render::TaskDelegate* delegate)
    : m_ncels(ncels)
    , m_curCel(0)
    , m_delegate(delegate) {
  }

  void notifyTaskProgress(double progress) override {
    if (m_delegate)
      m_delegate->notifyTaskProgress(
        (progress + m_curCel) / m_ncels);
  }

  bool continueTask() override {
    if (m_delegate)
      return m_delegate->continueTask();
    else
      return true;
  }

  void nextCel() {
    ++m_curCel;
  }

private:
  int m_ncels;
  int m_curCel;
  TaskDelegate* m_delegate;
};

} // anonymous namespace

SetPixelFormat::SetPixelFormat(Sprite* sprite,
                               const PixelFormat newFormat,
                               const render::Dithering& dithering,
                               doc::rgba_to_graya_func toGray,
                               render::TaskDelegate* delegate)
  : WithSprite(sprite)
  , m_oldFormat(sprite->pixelFormat())
  , m_newFormat(newFormat)
{
  if (sprite->pixelFormat() == newFormat)
    return;

  SuperDelegate superDel(sprite->uniqueCels().size(), delegate);

  for (Cel* cel : sprite->uniqueCels()) {
    ImageRef old_image = cel->imageRef();
    ImageRef new_image(
      render::convert_pixel_format
      (old_image.get(), nullptr, newFormat,
       dithering,
       sprite->rgbMap(cel->frame()),
       sprite->palette(cel->frame()),
       cel->layer()->isBackground(),
       old_image->maskColor(),
       toGray,
       &superDel));

    m_seq.add(new cmd::ReplaceImage(sprite, old_image, new_image));
    superDel.nextCel();
  }

  // Set all cels opacity to 100% if we are converting to indexed.
  // TODO remove this
  if (newFormat == IMAGE_INDEXED) {
    for (Cel* cel : sprite->uniqueCels()) {
      if (cel->opacity() < 255)
        m_seq.add(new cmd::SetCelOpacity(cel, 255));
    }
  }

  // When we are converting to grayscale color mode, we've to destroy
  // all palettes and put only one grayscaled-palette at the first
  // frame.
  if (newFormat == IMAGE_GRAYSCALE) {
    // Add cmds to revert all palette changes.
    PalettesList palettes = sprite->getPalettes();
    for (Palette* pal : palettes)
      if (pal->frame() != 0)
        m_seq.add(new cmd::RemovePalette(sprite, pal));

    std::unique_ptr<Palette> graypal(Palette::createGrayscale());
    if (*graypal != *sprite->palette(0))
      m_seq.add(new cmd::SetPalette(sprite, 0, graypal.get()));
  }
}

void SetPixelFormat::onExecute()
{
  m_seq.execute(context());
  setFormat(m_newFormat);
}

void SetPixelFormat::onUndo()
{
  m_seq.undo();
  setFormat(m_oldFormat);
}

void SetPixelFormat::onRedo()
{
  m_seq.redo();
  setFormat(m_newFormat);
}

void SetPixelFormat::setFormat(PixelFormat format)
{
  Sprite* sprite = this->sprite();

  sprite->setPixelFormat(format);
  sprite->incrementVersion();

  // Regenerate extras
  Doc* doc = static_cast<Doc*>(sprite->document());
  doc->setExtraCel(ExtraCelRef(nullptr));

  // Generate notification
  DocEvent ev(doc);
  ev.sprite(sprite);
  doc->notify_observers<DocEvent&>(&DocObserver::onPixelFormatChanged, ev);
}

} // namespace cmd
} // namespace app
