// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "base/fs.h"
#include "base/unique_ptr.h"
#include "doc/doc.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/alert.h"
#include "ui/button.h"

#include "generated_gif_options.h"

#include <gif_lib.h>

#ifdef _WIN32
  #include <io.h>
  #define posix_lseek  _lseek
#else
  #include <unistd.h>
  #define posix_lseek  _lseek
#endif

#if GIFLIB_MAJOR < 5
#define GifMakeMapObject MakeMapObject
#endif

namespace app {

using namespace base;

enum class DisposalMethod {
  NONE,
  DO_NOT_DISPOSE,
  RESTORE_BGCOLOR,
  RESTORE_PREVIOUS,
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
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  base::SharedPtr<FormatOptions> onGetFormatOptions(FileOp* fop) override;
};

FileFormat* CreateGifFormat()
{
  return new GifFormat;
}

static int interlaced_offset[] = { 0, 4, 2, 1 };
static int interlaced_jumps[] = { 8, 8, 4, 2 };

struct GifFilePtr {
public:
#if GIFLIB_MAJOR >= 5
  typedef int (*CloseFunc)(GifFileType*, int*);
#else
  typedef int (*CloseFunc)(GifFileType*);
#endif

  GifFilePtr(GifFileType* ptr, CloseFunc closeFunc) :
    m_ptr(ptr), m_closeFunc(closeFunc) {
  }

