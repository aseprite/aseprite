// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_spaces.h"
#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/file/gif_format.h"
#include "app/file/gif_options.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/util/autocrop.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "doc/doc.h"
#include "gfx/clip.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/button.h"

#include "gif_options.xml.h"

#include <algorithm>

#include <gif_lib.h>

#ifdef _WIN32
  #include <io.h>
  #define posix_lseek  _lseek
#else
  #include <unistd.h>
  #define posix_lseek  lseek
#endif

#if GIFLIB_MAJOR < 5
#define GifMakeMapObject MakeMapObject
#define GifFreeMapObject FreeMapObject
#define GifBitSize       BitSize
#endif

#define GIF_TRACE(...)

// GifBitSize can return 9 (it's a bug in giflib)
#define GifBitSizeLimited(v) (std::min(GifBitSize(v), 8))

namespace app {

using namespace base;

enum class DisposalMethod {
  NONE,
  DO_NOT_DISPOSE,
  RESTORE_BGCOLOR,
  RESTORE_PREVIOUS,
};

class GifFormat : public FileFormat {

  const char* onGetName() const override {
    return "gif";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("gif");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::GIF_ANIMATION;
  }

  int onGetFlags() const override {
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

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
};

FileFormat* CreateGifFormat()
{
  return new GifFormat;
}

static int interlaced_offset[] = { 0, 4, 2, 1 };
static int interlaced_jumps[] = { 8, 8, 4, 2 };

// TODO this should be part of a GifEncoder instance
// True if the GifEncoder should save the animation for Twitter:
// * Frames duration >= 2, and
// * Last frame 1/4 of its duration
static bool fix_last_frame_duration = false;

GifEncoderDurationFix::GifEncoderDurationFix(bool state)
{
  fix_last_frame_duration = state;
}

GifEncoderDurationFix::~GifEncoderDurationFix()
{
  fix_last_frame_duration = false;
}

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

static void process_disposal_method(const Image* previous,
                                    Image* current,
                                    const DisposalMethod disposal,
                                    const gfx::Rect& frameBounds,
                                    const color_t clearColor)
 {
  switch (disposal) {

    case DisposalMethod::NONE:
    case DisposalMethod::DO_NOT_DISPOSE:
      // Do nothing
      break;

    case DisposalMethod::RESTORE_BGCOLOR:
      fill_rect(current,
                frameBounds.x,
                frameBounds.y,
                frameBounds.x+frameBounds.w-1,
                frameBounds.y+frameBounds.h-1,
                clearColor);
      break;

    case DisposalMethod::RESTORE_PREVIOUS:
      current->copy(previous, gfx::Clip(frameBounds));
      break;
  }
}

static inline doc::color_t colormap2rgba(ColorMapObject* colormap, int i) {
  return doc::rgba(
    colormap->Colors[i].Red,
    colormap->Colors[i].Green,
    colormap->Colors[i].Blue, 255);
}

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
  GifDecoder(FileOp* fop, GifFileType* gifFile, int fd, size_t filesize)
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
    , m_remap(256)
    , m_hasLocalColormaps(false)
    , m_firstLocalColormap(nullptr) {
    GIF_TRACE("GIF: background index=%d\n", (int)m_gifFile->SBackGroundColor);
    GIF_TRACE("GIF: global colormap=%d, ncolors=%d\n",
              (m_gifFile->SColorMap ? 1: 0),
              (m_gifFile->SColorMap ? m_gifFile->SColorMap->ColorCount: 0));
  }

  ~GifDecoder() {
    if (m_firstLocalColormap)
      GifFreeMapObject(m_firstLocalColormap);
  }

  Sprite* releaseSprite() {
    return m_sprite.release();
  }

  bool decode() {
    GifRecordType recType;

    // Read record by record
    while ((recType = readRecordType()) != TERMINATE_RECORD_TYPE) {
      readRecord(recType);

      // Just one frame?
      if (m_fop->isOneFrame() && m_frameNum > 0)
        break;

      if (m_fop->isStop())
        break;

      if (m_filesize > 0) {
        int pos = posix_lseek(m_fd, 0, SEEK_CUR);
        m_fop->setProgress(double(pos) / double(m_filesize));
      }
    }

    if (m_sprite) {
      // Add entries to include the transparent color
      if (m_bgIndex >= m_sprite->palette(0)->size())
        m_sprite->palette(0)->resize(m_bgIndex+1);

      switch (m_sprite->pixelFormat()) {

        case IMAGE_INDEXED: {
          // Use the original global color map
          ColorMapObject* global = m_gifFile->SColorMap;
          if (!global)
            global = m_firstLocalColormap;
          if (global &&
              global->ColorCount >= m_sprite->palette(0)->size() &&
              !m_hasLocalColormaps) {
            remapToGlobalColormap(global);
          }
          break;
        }

        case IMAGE_RGB:
          // Avoid huge color palettes
          if (m_sprite->palette(0)->size() > 256) {
            reduceToAnOptimizedPalette();
          }
          break;
      }

      if (m_layer && m_opaque)
        m_layer->configureAsBackground();

      // sRGB is the default color space for GIF files
      m_sprite->setColorSpace(gfx::ColorSpace::MakeSRGB());

      return true;
    }
    else
      return false;
  }

private:

