// Aseprite Document IO Library
// Copyright (C) 2018-2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dio/aseprite_encoder.h"

#include "base/exception.h"
#include "dio/aseprite_common.h"
#include "dio/encode_delegate.h"
#include "dio/file_interface.h"
#include "dio/pixel_io.h"
#include "doc/doc.h"
#include "fixmath/fixmath.h"
#include "fmt/format.h"
#include "zlib.h"

#include <deque>

#define ASEFILE_TRACE(...) // TRACE(__VA_ARGS__)

namespace dio {

using namespace doc;

bool AsepriteEncoder::encode()
{
  // Write the header
  AsepriteHeader header;
  prepareHeader(&header);
  writeHeader(&header);

  const Sprite* sprite = delegate()->sprite();
  bool require_new_palette_chunk = false;
  for (Palette* pal : sprite->getPalettes()) {
    if (pal->size() > 256 || pal->hasAlpha()) {
      require_new_palette_chunk = true;
      break;
    }
  }

  // Write frames
  int outputFrame = 0;
  AsepriteExternalFiles ext_files;
  for (const frame_t frame : delegate()->framesSequence()) {
    // Prepare the frame header
    AsepriteFrameHeader frame_header;
    prepareFrameHeader(&frame_header);

    // Frame duration
    frame_header.duration = sprite->frameDuration(frame);

    if (outputFrame == 0) {
      // Check if we need the "external files" chunk
      writeExternalFilesChunk(&frame_header, ext_files, sprite);

      // Save color profile in first frame
      if (delegate()->preserveColorProfile())
        writeColorProfile(&frame_header, sprite);
    }

    // is the first frame or did the palette change?
    Palette* pal = sprite->palette(frame);
    int palFrom = 0, palTo = pal->size() - 1;
    if ( // First frame or..
      (frame == delegate()->fromFrame() ||
       // This palette is different from the previous frame palette
       sprite->palette(frame - 1)->countDiff(pal, &palFrom, &palTo) > 0)) {
      // Write new palette chunk
      if (require_new_palette_chunk) {
        writePaletteChunk(&frame_header, pal, palFrom, palTo);
      }
      else {
        // Use old color chunk only when the palette has 256 or less
        // colors, and we don't need the alpha channel (as this chunk
        // is smaller than the new palette chunk).
        writeColor2Chunk(&frame_header, pal);
      }
    }

    // Write extra chunks in the first frame
    if (frame == delegate()->fromFrame()) {
      // Write sprite user data only if needed
      if (!sprite->userData().isEmpty())
        writeUserDataChunk(&frame_header, ext_files, &sprite->userData());

      // Write tilesets
      writeTilesetChunks(&frame_header, ext_files, sprite->tilesets());

      // Writer frame tags
      if (!sprite->tags().empty()) {
        writeTagsChunk(&frame_header,
                       &sprite->tags(),
                       delegate()->fromFrame(),
                       delegate()->toFrame());
        // Write user data for tags
        for (Tag* tag : sprite->tags()) {
          writeUserDataChunk(&frame_header, ext_files, &(tag->userData()));
        }
      }

      // Write layer chunks.
      // In older versions layers were before tags, but now we put tags
      // before layers so older version don't get confused by the new
      // user data chunks for tags.
      for (Layer* child : sprite->root()->layers())
        writeLayers(&header, &frame_header, ext_files, child, 0);

      // Write slice chunks
      writeSliceChunks(&frame_header,
                       ext_files,
                       sprite->slices(),
                       delegate()->fromFrame(),
                       delegate()->toFrame());
    }

    // Write cel chunks
    writeCels(&frame_header, ext_files, sprite, sprite->root(), 0, frame);

    // Write the frame header
    writeFrameHeader(&frame_header);

    // Progress
    if (delegate()->frames() > 1)
      delegate()->progress(double(outputFrame + 1) / double(delegate()->frames()));
    ++outputFrame;

    if (delegate()->isCanceled())
      break;
  }

  // Write the missing field (filesize) of the header.
  writeHeaderFileSize(&header);

  if (!f()->ok()) {
    delegate()->error("Error writing file.\n");
    return false;
  }

  return true;
}

void AsepriteEncoder::prepareHeader(AsepriteHeader* header)
{
  const Sprite* sprite = delegate()->sprite();
  const frame_t firstFrame = delegate()->fromFrame();
  const frame_t totalFrames = delegate()->frames();
  const bool composeGroups = delegate()->composeGroups();

  header->pos = tell();

  header->size = 0;
  header->magic = ASE_FILE_MAGIC;
  header->frames = totalFrames;
  header->width = sprite->width();
  header->height = sprite->height();
  header->depth = (sprite->pixelFormat() == IMAGE_RGB       ? 32 :
                   sprite->pixelFormat() == IMAGE_GRAYSCALE ? 16 :
                   sprite->pixelFormat() == IMAGE_INDEXED   ? 8 :
                                                              0);
  header->flags = (ASE_FILE_FLAG_LAYER_WITH_OPACITY |
                   (composeGroups ? ASE_FILE_FLAG_COMPOSITE_GROUPS : 0) |
                   (sprite->useLayerUuids() ? ASE_FILE_FLAG_LAYER_WITH_UUID : 0));
  header->speed = sprite->frameDuration(firstFrame);
  header->next = 0;
  header->frit = 0;
  header->transparent_index = sprite->transparentColor();
  header->ignore[0] = 0;
  header->ignore[1] = 0;
  header->ignore[2] = 0;
  header->ncolors = sprite->palette(firstFrame)->size();
  header->pixel_width = sprite->pixelRatio().w;
  header->pixel_height = sprite->pixelRatio().h;
  header->grid_x = sprite->gridBounds().x;
  header->grid_y = sprite->gridBounds().y;
  header->grid_width = sprite->gridBounds().w;
  header->grid_height = sprite->gridBounds().h;
}

void AsepriteEncoder::writeHeader(const AsepriteHeader* header)
{
  seek(header->pos);

  write32(header->size);
  write16(header->magic);
  write16(header->frames);
  write16(header->width);
  write16(header->height);
  write16(header->depth);
  write32(header->flags);
  write16(header->speed);
  write32(header->next);
  write32(header->frit);
  write8(header->transparent_index);
  write8(header->ignore[0]);
  write8(header->ignore[1]);
  write8(header->ignore[2]);
  write16(header->ncolors);
  write8(header->pixel_width);
  write8(header->pixel_height);
  write16(header->grid_x);
  write16(header->grid_y);
  write16(header->grid_width);
  write16(header->grid_height);

  seek(header->pos + 128);
}

void AsepriteEncoder::writeHeaderFileSize(AsepriteHeader* header)
{
  header->size = tell() - header->pos;

  seek(header->pos);
  write32(header->size);

  seek(header->pos + header->size);
}

void AsepriteEncoder::prepareFrameHeader(AsepriteFrameHeader* frame_header)
{
  const int pos = tell();

  frame_header->size = pos;
  frame_header->magic = ASE_FILE_FRAME_MAGIC;
  frame_header->chunks = 0;
  frame_header->duration = 0;

  seek(pos + 16);
}

void AsepriteEncoder::writeFrameHeader(AsepriteFrameHeader* frame_header)
{
  const int pos = frame_header->size;
  const int end = tell();

  frame_header->size = end - pos;

  seek(pos);

  write32(frame_header->size);
  write16(frame_header->magic);
  write16(frame_header->chunks < 0xFFFF ? frame_header->chunks : 0xFFFF);
  write16(frame_header->duration);
  writePadding(2);
  write32(frame_header->chunks);

  seek(end);
}

void AsepriteEncoder::writeLayers(const AsepriteHeader* header,
                                  AsepriteFrameHeader* frame_header,
                                  const AsepriteExternalFiles& ext_files,
                                  const Layer* layer,
                                  int child_index)
{
  writeLayerChunk(header, frame_header, layer, child_index);
  if (!layer->userData().isEmpty())
    writeUserDataChunk(frame_header, ext_files, &layer->userData());

  if (layer->isGroup()) {
    for (const Layer* child : layer->layers())
      writeLayers(header, frame_header, ext_files, child, child_index + 1);
  }
}

layer_t AsepriteEncoder::writeCels(AsepriteFrameHeader* frame_header,
                                   const AsepriteExternalFiles& ext_files,
                                   const Sprite* sprite,
                                   const Layer* layer,
                                   layer_t layer_index,
                                   const frame_t frame)
{
  if (layer->isImage()) {
    const Cel* cel = layer->cel(frame);
    if (cel) {
      writeCelChunk(frame_header, cel, static_cast<const LayerImage*>(layer), layer_index);

      if (layer->isReference())
        writeCelExtraChunk(frame_header, cel);

      if (!cel->link() && !cel->data()->userData().isEmpty()) {
        writeUserDataChunk(frame_header, ext_files, &cel->data()->userData());
      }
    }
  }

  if (layer != sprite->root())
    ++layer_index;

  if (layer->isGroup()) {
    for (const Layer* child : layer->layers()) {
      layer_index = writeCels(frame_header, ext_files, sprite, child, layer_index, frame);
    }
  }

  return layer_index;
}

void AsepriteEncoder::writePadding(const size_t bytes)
{
  for (size_t c = 0; c < bytes; ++c)
    write8(0);
}

void AsepriteEncoder::writeString(const std::string& string)
{
  write16(string.size());

  for (size_t c = 0; c < string.size(); ++c)
    write8(string[c]);
}

void AsepriteEncoder::writeFloat(const float value)
{
  const int b = *(reinterpret_cast<const int*>(&value));
  write32(b);
}

void AsepriteEncoder::writeDouble(const double value)
{
  const long long b = *(reinterpret_cast<const long long*>(&value));
  write64(b);
}

void AsepriteEncoder::writePoint(const gfx::Point& point)
{
  write32(point.x);
  write32(point.y);
}

void AsepriteEncoder::writeSize(const gfx::Size& size)
{
  write32(size.w);
  write32(size.h);
}

void AsepriteEncoder::writeUuid(const base::Uuid& uuid)
{
  for (int i = 0; i < 16; ++i)
    write8(uuid[i]);
}

void AsepriteEncoder::writeChunkStart(AsepriteFrameHeader* frame_header,
                                      int type,
                                      AsepriteChunk* chunk)
{
  frame_header->chunks++;

  chunk->type = type;
  chunk->start = tell();

  write32(0);
  write16(0);
}

void AsepriteEncoder::writeChunkEnd(AsepriteChunk* chunk)
{
  const int chunk_end = tell();
  const int chunk_size = chunk_end - chunk->start;

  seek(chunk->start);
  write32(chunk_size);
  write16(chunk->type);
  seek(chunk_end);
}

void AsepriteEncoder::writeColor2Chunk(AsepriteFrameHeader* frame_header, const Palette* pal)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_FLI_COLOR2);

  write16(1); // Number of packets

  // First packet
  write8(0);                                    // skip 0 colors
  ASSERT(pal->size() <= 256);                   // For >256 we use the palette chunk
  write8(pal->size() == 256 ? 0 : pal->size()); // number of colors
  for (int c = 0; c < pal->size(); c++) {
    const color_t color = pal->getEntry(c);
    write8(rgba_getr(color));
    write8(rgba_getg(color));
    write8(rgba_getb(color));
  }
}

