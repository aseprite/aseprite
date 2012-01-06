/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#include "base/unique_ptr.h"
#include "document.h"
#include "file/file.h"
#include "file/file_format.h"
#include "file/format_options.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/raster.h"
#include "util/autocrop.h"

#include <gif_lib.h>

enum DisposalMethod {
  DISPOSAL_METHOD_NONE,
  DISPOSAL_METHOD_DO_NOT_DISPOSE,
  DISPOSAL_METHOD_RESTORE_BGCOLOR,
  DISPOSAL_METHOD_RESTORE_PREVIOUS,
};

struct GifFrame
{
  int x, y;
  int duration;
  int mask_index;
  Image* image;
  Palette* palette;
  DisposalMethod disposal_method;

  GifFrame()
    : x(0), y(0)
    , mask_index(-1)
    , image(0)
    , palette(0)
    , disposal_method(DISPOSAL_METHOD_NONE) {
  }
};

typedef std::vector<GifFrame> GifFrames;

struct GifData
{
  int sprite_w;
  int sprite_h;
  int bgcolor_index;
  GifFrames frames;
};

class GifFormat : public FileFormat
{
  const char* onGetName() const { return "gif"; }
  const char* onGetExtensions() const { return "gif"; }
  int onGetFlags() const {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_FRAMES |
      FILE_SUPPORT_PALETTES;
  }

  bool onLoad(FileOp* fop);
  bool onPostLoad(FileOp* fop) OVERRIDE;
  void onDestroyData(FileOp* fop) OVERRIDE;
  bool onSave(FileOp* fop);
};

FileFormat* CreateGifFormat()
{
  return new GifFormat;
}

static int interlaced_offset[] = { 0, 4, 2, 1 };
static int interlaced_jumps[] = { 8, 8, 4, 2 };