  GifRecordType readRecordType() {
    GifRecordType type;
    if (DGifGetRecordType(m_gifFile, &type) == GIF_ERROR)
      throw Exception("Invalid GIF record in file.\n");

    return type;
  }

  void readRecord(GifRecordType recordType) {
    switch (recordType) {

      case IMAGE_DESC_RECORD_TYPE:
        readImageDescRecord();
        break;

      case EXTENSION_RECORD_TYPE:
        readExtensionRecord();
        break;
    }
  }

  void readImageDescRecord() {
    if (DGifGetImageDesc(m_gifFile) == GIF_ERROR)
      throw Exception("Invalid GIF image descriptor.\n");

    // These are the bounds of the image to read.
    gfx::Rect frameBounds(
      m_gifFile->Image.Left,
      m_gifFile->Image.Top,
      m_gifFile->Image.Width,
      m_gifFile->Image.Height);

#if 0 // Generally GIF files should contain frame bounds inside the
      // canvas bounds (in other case the GIF will contain pixels that
      // are not visible). In case that some app creates an invalid
      // GIF files with bounds outside the canvas, we should support
      // to load the GIF file anyway (which is what is done by other
      // apps).
    if (!m_spriteBounds.contains(frameBounds))
      throw Exception("Image %d is out of sprite bounds.\n", (int)m_frameNum);
#endif

    // Create sprite if this is the first frame
    if (!m_sprite)
      createSprite();

    // Add a frame if it's necessary
    if (m_sprite->lastFrame() < m_frameNum)
      m_sprite->addFrame(m_frameNum);

    // Create a temporary image loading the frame pixels from the GIF file
    std::unique_ptr<Image> frameImage;
    // We don't know if a GIF file could contain empty bounds (width
    // or height=0), but we check this just in case.
    if (!frameBounds.isEmpty())
      frameImage.reset(readFrameIndexedImage(frameBounds));

    GIF_TRACE("GIF: Frame[%d] transparent index = %d\n", (int)m_frameNum, m_localTransparentIndex);

    if (m_frameNum == 0) {
      if (m_localTransparentIndex >= 0)
        m_opaque = false;
      else
        m_opaque = true;
    }

    // Merge this frame colors with the current palette
    if (frameImage)
      updatePalette(frameImage.get());

    // Convert the sprite to RGB if we have more than 256 colors
    if ((m_sprite->pixelFormat() == IMAGE_INDEXED) &&
        (m_sprite->palette(m_frameNum)->size() > 256)) {
      GIF_TRACE("GIF: Converting to RGB because we have %d colors\n",
                m_sprite->palette(m_frameNum)->size());

      convertIndexedSpriteToRgb();
    }

    // Composite frame with previous frame
    if (frameImage) {
      if (m_sprite->pixelFormat() == IMAGE_INDEXED) {
        compositeIndexedImageToIndexed(frameBounds, frameImage.get());
      }
      else {
        compositeIndexedImageToRgb(frameBounds, frameImage.get());
      }
    }

    // Create cel
    createCel();

    // Dispose/clear frame content
    process_disposal_method(m_previousImage.get(),
                            m_currentImage.get(),
                            m_disposalMethod,
                            frameBounds,
                            m_bgIndex);

    // Copy the current image into previous image
    copy_image(m_previousImage.get(), m_currentImage.get());

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
    std::unique_ptr<Image> frameImage(
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
    ColorMapObject* global = m_gifFile->SColorMap;
    ColorMapObject* colormap = m_gifFile->Image.ColorMap;

    if (!colormap) {
      // Doesn't have local map, use the global one
      colormap = global;
    }
    else if (!m_hasLocalColormaps) {
      if (!global) {
        if (!m_firstLocalColormap)
          m_firstLocalColormap = GifMakeMapObject(colormap->ColorCount,
                                                  colormap->Colors);
        global = m_firstLocalColormap;
      }

      if (global->ColorCount != colormap->ColorCount)
        m_hasLocalColormaps = true;
      else {
        for (int i=0; i<colormap->ColorCount; ++i) {
          if (global->Colors[i].Red   != colormap->Colors[i].Red ||
              global->Colors[i].Green != colormap->Colors[i].Green ||
              global->Colors[i].Blue  != colormap->Colors[i].Blue) {
            m_hasLocalColormaps = true;
            break;
          }
        }
      }
    }

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

    GIF_TRACE("GIF: Local colormap=%d, ncolors=%d\n", isLocalColormap, ncolors);

    // We'll calculate the list of used colormap indexes in this
    // frameImage.
    PalettePicks usedEntries(ncolors);
    if (isLocalColormap) {
      // With this we avoid discarding the transparent index when a
      // frame indicates that it uses a specific index as transparent
      // but the image is completely opaque anyway.
      if (m_localTransparentIndex >= 0 &&
          m_localTransparentIndex < ncolors) {
        usedEntries[m_localTransparentIndex] = true;
      }

      for (const auto& i : LockImageBits<IndexedTraits>(frameImage)) {
        if (i >= 0 && i < ncolors)
          usedEntries[i] = true;
      }
    }
    // Mark all entries as used if the colormap is global.
    else {
      usedEntries.all();
    }

    // Number of colors (indexes) used in the frame image.
    int usedNColors = usedEntries.picks();

    // Check if we need an extra color equal to the bg color in a
    // transparent frameImage.
    bool needsExtraBgColor = false;
    if (m_sprite->pixelFormat() == IMAGE_INDEXED &&
        !m_opaque && m_bgIndex != m_localTransparentIndex) {
      for (const auto& i : LockImageBits<IndexedTraits>(frameImage)) {
        if (i == m_bgIndex &&
            i != m_localTransparentIndex) {
          needsExtraBgColor = true;
          break;
        }
      }
    }

    std::unique_ptr<Palette> palette;
    if (m_frameNum == 0)
      palette.reset(new Palette(m_frameNum, usedNColors + (needsExtraBgColor ? 1: 0)));
    else {
      palette.reset(new Palette(*m_sprite->palette(m_frameNum-1)));
      palette->setFrame(m_frameNum);
    }
    resetRemap(std::max(ncolors, palette->size()));

    // Number of colors in the colormap that are part of the current
    // sprite palette.
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

    // All needed colors in the colormap are present in the current
    // palette.
    if (found == usedNColors)
      return;

    // In other case, we need to add the missing colors...

    // First index that acts like a base for new colors in palette.
    int base = (m_frameNum == 0 ? 0: palette->size());

    // Number of colors in the image that aren't in the palette.
    int missing = (usedNColors - found);

    GIF_TRACE("GIF: Bg index=%d,\n"
              "  Local transparent index=%d,\n"
              "  Need extra index to show bg color=%d,\n  "
              "  Found colors in palette=%d,\n"
              "  Used colors in local pixels=%d,\n"
              "  Base for new colors in palette=%d,\n"
              "  Colors in the image missing in the palette=%d,\n"
              "  New palette size=%d\n",
              m_bgIndex, m_localTransparentIndex, needsExtraBgColor,
              found, usedNColors, base, missing,
              base + missing + (needsExtraBgColor ? 1: 0));

    Palette oldPalette(*palette);
    palette->resize(base + missing + (needsExtraBgColor ? 1: 0));
    resetRemap(std::max(ncolors, palette->size()));

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
        palette->setEntry(j, colormap2rgba(colormap, i));
      }
      m_remap.map(i, j);
    }