void AsepriteEncoder::writePaletteChunk(AsepriteFrameHeader* frame_header,
                                        const Palette* pal,
                                        int from,
                                        int to)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_PALETTE);

  write32(pal->size());
  write32(from);
  write32(to);
  writePadding(8);

  for (int c = from; c <= to; ++c) {
    const color_t color = pal->getEntry(c);
    // TODO add support to save palette entry name
    write16(0); // Entry flags (without name)
    write8(rgba_getr(color));
    write8(rgba_getg(color));
    write8(rgba_getb(color));
    write8(rgba_geta(color));
  }
}

void AsepriteEncoder::writeLayerChunk(const AsepriteHeader* header,
                                      AsepriteFrameHeader* frame_header,
                                      const Layer* layer,
                                      const int child_level)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_LAYER);

  // Flags
  write16(static_cast<int>(layer->flags()) & static_cast<int>(LayerFlags::PersistentFlagsMask));

  // Layer type
  bool saveBlendInfo = false;
  int layerType = ASE_FILE_LAYER_IMAGE;
  if (layer->isImage()) {
    saveBlendInfo = true;
    if (layer->isTilemap())
      layerType = ASE_FILE_LAYER_TILEMAP;
  }
  else if (layer->isGroup()) {
    layerType = ASE_FILE_LAYER_GROUP;

    // If the "composite groups" flag is not specified, group layers
    // don't contain blend mode + opacity.
    if ((header->flags & ASE_FILE_FLAG_COMPOSITE_GROUPS) == ASE_FILE_FLAG_COMPOSITE_GROUPS) {
      saveBlendInfo = true;
    }
  }
  write16(layerType);

  // Layer child level
  write16(child_level);

  // Default width & height, and blend mode
  write16(0);
  write16(0);
  write16(saveBlendInfo ? int(layer->blendMode()) : 0);
  write8(saveBlendInfo ? layer->opacity() : 0);

  // Padding
  writePadding(3);

  // Layer name
  writeString(layer->name());

  // Tileset index
  if (layer->isTilemap())
    write32(static_cast<const LayerTilemap*>(layer)->tilesetIndex());

  if ((header->flags & ASE_FILE_FLAG_LAYER_WITH_UUID) == ASE_FILE_FLAG_LAYER_WITH_UUID)
    writeUuid(layer->uuid());
}