bool GifFormat::onLoad(FileOp* fop)
{
  UniquePtr<GifFileType, int(*)(GifFileType*)> gif_file(DGifOpenFileName(fop->filename.c_str()),
                                                        DGifCloseFile);
  if (!gif_file) {
    fop_error(fop, "Error loading GIF header.\n");
    return false;
  }

  GifData* data = new GifData;
  fop->format_data = reinterpret_cast<void*>(data);

  data->sprite_w = gif_file->SWidth;
  data->sprite_h = gif_file->SHeight;

  UniquePtr<Palette> current_palette(new Palette(0, 256));
  UniquePtr<Palette> previous_palette(new Palette(0, 256));

  // If the GIF image has a global palette, it has a valid
  // background color (so the GIF is not transparent).
  if (gif_file->SColorMap != NULL) {
    data->bgcolor_index = gif_file->SBackGroundColor;

    // Setup the first palette using the global color map.
    ColorMapObject* colormap = gif_file->SColorMap;
    for (int i=0; i<colormap->ColorCount; ++i) {
      current_palette->setEntry(i, _rgba(colormap->Colors[i].Red,
                                         colormap->Colors[i].Green,
                                         colormap->Colors[i].Blue, 255));
    }
  }
  else {
    data->bgcolor_index = -1;
  }

  // Scan the content of the GIF file (read record by record)
  GifRecordType record_type;
  size_t frame_num = 0;
  DisposalMethod disposal_method = DISPOSAL_METHOD_NONE;
  int transparent_index = -1;
  int frame_delay = -1;
  do {
    if (DGifGetRecordType(gif_file, &record_type) == GIF_ERROR)
      throw base::Exception("Invalid GIF record in file.\n");

    switch (record_type) {

      case IMAGE_DESC_RECORD_TYPE: {
        if (DGifGetImageDesc(gif_file) == GIF_ERROR)
          throw base::Exception("Invalid GIF image descriptor.\n");

        // These are the bounds of the image to read.
        int frame_x = gif_file->Image.Left;
        int frame_y = gif_file->Image.Top;
        int frame_w = gif_file->Image.Width;
        int frame_h = gif_file->Image.Height;

        if (frame_x < 0 || frame_y < 0 ||
            frame_x + frame_w > data->sprite_w ||
            frame_y + frame_h > data->sprite_h)
          throw base::Exception("Image %d is out of sprite bounds.\n", frame_num);

        // Add a new frames.
        if (frame_num >= data->frames.size())
          data->frames.resize(frame_num+1);

        data->frames[frame_num].x = frame_x;
        data->frames[frame_num].y = frame_y;

        // Set frame delay (1/100th seconds to milliseconds)
        if (frame_delay >= 0)
          data->frames[frame_num].duration = frame_delay*10;

        // Update palette for this frame (the first frame always need a palette).
        if (gif_file->Image.ColorMap) {
          ColorMapObject* colormap = gif_file->Image.ColorMap;
          for (int i=0; i<colormap->ColorCount; ++i) {
            current_palette->setEntry(i, _rgba(colormap->Colors[i].Red,
                                               colormap->Colors[i].Green,
                                               colormap->Colors[i].Blue, 255));
          }
        }

        if (frame_num == 0 || previous_palette->countDiff(current_palette, NULL, NULL)) {
          current_palette->setFrame(frame_num);

          data->frames[frame_num].palette = new Palette(*current_palette);
          data->frames[frame_num].palette->setFrame(frame_num);

          current_palette->copyColorsTo(previous_palette);
        }

        // Create a temporary image to load frame pixels.
        UniquePtr<Image> frame_image(image_new(IMAGE_INDEXED, frame_w, frame_h));
        IndexedTraits::address_t addr;

        if (gif_file->Image.Interlace) {
          // Need to perform 4 passes on the images.
          for (int i=0; i<4; ++i)
            for (int y = interlaced_offset[i]; y < frame_h; y += interlaced_jumps[i]) {
              addr = image_address_fast<IndexedTraits>(frame_image, 0, y);
              if (DGifGetLine(gif_file, addr, frame_w) == GIF_ERROR)
                throw base::Exception("Invalid interlaced image data.");
            }
        }
        else {
          for (int y = 0; y < frame_h; ++y) {
            addr = image_address_fast<IndexedTraits>(frame_image, 0, y);
            if (DGifGetLine(gif_file, addr, frame_w) == GIF_ERROR)
              throw base::Exception("Invalid image data (%d).\n", GifLastError());
          }
        }

        // Detach the pointer of the frame-image and put it in the list of frames.
        data->frames[frame_num].image = frame_image.release();
        data->frames[frame_num].disposal_method = disposal_method;
        data->frames[frame_num].mask_index = transparent_index;

        PRINTF("Frame[%d] transparent index  = %d\n", frame_num, transparent_index);

        ++frame_num;

        disposal_method = DISPOSAL_METHOD_NONE;
        transparent_index = -1;
        frame_delay = -1;
        break;
      }

      case EXTENSION_RECORD_TYPE: {
        GifByteType* extension;
        int ext_code;

        if (DGifGetExtension(gif_file, &ext_code, &extension) == GIF_ERROR)
          throw base::Exception("Invalid GIF extension record.\n");

        if (ext_code == GRAPHICS_EXT_FUNC_CODE) {
          if (extension[0] >= 4) {
            disposal_method   = (DisposalMethod)((extension[1] >> 2) & 7);
            transparent_index = (extension[1] & 1) ? extension[4]: -1;
            frame_delay       = (extension[3] << 8) | extension[2];

            TRACE("Disposal method: %d\nTransparent index: %d\nFrame delay: %d\n",
                  disposal_method, transparent_index, frame_delay);
          }
        }

        while (extension != NULL) {
          if (DGifGetExtensionNext(gif_file, &extension) == GIF_ERROR)
            throw base::Exception("Invalid GIF extension record.\n");
        }
        break;
      }

      case TERMINATE_RECORD_TYPE:
        break;

      default:
        break;
    }

    // Just one frame?
    if (frame_num > 0 && fop->oneframe)
      break;

    if (fop_is_stop(fop))
      break;
  } while (record_type != TERMINATE_RECORD_TYPE);

  fop->document = new Document(NULL);
  return true;
}

