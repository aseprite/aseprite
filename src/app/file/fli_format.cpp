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

#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/fli/fli.h"
#include "app/file/format_options.h"
#include "app/modules/palettes.h"
#include "base/file_handle.h"
#include "doc/doc.h"

#include <cstdio>

namespace app {

using namespace base;

static int get_time_precision(Sprite *sprite);

class FliFormat : public FileFormat {
  const char* onGetName() const { return "flc"; }
  const char* onGetExtensions() const { return "flc,fli"; }
  int onGetFlags() const {
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
#define SETPAL()                                                \
  do {                                                          \
      for (c=0; c<256; c++) {                                   \
        pal->setEntry(c, rgba(cmap[c*3],                       \
                               cmap[c*3+1],                     \
                               cmap[c*3+2], 255));              \
      }                                                         \
      pal->setFrame(frpos_out);                                 \
      sprite->setPalette(pal, true);                            \
    } while (0)

  unsigned char cmap[768];
  unsigned char omap[768];
  s_fli_header fli_header;
  int c, w, h;
  FrameNumber frpos_in;
  FrameNumber frpos_out;
  int index = 0;

  // Open the file to read in binary mode
  FileHandle f(open_file_with_exception(fop->filename, "rb"));

  fli_read_header(f, &fli_header);
  fseek(f, 128, SEEK_SET);

  if (fli_header.magic == NO_HEADER) {
    fop_error(fop, "The file doesn't have a FLIC header\n");
    return false;
  }

  /* size by frame */
  w = fli_header.width;
  h = fli_header.height;

  // Create the bitmaps
  base::UniquePtr<Image> bmp(Image::create(IMAGE_INDEXED, w, h));
  base::UniquePtr<Image> old(Image::create(IMAGE_INDEXED, w, h));
  base::UniquePtr<Palette> pal(new Palette(FrameNumber(0), 256));

  // Create the image
  Sprite* sprite = new Sprite(IMAGE_INDEXED, w, h, 256);
  LayerImage* layer = new LayerImage(sprite);
  sprite->folder()->addLayer(layer);
  layer->configureAsBackground();

  // Set frames and speed
  sprite->setTotalFrames(FrameNumber(fli_header.frames));
  sprite->setDurationForAllFrames(fli_header.speed);

  /* write frame by frame */
  for (frpos_in = frpos_out = FrameNumber(0);
       frpos_in < sprite->totalFrames();
       ++frpos_in) {
    /* read the frame */
    fli_read_frame(f, &fli_header,
                   (unsigned char *)old->getPixelAddress(0, 0), omap,
                   (unsigned char *)bmp->getPixelAddress(0, 0), cmap);

    /* first frame, or the frames changes, or the palette changes */
    if ((frpos_in == 0) ||
        (count_diff_between_images(old, bmp))
#ifndef USE_LINK /* TODO this should be configurable through a check-box */
        || (memcmp(omap, cmap, 768) != 0)
#endif
        ) {
      /* the image changes? */
      if (frpos_in != 0)
        ++frpos_out;

      // Add the new frame
      Image* image = Image::createCopy(bmp);
      index = sprite->stock()->addImage(image);

      Cel* cel = new Cel(frpos_out, index);
      layer->addCel(cel);

      /* first frame or the palette changes */
      if ((frpos_in == 0) || (memcmp(omap, cmap, 768) != 0))
        SETPAL();
    }
#ifdef USE_LINK
    /* the palette changes */
    else if (memcmp(omap, cmap, 768) != 0) {
      ++frpos_out;
      SETPAL();

      // Add link
      Cel* cel = new Cel(frpos_out, index);
      layer_add_cel(layer, cel);
    }
#endif
    // The palette and the image don't change: add duration to the last added frame
    else {
      sprite->setFrameDuration(frpos_out,
                               sprite->getFrameDuration(frpos_out)+fli_header.speed);
    }

    /* update the old image and color-map to the new ones to compare later */
    copy_image(old, bmp, 0, 0);
    memcpy(omap, cmap, 768);

    /* update progress */
    fop_progress(fop, (float)(frpos_in+1) / (float)(sprite->totalFrames()));
    if (fop_is_stop(fop))
      break;

    /* just one frame? */
    if (fop->oneframe)
      break;
  }

  // Update number of frames
  sprite->setTotalFrames(frpos_out.next());

  fop->createDocument(sprite);
  return true;
}

#ifdef ENABLE_SAVE
bool FliFormat::onSave(FileOp* fop)
{
  Sprite* sprite = fop->document->sprite();
  unsigned char cmap[768];
  unsigned char omap[768];
  s_fli_header fli_header;
  int c, times;
  Palette *pal;

  /* prepare fli header */
  fli_header.filesize = 0;
  fli_header.frames = 0;
  fli_header.width = sprite->width();
  fli_header.height = sprite->height();

  if ((fli_header.width == 320) && (fli_header.height == 200))
    fli_header.magic = HEADER_FLI;
  else
    fli_header.magic = HEADER_FLC;

  fli_header.depth = 8;
  fli_header.flags = 3;
  fli_header.speed = get_time_precision(sprite);
  fli_header.created = 0;
  fli_header.updated = 0;
  fli_header.aspect_x = 1;
  fli_header.aspect_y = 1;
  fli_header.oframe1 = fli_header.oframe2 = 0;

  /* open the file to write in binary mode */
  FileHandle f(open_file_with_exception(fop->filename, "wb"));

  fseek(f, 128, SEEK_SET);

  // Create the bitmaps
  base::UniquePtr<Image> bmp(Image::create(IMAGE_INDEXED, sprite->width(), sprite->height()));
  base::UniquePtr<Image> old(Image::create(IMAGE_INDEXED, sprite->width(), sprite->height()));

  // Write frame by frame
  for (FrameNumber frpos(0);
       frpos < sprite->totalFrames();
       ++frpos) {
    /* get color map */
    pal = sprite->getPalette(frpos);
    for (c=0; c<256; c++) {
      cmap[3*c  ] = rgba_getr(pal->getEntry(c));
      cmap[3*c+1] = rgba_getg(pal->getEntry(c));
      cmap[3*c+2] = rgba_getb(pal->getEntry(c));
    }

    /* render the frame in the bitmap */
    clear_image(bmp, 0);
    layer_render(sprite->folder(), bmp, 0, 0, frpos);

    /* how many times this frame should be written to get the same
       time that it has in the sprite */
    times = sprite->getFrameDuration(frpos) / fli_header.speed;

    for (c=0; c<times; c++) {
      /* write this frame */
      if (frpos == 0 && c == 0)
        fli_write_frame(f, &fli_header, NULL, NULL,
                        (unsigned char *)bmp->getPixelAddress(0, 0), cmap, W_ALL);
      else
        fli_write_frame(f, &fli_header,
                        (unsigned char *)old->getPixelAddress(0, 0), omap,
                        (unsigned char *)bmp->getPixelAddress(0, 0), cmap, W_ALL);

      /* update the old image and color-map to the new ones to compare later */
      copy_image(old, bmp, 0, 0);
      memcpy(omap, cmap, 768);
    }

    /* update progress */
    fop_progress(fop, (float)(frpos.next()) / (float)(sprite->totalFrames()));
  }

  // Write the header and close the file
  fli_write_header(f, &fli_header);

  return true;
}
#endif

static int get_time_precision(Sprite *sprite)
{
  int precision = 1000;

  for (FrameNumber c(0); c < sprite->totalFrames() && precision > 1; ++c) {
    int len = sprite->getFrameDuration(c);

    while (len / precision == 0)
      precision /= 10;
  }

  return precision;
}

} // namespace app