//////////////////////////////////////////////////////////////////////
// Pixel I/O
//////////////////////////////////////////////////////////////////////

namespace {

class ScanlinesGen {
public:
  virtual ~ScanlinesGen() {}
  virtual gfx::Size getImageSize() const = 0;
  virtual int getScanlineSize() const = 0;
  virtual const uint8_t* getScanlineAddress(int y) const = 0;
};

class ImageScanlines : public ScanlinesGen {
  const Image* m_image;

public:
  ImageScanlines(const Image* image) : m_image(image) {}
  gfx::Size getImageSize() const override { return gfx::Size(m_image->width(), m_image->height()); }
  int getScanlineSize() const override { return m_image->widthBytes(); }
  const uint8_t* getScanlineAddress(int y) const override { return m_image->getPixelAddress(0, y); }
};

class TilesetScanlines : public ScanlinesGen {
  const Tileset* m_tileset;

public:
  TilesetScanlines(const Tileset* tileset) : m_tileset(tileset) {}
  gfx::Size getImageSize() const override
  {
    return gfx::Size(m_tileset->grid().tileSize().w,
                     m_tileset->grid().tileSize().h * m_tileset->size());
  }
  int getScanlineSize() const override
  {
    return bytes_per_pixel_for_colormode(m_tileset->sprite()->colorMode()) *
           m_tileset->grid().tileSize().w;
  }
  const uint8_t* getScanlineAddress(int y) const override
  {
    const int h = m_tileset->grid().tileSize().h;
    const tile_index ti = (y / h);
    ASSERT(ti >= 0 && ti < m_tileset->size());
    const ImageRef image = m_tileset->get(ti);
    ASSERT(image);
    if (image)
      return image->getPixelAddress(0, y % h);
    return nullptr;
  }
};

//////////////////////////////////////////////////////////////////////
// Raw Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
void write_raw_image_templ(FileInterface* f, const ScanlinesGen* gen)
{
  const gfx::Size imgSize = gen->getImageSize();
  PixelIO<ImageTraits> pixel_io;
  int x, y;

  for (y = 0; y < imgSize.h; ++y) {
    typename ImageTraits::address_t address =
      (typename ImageTraits::address_t)gen->getScanlineAddress(y);

    for (x = 0; x < imgSize.w; ++x, ++address)
      pixel_io.write_pixel(f, *address);
  }
}

void write_raw_image(FileInterface* f, ScanlinesGen* gen, PixelFormat pixelFormat)
{
  switch (pixelFormat) {
    case IMAGE_RGB:       write_raw_image_templ<RgbTraits>(f, gen); break;
    case IMAGE_GRAYSCALE: write_raw_image_templ<GrayscaleTraits>(f, gen); break;
    case IMAGE_INDEXED:   write_raw_image_templ<IndexedTraits>(f, gen); break;
    case IMAGE_TILEMAP:   write_raw_image_templ<TilemapTraits>(f, gen); break;
  }
}

//////////////////////////////////////////////////////////////////////
// Compressed Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
void write_compressed_image_templ(FileInterface* f,
                                  ScanlinesGen* gen,
                                  base::buffer* compressedOutput)
{
  PixelIO<ImageTraits> pixel_io;
  z_stream zstream;
  int y, err;

  zstream.zalloc = (alloc_func)0;
  zstream.zfree = (free_func)0;
  zstream.opaque = (voidpf)0;
  err = deflateInit(&zstream, Z_DEFAULT_COMPRESSION);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in deflateInit().", err);

  std::vector<uint8_t> scanline(gen->getScanlineSize());
  std::vector<uint8_t> compressed(4096);

  const gfx::Size imgSize = gen->getImageSize();
  for (y = 0; y < imgSize.h; ++y) {
    typename ImageTraits::address_t address =
      (typename ImageTraits::address_t)gen->getScanlineAddress(y);

    pixel_io.write_scanline(address, imgSize.w, scanline.data());

    zstream.next_in = (Bytef*)scanline.data();
    zstream.avail_in = scanline.size();
    const int flush = (y == imgSize.h - 1 ? Z_FINISH : Z_NO_FLUSH);

    do {
      zstream.next_out = (Bytef*)compressed.data();
      zstream.avail_out = compressed.size();

      // Compress
      err = deflate(&zstream, flush);
      if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR)
        throw base::Exception("ZLib error %d in deflate().", err);

      const int output_bytes = compressed.size() - zstream.avail_out;
      if (output_bytes > 0) {
        if ((f->writeBytes(compressed.data(), output_bytes) != (size_t)output_bytes) || !f->ok())
          throw base::Exception("Error writing compressed image pixels.\n");

        // Save the whole compressed buffer to re-use in following
        // save options (so we don't have to re-compress the whole
        // tileset)
        if (compressedOutput) {
          const std::size_t n = compressedOutput->size();
          compressedOutput->resize(n + output_bytes);
          std::copy(compressed.begin(),
                    compressed.begin() + output_bytes,
                    compressedOutput->begin() + n);
        }
      }
    } while (zstream.avail_out == 0);
  }

