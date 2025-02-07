// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
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
#include "app/cmd/set_transparent_color.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/document.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "render/quantization.h"
#include "render/task_delegate.h"

namespace app { namespace cmd {

using namespace doc;

namespace {

class SuperDelegate : public render::TaskDelegate {
public:
  SuperDelegate(int nimages, render::TaskDelegate* delegate)
    : m_nimages(nimages)
    , m_curImage(0)
    , m_delegate(delegate)
  {
  }

  void notifyTaskProgress(double progress) override
  {
    if (m_delegate)
      m_delegate->notifyTaskProgress((progress + m_curImage) / m_nimages);
  }

  bool continueTask() override
  {
    if (m_delegate)
      return m_delegate->continueTask();
    else
      return true;
  }

  void nextImage() { ++m_curImage; }

private:
  int m_nimages;
  int m_curImage;
  TaskDelegate* m_delegate;
};

} // anonymous namespace

SetPixelFormat::SetPixelFormat(Sprite* sprite,
                               const PixelFormat newFormat,
                               const render::Dithering& dithering,
                               const doc::RgbMapAlgorithm mapAlgorithm,
                               doc::rgba_to_graya_func toGray,
                               render::TaskDelegate* delegate,
                               const FitCriteria fitCriteria)
  : WithSprite(sprite)
  , m_oldFormat(sprite->pixelFormat())
  , m_newFormat(newFormat)
{
  if (sprite->pixelFormat() == newFormat)
    return;

  // Calculate the number of images to convert just to show a proper
  // progress bar.
  tile_index nimages = 0;
  for (Cel* cel : sprite->uniqueCels())
    if (!cel->layer()->isTilemap())
      ++nimages;
  if (sprite->hasTilesets()) {
    for (Tileset* tileset : *sprite->tilesets()) {
      if (tileset)
        nimages += tileset->size();
    }
  }

  SuperDelegate superDel(nimages, delegate);

  // Convert cel images
  for (Cel* cel : sprite->uniqueCels()) {
    if (cel->layer()->isTilemap())
      continue;

    ImageRef oldImage = cel->imageRef();
    convertImage(sprite,
                 dithering,
                 oldImage,
                 cel->frame(),
                 cel->layer()->isBackground(),
                 mapAlgorithm,
                 toGray,
                 &superDel,
                 fitCriteria);

    superDel.nextImage();
  }

  // Convert tileset images
  if (sprite->hasTilesets()) {
    for (Tileset* tileset : *sprite->tilesets()) {
      if (!tileset)
        continue;

      for (tile_index i = 0; i < tileset->size(); ++i) {
        ImageRef oldImage = tileset->get(i);
        if (oldImage) {
          convertImage(sprite,
                       dithering,
                       oldImage,
                       0,     // TODO select a frame or generate other tilesets?
                       false, // TODO is background? it depends of the layer where this tileset is
                              // used
                       mapAlgorithm,
                       toGray,
                       &superDel,
                       fitCriteria);
        }
        superDel.nextImage();
      }
    }
  }

  // By default, when converting to RGB or grayscale, the mask color
  // is always 0.
  int newMaskIndex = 0;
  if (newFormat == IMAGE_INDEXED) {
    // Set all cels opacity to 100% if we are converting to indexed.
    // TODO remove this (?)
    for (Cel* cel : sprite->uniqueCels()) {
      if (cel->opacity() < 255)
        m_pre.add(new cmd::SetCelOpacity(cel, 255));
    }

    // When converting to indexed mode the mask color depends if the
    // palette includes a fully transparent entry.
    newMaskIndex = sprite->palette(0)->findMaskColor();
    if (newMaskIndex < 0)
      newMaskIndex = 0;

    // We change the transparent color after (m_post) changing the
    // color mode (when we are already in indexed mode).
    if (newMaskIndex != sprite->transparentColor())
      m_post.add(new cmd::SetTransparentColor(sprite, newMaskIndex));
  }
  else if (m_oldFormat == IMAGE_INDEXED) {
    // We change the transparent color before (m_pre) changing the
    // color mode (when we are still in indexed mode).
    if (newMaskIndex != sprite->transparentColor())
      m_pre.add(new cmd::SetTransparentColor(sprite, newMaskIndex));
  }
  else {
    // RGB <-> Grayscale
    ASSERT(sprite->transparentColor() == 0);
  }

  // When we are converting to grayscale color mode, we've to destroy
  // all palettes and put only one grayscaled-palette at the first
  // frame.
  if (newFormat == IMAGE_GRAYSCALE) {
    // Add cmds to revert all palette changes.
    PalettesList palettes = sprite->getPalettes();
    for (Palette* pal : palettes)
      if (pal->frame() != 0)
        m_pre.add(new cmd::RemovePalette(sprite, pal));

    std::unique_ptr<Palette> graypal(Palette::createGrayscale());
    if (*graypal != *sprite->palette(0))
      m_pre.add(new cmd::SetPalette(sprite, 0, graypal.get()));
  }
}

void SetPixelFormat::onExecute()
{
  m_pre.execute(context());
  setFormat(m_newFormat);
  m_post.execute(context());
}

void SetPixelFormat::onUndo()
{
  m_post.undo();
  setFormat(m_oldFormat);
  m_pre.undo();
}

void SetPixelFormat::onRedo()
{
  m_pre.redo();
  setFormat(m_newFormat);
  m_post.redo();
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

void SetPixelFormat::convertImage(doc::Sprite* sprite,
                                  const render::Dithering& dithering,
                                  const doc::ImageRef& oldImage,
                                  const doc::frame_t frame,
                                  const bool isBackground,
                                  const doc::RgbMapAlgorithm mapAlgorithm,
                                  doc::rgba_to_graya_func toGray,
                                  render::TaskDelegate* delegate,
                                  const doc::FitCriteria fitCriteria)
{
  ASSERT(oldImage);
  ASSERT(oldImage->pixelFormat() != IMAGE_TILEMAP);

  // Making the RGBMap for Image->INDEXDED conversion.
  // TODO: this is needed only when newImage
  RgbMap* rgbmap;
  int newMaskIndex = (isBackground ? -1 : 0);
  if (m_newFormat == IMAGE_INDEXED) {
    rgbmap = sprite->rgbMap(frame, sprite->rgbMapForSprite(), mapAlgorithm, fitCriteria);
    if (m_oldFormat == IMAGE_INDEXED)
      newMaskIndex = sprite->transparentColor();
    else
      newMaskIndex = rgbmap->maskIndex();
  }
  else {
    rgbmap = nullptr;
  }
  ImageRef newImage(render::convert_pixel_format(oldImage.get(),
                                                 nullptr,
                                                 m_newFormat,
                                                 dithering,
                                                 rgbmap,
                                                 sprite->palette(frame),
                                                 isBackground,
                                                 newMaskIndex,
                                                 toGray,
                                                 delegate));

  m_pre.add(new cmd::ReplaceImage(sprite, oldImage, newImage));
}

}} // namespace app::cmd