bool GifFormat::onPostLoad(FileOp* fop)
{
  GifData* data = reinterpret_cast<GifData*>(fop->format_data);
  if (!data)
    return true;

  int imgtype = IMAGE_INDEXED;
  bool askForConversion = false;

  if (!fop->oneframe) {
    int global_mask_index = -1;

    for (GifFrames::iterator
           frame_it=data->frames.begin(),
           frame_end=data->frames.end(); frame_it != frame_end; ++frame_it) {

      // Convert the indexed image to RGB
      for (int y=0; y<frame_it->image->h; ++y) {
        for (int x=0; x<frame_it->image->w; ++x) {
          int pixel_index = image_getpixel_fast<IndexedTraits>(frame_it->image, x, y);

          if (pixel_index >= 0 && pixel_index < 256) {
            // This pixel matches the frame's transparent color
            if (pixel_index == frame_it->mask_index) {
              // If we haven't set a background color yet, this is our new background color.
              if (global_mask_index < 0) {
                global_mask_index = pixel_index;
              }
            }
            else {
              // Drawing the mask color
              if (global_mask_index == pixel_index) {
                askForConversion = true;
                goto done;
              }
            }
          }
        }
      }
    }

    // New background color
    data->bgcolor_index = global_mask_index;

  done:;
  }

  if (askForConversion) {
    int result =
      Alert::show("GIF Conversion"
                  "<<The selected file: %s"
                  "<<is a transparent GIF image which uses multiple background colors."
                  "<<ASEPRITE cannot handle this kind of GIF correctly in Indexed format."
                  "<<What would you like to do?"
                  "||Convert to &RGBA||Keep &Indexed||&Cancel",
                  fop->document->getFilename());

    if (result == 1)
      imgtype = IMAGE_RGB;
    else if (result != 2)
      return false;
  }

  // Create the sprite with the GIF dimension
  UniquePtr<Sprite> sprite(new Sprite(imgtype, data->sprite_w, data->sprite_h, 256));

  // Create the main layer
  LayerImage* layer = new LayerImage(sprite);
  sprite->getFolder()->add_layer(layer);

  if (imgtype == IMAGE_INDEXED) {
    if (data->bgcolor_index >= 0)
      sprite->setTransparentColor(data->bgcolor_index);
    else
      layer->configureAsBackground();
  }

  // The previous image is used to support the special disposal method
  // of GIF frames DISPOSAL_METHOD_RESTORE_PREVIOUS (number 3 in
  // Graphics Extension)
  UniquePtr<Image> current_image(image_new(imgtype, data->sprite_w, data->sprite_h));
  UniquePtr<Image> previous_image(image_new(imgtype, data->sprite_w, data->sprite_h));

  // Clear both images with the transparent color (alpha = 0).
  uint32_t bgcolor = (imgtype == IMAGE_RGB ? _rgba(0, 0, 0, 0):
                      (data->bgcolor_index >= 0 ? data->bgcolor_index: 0));
  image_clear(current_image, bgcolor);
  image_clear(previous_image, bgcolor);

  // Add all frames in the sprite.
  sprite->setTotalFrames(data->frames.size());
  Palette* current_palette = NULL;

  size_t frame_num = 0;
  for (GifFrames::iterator
         frame_it=data->frames.begin(),
         frame_end=data->frames.end(); frame_it != frame_end; ++frame_it, ++frame_num) {

    // Set frame duration
    sprite->setFrameDuration(frame_num, frame_it->duration);

    // Set frame palette
    if (frame_it->palette) {
      sprite->setPalette(frame_it->palette, true);
      current_palette = frame_it->palette;
    }

    switch (imgtype) {

      case IMAGE_INDEXED:
        for (int y = 0; y < frame_it->image->h; ++y)
          for (int x = 0; x < frame_it->image->w; ++x) {
            int pixel_index = image_getpixel_fast<IndexedTraits>(frame_it->image, x, y);
            if (pixel_index != frame_it->mask_index)
              image_putpixel_fast<IndexedTraits>(current_image,
                                                 frame_it->x + x,
                                                 frame_it->y + y,
                                                 pixel_index);
          }
        break;

      case IMAGE_RGB:
        // Convert the indexed image to RGB
        for (int y = 0; y < frame_it->image->h; ++y)
          for (int x = 0; x < frame_it->image->w; ++x) {
            int pixel_index = image_getpixel_fast<IndexedTraits>(frame_it->image, x, y);
            if (pixel_index != frame_it->mask_index)
              image_putpixel_fast<RgbTraits>(current_image,
                                             frame_it->x + x,
                                             frame_it->y + y,
                                             current_palette->getEntry(pixel_index));
          }
        break;

    }

    // Create a new Cel and a image with the whole content of "current_image"
    Cel* cel = new Cel(frame_num, 0);
    try {
      Image* cel_image = image_new_copy(current_image);
      try {
        // Add the image in the sprite's stock and update the cel's
        // reference to the new stock's image.
        cel->setImage(sprite->getStock()->addImage(cel_image));
      }
      catch (...) {
        delete cel_image;
        throw;
      }

      layer->addCel(cel);
    }
    catch (...) {
      delete cel;
      throw;
    }

    // The current_image was already copied to represent the
    // current frame (frame_num), so now we have to clear the
    // area occupied by frame_image using the desired disposal
    // method.
    switch (frame_it->disposal_method) {

      case DISPOSAL_METHOD_NONE:
      case DISPOSAL_METHOD_DO_NOT_DISPOSE:
        // Do nothing
        break;

      case DISPOSAL_METHOD_RESTORE_BGCOLOR:
        image_rectfill(current_image,
                       frame_it->x,
                       frame_it->y,
                       frame_it->x+frame_it->image->w-1,
                       frame_it->y+frame_it->image->h-1,
                       bgcolor);
        break;

      case DISPOSAL_METHOD_RESTORE_PREVIOUS:
        image_copy(current_image, previous_image, 0, 0);
        break;
    }

    // Update previous_image with current_image only if the
    // disposal method is not "restore previous" (which means
    // that we have already updated current_image from
    // previous_image).
    if (frame_it->disposal_method != DISPOSAL_METHOD_RESTORE_PREVIOUS)
      image_copy(previous_image, current_image, 0, 0);
  }

  fop->document->addSprite(sprite);
  sprite.release();             // Now the sprite is owned by fop->document

  return true;
}