  err = deflateEnd(&zstream);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in deflateEnd().", err);
}

void write_compressed_image(FileInterface* f,
                            ScanlinesGen* gen,
                            PixelFormat pixelFormat,
                            base::buffer* compressedOutput = nullptr)
{
  switch (pixelFormat) {
    case IMAGE_RGB: write_compressed_image_templ<RgbTraits>(f, gen, compressedOutput); break;
    case IMAGE_GRAYSCALE:
      write_compressed_image_templ<GrayscaleTraits>(f, gen, compressedOutput);
      break;
    case IMAGE_INDEXED:
      write_compressed_image_templ<IndexedTraits>(f, gen, compressedOutput);
      break;
    case IMAGE_TILEMAP:
      write_compressed_image_templ<TilemapTraits>(f, gen, compressedOutput);
      break;
  }
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Cel Chunk
//////////////////////////////////////////////////////////////////////

void AsepriteEncoder::writeCelChunk(AsepriteFrameHeader* frame_header,
                                    const Cel* cel,
                                    const LayerImage* layer,
                                    const layer_t layer_index)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_CEL);
  const frame_t firstFrame = delegate()->fromFrame();
  const Cel* link = cel->link();

  // In case the original link is outside the ROI, we've to find the
  // first linked cel that is inside the ROI.
  if (link && link->frame() < firstFrame) {
    link = nullptr;
    for (frame_t i = firstFrame; i <= cel->frame(); ++i) {
      link = layer->cel(i);
      if (link && link->image()->id() == cel->image()->id())
        break;
    }
    if (link == cel)
      link = nullptr;
  }

  const int cel_type = (link                      ? ASE_FILE_LINK_CEL :
                        cel->layer()->isTilemap() ? delegate()->preferredTilemapCelType() :
                                                    delegate()->preferredCelType());

  write16(layer_index);
  write16(cel->x());
  write16(cel->y());
  write8(cel->opacity());
  write16(cel_type);
  write16(cel->zIndex());
  writePadding(5);

  switch (cel_type) {
    case ASE_FILE_RAW_CEL: {
      const Image* image = cel->image();

      if (image) {
        // Width and height
        write16(image->width());
        write16(image->height());

        // Pixel data
        ImageScanlines scan(image);
        write_raw_image(f(), &scan, image->pixelFormat());
      }
      else {
        // Width and height
        write16(0);
        write16(0);
      }
      break;
    }

    case ASE_FILE_LINK_CEL:
      ASSERT(link);
      // This can happen only if preferredTilemapCelType() or
      // preferredCelType() returned ASE_FILE_LINK_CEL.
      if (!link)
        throw base::Exception("Invalid linked cel type returned by the delegate");

      // Linked cel to another frame
      write16(link->frame() - firstFrame);
      break;

    case ASE_FILE_COMPRESSED_CEL: {
      const Image* image = cel->image();
      ASSERT(image);
      if (image) {
        // Width and height
        write16(image->width());
        write16(image->height());

        ImageScanlines scan(image);
        write_compressed_image(f(), &scan, image->pixelFormat());
      }
      else {
        // Width and height
        write16(0);
        write16(0);
      }
      break;
    }

    case ASE_FILE_COMPRESSED_TILEMAP: {
      const Image* image = cel->image();
      ASSERT(image);
      ASSERT(image->pixelFormat() == IMAGE_TILEMAP);

      write16(image->width());
      write16(image->height());
      write16(32); // TODO use different bpp when possible
      write32(tile_i_mask);
      write32(tile_f_xflip);
      write32(tile_f_yflip);
      write32(tile_f_dflip);
      writePadding(10);

      ImageScanlines scan(image);
      write_compressed_image(f(), &scan, IMAGE_TILEMAP);
    }
  }
}