    if (needsExtraBgColor) {
      int i = m_bgIndex;
      int j = base++;
      palette->setEntry(j, colormap2rgba(colormap, i));
      m_remap.map(i, j);
    }

    ASSERT(base == palette->size());
    m_sprite->setPalette(palette.get(), false);
  }

  void compositeIndexedImageToIndexed(const gfx::Rect& frameBounds,
                                      const Image* frameImage) {
    gfx::Clip clip(frameBounds.x, frameBounds.y, 0, 0,
                   frameBounds.w, frameBounds.h);
    if (!clip.clip(m_currentImage->width(),
                   m_currentImage->height(),
                   frameImage->width(),
                   frameImage->height()))
      return;

    const LockImageBits<IndexedTraits> srcBits(frameImage, clip.srcBounds());
    LockImageBits<IndexedTraits> dstBits(m_currentImage.get(), clip.dstBounds());

    auto srcIt = srcBits.begin(), srcEnd = srcBits.end();
    auto dstIt = dstBits.begin(), dstEnd = dstBits.end();

    // Compose the frame image with the previous frame
    for (; srcIt != srcEnd && dstIt != dstEnd; ++srcIt, ++dstIt) {
      color_t i = *srcIt;
      if (int(i) == m_localTransparentIndex)
        continue;

      i = m_remap[i];
      *dstIt = i;
    }

    ASSERT(srcIt == srcEnd);
    ASSERT(dstIt == dstEnd);
  }