  ~GifFilePtr() {
#if GIFLIB_MAJOR >= 5
    int errCode;
    m_closeFunc(m_ptr, &errCode);
#else
    m_closeFunc(m_ptr);
#endif
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

// Decodes a GIF file trying to keep the image in Indexed format. If
// it's not possible to handle it as Indexed (e.g. it contains more
// than 256 colors), the file will be automatically converted to RGB.
//
// This is a complex process because GIF files are made to be composed
// over RGB output. Each frame is composed over the previous frame,
// and combinations of local colormaps can output any number of
// colors, not just 256. So previous RGB colors must be kept and
// merged with new colormaps.
class GifDecoder {
public:
  GifDecoder(FileOp* fop, GifFileType* gifFile, int fd, int filesize)
    : m_fop(fop)
    , m_gifFile(gifFile)
    , m_fd(fd)
    , m_filesize(filesize)
    , m_sprite(nullptr)
    , m_spriteBounds(0, 0, m_gifFile->SWidth, m_gifFile->SHeight)
    , m_frameNum(0)
    , m_opaque(false)
    , m_disposalMethod(DisposalMethod::NONE)
    , m_bgIndex(m_gifFile->SBackGroundColor >= 0 ? m_gifFile->SBackGroundColor: 0)
    , m_localTransparentIndex(-1)
    , m_frameDelay(1)
    , m_remap(256) {
    PRINTF("GIF background index = %d\n", (int)m_gifFile->SBackGroundColor);
  }

  Sprite* releaseSprite() {
    return m_sprite.release();
  }

  bool decode() {
    GifRecordType recType;

    // Read record by record
    while ((recType = readRecordType()) != TERMINATE_RECORD_TYPE) {
      processRecord(recType);

      // Just one frame?
      if (m_fop->oneframe && m_frameNum > 0)
        break;

      if (fop_is_stop(m_fop))
        break;

      if (m_filesize > 0) {
        int pos = posix_lseek(m_fd, 0, SEEK_CUR);
        fop_progress(m_fop, double(pos) / double(m_filesize));
      }
    }

    if (m_layer && m_opaque)
      m_layer->configureAsBackground();

    return (m_sprite != nullptr);
  }

private:

  GifRecordType readRecordType() {
    GifRecordType type;
    if (DGifGetRecordType(m_gifFile, &type) == GIF_ERROR)
      throw Exception("Invalid GIF record in file.\n");

    return type;
  }

  void processRecord(GifRecordType recordType) {
    switch (recordType) {

      case IMAGE_DESC_RECORD_TYPE:
        processImageDescRecord();
        break;

      case EXTENSION_RECORD_TYPE:
        processExtensionRecord();
        break;
    }
  }

  void processImageDescRecord() {
    if (DGifGetImageDesc(m_gifFile) == GIF_ERROR)
      throw Exception("Invalid GIF image descriptor.\n");

    // These are the bounds of the image to read.
    gfx::Rect frameBounds(
      m_gifFile->Image.Left,
      m_gifFile->Image.Top,
      m_gifFile->Image.Width,
      m_gifFile->Image.Height);

    if (!m_spriteBounds.contains(frameBounds))
      throw Exception("Image %d is out of sprite bounds.\n", (int)m_frameNum);

    // Create sprite if this is the first frame
    if (!m_sprite)
      createSprite();

    // Add a frame if it's necessary
    if (m_sprite->lastFrame() < m_frameNum)
      m_sprite->addFrame(m_frameNum);

    // Create a temporary image loading the frame pixels from the GIF file
    UniquePtr<Image> frameImage(
      readFrameIndexedImage(frameBounds));

    PRINTF("Frame[%d] transparent index = %d\n", (int)m_frameNum, m_localTransparentIndex);

    if (m_frameNum == 0) {
      if (m_localTransparentIndex >= 0)
        m_opaque = false;
      else
        m_opaque = true;
    }

    // Merge this frame colors with the current palette
    updatePalette(frameImage);

    // Convert the sprite to RGB if we have more than 256 colors
    if ((m_sprite->pixelFormat() == IMAGE_INDEXED) &&
        (m_sprite->palette(m_frameNum)->size() > 256)) {
      convertIndexedSpriteToRgb();
    }

    // Composite frame with previous frame
    if (m_sprite->pixelFormat() == IMAGE_INDEXED) {
      compositeIndexedImageToIndexed(frameBounds, frameImage);
    }
    else {
      compositeIndexedImageToRgb(frameBounds, frameImage);
    }

    // Create cel
    createCel();

    // Dispose/clear frame content
    processDisposalMethod(frameBounds);

    // Set frame delay (1/100th seconds to milliseconds)
    if (m_frameDelay >= 0)
      m_sprite->setFrameDuration(m_frameNum, m_frameDelay*10);

    // Reset extension variables
    m_disposalMethod = DisposalMethod::NONE;
    m_localTransparentIndex = -1;
    m_frameDelay = 1;

    // Next frame
    ++m_frameNum;
  }

  Image* readFrameIndexedImage(const gfx::Rect& frameBounds) {
    UniquePtr<Image> frameImage(
      Image::create(IMAGE_INDEXED, frameBounds.w, frameBounds.h));

    IndexedTraits::address_t addr;

    if (m_gifFile->Image.Interlace) {
      // Need to perform 4 passes on the image
      for (int i=0; i<4; ++i)
        for (int y = interlaced_offset[i]; y < frameBounds.h; y += interlaced_jumps[i]) {
          addr = frameImage->getPixelAddress(0, y);
          if (DGifGetLine(m_gifFile, addr, frameBounds.w) == GIF_ERROR)
            throw Exception("Invalid interlaced image data.");
        }
    }
    else {
      for (int y = 0; y < frameBounds.h; ++y) {
        addr = frameImage->getPixelAddress(0, y);
        if (DGifGetLine(m_gifFile, addr, frameBounds.w) == GIF_ERROR)
          throw Exception("Invalid image data (%d).\n"
#if GIFLIB_MAJOR >= 5
                          , m_gifFile->Error
#else
                          , GifLastError()
#endif
                          );
      }
    }

    return frameImage.release();
  }

  ColorMapObject* getFrameColormap() {
    ColorMapObject* colormap = m_gifFile->Image.ColorMap;
    if (!colormap)
      colormap = m_gifFile->SColorMap;
    if (!colormap)
      throw Exception("There is no color map.");
    return colormap;
  }

  // Adds colors used in the GIF frame so we can draw it over
  // m_currentImage. If the frame contains a local colormap, we try to
  // find them in the current sprite palette (using
  // Palette::findExactMatch()) so we don't add duplicated entries.
  // To do so we use a Remap (m_remap variable) which matches the
  // original GIF frame colors with the current sprite colors.
  void updatePalette(const Image* frameImage) {
    ColorMapObject* colormap = getFrameColormap();
    int ncolors = colormap->ColorCount;
    bool isLocalColormap = (m_gifFile->Image.ColorMap ? true: false);

    // Get the list of used palette entries
    PalettePicks usedEntries(ncolors);
    if (isLocalColormap) {
      for (const auto& i : LockImageBits<IndexedTraits>(frameImage)) {
        if (i >= 0 && i < ncolors)
          usedEntries[i] = true;
      }
    }
    // Used all entries if the colormap is global
    else
      usedEntries.all();

    int usedncolors = usedEntries.picks();

    // Check if we need an extra color equal to the bg color in a
    // transparent frameImage.
    bool needsExtraBgColor = false;
    if (!m_opaque && m_sprite->pixelFormat() == IMAGE_INDEXED) {
      for (const auto& i : LockImageBits<IndexedTraits>(frameImage)) {
        if (i == m_bgIndex &&
            i != m_localTransparentIndex) {
          needsExtraBgColor = true;
          break;
        }
      }
    }

    if (m_frameNum > 0 && !isLocalColormap)
      return;

    UniquePtr<Palette> palette;

    if (m_frameNum == 0)
      palette.reset(new Palette(m_frameNum, usedncolors + (needsExtraBgColor ? 1: 0)));
    else {
      palette.reset(new Palette(*m_sprite->palette(m_frameNum-1)));
      palette->setFrame(m_frameNum);
    }
    resetRemap(MAX(ncolors, palette->size()));

    int found = 0;
    if (m_frameNum > 0) {
      for (int i=0; i<ncolors; ++i) {
        if (!usedEntries[i])
          continue;

        int j = palette->findExactMatch(
          colormap->Colors[i].Red,
          colormap->Colors[i].Green,
          colormap->Colors[i].Blue, 255,
          (m_opaque ? -1: m_bgIndex));
        if (j >= 0) {
          m_remap.map(i, j);
          ++found;
        }
      }
    }

    if (found == usedncolors)
      return;

    Palette oldPalette(*palette);
    int base = (m_frameNum == 0 ? 0: palette->size());
    int missing = usedncolors - found;
    palette->resize(base + missing + (needsExtraBgColor ? 1: 0));
    resetRemap(MAX(ncolors, palette->size()));

    for (int i=0; i<ncolors; ++i) {
      if (!usedEntries[i])
        continue;

      int j = -1;
      if (m_frameNum > 0) {
        j = oldPalette.findExactMatch(
          colormap->Colors[i].Red,
          colormap->Colors[i].Green,
          colormap->Colors[i].Blue, 255,
          (m_opaque ? -1: m_bgIndex));
      }

      if (j < 0) {
        j = base++;
        palette->setEntry(
          j, rgba(
            colormap->Colors[i].Red,
            colormap->Colors[i].Green,
            colormap->Colors[i].Blue, 255));
      }
      m_remap.map(i, j);
    }

    if (needsExtraBgColor) {
      int j = base++;
      palette->setEntry(j, palette->getEntry(m_bgIndex));
      m_remap.map(m_bgIndex, j);
    }

    ASSERT(base == palette->size());
    m_sprite->setPalette(palette, false);
  }

  void compositeIndexedImageToIndexed(const gfx::Rect& frameBounds,
                                      const Image* frameImage) {
    // Compose the frame image with the previous frame
    for (int y=0; y<frameBounds.h; ++y) {
      for (int x=0; x<frameBounds.w; ++x) {
        color_t i = get_pixel_fast<IndexedTraits>(frameImage, x, y);
        if (i == m_localTransparentIndex)
          continue;

        i = m_remap[i];
        put_pixel_fast<IndexedTraits>(m_currentImage.get(),
                                      frameBounds.x + x,
                                      frameBounds.y + y, i);
      }
    }
  }

  void compositeIndexedImageToRgb(const gfx::Rect& frameBounds,
                                  const Image* frameImage) {
    ColorMapObject* colormap = getFrameColormap();

    // Compose the frame image with the previous frame
    for (int y=0; y<frameBounds.h; ++y) {
      for (int x=0; x<frameBounds.w; ++x) {
        color_t i = get_pixel_fast<IndexedTraits>(frameImage, x, y);
        if (i == m_localTransparentIndex)
          continue;

        i = rgba(
          colormap->Colors[i].Red,
          colormap->Colors[i].Green,
          colormap->Colors[i].Blue, 255);

        put_pixel_fast<RgbTraits>(m_currentImage.get(),
                                  frameBounds.x + x,
                                  frameBounds.y + y, i);
      }
    }
  }

  void createCel() {
    Cel* cel = new Cel(m_frameNum, ImageRef(0));
    try {
      ImageRef celImage(Image::createCopy(m_currentImage.get()));
      try {
        cel->data()->setImage(celImage);
      }
      catch (...) {
        throw;
      }
      m_layer->addCel(cel);
    }
    catch (...) {
      delete cel;
      throw;
    }
  }

  void processDisposalMethod(const gfx::Rect& frameBounds) {
    // The m_currentImage was already copied to represent the current
    // frame (m_frameNum), so now we have to clear the area occupied
    // by frameImage using the desired disposal method.
    switch (m_disposalMethod) {

      case DisposalMethod::NONE:
      case DisposalMethod::DO_NOT_DISPOSE:
        // Do nothing
        break;

      case DisposalMethod::RESTORE_BGCOLOR:
        fill_rect(m_currentImage.get(),
                  frameBounds.x,
                  frameBounds.y,
                  frameBounds.x+frameBounds.w-1,
                  frameBounds.y+frameBounds.h-1,
                  m_bgIndex);
        break;

      case DisposalMethod::RESTORE_PREVIOUS:
        copy_image(m_currentImage.get(), m_previousImage.get());
        break;
    }

    // Update previous_image with current_image only if the
    // disposal method is not "restore previous" (which means
    // that we have already updated current_image from
    // previous_image).
    if (m_disposalMethod != DisposalMethod::RESTORE_PREVIOUS)
      copy_image(m_previousImage.get(), m_currentImage.get());
  }

  void processExtensionRecord() {
    int extCode;
    GifByteType* extension;
    if (DGifGetExtension(m_gifFile, &extCode, &extension) == GIF_ERROR)
      throw Exception("Invalid GIF extension record.\n");

    if (extCode == GRAPHICS_EXT_FUNC_CODE) {
      if (extension[0] >= 4) {
        m_disposalMethod        = (DisposalMethod)((extension[1] >> 2) & 7);
        m_localTransparentIndex = (extension[1] & 1) ? extension[4]: -1;
        m_frameDelay            = (extension[3] << 8) | extension[2];

        // PRINTF("Disposal method: %d\nTransparent index: %d\nFrame delay: %d\n",
        //   disposal_method, transparentIndex, frame_delay);
      }
    }

    while (extension) {
      if (DGifGetExtensionNext(m_gifFile, &extension) == GIF_ERROR)
        throw Exception("Invalid GIF extension record.\n");
    }
  }

  void createSprite() {
    ColorMapObject* colormap = nullptr;
    if (m_gifFile->SColorMap) {
      colormap = m_gifFile->SColorMap;
    }
    else if (m_gifFile->Image.ColorMap) {
      colormap = m_gifFile->Image.ColorMap;
    }
    int ncolors = (colormap ? colormap->ColorCount: 1);
    int w = m_spriteBounds.w;
    int h = m_spriteBounds.h;;

    m_sprite.reset(new Sprite(IMAGE_INDEXED, w, h, ncolors));
    m_sprite->setTransparentColor(m_bgIndex);

    m_currentImage.reset(Image::create(IMAGE_INDEXED, w, h));
    m_previousImage.reset(Image::create(IMAGE_INDEXED, w, h));
    clear_image(m_currentImage.get(), m_bgIndex);
    clear_image(m_previousImage.get(), m_bgIndex);

    m_layer = new LayerImage(m_sprite.get());
    m_sprite->folder()->addLayer(m_layer);
  }

  void resetRemap(int ncolors) {
    m_remap = Remap(ncolors);
    for (int i=0; i<ncolors; ++i)
      m_remap.map(i, i);
  }

  // Converts the whole sprite read so far because it contains more
  // than 256 colors at the same time.
  void convertIndexedSpriteToRgb() {
    for (Cel* cel : m_sprite->uniqueCels()) {
      Image* oldImage = cel->image();

      ImageRef newImage(
        render::convert_pixel_format
        (oldImage, NULL, IMAGE_RGB, DitheringMethod::NONE,
         m_sprite->rgbMap(cel->frame()),
         m_sprite->palette(cel->frame()),
         m_opaque,              // is background
         m_bgIndex));

      m_sprite->replaceImage(oldImage->id(), newImage);
    }

    m_currentImage.reset(
      render::convert_pixel_format
      (m_currentImage.get(), NULL, IMAGE_RGB, DitheringMethod::NONE,
       m_sprite->rgbMap(m_frameNum),
       m_sprite->palette(m_frameNum),
       m_opaque,
       m_bgIndex));

    m_previousImage.reset(
      render::convert_pixel_format
      (m_previousImage.get(), NULL, IMAGE_RGB, DitheringMethod::NONE,
       m_sprite->rgbMap(MAX(0, m_frameNum-1)),
       m_sprite->palette(MAX(0, m_frameNum-1)),
       m_opaque,
       m_bgIndex));

    m_sprite->setPixelFormat(IMAGE_RGB);
  }

  FileOp* m_fop;
  GifFileType* m_gifFile;
  int m_fd;
  int m_filesize;
  UniquePtr<Sprite> m_sprite;
  gfx::Rect m_spriteBounds;
  LayerImage* m_layer;
  int m_frameNum;
  bool m_opaque;
  DisposalMethod m_disposalMethod;
  int m_bgIndex;
  int m_localTransparentIndex;
  int m_frameDelay;
  ImageRef m_currentImage;
  ImageRef m_previousImage;
  Remap m_remap;
};

bool GifFormat::onLoad(FileOp* fop)
{
  // The filesize is used only to report some progress when we decode
  // the GIF file.
  int filesize = base::file_size(fop->filename);

#if GIFLIB_MAJOR >= 5
  int errCode = 0;
#endif
  int fd = open_file_descriptor_with_exception(fop->filename, "rb");
  GifFilePtr gif_file(DGifOpenFileHandle(fd
#if GIFLIB_MAJOR >= 5
                                         , &errCode
#endif
                                         ), &DGifCloseFile);

  if (!gif_file) {
    fop_error(fop, "Error loading GIF header.\n");
    return false;
  }

  GifDecoder decoder(fop, gif_file, fd, filesize);
  if (decoder.decode()) {
    fop->createDocument(decoder.releaseSprite());
    return true;
  }
  else
    return false;
}

#ifdef ENABLE_SAVE
static int next_power_of_two(int color_map_size)
{
  for (int i = 30; i >= 0; --i) {
    if (color_map_size & (1 << i)) {
      color_map_size = (1 << (i + (color_map_size & (1 << (i - 1)) ? 1: 0)));
      break;
    }
  }
  ASSERT(color_map_size > 0 && color_map_size <= 256);
  return color_map_size;
}

bool GifFormat::onSave(FileOp* fop)
{
#if GIFLIB_MAJOR >= 5
  int errCode = 0;
#endif
  GifFilePtr gif_file(EGifOpenFileHandle(open_file_descriptor_with_exception(fop->filename, "wb")
#if GIFLIB_MAJOR >= 5
                                         , &errCode
#endif
                                         ), &EGifCloseFile);

  if (!gif_file)
    throw Exception("Error creating GIF file.\n");

  base::SharedPtr<GifOptions> gif_options = fop->seq.format_options;
  Sprite* sprite = fop->document->sprite();
  int sprite_w = sprite->width();
  int sprite_h = sprite->height();
  PixelFormat sprite_format = sprite->pixelFormat();
  bool interlaced = gif_options->interlaced();
  int loop = (gif_options->loop() ? 0: -1);
  bool has_background = (sprite->backgroundLayer() && sprite->backgroundLayer()->isVisible());
  int background_color = (sprite_format == IMAGE_INDEXED ? sprite->transparentColor(): 0);
  int transparent_index = (has_background ? -1: sprite->transparentColor());

  Palette current_palette = *sprite->palette(frame_t(0));
  Palette previous_palette(current_palette);
  RgbMap rgbmap;

  // The color map must be a power of two.
  int color_map_size = next_power_of_two(current_palette.size());
  ColorMapObject* color_map = NULL;
  int bpp;

  // We use a global color map only if this is a transparent GIF
  if (!has_background) {
    color_map = GifMakeMapObject(color_map_size, NULL);
    if (color_map == NULL)
      throw std::bad_alloc();

    for (int i = 0; i < color_map_size; ++i) {
      color_t color;
      if (i < current_palette.size())
        color = current_palette.getEntry(i);
      else
        color = rgba(0, 0, 0, 255);

      color_map->Colors[i].Red   = rgba_getr(color);
      color_map->Colors[i].Green = rgba_getg(color);
      color_map->Colors[i].Blue  = rgba_getb(color);
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

  render::Render render;
  render.setBgType(render::BgType::NONE);

  // Check if the user wants one optimized palette for all frames.
  if (sprite_format != IMAGE_INDEXED &&
      gif_options->quantize() == GifOptions::QuantizeAll) {
    // Feed the optimizer with all rendered frames.
    render::PaletteOptimizer optimizer;
    for (frame_t frame_num(0); frame_num<sprite->totalFrames(); ++frame_num) {
      clear_image(buffer_image, background_color);
      render.renderSprite(buffer_image, sprite, frame_num);
      optimizer.feedWithImage(buffer_image, false);
    }

    current_palette.makeBlack();
    optimizer.calculate(&current_palette, has_background);

    rgbmap.regenerate(&current_palette, transparent_index);
  }

  for (frame_t frame_num(0); frame_num<sprite->totalFrames(); ++frame_num) {
    // If the sprite is RGB or Grayscale, we must to convert it to Indexed on the fly.
    if (sprite_format != IMAGE_INDEXED) {
      clear_image(buffer_image, background_color);
      render.renderSprite(buffer_image, sprite, frame_num);

      switch (gif_options->quantize()) {
        case GifOptions::NoQuantize:
          sprite->palette(frame_num)->copyColorsTo(&current_palette);
          rgbmap.regenerate(&current_palette, transparent_index);
          break;
        case GifOptions::QuantizeEach:
          {
            current_palette.makeBlack();

            std::vector<Image*> imgarray(1);
            imgarray[0] = buffer_image;
            render::create_palette_from_images(imgarray, &current_palette, has_background, false);
            rgbmap.regenerate(&current_palette, transparent_index);
          }
          break;
        case GifOptions::QuantizeAll:
          // Do nothing, we've already calculate the palette for all frames.
          break;
      }

      render::convert_pixel_format(
        buffer_image,
        current_image,
        IMAGE_INDEXED,
        gif_options->dithering(),
        &rgbmap,
        &current_palette,
        has_background,
        current_image->maskColor());
    }
    // If the sprite is Indexed, we can render directly into "current_image".
    else {
      clear_image(current_image, background_color);
      render.renderSprite(current_image, sprite, frame_num);
    }

    if (frame_num == 0) {
      frame_x = 0;
      frame_y = 0;
      frame_w = sprite->width();
      frame_h = sprite->height();
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
#if GIFLIB_MAJOR >= 5
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

#else
      unsigned char extension_bytes[11];

      memcpy(extension_bytes, "NETSCAPE2.0", 11);
      if (EGifPutExtensionFirst(gif_file, APPLICATION_EXT_FUNC_CODE, 11, extension_bytes) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record for frame %d.\n", (int)frame_num);

      extension_bytes[0] = 1;
      extension_bytes[1] = (loop & 0xff);
      extension_bytes[2] = (loop >> 8) & 0xff;
      if (EGifPutExtensionNext(gif_file, APPLICATION_EXT_FUNC_CODE, 3, extension_bytes) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record for frame %d.\n", (int)frame_num);

      if (EGifPutExtensionLast(gif_file, APPLICATION_EXT_FUNC_CODE, 0, NULL) == GIF_ERROR)
        throw Exception("Error writing GIF graphics extension record for frame %d.\n", (int)frame_num);
#endif
    }

    // Write graphics extension record (to save the duration of the
    // frame and maybe the transparency index).
    {
      unsigned char extension_bytes[5];
      DisposalMethod disposal_method =
        (sprite->backgroundLayer() ? DisposalMethod::DO_NOT_DISPOSE:
                                     DisposalMethod::RESTORE_BGCOLOR);
      int frame_delay = sprite->frameDuration(frame_num) / 10;

      extension_bytes[0] = (((int(disposal_method) & 7) << 2) |
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
        color_map_size = next_power_of_two(current_palette.size());
        image_color_map = GifMakeMapObject(color_map_size, NULL);
        if (image_color_map == NULL)
          throw std::bad_alloc();
      }

      for (int i = 0; i < color_map_size; ++i) {
        color_t color;
        if (i < current_palette.size())
          color = current_palette.getEntry(i);
        else
          color = rgba(0, 0, 0, 255);

        image_color_map->Colors[i].Red   = rgba_getr(color);
        image_color_map->Colors[i].Green = rgba_getg(color);
        image_color_map->Colors[i].Blue  = rgba_getb(color);
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

    copy_image(previous_image, current_image);
  }

  return true;
#endif
}

base::SharedPtr<FormatOptions> GifFormat::onGetFormatOptions(FileOp* fop)
{
  base::SharedPtr<GifOptions> gif_options;
  if (fop->document->getFormatOptions())
    gif_options = base::SharedPtr<GifOptions>(fop->document->getFormatOptions());

  if (!gif_options)
    gif_options.reset(new GifOptions);

  // Non-interactive mode
  if (!fop->context || !fop->context->isUIAvailable())
    return gif_options;

  try {
    // Configuration parameters
    gif_options->setQuantize((GifOptions::Quantize)get_config_int("GIF", "Quantize", (int)gif_options->quantize()));
    gif_options->setInterlaced(get_config_bool("GIF", "Interlaced", gif_options->interlaced()));
    gif_options->setLoop(get_config_bool("GIF", "Loop", gif_options->loop()));
    gif_options->setDithering((doc::DitheringMethod)get_config_int("GIF", "Dither", (int)gif_options->dithering()));

    // Load the window to ask to the user the GIF options he wants.

    app::gen::GifOptions win;
    win.rgbOptions()->setVisible(fop->document->sprite()->pixelFormat() != IMAGE_INDEXED);

    switch (gif_options->quantize()) {
      case GifOptions::NoQuantize: win.noQuantize()->setSelected(true); break;
      case GifOptions::QuantizeEach: win.quantizeEach()->setSelected(true); break;
      case GifOptions::QuantizeAll: win.quantizeAll()->setSelected(true); break;
    }
    win.interlaced()->setSelected(gif_options->interlaced());
    win.loop()->setSelected(gif_options->loop());

    win.dither()->setEnabled(true);
    win.dither()->setSelected(gif_options->dithering() == doc::DitheringMethod::ORDERED);

    win.openWindowInForeground();

    if (win.getKiller() == win.ok()) {
      if (win.quantizeAll()->isSelected())
        gif_options->setQuantize(GifOptions::QuantizeAll);
      else if (win.quantizeEach()->isSelected())
        gif_options->setQuantize(GifOptions::QuantizeEach);
      else if (win.noQuantize()->isSelected())
        gif_options->setQuantize(GifOptions::NoQuantize);

      gif_options->setInterlaced(win.interlaced()->isSelected());
      gif_options->setLoop(win.loop()->isSelected());
      gif_options->setDithering(win.dither()->isSelected() ?
        doc::DitheringMethod::ORDERED:
        doc::DitheringMethod::NONE);

      set_config_int("GIF", "Quantize", gif_options->quantize());
      set_config_bool("GIF", "Interlaced", gif_options->interlaced());
      set_config_bool("GIF", "Loop", gif_options->loop());
      set_config_int("GIF", "Dither", int(gif_options->dithering()));
    }
    else {
      gif_options.reset(NULL);
    }

    return gif_options;
  }
  catch (std::exception& e) {
    Console::showException(e);
    return base::SharedPtr<GifOptions>(0);
  }
}

} // namespace app