void AsepriteEncoder::writeCelExtraChunk(AsepriteFrameHeader* frame_header, const Cel* cel)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_CEL_EXTRA);

  ASSERT(cel->layer()->isReference());

  const gfx::RectF& bounds = cel->boundsF();

  write32(ASE_CEL_EXTRA_FLAG_PRECISE_BOUNDS);
  write32(fixmath::ftofix(bounds.x));
  write32(fixmath::ftofix(bounds.y));
  write32(fixmath::ftofix(bounds.w));
  write32(fixmath::ftofix(bounds.h));
  writePadding(16);
}

void AsepriteEncoder::writeColorProfile(AsepriteFrameHeader* frame_header, const Sprite* sprite)
{
  const gfx::ColorSpaceRef& cs = sprite->colorSpace();
  if (!cs) // No color
    return;

  int type = ASE_FILE_NO_COLOR_PROFILE;
  switch (cs->type()) {
    case gfx::ColorSpace::None: return; // Without color profile, don't write this chunk.

    case gfx::ColorSpace::sRGB: type = ASE_FILE_SRGB_COLOR_PROFILE; break;
    case gfx::ColorSpace::ICC:  type = ASE_FILE_ICC_COLOR_PROFILE; break;
    default:
      ASSERT(false); // Unknown color profile
      return;
  }

  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_COLOR_PROFILE);
  write16(type);
  write16(cs->hasGamma() ? ASE_COLOR_PROFILE_FLAG_GAMMA : 0);

  fixmath::fixed gamma = 0;
  if (cs->hasGamma())
    gamma = fixmath::ftofix(cs->gamma());
  write32(gamma);
  writePadding(8);

  if (cs->type() == gfx::ColorSpace::ICC) {
    const size_t size = cs->iccSize();
    const void* data = cs->iccData();
    write32(size);
    if (size && data)
      writeBytes((uint8_t*)data, size);
  }
}

void AsepriteEncoder::writeMaskChunk(AsepriteFrameHeader* frame_header, Mask* mask)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_MASK);

  int c, u, v, byte;
  const gfx::Rect& bounds(mask->bounds());

  write16(bounds.x);
  write16(bounds.y);
  write16(bounds.w);
  write16(bounds.h);
  writePadding(8);

  // Name
  writeString(mask->name());

  // Bitmap
  for (v = 0; v < bounds.h; v++)
    for (u = 0; u < (bounds.w + 7) / 8; u++) {
      byte = 0;
      for (c = 0; c < 8; c++)
        if (get_pixel(mask->bitmap(), (u * 8) + c, v))
          byte |= (1 << (7 - c));
      write8(byte);
    }
}

void AsepriteEncoder::writeTagsChunk(AsepriteFrameHeader* frame_header,
                                     const Tags* tags,
                                     const frame_t fromFrame,
                                     const frame_t toFrame)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_TAGS);

  int ntags = 0;
  for (const Tag* tag : *tags) {
    // Skip tags that are outside of the given ROI
    if (tag->fromFrame() > toFrame || tag->toFrame() < fromFrame)
      continue;
    ++ntags;
  }

  write16(ntags);
  write32(0); // 8 reserved bytes
  write32(0);

  for (const Tag* tag : *tags) {
    if (tag->fromFrame() > toFrame || tag->toFrame() < fromFrame)
      continue;

    const frame_t from = std::clamp(tag->fromFrame() - fromFrame, 0, toFrame - fromFrame);
    const frame_t to = std::clamp(tag->toFrame() - fromFrame, from, toFrame - fromFrame);

    write16(from);
    write16(to);
    write8((int)tag->aniDir());

    write16(std::clamp(tag->repeat(), 0, Tag::kMaxRepeat)); // repeat
    write16(0);                                             // 6 reserved bytes
    write32(0);

    write8(rgba_getr(tag->color()));
    write8(rgba_getg(tag->color()));
    write8(rgba_getb(tag->color()));
    write8(0);

    writeString(tag->name());
  }
}