  void compositeIndexedImageToRgb(const gfx::Rect& frameBounds,
                                  const Image* frameImage) {
    gfx::Clip clip(frameBounds.x, frameBounds.y, 0, 0,
                   frameBounds.w, frameBounds.h);
    if (!clip.clip(m_currentImage->width(),
                   m_currentImage->height(),
                   frameImage->width(),
                   frameImage->height()))
      return;

    const LockImageBits<IndexedTraits> srcBits(frameImage, clip.srcBounds());
    LockImageBits<RgbTraits> dstBits(m_currentImage.get(), clip.dstBounds());

    auto srcIt = srcBits.begin(), srcEnd = srcBits.end();
    auto dstIt = dstBits.begin(), dstEnd = dstBits.end();

    ColorMapObject* colormap = getFrameColormap();

    // Compose the frame image with the previous frame
    for (; srcIt != srcEnd && dstIt != dstEnd; ++srcIt, ++dstIt) {
      color_t i = *srcIt;
      if (int(i) == m_localTransparentIndex)
        continue;

      i = rgba(
        colormap->Colors[i].Red,
        colormap->Colors[i].Green,
        colormap->Colors[i].Blue, 255);

      *dstIt = i;
    }

    ASSERT(srcIt == srcEnd);
    ASSERT(dstIt == dstEnd);
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

  void readExtensionRecord() {
    int extCode;
    GifByteType* extension;
    if (DGifGetExtension(m_gifFile, &extCode, &extension) == GIF_ERROR)
      throw Exception("Invalid GIF extension record.\n");

    if (extCode == GRAPHICS_EXT_FUNC_CODE) {
      if (extension[0] >= 4) {
        m_disposalMethod        = (DisposalMethod)((extension[1] >> 2) & 7);
        m_localTransparentIndex = (extension[1] & 1) ? extension[4]: -1;
        m_frameDelay            = (extension[3] << 8) | extension[2];

        GIF_TRACE("GIF: Disposal method: %d\n  Transparent index: %d\n  Frame delay: %d\n",
                  m_disposalMethod, m_localTransparentIndex, m_frameDelay);
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
    int h = m_spriteBounds.h;

    m_sprite.reset(new Sprite(ImageSpec(ColorMode::INDEXED, w, h), ncolors));
    m_sprite->setTransparentColor(m_bgIndex);

    m_currentImage.reset(Image::create(IMAGE_INDEXED, w, h));
    m_previousImage.reset(Image::create(IMAGE_INDEXED, w, h));
    m_currentImage->setMaskColor(m_bgIndex);
    m_previousImage->setMaskColor(m_bgIndex);
    clear_image(m_currentImage.get(), m_bgIndex);
    clear_image(m_previousImage.get(), m_bgIndex);

    m_layer = new LayerImage(m_sprite.get());
    m_sprite->root()->addLayer(m_layer);
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
        (oldImage, nullptr, IMAGE_RGB,
         render::Dithering(),
         nullptr,
         m_sprite->palette(cel->frame()),
         m_opaque,
         m_bgIndex,
         nullptr));

      m_sprite->replaceImage(oldImage->id(), newImage);
    }

    m_currentImage.reset(
      render::convert_pixel_format
      (m_currentImage.get(), NULL, IMAGE_RGB,
       render::Dithering(),
       nullptr,
       m_sprite->palette(m_frameNum),
       m_opaque,
       m_bgIndex));

    m_previousImage.reset(
      render::convert_pixel_format
      (m_previousImage.get(), NULL, IMAGE_RGB,
       render::Dithering(),
       nullptr,
       m_sprite->palette(std::max(0, m_frameNum-1)),
       m_opaque,
       m_bgIndex));

    m_sprite->setPixelFormat(IMAGE_RGB);
  }

  void remapToGlobalColormap(ColorMapObject* colormap) {
    Palette* oldPalette = m_sprite->palette(0);
    Palette newPalette(0, colormap->ColorCount);

    for (int i=0; i<colormap->ColorCount; ++i) {
      newPalette.setEntry(i, colormap2rgba(colormap, i));;
    }

    Remap remap = create_remap_to_change_palette(
      oldPalette, &newPalette, m_bgIndex,
      m_opaque); // We cannot remap the transparent color if the
                 // sprite isn't opaque, because we
                 // cannot write the header again

    for (Cel* cel : m_sprite->uniqueCels())
      doc::remap_image(cel->image(), remap);

    m_sprite->setPalette(&newPalette, false);
  }

  void reduceToAnOptimizedPalette() {
    render::PaletteOptimizer optimizer;
    const Palette* palette = m_sprite->palette(0);

    // Feed the palette optimizer with pixels inside frameBounds
    for (int i=0; i<palette->size(); ++i) {
      optimizer.feedWithRgbaColor(palette->getEntry(i));
    }

    Palette newPalette(0, 256);
    optimizer.calculate(&newPalette, m_bgIndex);
    m_sprite->setPalette(&newPalette, false);
  }

  FileOp* m_fop;
  GifFileType* m_gifFile;
  int m_fd;
  size_t m_filesize;
  std::unique_ptr<Sprite> m_sprite;
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
  bool m_hasLocalColormaps;     // Indicates that this fila contains local colormaps

  // This is a copy of the first local color map. It's used to see if
  // all local colormaps are the same, so we can use it as a global
  // colormap.
  ColorMapObject* m_firstLocalColormap;
};

bool GifFormat::onLoad(FileOp* fop)
{
  // The filesize is used only to report some progress when we decode
  // the GIF file.
  size_t filesize = base::file_size(fop->filename());

#if GIFLIB_MAJOR >= 5
  int errCode = 0;
#endif
  int fd = open_file_descriptor_with_exception(fop->filename(), "rb");
  GifFilePtr gif_file(DGifOpenFileHandle(fd
#if GIFLIB_MAJOR >= 5
                                         , &errCode
#endif
                                         ), &DGifCloseFile);

  if (!gif_file) {
    fop->setError("Error loading GIF header.\n");
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

class GifEncoder {
public:
  typedef int gifframe_t;

  GifEncoder(FileOp* fop, GifFileType* gifFile)
    : m_fop(fop)
    , m_gifFile(gifFile)
    , m_document(fop->document())
    , m_sprite(fop->document()->sprite())
    , m_spriteBounds(m_sprite->bounds())
    , m_hasBackground(m_sprite->backgroundLayer() ? true: false)
    , m_bitsPerPixel(1)
    , m_globalColormap(nullptr)
    , m_quantizeColormaps(false) {
    if (m_sprite->pixelFormat() == IMAGE_INDEXED) {
      for (Palette* palette : m_sprite->getPalettes()) {
        int bpp = GifBitSizeLimited(palette->size());
        m_bitsPerPixel = std::max(m_bitsPerPixel, bpp);
      }
    }
    else {
      m_bitsPerPixel = 8;
    }

    if (m_sprite->pixelFormat() == IMAGE_INDEXED &&
        m_sprite->getPalettes().size() == 1) {
      // If some layer has opacity < 255 or a different blend mode, we
      // need to create color palettes.
      for (const Layer* layer : m_sprite->allVisibleLayers()) {
        if (layer->isVisible() && layer->isImage()) {
          const LayerImage* imageLayer = static_cast<const LayerImage*>(layer);
          if (imageLayer->opacity() < 255 ||
              imageLayer->blendMode() != BlendMode::NORMAL) {
            m_quantizeColormaps = true;
            break;
          }
        }
      }

      if (!m_quantizeColormaps) {
        m_globalColormap = createColorMap(m_sprite->palette(0));
        m_bgIndex = m_sprite->transparentColor();
      }
      else
        m_bgIndex = 0;
    }
    else {
      m_bgIndex = 0;
      m_quantizeColormaps = true;
    }

    m_transparentIndex = (m_hasBackground ? -1: m_bgIndex);

    if (m_hasBackground)
      m_clearColor = m_sprite->palette(0)->getEntry(m_bgIndex);
    else
      m_clearColor = rgba(0, 0, 0, 0);

    const auto gifOptions = std::static_pointer_cast<GifOptions>(fop->formatOptions());

    LOG("GIF: Saving with options: interlaced=%d loop=%d\n",
        gifOptions->interlaced(), gifOptions->loop());

    m_interlaced = gifOptions->interlaced();
    m_loop = (gifOptions->loop() ? 0: -1);

    for (int i=0; i<3; ++i)
      m_images[i].reset(Image::create(IMAGE_RGB,
                                      m_spriteBounds.w,
                                      m_spriteBounds.h));
  }

  ~GifEncoder() {
    if (m_globalColormap)
      GifFreeMapObject(m_globalColormap);
  }

  bool encode() {
    writeHeader();
    if (m_loop >= 0)
      writeLoopExtension();

    // Previous and next images are used to decide the best disposal
    // method (e.g. if it's more convenient to restore the background
    // color or to restore the previous frame to reach the next one).
    m_previousImage = m_images[0].get();
    m_currentImage = m_images[1].get();
    m_nextImage = m_images[2].get();

    auto frame_beg = m_fop->roi().selectedFrames().begin();
#if _DEBUG
    auto frame_end = m_fop->roi().selectedFrames().end();
#endif
    auto frame_it = frame_beg;

    // In this code "gifFrame" will be the GIF frame, and "frame" will
    // be the doc::Sprite frame.
    gifframe_t nframes = totalFrames();
    for (gifframe_t gifFrame=0; gifFrame<nframes; ++gifFrame) {
      ASSERT(frame_it != frame_end);
      frame_t frame = *frame_it;
      ++frame_it;

      if (gifFrame == 0)
        renderFrame(frame, m_nextImage);
      else
        std::swap(m_previousImage, m_currentImage);

      // Render next frame
      std::swap(m_currentImage, m_nextImage);
      if (gifFrame+1 < nframes)
        renderFrame(*frame_it, m_nextImage);

      gfx::Rect frameBounds;
      DisposalMethod disposal;
      calculateBestDisposalMethod(gifFrame, frameBounds, disposal);

      // TODO We could join both frames in a longer one (with more duration)
      if (frameBounds.isEmpty())
        frameBounds = gfx::Rect(0, 0, 1, 1);

      writeImage(gifFrame, frame, frameBounds, disposal,
                 // Only the last frame in the animation needs the fix
                 (fix_last_frame_duration && gifFrame == nframes-1));

      // Dispose/clear frame content
      process_disposal_method(m_previousImage,
                              m_currentImage,
                              disposal,
                              frameBounds,
                              m_clearColor);

      m_fop->setProgress(double(gifFrame+1) / double(nframes));
    }
    return true;
  }

private:

  doc::frame_t totalFrames() const {
    return m_fop->roi().frames();
  }

  void writeHeader() {
    if (EGifPutScreenDesc(m_gifFile,
                          m_spriteBounds.w,
                          m_spriteBounds.h,
                          m_bitsPerPixel,
                          m_bgIndex, m_globalColormap) == GIF_ERROR)
      throw Exception("Error writing GIF header.\n");
  }

  void writeLoopExtension() {
#if GIFLIB_MAJOR >= 5
    if (EGifPutExtensionLeader(m_gifFile, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record (header section).");

    unsigned char extension_bytes[11];
    memcpy(extension_bytes, "NETSCAPE2.0", 11);
    if (EGifPutExtensionBlock(m_gifFile, 11, extension_bytes) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record (first block).");

    extension_bytes[0] = 1;
    extension_bytes[1] = (m_loop & 0xff);
    extension_bytes[2] = (m_loop >> 8) & 0xff;
    if (EGifPutExtensionBlock(m_gifFile, 3, extension_bytes) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record (second block).");

    if (EGifPutExtensionTrailer(m_gifFile) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record (trailer section).");

#else
    unsigned char extension_bytes[11];

    memcpy(extension_bytes, "NETSCAPE2.0", 11);
    if (EGifPutExtensionFirst(m_gifFile, APPLICATION_EXT_FUNC_CODE, 11, extension_bytes) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record.\n");

    extension_bytes[0] = 1;
    extension_bytes[1] = (m_loop & 0xff);
    extension_bytes[2] = (m_loop >> 8) & 0xff;
    if (EGifPutExtensionNext(m_gifFile, APPLICATION_EXT_FUNC_CODE, 3, extension_bytes) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record.\n");

    if (EGifPutExtensionLast(m_gifFile, APPLICATION_EXT_FUNC_CODE, 0, NULL) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record.\n");
#endif
  }

  // Writes graphics extension record (to save the duration of the
  // frame and maybe the transparency index).
  void writeExtension(const gifframe_t gifFrame,
                      const frame_t frame,
                      const int transparentIndex,
                      const DisposalMethod disposalMethod,
                      const bool fixDuration) {
    unsigned char extension_bytes[5];
    int frameDelay = m_sprite->frameDuration(frame) / 10;

    // Fix duration for Twitter. It looks like the last frame must be
    // 1/4 of its duration for some strange reason in the Twitter
    // conversion from GIF to video.
    if (fixDuration)
      frameDelay = std::max(2, frameDelay/4);
    if (fix_last_frame_duration)
      frameDelay = std::max(2, frameDelay);

    extension_bytes[0] = (((int(disposalMethod) & 7) << 2) |
                          (transparentIndex >= 0 ? 1: 0));
    extension_bytes[1] = (frameDelay & 0xff);
    extension_bytes[2] = (frameDelay >> 8) & 0xff;
    extension_bytes[3] = (transparentIndex >= 0 ? transparentIndex: 0);

    if (EGifPutExtension(m_gifFile, GRAPHICS_EXT_FUNC_CODE, 4, extension_bytes) == GIF_ERROR)
      throw Exception("Error writing GIF graphics extension record for frame %d.\n", gifFrame);
  }

  static gfx::Rect calculateFrameBounds(Image* a, Image* b) {
    gfx::Rect frameBounds;
    int x1, y1, x2, y2;

    if (get_shrink_rect2(&x1, &y1, &x2, &y2, a, b)) {
      frameBounds.x = x1;
      frameBounds.y = y1;
      frameBounds.w = x2 - x1 + 1;
      frameBounds.h = y2 - y1 + 1;
    }

    return frameBounds;
  }

  void calculateBestDisposalMethod(gifframe_t gifFrame, gfx::Rect& frameBounds,
                                   DisposalMethod& disposal) {
    if (m_hasBackground) {
      disposal = DisposalMethod::DO_NOT_DISPOSE;
    }
    else {
      disposal = DisposalMethod::RESTORE_BGCOLOR;
    }

    if (gifFrame == 0) {
      frameBounds = m_spriteBounds;
    }
    else {
      gfx::Rect prev, next;

      if (gifFrame-1 >= 0)
        prev = calculateFrameBounds(m_currentImage, m_previousImage);

      if (!m_hasBackground &&
          gifFrame+1 < totalFrames())
        next = calculateFrameBounds(m_currentImage, m_nextImage);

      frameBounds = prev.createUnion(next);

      // Special case were it's better to restore the previous frame
      // when we dispose the current one than clearing with the bg
      // color.
      if (m_hasBackground && !prev.isEmpty()) {
        gfx::Rect prevNext = calculateFrameBounds(m_previousImage, m_nextImage);
        if (!prevNext.isEmpty() &&
            frameBounds.contains(prevNext) &&
            prevNext.w*prevNext.h < frameBounds.w*frameBounds.h) {
          disposal = DisposalMethod::RESTORE_PREVIOUS;
        }
      }

      GIF_TRACE("GIF: frameBounds=%d %d %d %d  prev=%d %d %d %d  next=%d %d %d %d\n",
                frameBounds.x, frameBounds.y, frameBounds.w, frameBounds.h,
                prev.x, prev.y, prev.w, prev.h,
                next.x, next.y, next.w, next.h);
    }
  }

  void writeImage(const gifframe_t gifFrame,
                  const frame_t frame,
                  const gfx::Rect& frameBounds,
                  const DisposalMethod disposal,
                  const bool fixDuration) {
    std::unique_ptr<Palette> framePaletteRef;
    std::unique_ptr<RgbMap> rgbmapRef;
    Palette* framePalette = m_sprite->palette(frame);
    RgbMap* rgbmap = m_sprite->rgbMap(frame);

    // Create optimized palette for RGB/Grayscale images
    if (m_quantizeColormaps) {
      framePaletteRef.reset(createOptimizedPalette(frameBounds));
      framePalette = framePaletteRef.get();

      rgbmapRef.reset(new RgbMap);
      rgbmap = rgbmapRef.get();
      rgbmap->regenerate(framePalette, m_transparentIndex);
    }

    // We will store the frameBounds pixels in frameImage, with the
    // indexes that must be stored in the GIF file for this specific
    // frame.
    if (!m_frameImageBuf)
      m_frameImageBuf.reset(new ImageBuffer);

    ImageRef frameImage(Image::create(IMAGE_INDEXED,
                                      frameBounds.w,
                                      frameBounds.h,
                                      m_frameImageBuf));

    // Convert the frameBounds area of m_currentImage (RGB) to frameImage (Indexed)
    // bool needsTransparent = false;
    PalettePicks usedColors(framePalette->size());

    // If the sprite needs a transparent color we mark it as used so
    // the palette includes a spot for it. It doesn't matter if the
    // image doesn't use the transparent index, if the sprite isn't
    // opaque we need the transparent index anyway.
    if (m_transparentIndex >= 0) {
      int i = m_transparentIndex;
      if (i >= usedColors.size())
        usedColors.resize(i+1);
      usedColors[i] = true;
    }

    {
      const LockImageBits<RgbTraits> srcBits(m_currentImage, frameBounds);
      LockImageBits<IndexedTraits> dstBits(
        frameImage.get(), gfx::Rect(0, 0, frameBounds.w, frameBounds.h));

      auto srcIt = srcBits.begin();
      auto dstIt = dstBits.begin();

      for (int y=0; y<frameBounds.h; ++y) {
        for (int x=0; x<frameBounds.w; ++x, ++srcIt, ++dstIt) {
          ASSERT(srcIt != srcBits.end());
          ASSERT(dstIt != dstBits.end());

          color_t color = *srcIt;
          int i;

          if (rgba_geta(color) >= 128) {
            i = framePalette->findExactMatch(
              rgba_getr(color),
              rgba_getg(color),
              rgba_getb(color),
              255,
              m_transparentIndex);
            if (i < 0)
              i = rgbmap->mapColor(rgba_getr(color),
                                   rgba_getg(color),
                                   rgba_getb(color),
                                   255);
          }
          else {
            ASSERT(m_transparentIndex >= 0);
            if (m_transparentIndex >= 0)
              i = m_transparentIndex;
            else
              i = m_bgIndex;
          }

          ASSERT(i >= 0);

          // This can happen when transparent color is outside the
          // palette range (TODO something that shouldn't be possible
          // from the program).
          if (i >= usedColors.size())
            usedColors.resize(i+1);
          usedColors[i] = true;

          *dstIt = i;
        }
      }
    }

    int usedNColors = usedColors.picks();

    Remap remap(256);
    for (int i=0; i<remap.size(); ++i)
      remap.map(i, i);

    int localTransparent = m_transparentIndex;
    ColorMapObject* colormap = m_globalColormap;
    if (!colormap) {
      Palette reducedPalette(0, usedNColors);

      for (int i=0, j=0; i<framePalette->size(); ++i) {
        if (usedColors[i]) {
          reducedPalette.setEntry(j, framePalette->getEntry(i));
          remap.map(i, j);
          ++j;
        }
      }

      colormap = createColorMap(&reducedPalette);
      if (localTransparent >= 0)
        localTransparent = remap[localTransparent];
    }

    if (localTransparent >= 0 && m_transparentIndex != localTransparent)
      remap.map(m_transparentIndex, localTransparent);

    // Write extension record.
    writeExtension(gifFrame, frame, localTransparent,
                   disposal, fixDuration);

    // Write the image record.
    if (EGifPutImageDesc(m_gifFile,
                         frameBounds.x, frameBounds.y,
                         frameBounds.w, frameBounds.h,
                         m_interlaced ? 1: 0,
                         (colormap != m_globalColormap ? colormap: nullptr)) == GIF_ERROR) {
      throw Exception("Error writing GIF frame %d.\n", gifFrame);
    }

    std::vector<uint8_t> scanline(frameBounds.w);

    // Write the image data (pixels).
    if (m_interlaced) {
      // Need to perform 4 passes on the images.
      for (int i=0; i<4; ++i)
        for (int y=interlaced_offset[i]; y<frameBounds.h; y+=interlaced_jumps[i]) {
          IndexedTraits::address_t addr =
            (IndexedTraits::address_t)frameImage->getPixelAddress(0, y);

          for (int i=0; i<frameBounds.w; ++i, ++addr)
            scanline[i] = remap[*addr];

          if (EGifPutLine(m_gifFile, &scanline[0], frameBounds.w) == GIF_ERROR)
            throw Exception("Error writing GIF image scanlines for frame %d.\n", gifFrame);
        }
    }
    else {
      // Write all image scanlines (not interlaced in this case).
      for (int y=0; y<frameBounds.h; ++y) {
        IndexedTraits::address_t addr =
          (IndexedTraits::address_t)frameImage->getPixelAddress(0, y);

        for (int i=0; i<frameBounds.w; ++i, ++addr)
          scanline[i] = remap[*addr];

        if (EGifPutLine(m_gifFile, &scanline[0], frameBounds.w) == GIF_ERROR)
          throw Exception("Error writing GIF image scanlines for frame %d.\n", gifFrame);
      }
    }

    if (colormap && colormap != m_globalColormap)
      GifFreeMapObject(colormap);
  }

  Palette* createOptimizedPalette(const gfx::Rect& frameBounds) {
    render::PaletteOptimizer optimizer;

    // Feed the palette optimizer with pixels inside frameBounds
    for (const auto& color : LockImageBits<RgbTraits>(m_currentImage, frameBounds)) {
      if (rgba_geta(color) >= 128)
        optimizer.feedWithRgbaColor(
          rgba(rgba_getr(color),
               rgba_getg(color),
               rgba_getb(color), 255));
    }

    Palette* palette = new Palette(0, 256);
    optimizer.calculate(palette, m_transparentIndex);
    return palette;
  }

  void renderFrame(frame_t frame, Image* dst) {
    render::Render render;
    render.setNewBlend(m_fop->newBlend());

    render.setBgType(render::BgType::NONE);
    clear_image(dst, m_clearColor);
    render.renderSprite(dst, m_sprite, frame);
  }

private:

  ColorMapObject* createColorMap(const Palette* palette) {
    int n = 1 << GifBitSizeLimited(palette->size());
    ColorMapObject* colormap = GifMakeMapObject(n, nullptr);

    // Color space conversions
    ConvertCS convert = convert_from_custom_to_srgb(
      m_document->osColorSpace());

    for (int i=0; i<n; ++i) {
      color_t color;
      if (i < palette->size())
        color = palette->getEntry(i);
      else
        color = rgba(0, 0, 0, 255);

      color = convert(color);

      colormap->Colors[i].Red   = rgba_getr(color);
      colormap->Colors[i].Green = rgba_getg(color);
      colormap->Colors[i].Blue  = rgba_getb(color);
    }

    return colormap;
  }

  FileOp* m_fop;
  GifFileType* m_gifFile;
  const Doc* m_document;
  const Sprite* m_sprite;
  gfx::Rect m_spriteBounds;
  bool m_hasBackground;
  int m_bgIndex;
  color_t m_clearColor;
  int m_transparentIndex;
  int m_bitsPerPixel;
  ColorMapObject* m_globalColormap;
  bool m_quantizeColormaps;
  bool m_interlaced;
  int m_loop;
  ImageBufferPtr m_frameImageBuf;
  ImageRef m_images[3];
  Image* m_previousImage;
  Image* m_currentImage;
  Image* m_nextImage;
};

bool GifFormat::onSave(FileOp* fop)
{
#if GIFLIB_MAJOR >= 5
  int errCode = 0;
#endif
  int fd = base::open_file_descriptor_with_exception(fop->filename(), "wb");
  GifFilePtr gif_file(EGifOpenFileHandle(fd
#if GIFLIB_MAJOR >= 5
                                         , &errCode
#endif
                                         ), &EGifCloseFile);

  if (!gif_file)
    throw Exception("Error creating GIF file.\n");

  GifEncoder encoder(fop, gif_file);
  bool result = encoder.encode();
  if (result)
    base::sync_file_descriptor(fd);
  return result;
}

#endif  // ENABLE_SAVE

FormatOptionsPtr GifFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<GifOptions>();
#ifdef ENABLE_UI
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();

      if (pref.isSet(pref.gif.interlaced))
        opts->setInterlaced(pref.gif.interlaced());
      if (pref.isSet(pref.gif.loop))
        opts->setLoop(pref.gif.loop());

      if (pref.gif.showAlert()) {
        app::gen::GifOptions win;
        win.interlaced()->setSelected(opts->interlaced());
        win.loop()->setSelected(opts->loop());

        win.openWindowInForeground();

        if (win.closer() == win.ok()) {
          pref.gif.interlaced(win.interlaced()->isSelected());
          pref.gif.loop(win.loop()->isSelected());
          pref.gif.showAlert(!win.dontShow()->isSelected());

          opts->setInterlaced(pref.gif.interlaced());
          opts->setLoop(pref.gif.loop());
        }
        else {
          opts.reset();
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<GifOptions>(nullptr);
    }
  }
#endif // ENABLE_UI
  return opts;
}

} // namespace app
