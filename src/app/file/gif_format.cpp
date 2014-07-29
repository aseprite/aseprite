/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/console.h"
#include "app/context.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/file/gif_options.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/util/autocrop.h"
#include "base/file_handle.h"
#include "base/unique_ptr.h"
#include "raster/raster.h"
#include "ui/alert.h"
#include "ui/button.h"

#include "generated_gif_options.h"

#include <gif_lib.h>

namespace app {

using namespace base;

enum DisposalMethod {
  DISPOSAL_METHOD_NONE,
  DISPOSAL_METHOD_DO_NOT_DISPOSE,
  DISPOSAL_METHOD_RESTORE_BGCOLOR,
  DISPOSAL_METHOD_RESTORE_PREVIOUS,
};

struct GifFrame {
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

class GifFormat : public FileFormat {

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
      FILE_SUPPORT_PALETTES |
      FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop);
  bool onPostLoad(FileOp* fop) OVERRIDE;
  void onDestroyData(FileOp* fop) OVERRIDE;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) OVERRIDE;
#endif
  SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) OVERRIDE;
};

FileFormat* CreateGifFormat()
{
  return new GifFormat;
}

static int interlaced_offset[] = { 0, 4, 2, 1 };
static int interlaced_jumps[] = { 8, 8, 4, 2 };

struct GifFilePtr {
public:
  typedef int (*CloseFunc)(GifFileType*, int*);

  GifFilePtr(GifFileType* ptr, CloseFunc closeFunc) :
    m_ptr(ptr), m_closeFunc(closeFunc) {
  }

  ~GifFilePtr() {
    int errCode;
    m_closeFunc(m_ptr, &errCode);
  }

  operator GifFileType*() {
    return m_ptr;
  }

  GifFileType* operator->() {
    return m_ptr;
  }

private:
  GifFileType* m_ptr;
  CloseFunc m_closeFunc;
};