void AsepriteEncoder::writeUserDataChunk(AsepriteFrameHeader* frame_header,
                                         const AsepriteExternalFiles& ext_files,
                                         const UserData* userData)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_USER_DATA);

  const size_t nmaps = count_nonempty_properties_maps(userData->propertiesMaps());
  int flags = 0;
  if (!userData->text().empty())
    flags |= ASE_USER_DATA_FLAG_HAS_TEXT;
  if (rgba_geta(userData->color()))
    flags |= ASE_USER_DATA_FLAG_HAS_COLOR;
  if (nmaps)
    flags |= ASE_USER_DATA_FLAG_HAS_PROPERTIES;
  write32(flags);

  if (flags & ASE_USER_DATA_FLAG_HAS_TEXT)
    writeString(userData->text());

  if (flags & ASE_USER_DATA_FLAG_HAS_COLOR) {
    write8(rgba_getr(userData->color()));
    write8(rgba_getg(userData->color()));
    write8(rgba_getb(userData->color()));
    write8(rgba_geta(userData->color()));
  }

  if (flags & ASE_USER_DATA_FLAG_HAS_PROPERTIES) {
    writePropertiesMaps(ext_files, nmaps, userData->propertiesMaps());
  }
}

void AsepriteEncoder::writeSliceChunks(AsepriteFrameHeader* frame_header,
                                       const AsepriteExternalFiles& ext_files,
                                       const Slices& slices,
                                       const frame_t fromFrame,
                                       const frame_t toFrame)
{
  for (Slice* slice : slices) {
    // Skip slices that are outside of the given ROI
    if (slice->range(fromFrame, toFrame).empty())
      continue;

    writeSliceChunk(frame_header, slice, fromFrame, toFrame);

    if (!slice->userData().isEmpty())
      writeUserDataChunk(frame_header, ext_files, &slice->userData());
  }
}

void AsepriteEncoder::writeSliceChunk(AsepriteFrameHeader* frame_header,
                                      Slice* slice,
                                      const frame_t fromFrame,
                                      const frame_t toFrame)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_SLICE);

  const frame_t firstFromFrame = slice->empty() ? fromFrame : slice->fromFrame();
  auto range = slice->range(firstFromFrame, toFrame);
  ASSERT(!range.empty());

  int flags = 0;
  for (auto* key : range) {
    if (key) {
      if (key->hasCenter())
        flags |= ASE_SLICE_FLAG_HAS_CENTER_BOUNDS;
      if (key->hasPivot())
        flags |= ASE_SLICE_FLAG_HAS_PIVOT_POINT;
    }
  }

  write32(range.countKeys()); // number of keys
  write32(flags);             // flags
  write32(0);                 // 4 bytes reserved
  writeString(slice->name()); // slice name

  frame_t frame = firstFromFrame;
  const SliceKey* oldKey = nullptr;
  for (auto* key : range) {
    if (frame == firstFromFrame || key != oldKey) {
      write32(frame);
      write32((int32_t)(key ? key->bounds().x : 0));
      write32((int32_t)(key ? key->bounds().y : 0));
      write32(key ? key->bounds().w : 0);
      write32(key ? key->bounds().h : 0);

      if (flags & ASE_SLICE_FLAG_HAS_CENTER_BOUNDS) {
        if (key && key->hasCenter()) {
          write32((int32_t)key->center().x);
          write32((int32_t)key->center().y);
          write32(key->center().w);
          write32(key->center().h);
        }
        else {
          write32(0);
          write32(0);
          write32(0);
          write32(0);
        }
      }

      if (flags & ASE_SLICE_FLAG_HAS_PIVOT_POINT) {
        if (key && key->hasPivot()) {
          write32((int32_t)key->pivot().x);
          write32((int32_t)key->pivot().y);
        }
        else {
          write32(0);
          write32(0);
        }
      }

      oldKey = key;
    }
    ++frame;
  }
}