void GifFormat::onDestroyData(FileOp* fop)
{
  GifData* data = reinterpret_cast<GifData*>(fop->format_data);
  if (data) {
    GifFrames::iterator frame_it = data->frames.begin();
    GifFrames::iterator frame_end = data->frames.end();

    for (; frame_it != frame_end; ++frame_it) {
      delete frame_it->image;
      delete frame_it->palette;
    }

    delete data;
  }
}

bool GifFormat::onSave(FileOp* fop)
{
  UniquePtr<GifFileType, int(*)(GifFileType*)> gif_file(EGifOpenFileName(fop->filename.c_str(), 0),
                                                        EGifCloseFile);
  if (!gif_file)
    throw base::Exception("Error creating GIF file.\n");

  Sprite* sprite = fop->document->getSprite();
  int sprite_w = sprite->getWidth();
  int sprite_h = sprite->getHeight();
  int sprite_imgtype = sprite->getImgType();
  bool interlace = false;
  int loop = 0;
  int background_color = (sprite_imgtype == IMAGE_INDEXED ? sprite->getTransparentColor(): 0);
  int transparent_index = (sprite->getBackgroundLayer() ? -1: sprite->getTransparentColor());

  Palette* current_palette = sprite->getPalette(0);
  Palette* previous_palette = current_palette;
  ColorMapObject* color_map = MakeMapObject(current_palette->size(), NULL);
  for (int i = 0; i < current_palette->size(); ++i) {
    color_map->Colors[i].Red   = _rgba_getr(current_palette->getEntry(i));
    color_map->Colors[i].Green = _rgba_getg(current_palette->getEntry(i));
    color_map->Colors[i].Blue  = _rgba_getb(current_palette->getEntry(i));
  }

  if (EGifPutScreenDesc(gif_file, sprite_w, sprite_h,
                        color_map->BitsPerPixel,
                        background_color, color_map) == GIF_ERROR)
    throw base::Exception("Error writing GIF header.\n");

  UniquePtr<Image> buffer_image;
  UniquePtr<Image> current_image(image_new(IMAGE_INDEXED, sprite_w, sprite_h));
  UniquePtr<Image> previous_image(image_new(IMAGE_INDEXED, sprite_w, sprite_h));
  int frame_x, frame_y, frame_w, frame_h;
  int u1, v1, u2, v2;
  int i1, j1, i2, j2;

  // If the sprite is not Indexed type, we will need a temporary
  // buffer to render the full RGB or Grayscale sprite.
  if (sprite_imgtype != IMAGE_INDEXED)
    buffer_image.reset(image_new(sprite_imgtype, sprite_w, sprite_h));

  image_clear(current_image, background_color);
  image_clear(previous_image, background_color);

  for (int frame_num=0; frame_num<sprite->getTotalFrames(); ++frame_num) {
    current_palette = sprite->getPalette(frame_num);

    // If the sprite is RGB or Grayscale, we must to convert it to Indexed on the fly.
    if (sprite_imgtype != IMAGE_INDEXED) {
      image_clear(buffer_image, 0);
      layer_render(sprite->getFolder(), buffer_image, 0, 0, frame_num);

      switch (sprite_imgtype) {

        // Convert the RGB image to Indexed
        case IMAGE_RGB:
          for (int y = 0; y < sprite_h; ++y)
            for (int x = 0; x < sprite_w; ++x) {
              uint32_t pixel_value = image_getpixel_fast<RgbTraits>(buffer_image, x, y);
              image_putpixel_fast<IndexedTraits>(current_image, x, y,
                                                 (_rgba_geta(pixel_value) >= 128) ?
                                                 current_palette->findBestfit(_rgba_getr(pixel_value),
                                                                              _rgba_getg(pixel_value),
                                                                              _rgba_getb(pixel_value)):
                                                 transparent_index);
            }
          break;

        // Convert the Grayscale image to Indexed
        case IMAGE_GRAYSCALE:
          for (int y = 0; y < sprite_h; ++y)
            for (int x = 0; x < sprite_w; ++x) {
              uint16_t pixel_value = image_getpixel_fast<GrayscaleTraits>(buffer_image, x, y);
              image_putpixel_fast<IndexedTraits>(current_image, x, y,
                                                 (_graya_geta(pixel_value) >= 128) ?
                                                 current_palette->findBestfit(_graya_getv(pixel_value),
                                                                              _graya_getv(pixel_value),
                                                                              _graya_getv(pixel_value)):
                                                 transparent_index);
            }
          break;
      }
    }
    // If the sprite is Indexed, we can render directly into "current_image".
    else {
      image_clear(current_image, background_color);
      layer_render(sprite->getFolder(), current_image, 0, 0, frame_num);
    }

    if (frame_num == 0) {
      frame_x = 0;
      frame_y = 0;
      frame_w = sprite->getWidth();
      frame_h = sprite->getHeight();
    }
    else {
      // Get the rectangle where start differences with the previous frame.
      if (get_shrink_rect2(&u1, &v1, &u2, &v2, current_image, previous_image)) {
        // Check the minimal area with the background color.
        if (get_shrink_rect(&i1, &j1, &i2, &j2, current_image, background_color)) {
          frame_x = MIN(u1, i1);
          frame_y = MIN(v1, j1);
          frame_w = MAX(u2, i2) - MIN(u1, i1) + 1;
          frame_h = MAX(v2, j2) - MIN(v1, j1) + 1;
        }
      }
    }

    // Specify loop extension.
    if (frame_num == 0 && loop >= 0) {
      unsigned char extension_bytes[11];

      memcpy(extension_bytes, "NETSCAPE2.0", 11);
      if (EGifPutExtensionFirst(gif_file, APPLICATION_EXT_FUNC_CODE, 11, extension_bytes) == GIF_ERROR)
        throw base::Exception("Error writing GIF graphics extension record for frame %d.\n", frame_num);

      extension_bytes[0] = 1;
      extension_bytes[1] = (loop & 0xff);
      extension_bytes[2] = (loop >> 8) & 0xff;
      if (EGifPutExtensionNext(gif_file, APPLICATION_EXT_FUNC_CODE, 3, extension_bytes) == GIF_ERROR)
        throw base::Exception("Error writing GIF graphics extension record for frame %d.\n", frame_num);

      if (EGifPutExtensionLast(gif_file, APPLICATION_EXT_FUNC_CODE, 0, NULL) == GIF_ERROR)
        throw base::Exception("Error writing GIF graphics extension record for frame %d.\n", frame_num);
    }

    // Write graphics extension record (to save the duration of the
    // frame and maybe the transparency index).
    {
      unsigned char extension_bytes[5];
      int disposal_method = (sprite->getBackgroundLayer() ? DISPOSAL_METHOD_DO_NOT_DISPOSE:
                                                            DISPOSAL_METHOD_RESTORE_BGCOLOR);
      int frame_delay = sprite->getFrameDuration(frame_num) / 10;

      extension_bytes[0] = (((disposal_method & 7) << 2) |
                            (transparent_index >= 0 ? 1: 0));
      extension_bytes[1] = (frame_delay & 0xff);
      extension_bytes[2] = (frame_delay >> 8) & 0xff;
      extension_bytes[3] = (transparent_index >= 0 ? transparent_index: 0);

      if (EGifPutExtension(gif_file, GRAPHICS_EXT_FUNC_CODE, 4, extension_bytes) == GIF_ERROR)
        throw base::Exception("Error writing GIF graphics extension record for frame %d.\n", frame_num);
    }

    // Image color map
    ColorMapObject* image_color_map = NULL;
    if (current_palette != previous_palette) {
      image_color_map = MakeMapObject(current_palette->size(), NULL);
      for (int i = 0; i < current_palette->size(); ++i) {
        image_color_map->Colors[i].Red   = _rgba_getr(current_palette->getEntry(i));
        image_color_map->Colors[i].Green = _rgba_getg(current_palette->getEntry(i));
        image_color_map->Colors[i].Blue  = _rgba_getb(current_palette->getEntry(i));
      }
      previous_palette = current_palette;
    }

    // Write the image record.
    if (EGifPutImageDesc(gif_file,
                         frame_x, frame_y,
                         frame_w, frame_h, interlace ? 1: 0,
                         image_color_map) == GIF_ERROR)
      throw base::Exception("Error writing GIF frame %d.\n", frame_num);

    // Write the image data (pixels).
    if (interlace) {
      // Need to perform 4 passes on the images.
      for (int i=0; i<4; ++i)
        for (int y = interlaced_offset[i]; y < frame_h; y += interlaced_jumps[i]) {
          IndexedTraits::address_t addr = image_address_fast<IndexedTraits>(current_image, frame_x, frame_y + y);
          if (EGifPutLine(gif_file, addr, frame_w) == GIF_ERROR)
            throw base::Exception("Error writing GIF image scanlines for frame %d.\n", frame_num);
        }
    }
    else {
      // Write all image scanlines (not interlaced in this case).
      for (int y = 0; y < frame_h; ++y) {
        IndexedTraits::address_t addr = image_address_fast<IndexedTraits>(current_image, frame_x, frame_y + y);
        if (EGifPutLine(gif_file, addr, frame_w) == GIF_ERROR)
          throw base::Exception("Error writing GIF image scanlines for frame %d.\n", frame_num);
      }
    }

    image_copy(previous_image, current_image, 0, 0);
  }

  return true;
}