bool GifFormat::onLoad(FileOp* fop)
{
  int errCode;
  GifFilePtr gif_file(DGifOpenFileHandle(open_file_descriptor_with_exception(fop->filename, "rb"), &errCode), &DGifCloseFile);

  if (!gif_file) {
    fop_error(fop, "Error loading GIF header.\n");
    return false;
  }

  GifData* data = new GifData;
  fop->format_data = reinterpret_cast<void*>(data);

  data->sprite_w = gif_file->SWidth;
  data->sprite_h = gif_file->SHeight;

  UniquePtr<Palette> current_palette(new Palette(FrameNumber(0), 256));
  UniquePtr<Palette> previous_palette(new Palette(FrameNumber(0), 256));

  // If the GIF image has a global palette, it has a valid
  // background color (so the GIF is not transparent).
  if (gif_file->SColorMap != NULL) {
    data->bgcolor_index = gif_file->SBackGroundColor;

    // Setup the first palette using the global color map.
    ColorMapObject* colormap = gif_file->SColorMap;
    for (int i=0; i<colormap->ColorCount; ++i) {
      current_palette->setEntry(i,
        rgba(
          colormap->Colors[i].Red,
          colormap->Colors[i].Green,
          colormap->Colors[i].Blue, 255));
    }
  }
  else {
    data->bgcolor_index = -1;
  }

  // Scan the content of the GIF file (read record by record)
  GifRecordType record_type;
  FrameNumber frame_num(0);
  DisposalMethod disposal_method = DISPOSAL_METHOD_NONE;
  int transparent_index = -1;
  int frame_delay = -1;
  do {
    if (DGifGetRecordType(gif_file, &record_type) == GIF_ERROR)
      throw Exception("Invalid GIF record in file.\n");

    switch (record_type) {

      case IMAGE_DESC_RECORD_TYPE: {
        if (DGifGetImageDesc(gif_file) == GIF_ERROR)
          throw Exception("Invalid GIF image descriptor.\n");

        // These are the bounds of the image to read.
        int frame_x = gif_file->Image.Left;
        int frame_y = gif_file->Image.Top;
        int frame_w = gif_file->Image.Width;
        int frame_h = gif_file->Image.Height;

        if (frame_x < 0 || frame_y < 0 ||
            frame_x + frame_w > data->sprite_w ||
            frame_y + frame_h > data->sprite_h)
          throw Exception("Image %d is out of sprite bounds.\n", (int)frame_num);

        // Add a new frames.
        if (frame_num >= FrameNumber(data->frames.size()))
          data->frames.resize(frame_num.next());

        data->frames[frame_num].x = frame_x;
        data->frames[frame_num].y = frame_y;

        // Set frame delay (1/100th seconds to milliseconds)
        if (frame_delay >= 0)
          data->frames[frame_num].duration = frame_delay*10;

        // Update palette for this frame (the first frame always need a palette).
        if (gif_file->Image.ColorMap) {
          ColorMapObject* colormap = gif_file->Image.ColorMap;
          for (int i=0; i<colormap->ColorCount; ++i) {
            current_palette->setEntry(i,
              rgba(
                colormap->Colors[i].Red,
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
        UniquePtr<Image> frame_image(Image::create(IMAGE_INDEXED, frame_w, frame_h));
        IndexedTraits::address_t addr;

        if (gif_file->Image.Interlace) {
          // Need to perform 4 passes on the images.
          for (int i=0; i<4; ++i)
            for (int y = interlaced_offset[i]; y < frame_h; y += interlaced_jumps[i]) {
              addr = frame_image->getPixelAddress(0, y);
              if (DGifGetLine(gif_file, addr, frame_w) == GIF_ERROR)
                throw Exception("Invalid interlaced image data.");
            }
        }
        else {
          for (int y = 0; y < frame_h; ++y) {
            addr = frame_image->getPixelAddress(0, y);
            if (DGifGetLine(gif_file, addr, frame_w) == GIF_ERROR)
              throw Exception("Invalid image data (%d).\n", gif_file->Error);
          }
        }

        // Detach the pointer of the frame-image and put it in the list of frames.
        data->frames[frame_num].image = frame_image.release();
        data->frames[frame_num].disposal_method = disposal_method;
        data->frames[frame_num].mask_index = transparent_index;

        // PRINTF("Frame[%d] transparent index  = %d\n", (int)frame_num, transparent_index);

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
          throw Exception("Invalid GIF extension record.\n");

        if (ext_code == GRAPHICS_EXT_FUNC_CODE) {
          if (extension[0] >= 4) {
            disposal_method   = (DisposalMethod)((extension[1] >> 2) & 7);
            transparent_index = (extension[1] & 1) ? extension[4]: -1;
            frame_delay       = (extension[3] << 8) | extension[2];

            // PRINTF("Disposal method: %d\nTransparent index: %d\nFrame delay: %d\n",
            //   disposal_method, transparent_index, frame_delay);
          }
        }

        while (extension != NULL) {
          if (DGifGetExtensionNext(gif_file, &extension) == GIF_ERROR)
            throw Exception("Invalid GIF extension record.\n");
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

  fop->createDocument(NULL);    // The sprite is set in onPostLoad()
  return true;
}

bool GifFormat::onPostLoad(FileOp* fop)
{
  GifData* data = reinterpret_cast<GifData*>(fop->format_data);
  if (!data)
    return true;

  PixelFormat pixelFormat = IMAGE_INDEXED;
  bool askForConversion = false;

  if (!fop->oneframe) {
    int global_mask_index = -1;

    for (GifFrames::iterator
           frame_it=data->frames.begin(),
           frame_end=data->frames.end(); frame_it != frame_end; ++frame_it) {

      // Convert the indexed image to RGB
      for (int y=0; y<frame_it->image->getHeight(); ++y) {
        for (int x=0; x<frame_it->image->getWidth(); ++x) {
          int pixel_index = get_pixel_fast<IndexedTraits>(frame_it->image, x, y);

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
      ui::Alert::show("GIF Conversion"
                      "<<The selected file: %s"
                      "<<is a transparent GIF image which uses multiple background colors."
                      "<<" PACKAGE " cannot handle this kind of GIF correctly in Indexed format."
                      "<<What would you like to do?"
                      "||Convert to &RGBA||Keep &Indexed||&Cancel",
                      fop->document->name().c_str());

    if (result == 1)
      pixelFormat = IMAGE_RGB;
    else if (result != 2)
      return false;
  }

  // Create the sprite with the GIF dimension
  // TODO instead of 256 use the number of colors from the document
  UniquePtr<Sprite> sprite(new Sprite(pixelFormat, data->sprite_w, data->sprite_h, 256));

  // Create the main layer
  LayerImage* layer = new LayerImage(sprite);
  sprite->getFolder()->addLayer(layer);

  if (pixelFormat == IMAGE_INDEXED) {
    if (data->bgcolor_index >= 0)
      sprite->setTransparentColor(data->bgcolor_index);
    else
      layer->configureAsBackground();
  }

  // The previous image is used to support the special disposal method
  // of GIF frames DISPOSAL_METHOD_RESTORE_PREVIOUS (number 3 in
  // Graphics Extension)
  UniquePtr<Image> current_image(Image::create(pixelFormat, data->sprite_w, data->sprite_h));
  UniquePtr<Image> previous_image(Image::create(pixelFormat, data->sprite_w, data->sprite_h));

  // Clear both images with the transparent color (alpha = 0).
  uint32_t bgcolor = (pixelFormat == IMAGE_RGB ? rgba(0, 0, 0, 0):
                      (data->bgcolor_index >= 0 ? data->bgcolor_index: 0));
  clear_image(current_image, bgcolor);
  clear_image(previous_image, bgcolor);

  // Add all frames in the sprite.
  sprite->setTotalFrames(FrameNumber(data->frames.size()));
  Palette* current_palette = NULL;

  FrameNumber frame_num(0);
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

    switch (pixelFormat) {

      case IMAGE_INDEXED:
        for (int y = 0; y < frame_it->image->getHeight(); ++y)
          for (int x = 0; x < frame_it->image->getWidth(); ++x) {
            int pixel_index = get_pixel_fast<IndexedTraits>(frame_it->image, x, y);
            if (pixel_index != frame_it->mask_index)
              put_pixel_fast<IndexedTraits>(current_image,
                                            frame_it->x + x,
                                            frame_it->y + y,
                                            pixel_index);
          }
        break;

      case IMAGE_RGB:
        // Convert the indexed image to RGB
        for (int y = 0; y < frame_it->image->getHeight(); ++y)
          for (int x = 0; x < frame_it->image->getWidth(); ++x) {
            int pixel_index = get_pixel_fast<IndexedTraits>(frame_it->image, x, y);
            if (pixel_index != frame_it->mask_index)
              put_pixel_fast<RgbTraits>(current_image,
                                        frame_it->x + x,
                                        frame_it->y + y,
                                        current_palette->getEntry(pixel_index));
          }
        break;

    }

    // Create a new Cel and a image with the whole content of "current_image"
    Cel* cel = new Cel(frame_num, 0);
    try {
      Image* cel_image = Image::createCopy(current_image);
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
        fill_rect(current_image,
                  frame_it->x,
                  frame_it->y,
                  frame_it->x+frame_it->image->getWidth()-1,
                  frame_it->y+frame_it->image->getHeight()-1,
                  bgcolor);
        break;

      case DISPOSAL_METHOD_RESTORE_PREVIOUS:
        copy_image(current_image, previous_image, 0, 0);
        break;
    }

    // Update previous_image with current_image only if the
    // disposal method is not "restore previous" (which means
    // that we have already updated current_image from
    // previous_image).
    if (frame_it->disposal_method != DISPOSAL_METHOD_RESTORE_PREVIOUS)
      copy_image(previous_image, current_image, 0, 0);
  }

  fop->document->sprites().add(sprite);
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

#ifdef ENABLE_SAVE
bool GifFormat::onSave(FileOp* fop)
{
  int errCode;
  GifFilePtr gif_file(EGifOpenFileHandle(open_file_descriptor_with_exception(fop->filename, "wb"), &errCode), &EGifCloseFile);

  if (!gif_file)
    throw Exception("Error creating GIF file.\n");

  SharedPtr<GifOptions> gif_options = fop->seq.format_options;
  Sprite* sprite = fop->document->sprite();
  int sprite_w = sprite->getWidth();
  int sprite_h = sprite->getHeight();
  PixelFormat sprite_format = sprite->getPixelFormat();
  bool interlaced = gif_options->interlaced();
  int loop = 0;
  bool has_background = (sprite->getBackgroundLayer() ? true: false);
  int background_color = (sprite_format == IMAGE_INDEXED ? sprite->getTransparentColor(): 0);
  int transparent_index = (has_background ? -1: sprite->getTransparentColor());

  Palette current_palette = *sprite->getPalette(FrameNumber(0));
  Palette previous_palette(current_palette);
  RgbMap rgbmap;

  // The color map must be a power of two.
  int color_map_size = current_palette.size();
  for (int i = 30; i >= 0; --i) {
    if (color_map_size & (1 << i)) {
      color_map_size = (1 << (i + (color_map_size & (1 << (i - 1)) ? 1: 0)));
      break;
    }
  }
  ASSERT(color_map_size > 0 && color_map_size <= 256);

  ColorMapObject* color_map = NULL;
  int bpp;

  // We use a global color map only if this is a transparent GIF
  if (!has_background) {
    color_map = GifMakeMapObject(color_map_size, NULL);
    if (color_map == NULL)
      throw std::bad_alloc();

    int i;
    for (i = 0; i < current_palette.size(); ++i) {
      color_map->Colors[i].Red   = rgba_getr(current_palette.getEntry(i));
      color_map->Colors[i].Green = rgba_getg(current_palette.getEntry(i));
      color_map->Colors[i].Blue  = rgba_getb(current_palette.getEntry(i));
    }
    for (; i < color_map_size; ++i) {
      color_map->Colors[i].Red   = 0;
      color_map->Colors[i].Green = 0;
      color_map->Colors[i].Blue  = 0;
    }

    bpp = color_map->BitsPerPixel;
  }
  else
    bpp = 8;

  if (EGifPutScreenDesc(gif_file, sprite_w, sprite_h, bpp,
                        background_color, color_map) == GIF_ERROR)
    throw Exception("Error writing GIF header.\n");

  UniquePtr<Image> buffer_image;
  UniquePtr<Image> current_image(Image::create(IMAGE_INDEXED, sprite_w, sprite_h));
  UniquePtr<Image> previous_image(Image::create(IMAGE_INDEXED, sprite_w, sprite_h));
  int frame_x, frame_y, frame_w, frame_h;
  int u1, v1, u2, v2;
  int i1, j1, i2, j2;

  // If the sprite is not Indexed type, we will need a temporary
  // buffer to render the full RGB or Grayscale sprite.
  if (sprite_format != IMAGE_INDEXED)
    buffer_image.reset(Image::create(sprite_format, sprite_w, sprite_h));

  clear_image(current_image, background_color);
  clear_image(previous_image, background_color);

  ColorMapObject* image_color_map = NULL;

  for (FrameNumber frame_num(0); frame_num<sprite->getTotalFrames(); ++frame_num) {
    // If the sprite is RGB or Grayscale, we must to convert it to Indexed on the fly.
    if (sprite_format != IMAGE_INDEXED) {
      clear_image(buffer_image, background_color);
      layer_render(sprite->getFolder(), buffer_image, 0, 0, frame_num);

      switch (gif_options->quantize()) {
        case GifOptions::NoQuantize:
          sprite->getPalette(frame_num)->copyColorsTo(&current_palette);
          break;
        case GifOptions::QuantizeEach:
        case GifOptions::QuantizeAll:
          {
            current_palette.makeBlack();

            std::vector<Image*> imgarray(1);
            imgarray[0] = buffer_image;
            raster::quantization::create_palette_from_images(imgarray, &current_palette, has_background);
          }
          break;
      }

      rgbmap.regenerate(&current_palette, transparent_index);

      quantization::convert_pixel_format(
        buffer_image,
        current_image,
        IMAGE_INDEXED,
        gif_options->dithering(),
        &rgbmap,
        &current_palette,
        has_background);
    }
    // If the sprite is Indexed, we can render directly into "current_image".
    else {
      clear_image(current_image, background_color);
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
      if (EGifPutExtensionLeader(gif_file, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record (header section).");

      unsigned char extension_bytes[11];
      memcpy(extension_bytes, "NETSCAPE2.0", 11);
      if (EGifPutExtensionBlock(gif_file, 11, extension_bytes) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record (first block).");

      extension_bytes[0] = 1;
      extension_bytes[1] = (loop & 0xff);
      extension_bytes[2] = (loop >> 8) & 0xff;
      if (EGifPutExtensionBlock(gif_file, 3, extension_bytes) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record (second block).");

      if (EGifPutExtensionTrailer(gif_file) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record (trailer section).");
    }

    // Add Aseprite block (at this moment, it's empty).
    if (frame_num == 0) {
      if (EGifPutExtensionLeader(gif_file, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR)
        throw Exception("Error writing GIF comment (header section).");

      unsigned char extension_bytes[11];
      memcpy(extension_bytes, "ASEPRITE1.0", 11);
      if (EGifPutExtensionBlock(gif_file, sizeof(extension_bytes), extension_bytes) == GIF_ERROR)
        throw Exception("Error writing GIF comment (first block).");

      if (EGifPutExtensionTrailer(gif_file) == GIF_ERROR)
        throw Exception("Error writing GIF comment (trailer section).");
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
        throw Exception("Error writing GIF graphics extension record for frame %d.\n", (int)frame_num);
    }

    // Image color map
    if ((!color_map && frame_num == 0) ||
        (current_palette.countDiff(&previous_palette, NULL, NULL) > 0)) {
      if (!image_color_map) {
        image_color_map = GifMakeMapObject(current_palette.size(), NULL);
        if (image_color_map == NULL)
          throw std::bad_alloc();
      }

      for (int i = 0; i < current_palette.size(); ++i) {
        image_color_map->Colors[i].Red   = rgba_getr(current_palette.getEntry(i));
        image_color_map->Colors[i].Green = rgba_getg(current_palette.getEntry(i));
        image_color_map->Colors[i].Blue  = rgba_getb(current_palette.getEntry(i));
      }

      current_palette.copyColorsTo(&previous_palette);
    }

    // Write the image record.
    if (EGifPutImageDesc(gif_file,
                         frame_x, frame_y,
                         frame_w, frame_h, interlaced ? 1: 0,
                         image_color_map) == GIF_ERROR)
      throw Exception("Error writing GIF frame %d.\n", (int)frame_num);

    // Write the image data (pixels).
    if (interlaced) {
      // Need to perform 4 passes on the images.
      for (int i=0; i<4; ++i)
        for (int y = interlaced_offset[i]; y < frame_h; y += interlaced_jumps[i]) {
          IndexedTraits::address_t addr =
            (IndexedTraits::address_t)current_image->getPixelAddress(frame_x, frame_y + y);

          if (EGifPutLine(gif_file, addr, frame_w) == GIF_ERROR)
            throw Exception("Error writing GIF image scanlines for frame %d.\n", (int)frame_num);
        }
    }
    else {
      // Write all image scanlines (not interlaced in this case).
      for (int y=0; y<frame_h; ++y) {
        IndexedTraits::address_t addr =
          (IndexedTraits::address_t)current_image->getPixelAddress(frame_x, frame_y + y);

        if (EGifPutLine(gif_file, addr, frame_w) == GIF_ERROR)
          throw Exception("Error writing GIF image scanlines for frame %d.\n", (int)frame_num);
      }
    }

    copy_image(previous_image, current_image, 0, 0);
  }

  return true;
}
#endif

SharedPtr<FormatOptions> GifFormat::onGetFormatOptions(FileOp* fop)
{
  SharedPtr<GifOptions> gif_options;
  if (fop->document->getFormatOptions() != NULL)
    gif_options = SharedPtr<GifOptions>(fop->document->getFormatOptions());

  if (!gif_options)
    gif_options.reset(new GifOptions);

  // Non-interactive mode
  if (!fop->context || !fop->context->isUiAvailable())
    return gif_options;

  try {
    // Configuration parameters
    gif_options->setQuantize((GifOptions::Quantize)get_config_int("GIF", "Quantize", (int)gif_options->quantize()));
    gif_options->setInterlaced(get_config_bool("GIF", "Interlaced", gif_options->interlaced()));
    gif_options->setDithering((raster::DitheringMethod)get_config_int("GIF", "Dither", (int)gif_options->dithering()));

    // Load the window to ask to the user the GIF options he wants.

    app::gen::GifOptions win;
    win.rgbOptions()->setVisible(fop->document->sprite()->getPixelFormat() != IMAGE_INDEXED);

    switch (gif_options->quantize()) {
      case GifOptions::NoQuantize: win.noQuantize()->setSelected(true); break;
      case GifOptions::QuantizeEach: win.quantizeEach()->setSelected(true); break;
      case GifOptions::QuantizeAll: win.quantizeAll()->setSelected(true); break;
    }
    win.interlaced()->setSelected(gif_options->interlaced());

    win.dither()->setEnabled(true);
    win.dither()->setSelected(gif_options->dithering() == raster::DITHERING_ORDERED);

    win.openWindowInForeground();

    if (win.getKiller() == win.ok()) {
      if (win.quantizeAll()->isSelected())
        gif_options->setQuantize(GifOptions::QuantizeAll);
      else if (win.quantizeEach()->isSelected())
        gif_options->setQuantize(GifOptions::QuantizeEach);
      else if (win.noQuantize()->isSelected())
        gif_options->setQuantize(GifOptions::NoQuantize);

      gif_options->setInterlaced(win.interlaced()->isSelected());
      gif_options->setDithering(win.dither()->isSelected() ?
        raster::DITHERING_ORDERED:
        raster::DITHERING_NONE);

      set_config_int("GIF", "Quantize", gif_options->quantize());
      set_config_bool("GIF", "Interlaced", gif_options->interlaced());
      set_config_int("GIF", "Dither", gif_options->dithering());
    }
    else {
      gif_options.reset(NULL);
    }

    return gif_options;
  }
  catch (std::exception& e) {
    Console::showException(e);
    return SharedPtr<GifOptions>(0);
  }
}

} // namespace app