void AsepriteEncoder::writeExternalFilesChunk(AsepriteFrameHeader* frame_header,
                                              AsepriteExternalFiles& ext_files,
                                              const Sprite* sprite)
{
  auto putExtentionIds = [](const UserData::PropertiesMaps& propertiesMaps,
                            AsepriteExternalFiles& ext_files) {
    for (const auto& propertiesMap : propertiesMaps) {
      if (!propertiesMap.first.empty())
        ext_files.insert(ASE_EXTERNAL_FILE_EXTENSION, propertiesMap.first);
    }
  };

  for (const Tileset* tileset : *sprite->tilesets()) {
    if (!tileset)
      continue;

    if (!tileset->externalFilename().empty()) {
      ext_files.insert(ASE_EXTERNAL_FILE_TILESET, tileset->externalFilename());
    }

    putExtentionIds(tileset->userData().propertiesMaps(), ext_files);

    for (tile_index i = 0; i < tileset->size(); ++i) {
      UserData tileData = tileset->getTileData(i);
      putExtentionIds(tileData.propertiesMaps(), ext_files);
    }
  }

  putExtentionIds(sprite->userData().propertiesMaps(), ext_files);

  for (Tag* tag : sprite->tags()) {
    putExtentionIds(tag->userData().propertiesMaps(), ext_files);
  }

  // Go through all the layers collecting all the extension IDs we find
  std::deque<Layer*> layers(sprite->root()->layers().begin(), sprite->root()->layers().end());
  while (!layers.empty()) {
    auto* layer = layers.front();
    layers.pop_front();

    putExtentionIds(layer->userData().propertiesMaps(), ext_files);
    if (layer->isGroup()) {
      auto childLayers = layer->layers();
      layers.insert(layers.end(), childLayers.begin(), childLayers.end());
    }
    else if (layer->isImage()) {
      for (const frame_t frame : delegate()->framesSequence()) {
        const Cel* cel = layer->cel(frame);
        if (cel && !cel->link()) {
          putExtentionIds(cel->data()->userData().propertiesMaps(), ext_files);
        }
      }
    }
  }

  for (Slice* slice : sprite->slices()) {
    // Skip slices that are outside of the given ROI
    if (slice->range(delegate()->fromFrame(), delegate()->toFrame()).empty())
      continue;

    putExtentionIds(slice->userData().propertiesMaps(), ext_files);
  }

  // Tile management plugin
  if (sprite->hasTileManagementPlugin()) {
    ext_files.insert(ASE_EXTERNAL_FILE_TILE_MANAGEMENT, sprite->tileManagementPlugin());
  }

  // No external files to write
  if (ext_files.items().empty())
    return;

  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_EXTERNAL_FILE);
  write32(ext_files.items().size()); // Number of entries
  writePadding(8);
  for (const auto& it : ext_files.items()) {
    write32(it.first);      // ID
    write8(it.second.type); // Type
    writePadding(7);
    writeString(it.second.fn); // Filename
  }
}

void AsepriteEncoder::writeTilesetChunks(AsepriteFrameHeader* frame_header,
                                         const AsepriteExternalFiles& ext_files,
                                         const Tilesets* tilesets)
{
  tileset_index si = 0;
  for (const Tileset* tileset : *tilesets) {
    if (tileset) {
      writeTilesetChunk(frame_header, ext_files, tileset, si);
      writeUserDataChunk(frame_header, ext_files, &tileset->userData());

      // Write tile UserData
      for (tile_index i = 0; i < tileset->size(); ++i) {
        const UserData tileData = tileset->getTileData(i);
        writeUserDataChunk(frame_header, ext_files, &tileData);
      }
    }
    ++si;
  }
}

void AsepriteEncoder::writeTilesetChunk(AsepriteFrameHeader* frame_header,
                                        const AsepriteExternalFiles& ext_files,
                                        const Tileset* tileset,
                                        const tileset_index si)
{
  const ChunkWriter chunk(this, frame_header, ASE_FILE_CHUNK_TILESET);

  // We always save with the tile zero as the empty tile now
  int flags = ASE_TILESET_FLAG_ZERO_IS_NOTILE;
  if (!tileset->externalFilename().empty())
    flags |= ASE_TILESET_FLAG_EXTERNAL_FILE;
  else
    flags |= ASE_TILESET_FLAG_EMBEDDED;

  const tile_flags tf = tileset->matchFlags();
  if (tf & tile_f_xflip)
    flags |= ASE_TILESET_FLAG_MATCH_XFLIP;
  if (tf & tile_f_yflip)
    flags |= ASE_TILESET_FLAG_MATCH_YFLIP;
  if (tf & tile_f_dflip)
    flags |= ASE_TILESET_FLAG_MATCH_DFLIP;

  write32(si);    // Tileset ID
  write32(flags); // Tileset Flags
  write32(tileset->size());
  write16(tileset->grid().tileSize().w);
  write16(tileset->grid().tileSize().h);
  write16(short(tileset->baseIndex()));
  writePadding(14);
  writeString(tileset->name()); // tileset name

  // Flag 1 = external tileset
  if (flags & ASE_TILESET_FLAG_EXTERNAL_FILE) {
    uint32_t file_id = 0;
    if (ext_files.getIDByFilename(ASE_EXTERNAL_FILE_TILESET, tileset->externalFilename(), file_id)) {
      write32(file_id);
      write32(tileset->externalTileset());
    }
    else {
      ASSERT(false); // Impossible state (corrupted memory or we
                     // forgot to add the tileset external file to
                     // "ext_files")

      write32(0);
      write32(0);
      delegate()->error("Error writing tileset external reference.\n");
    }
  }

  // Flag 2 = tileset
  if (flags & ASE_TILESET_FLAG_EMBEDDED) {
    const size_t beg = tell();

    // Save the cached tileset compressed data
    if (!tileset->compressedData().empty() &&
        tileset->compressedDataVersion() == tileset->version()) {
      const base::buffer& data = tileset->compressedData();

      ASEFILE_TRACE("[%d] saving compressed tileset (%s)\n",
                    tileset->id(),
                    base::get_pretty_memory_size(data.size()).c_str());

      write32(data.size()); // Compressed data length
      writeBytes((uint8_t*)data.data(), data.size());
    }
    // Compress and save the tileset now
    else {
      write32(0); // Field for compressed data length (completed later)
      TilesetScanlines gen(tileset);

      ASEFILE_TRACE("[%d] recompressing tileset\n", tileset->id());

      base::buffer compressedData;
      base::buffer* compressedDataPtr = nullptr;
      if (delegate()->cacheCompressedTilesets())
        compressedDataPtr = &compressedData;

      write_compressed_image(f(), &gen, tileset->sprite()->pixelFormat(), compressedDataPtr);

      // As we've just compressed the tileset, we can cache this same
      // data (so saving the file again will not need recompressing).
      if (compressedDataPtr)
        tileset->setCompressedData(compressedData);

      const size_t end = tell();
      seek(beg);
      write32(end - beg - 4); // Save the compressed data length
      seek(end);
    }
  }
}

