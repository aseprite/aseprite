// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/modules/palettes.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "flic/flic.h"
#include "render/render.h"

#include <cstdio>

namespace app {

using namespace base;

class FliFormat : public FileFormat {
  const char* onGetName() const override { return "flc"; }
  const char* onGetExtensions() const  override{ return "flc,fli"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_FRAMES |
      FILE_SUPPORT_PALETTES;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateFliFormat()
{
  return new FliFormat;
}

bool FliFormat::onLoad(FileOp* fop)
{
  // Open the file to read in binary mode
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();
  flic::StdioFileInterface finterface(f);
  flic::Decoder decoder(&finterface);

  flic::Header header;
  if (!decoder.readHeader(header)) {
    fop->setError("The file doesn't have a FLIC header\n");
    return false;
  }

  // Size by frame
  int w = header.width;
  int h = header.height;

  // Create a temporal bitmap
  ImageRef bmp(Image::create(IMAGE_INDEXED, w, h));
  Palette pal(0, 1);
  Cel* prevCel = nullptr;

  // Create the sprite
  Sprite* sprite = new Sprite(IMAGE_INDEXED, w, h, 256);
  LayerImage* layer = new LayerImage(sprite);
  sprite->root()->addLayer(layer);
  layer->configureAsBackground();

  // Set frames and speed
  sprite->setTotalFrames(frame_t(header.frames));
  sprite->setDurationForAllFrames(header.speed);

  flic::Frame fliFrame;
  flic::Colormap oldFliColormap;
  fliFrame.pixels = bmp->getPixelAddress(0, 0);
  fliFrame.rowstride = IndexedTraits::getRowStrideBytes(bmp->width());

  frame_t frame_out = 0;
  for (frame_t frame_in=0;
       frame_in<sprite->totalFrames();
       ++frame_in) {
    // Read the frame
    if (!decoder.readFrame(fliFrame)) {
      fop->setError("Error reading frame %d\n", frame_in);
      continue;
    }

    // Palette change
    bool palChange = false;
    if (frame_out == 0 || oldFliColormap != fliFrame.colormap) {
      oldFliColormap = fliFrame.colormap;

      pal.resize(fliFrame.colormap.size());
      for (int c=0; c<int(fliFrame.colormap.size()); c++) {
        pal.setEntry(c, rgba(fliFrame.colormap[c].r,
                             fliFrame.colormap[c].g,
                             fliFrame.colormap[c].b, 255));
      }
      pal.setFrame(frame_out);
      sprite->setPalette(&pal, true);

      palChange = true;
    }

    // First frame, or the frame changes
    if (!prevCel ||
        (count_diff_between_images(prevCel->image(), bmp.get()))) {
      // Add the new frame
      ImageRef image(Image::createCopy(bmp.get()));
      Cel* cel = new Cel(frame_out, image);
      layer->addCel(cel);

      prevCel = cel;
      ++frame_out;
    }
    else if (palChange) {
      Cel* cel = Cel::createLink(prevCel);
      cel->setFrame(frame_out);
      layer->addCel(cel);

      ++frame_out;
    }
    // The palette and the image don't change: add duration to the last added frame
    else {
      sprite->setFrameDuration(
        frame_out-1, sprite->frameDuration(frame_out-1) + header.speed);
    }

    if (header.frames > 0)
      fop->setProgress((float)(frame_in+1) / (float)(header.frames));

    if (fop->isStop())
      break;

    if (fop->isOneFrame())
      break;
  }

  if (frame_out > 0)
    sprite->setTotalFrames(frame_out);

  fop->createDocument(sprite);
  return true;
}

#ifdef ENABLE_SAVE

static int get_time_precision(const Sprite *sprite,
                              const doc::frame_t fromFrame,
                              const doc::frame_t toFrame)
{
  // Check if all frames have the same duration
  bool constantFrameRate = true;
  for (frame_t c=fromFrame+1; c <= toFrame; ++c) {
    if (sprite->frameDuration(c-1) != sprite->frameDuration(c)) {
      constantFrameRate = false;
      break;
    }
  }
  if (constantFrameRate)
    return sprite->frameDuration(0);

  int precision = 1000;
  for (frame_t c=fromFrame; c <= toFrame && precision > 1; ++c) {
    int len = sprite->frameDuration(c);
    while (len / precision == 0)
      precision /= 10;
  }
  return precision;
}

bool FliFormat::onSave(FileOp* fop)
{
  const Sprite* sprite = fop->document()->sprite();

  // Open the file to write in binary mode
  FileHandle handle(open_file_with_exception(fop->filename(), "wb"));
  FILE* f = handle.get();
  flic::StdioFileInterface finterface(f);
  flic::Encoder encoder(&finterface);

  flic::Header header;
  header.frames = 0;
  header.width = sprite->width();
  header.height = sprite->height();
  header.speed = get_time_precision(
    sprite,
    fop->roi().fromFrame(),
    fop->roi().toFrame());
  encoder.writeHeader(header);

  // Create the bitmaps
  ImageRef bmp(Image::create(IMAGE_INDEXED, sprite->width(), sprite->height()));
  render::Render render;

  // Write frame by frame
  flic::Frame fliFrame;
  fliFrame.pixels = bmp->getPixelAddress(0, 0);
  fliFrame.rowstride = IndexedTraits::getRowStrideBytes(bmp->width());
  frame_t nframes = fop->roi().frames();
  for (frame_t frame_it=0; frame_it <= nframes; ++frame_it) {
    frame_t frame = fop->roi().fromFrame() + (frame_it % nframes);
    const Palette* pal = sprite->palette(frame);
    int size = MIN(256, pal->size());

    for (int c=0; c<size; c++) {
      color_t color = pal->getEntry(c);
      fliFrame.colormap[c].r = rgba_getr(color);
      fliFrame.colormap[c].g = rgba_getg(color);
      fliFrame.colormap[c].b = rgba_getb(color);
    }

    // Render the frame in the bitmap
    render.renderSprite(bmp.get(), sprite, frame);

    // How many times this frame should be written to get the same
    // time that it has in the sprite
    if (frame_it < nframes) {
      int times = sprite->frameDuration(frame) / header.speed;
      times = MAX(1, times);
      for (int c=0; c<times; c++)
        encoder.writeFrame(fliFrame);
    }
    else {
      encoder.writeRingFrame(fliFrame);
    }

    // Update progress
    fop->setProgress((float)(frame_it+1) / (float)(nframes+1));
  }

  return true;
}

#endif  // ENABLE_SAVE

} // namespace app