void AsepriteEncoder::writePropertyValue(const UserData::Variant& value)
{
  switch (value.type()) {
    case USER_DATA_PROPERTY_TYPE_NULLPTR: ASSERT(false); break;
    case USER_DATA_PROPERTY_TYPE_BOOL:
      write8(static_cast<uint8_t>(*std::get_if<bool>(&value)));
      break;
    case USER_DATA_PROPERTY_TYPE_INT8:   write8(*std::get_if<int8_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_UINT8:  write8(*std::get_if<uint8_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_INT16:  write16(*std::get_if<int16_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_UINT16: write16(*std::get_if<uint16_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_INT32:  write32(*std::get_if<int32_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_UINT32: write32(*std::get_if<uint32_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_INT64:  write64(*std::get_if<int64_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_UINT64: write64(*std::get_if<uint64_t>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_FIXED:  write32(std::get_if<UserData::Fixed>(&value)->value); break;
    case USER_DATA_PROPERTY_TYPE_FLOAT:  writeFloat(*std::get_if<float>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_DOUBLE: writeDouble(*std::get_if<double>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_STRING: writeString(*std::get_if<std::string>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_POINT:  writePoint(*std::get_if<gfx::Point>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_SIZE:   writeSize(*std::get_if<gfx::Size>(&value)); break;
    case USER_DATA_PROPERTY_TYPE_RECT:   {
      const auto& rc = *std::get_if<gfx::Rect>(&value);
      writePoint(rc.origin());
      writeSize(rc.size());
      break;
    }
    case USER_DATA_PROPERTY_TYPE_VECTOR: {
      const auto& vector = *std::get_if<UserData::Vector>(&value);
      write32(vector.size());

      const uint16_t type = all_elements_of_same_type(vector);
      write16(type);

      for (const auto& elem : vector) {
        UserData::Variant v = elem;

        // Reduce each element if possible, because each element has
        // its own type.
        if (type == 0) {
          if (is_reducible_int(v)) {
            v = reduce_int_type_size(v);
          }
          write16(v.type());
        }
        // Reduce to the smaller/common int type.
        else if (is_reducible_int(v) && type < v.type()) {
          v = cast_to_smaller_int_type(v, type);
        }

        writePropertyValue(v);
      }
      break;
    }
    case USER_DATA_PROPERTY_TYPE_PROPERTIES: {
      const auto& properties = *std::get_if<UserData::Properties>(&value);
      ASSERT(properties.size() > 0);
      write32(properties.size());

      for (const auto& property : properties) {
        const std::string& name = property.first;
        writeString(name);

        UserData::Variant v = property.second;
        if (is_reducible_int(v)) {
          v = reduce_int_type_size(v);
        }
        write16(v.type());

        writePropertyValue(v);
      }
      break;
    }
    case USER_DATA_PROPERTY_TYPE_UUID: {
      const auto& uuid = *std::get_if<base::Uuid>(&value);
      writeUuid(uuid);
      break;
    }
  }
}

void AsepriteEncoder::writePropertiesMaps(const AsepriteExternalFiles& ext_files,
                                          size_t nmaps,
                                          const UserData::PropertiesMaps& propertiesMaps)
{
  ASSERT(nmaps > 0);

  const long startPos = tell();
  // We zero the size in bytes of all properties maps stored in this
  // chunk for now. (actual value is calculated after serialization
  // of all properties maps, at which point this field is overwritten)
  write32(0);

  write32(nmaps);
  for (const auto& propertiesMap : propertiesMaps) {
    const UserData::Properties& properties = propertiesMap.second;
    // Skip properties map if it doesn't have any property
    if (properties.empty())
      continue;

    const std::string& extensionKey = propertiesMap.first;
    uint32_t extensionId = 0;
    if (!extensionKey.empty() &&
        !ext_files.getIDByFilename(ASE_EXTERNAL_FILE_EXTENSION, extensionKey, extensionId)) {
      // This shouldn't ever happen, but if it does...  most likely
      // it is because we forgot to add the extensionID to the
      // ext_files object. And this could happen if someone adds the
      // possibility to store custom properties to some object that
      // didn't support it previously.
      ASSERT(false);
      delegate()->error(fmt::format("Error writing properties for extension '{}'.", extensionKey));

      // We have to write something for this extensionId, because we
      // wrote the number of expected property maps (nmaps) in the
      // header.
      // continue;
    }
    write32(extensionId);
    writePropertyValue(properties);
  }
  const long endPos = tell();
  // We can overwrite the properties maps size now
  seek(startPos);
  write32(endPos - startPos);
  // Let's go back to where we were
  seek(endPos);
}

} // namespace dio
