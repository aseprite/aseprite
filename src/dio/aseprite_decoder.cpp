// Aseprite Document IO Library
// Copyright (c) 2018-2026 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dio/aseprite_decoder.h"

#include "base/cfile.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/mask_shift.h"
#include "dio/aseprite_common.h"
#include "dio/decode_delegate.h"
#include "dio/file_interface.h"
#include "dio/pixel_io.h"
#include "doc/doc.h"
#include "doc/util.h"
#include "fixmath/fixmath.h"
#include "fmt/format.h"
#include "gfx/color_space.h"
#include "zlib.h"

#include <cstdio>
#include <vector>

namespace dio {

using namespace doc;

bool AsepriteDecoder::decode()
{
  bool ignore_old_color_chunks = false;

  AsepriteHeader header;
  if (!readHeader(&header)) {
    delegate()->error("Error reading header");
    return false;
  }

  if (header.depth != 32 && header.depth != 16 && header.depth != 8) {
    delegate()->error(fmt::format("Invalid color depth {0}", header.depth));
    return false;
  }

  if (header.width < 1 || header.height < 1) {
    delegate()->error(fmt::format("Invalid sprite size {0}x{1}", header.width, header.height));
    return false;
  }

  // Create the new sprite
  std::unique_ptr<Sprite> sprite(
    std::make_unique<Sprite>(ImageSpec(header.depth == 32 ? ColorMode::RGB :
                                       header.depth == 16 ? ColorMode::GRAYSCALE :
                                                            ColorMode::INDEXED,
                                       header.width,
                                       header.height),
                             header.ncolors));

  // Set frames and speed
  sprite->setTotalFrames(frame_t(header.frames));
  sprite->setDurationForAllFrames(header.speed);

  // Set transparent entry
  sprite->setTransparentColor(header.transparent_index);

  // Set pixel ratio
  sprite->setPixelRatio(PixelRatio(header.pixel_width, header.pixel_height));

  // Set grid bounds
  sprite->setGridBounds(
    gfx::Rect(header.grid_x, header.grid_y, header.grid_width, header.grid_height));

  sprite->useLayerUuids((header.flags & ASE_FILE_FLAG_LAYER_WITH_UUID) ==
                        ASE_FILE_FLAG_LAYER_WITH_UUID);

  // Prepare variables for layer chunks
  Layer* last_layer = sprite->root();
  WithUserData* last_object_with_user_data = sprite.get();
  Cel* last_cel = nullptr;
  auto tag_it = sprite->tags().begin();
  auto tag_end = sprite->tags().end();

  m_allLayers.clear();

  int current_level = -1;
  AsepriteExternalFiles extFiles;

  // Just one frame?
  frame_t nframes = sprite->totalFrames();
  if (nframes > 1 && delegate()->decodeOneFrame())
    nframes = 1;

  // Read frame by frame to end-of-file
  for (frame_t frame = 0; frame < nframes; ++frame) {
    // Start frame position
    const size_t frame_pos = tell();
    delegate()->progress((float)frame_pos / (float)header.size);

    // Read frame header
    AsepriteFrameHeader frame_header;
    readFrameHeader(&frame_header);

    // Correct frame type
    if (frame_header.magic == ASE_FILE_FRAME_MAGIC) {
      // Use frame-duration field?
      if (frame_header.duration > 0)
        sprite->setFrameDuration(frame, frame_header.duration);

      // Read chunks
      for (uint32_t c = 0; c < frame_header.chunks; c++) {
        // Start chunk position
        const size_t chunk_pos = tell();
        delegate()->progress((float)chunk_pos / (float)header.size);

        // Read chunk information
        const int chunk_size = read32();
        const int chunk_type = read16();

        switch (chunk_type) {
          case ASE_FILE_CHUNK_FLI_COLOR:
          case ASE_FILE_CHUNK_FLI_COLOR2:
            if (!ignore_old_color_chunks) {
              Palette* prevPal = sprite->palette(frame);
              const std::unique_ptr<Palette> pal(chunk_type == ASE_FILE_CHUNK_FLI_COLOR ?
                                                   readColorChunk(prevPal, frame) :
                                                   readColor2Chunk(prevPal, frame));

              if (prevPal->countDiff(pal.get(), NULL, NULL) > 0)
                sprite->setPalette(pal.get(), true);
            }
            break;

          case ASE_FILE_CHUNK_PALETTE: {
            Palette* prevPal = sprite->palette(frame);
            const std::unique_ptr<Palette> pal(readPaletteChunk(prevPal, frame));

            if (prevPal->countDiff(pal.get(), NULL, NULL) > 0)
              sprite->setPalette(pal.get(), true);

            ignore_old_color_chunks = true;
            break;
          }

          case ASE_FILE_CHUNK_LAYER: {
            Layer* newLayer = readLayerChunk(&header, sprite.get(), &last_layer, &current_level);
            if (newLayer) {
              m_allLayers.push_back(newLayer);
              last_object_with_user_data = newLayer;
            }
            else {
              // Add a null layer only to match the "layer index" in cel chunk
              m_allLayers.push_back(nullptr);
              last_object_with_user_data = nullptr;
            }
            break;
          }

          case ASE_FILE_CHUNK_CEL: {
            Cel* cel = readCelChunk(frame, sprite->pixelFormat(), &header, chunk_pos + chunk_size);
            if (cel) {
              last_cel = cel;
              last_object_with_user_data = cel->data();
            }
            else {
              last_object_with_user_data = nullptr;
            }
            break;
          }

          case ASE_FILE_CHUNK_CEL_EXTRA: {
            if (last_cel)
              readCelExtraChunk(last_cel);
            break;
          }

          case ASE_FILE_CHUNK_COLOR_PROFILE: {
            readColorProfile(sprite.get());
            break;
          }

          case ASE_FILE_CHUNK_EXTERNAL_FILE:
            readExternalFiles(extFiles);

            // Tile management plugin
            if (!extFiles.empty()) {
              std::string fn = extFiles.tileManagementPlugin();
              if (!fn.empty())
                sprite->setTileManagementPlugin(fn);
            }
            break;

          case ASE_FILE_CHUNK_MASK: {
            Mask* mask = readMaskChunk();
            if (mask)
              delete mask; // TODO add the mask in some place?
            else
              delegate()->error("Warning: Cannot load a mask chunk");
            break;
          }

          case ASE_FILE_CHUNK_PATH:
            // Ignore
            break;

          case ASE_FILE_CHUNK_TAGS:
            readTagsChunk(&sprite->tags());
            tag_it = sprite->tags().begin();
            tag_end = sprite->tags().end();

            if (tag_it != tag_end)
              last_object_with_user_data = *tag_it;
            else
              last_object_with_user_data = nullptr;
            break;

          case ASE_FILE_CHUNK_SLICES: {
            readSlicesChunk(sprite->slices());
            break;
          }

          case ASE_FILE_CHUNK_SLICE: {
            Slice* slice = readSliceChunk(sprite->slices());
            if (slice)
              last_object_with_user_data = slice;
            break;
          }

          case ASE_FILE_CHUNK_USER_DATA: {
            UserData userData;
            readUserDataChunk(&userData, extFiles);

            if (last_object_with_user_data) {
              last_object_with_user_data->setUserData(userData);

              switch (last_object_with_user_data->type()) {
                case ObjectType::Tag:
                  // Tags are a special case, user data for tags come
                  // all together (one next to other) after the tags
                  // chunk, in the same order:
                  //
                  // * TAGS CHUNK (TAG1, TAG2, ..., TAGn)
                  // * USER DATA CHUNK FOR TAG1
                  // * USER DATA CHUNK FOR TAG2
                  // * ...
                  // * USER DATA CHUNK FOR TAGn
                  //
                  // So here we expect that the next user data chunk
                  // will correspond to the next tag in the tags
                  // collection.
                  ++tag_it;

                  if (tag_it != tag_end)
                    last_object_with_user_data = *tag_it;
                  else
                    last_object_with_user_data = nullptr;
                  break;
                case ObjectType::Tileset:
                  // Read tiles user datas.
                  // TODO: Should we refactor how tile user data is handled so we can actually
                  // decode this user data chunks the same way as the user data chunks for
                  // the tags?
                  Tileset* tileset = static_cast<Tileset*>(last_object_with_user_data);
                  readTilesData(tileset, extFiles);
                  last_object_with_user_data = nullptr;
                  break;
              }
            }
            break;
          }

          case ASE_FILE_CHUNK_TILESET: {
            Tileset* tileset = readTilesetChunk(sprite.get(), &header, extFiles);
            if (tileset)
              last_object_with_user_data = tileset;
            break;
          }

          default:
            delegate()->incompatibilityError(
              fmt::format("Warning: Unsupported chunk type {0} (skipping)", chunk_type));
            break;
        }

        // Skip chunk size
        seek(chunk_pos + chunk_size);
      }
    }

    // Skip frame size
    seek(frame_pos + frame_header.size);

    if (delegate()->isCanceled())
      break;
  }

  delegate()->onSprite(sprite.release());
  return true;
}

bool AsepriteDecoder::readHeader(AsepriteHeader* header)
{
  const size_t headerPos = tell();

  header->size = read32();
  header->magic = read16();

  // Developers can open any .ase file
#if !defined(ENABLE_DEVMODE)
  if (header->magic != ASE_FILE_MAGIC)
    return false;
#endif

  header->frames = read16();
  header->width = read16();
  header->height = read16();
  header->depth = read16();
  header->flags = read32();
  header->speed = read16();
  header->next = read32();
  header->frit = read32();
  header->transparent_index = read8();
  header->ignore[0] = read8();
  header->ignore[1] = read8();
  header->ignore[2] = read8();
  header->ncolors = read16();
  header->pixel_width = read8();
  header->pixel_height = read8();
  header->grid_x = (int16_t)read16();
  header->grid_y = (int16_t)read16();
  header->grid_width = read16();
  header->grid_height = read16();

  if (header->depth != 8) // Transparent index only valid for indexed images
    header->transparent_index = 0;

  if (header->ncolors == 0) // 0 means 256 (old .ase files)
    header->ncolors = 256;

  if (header->pixel_width == 0 || header->pixel_height == 0) {
    header->pixel_width = 1;
    header->pixel_height = 1;
  }

#if defined(ENABLE_DEVMODE)
  // This is useful to read broken .ase files
  if (header->magic != ASE_FILE_MAGIC) {
    header->frames = 256; // Frames number might be not enought for some files
    header->width = 1024; // Size doesn't matter, the sprite can be crop
    header->height = 1024;
  }
#endif

  seek(headerPos + 128);
  return true;
}

void AsepriteDecoder::readFrameHeader(AsepriteFrameHeader* frame_header)
{
  frame_header->size = read32();
  frame_header->magic = read16();
  frame_header->chunks = read16();
  frame_header->duration = read16();
  readPadding(2);
  uint32_t nchunks = read32();

  if (frame_header->chunks == 0xFFFF && frame_header->chunks < nchunks)
    frame_header->chunks = nchunks;
}

void AsepriteDecoder::readPadding(int bytes)
{
  for (int c = 0; c < bytes; c++)
    read8();
}

std::string AsepriteDecoder::readString()
{
  int length = read16();
  if (length == EOF)
    return "";

  std::string string;
  string.reserve(length + 1);

  for (int c = 0; c < length; c++)
    string.push_back(read8());

  return string;
}

float AsepriteDecoder::readFloat()
{
  int b1, b2, b3, b4;

  if ((b1 = read8()) == EOF)
    return EOF;

  if ((b2 = read8()) == EOF)
    return EOF;

  if ((b3 = read8()) == EOF)
    return EOF;

  if ((b4 = read8()) == EOF)
    return EOF;

  // Little endian.
  int v = ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
  return *reinterpret_cast<float*>(&v);
}

double AsepriteDecoder::readDouble()
{
  int b1, b2, b3, b4, b5, b6, b7, b8;

  if ((b1 = read8()) == EOF)
    return EOF;

  if ((b2 = read8()) == EOF)
    return EOF;

  if ((b3 = read8()) == EOF)
    return EOF;

  if ((b4 = read8()) == EOF)
    return EOF;

  if ((b5 = read8()) == EOF)
    return EOF;

  if ((b6 = read8()) == EOF)
    return EOF;

  if ((b7 = read8()) == EOF)
    return EOF;

  if ((b8 = read8()) == EOF)
    return EOF;

  // Little endian.
  long long v = (((long long)b8 << 56) | ((long long)b7 << 48) | ((long long)b6 << 40) |
                 ((long long)b5 << 32) | ((long long)b4 << 24) | ((long long)b3 << 16) |
                 ((long long)b2 << 8) | (long long)b1);
  return *reinterpret_cast<double*>(&v);
}

Palette* AsepriteDecoder::readColorChunk(Palette* prevPal, frame_t frame)
{
  int i, c, r, g, b, packets, skip, size;
  Palette* pal = new Palette(*prevPal);
  pal->setFrame(frame);

  packets = read16(); // Number of packets
  skip = 0;

  // Read all packets
  for (i = 0; i < packets; i++) {
    skip += read8();
    size = read8();
    if (!size)
      size = 256;

    for (c = skip; c < skip + size; c++) {
      r = read8();
      g = read8();
      b = read8();
      pal->setEntry(
        c,
        rgba(scale_6bits_to_8bits(r), scale_6bits_to_8bits(g), scale_6bits_to_8bits(b), 255));
    }
  }

  return pal;
}

Palette* AsepriteDecoder::readColor2Chunk(Palette* prevPal, frame_t frame)
{
  int i, c, r, g, b, packets, skip, size;
  Palette* pal = new Palette(*prevPal);
  pal->setFrame(frame);

  packets = read16(); // Number of packets
  skip = 0;

  // Read all packets
  for (i = 0; i < packets; i++) {
    skip += read8();
    size = read8();
    if (!size)
      size = 256;

    for (c = skip; c < skip + size; c++) {
      r = read8();
      g = read8();
      b = read8();
      pal->setEntry(c, rgba(r, g, b, 255));
    }
  }

  return pal;
}

Palette* AsepriteDecoder::readPaletteChunk(Palette* prevPal, frame_t frame)
{
  Palette* pal = new Palette(*prevPal);
  pal->setFrame(frame);

  int newSize = read32();
  int from = read32();
  int to = read32();
  readPadding(8);

  if (newSize > 0)
    pal->resize(newSize);

  for (int c = from; c <= to; ++c) {
    int flags = read16();
    int r = read8();
    int g = read8();
    int b = read8();
    int a = read8();
    pal->setEntry(c, rgba(r, g, b, a));

    // Skip name
    if (flags & ASE_PALETTE_FLAG_HAS_NAME) {
      std::string name = readString();
      // Ignore color entry name
    }
  }

  return pal;
}

Layer* AsepriteDecoder::readLayerChunk(AsepriteHeader* header,
                                       Sprite* sprite,
                                       Layer** previous_layer,
                                       int* current_level)
{
  // Read chunk data
  int flags = read16();
  int layer_type = read16();
  int child_level = read16();
  read16();                 // default width
  read16();                 // default height
  int blendmode = read16(); // blend mode
  int opacity = read8();    // opacity
  readPadding(3);
  std::string name = readString();

  Layer* layer = nullptr;
  switch (layer_type) {
    case ASE_FILE_LAYER_IMAGE:   layer = new LayerImage(sprite); break;

    case ASE_FILE_LAYER_GROUP:   layer = new LayerGroup(sprite); break;

    case ASE_FILE_LAYER_TILEMAP: {
      const tileset_index tsi = read32();
      if (!sprite->tilesets()->get(tsi)) {
        delegate()->error(fmt::format("Error: tileset {0} not found", tsi));
        return nullptr;
      }
      layer = new LayerTilemap(sprite, tsi);
      break;
    }

    default:
      delegate()->incompatibilityError(fmt::format("Unknown layer type found: {0}", layer_type));
      break;
  }

  if (!layer)
    return nullptr;

  // Read UUID if usage is enabled
  if (sprite->useLayerUuids())
    layer->setUuid(readUuid());

  const bool composeGroups = (header->flags & ASE_FILE_FLAG_COMPOSITE_GROUPS);

  if ((layer->isImage() || (layer->isGroup() && composeGroups)) &&
      // Only transparent layers can have blend mode and opacity
      !(flags & int(LayerFlags::Background))) {
    layer->setBlendMode((BlendMode)blendmode);
    if (header->flags & ASE_FILE_FLAG_LAYER_WITH_OPACITY)
      layer->setOpacity(opacity);
  }

  // flags
  layer->setFlags(
    static_cast<LayerFlags>(flags & static_cast<int>(LayerFlags::PersistentFlagsMask)));

  // name
  layer->setName(name.c_str());

  // Child level
  if (child_level == *current_level)
    (*previous_layer)->parent()->addLayer(layer);
  else if (child_level > *current_level)
    static_cast<LayerGroup*>(*previous_layer)->addLayer(layer);
  else if (child_level < *current_level) {
    LayerGroup* parent = (*previous_layer)->parent();
    ASSERT(parent);
    if (parent) {
      int levels = (*current_level - child_level);
      while (levels--) {
        ASSERT(parent->parent());
        if (!parent->parent())
          break;
        parent = parent->parent();
      }
      parent->addLayer(layer);
    }
  }

  *previous_layer = layer;
  *current_level = child_level;

  return layer;
}

//////////////////////////////////////////////////////////////////////
// Raw Image
//////////////////////////////////////////////////////////////////////

namespace {

template<typename ImageTraits>
void read_raw_image_templ(FileInterface* f,
                          DecodeDelegate* delegate,
                          Image* image,
                          const AsepriteHeader* header)
{
  PixelIO<ImageTraits> pixel_io;
  int x, y;
  int w = image->width();
  int h = image->height();

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      put_pixel_fast<ImageTraits>(image, x, y, pixel_io.read_pixel(f));
    }
    delegate->progress((float)f->tell() / (float)header->size);
  }
}

void read_raw_image(FileInterface* f,
                    DecodeDelegate* delegate,
                    Image* image,
                    const AsepriteHeader* header)
{
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       read_raw_image_templ<RgbTraits>(f, delegate, image, header); break;

    case IMAGE_GRAYSCALE: read_raw_image_templ<GrayscaleTraits>(f, delegate, image, header); break;

    case IMAGE_INDEXED:   read_raw_image_templ<IndexedTraits>(f, delegate, image, header); break;

    case IMAGE_TILEMAP:   read_raw_image_templ<TilemapTraits>(f, delegate, image, header); break;
  }
}

//////////////////////////////////////////////////////////////////////
// Compressed Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
void read_compressed_image_templ(FileInterface* f,
                                 DecodeDelegate* delegate,
                                 Image* image,
                                 const AsepriteHeader* header,
                                 const size_t chunk_end)
{
  PixelIO<ImageTraits> pixel_io;
  z_stream zstream;
  zstream.zalloc = (alloc_func)0;
  zstream.zfree = (free_func)0;
  zstream.opaque = (voidpf)0;

  int err = inflateInit(&zstream);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in inflateInit().", err);

  const int width = image->width();
  const int widthBytes = image->widthBytes();
  std::vector<uint8_t> scanline(widthBytes);
  std::vector<uint8_t> compressed(4096);
  std::vector<uint8_t> uncompressed(4096);
  int scanline_offset = 0;
  int y = 0;

  while (true) {
    size_t input_bytes;

    if (f->tell() + compressed.size() > chunk_end) {
      input_bytes = chunk_end - f->tell(); // Remaining bytes
      ASSERT(input_bytes < compressed.size());

      if (input_bytes == 0)
        break; // Done, we consumed all chunk
    }
    else {
      input_bytes = compressed.size();
    }

    size_t bytes_read = f->readBytes(&compressed[0], input_bytes);

    // Error reading "input_bytes" bytes, broken file? chunk without
    // enough compressed data?
    if (bytes_read == 0) {
      delegate->error(fmt::format("Error reading {} bytes of compressed data", input_bytes));
      break;
    }

    zstream.next_in = (Bytef*)&compressed[0];
    zstream.avail_in = bytes_read;

    do {
      zstream.next_out = (Bytef*)&uncompressed[0];
      zstream.avail_out = uncompressed.size();

      err = inflate(&zstream, Z_NO_FLUSH);
      if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR)
        throw base::Exception("ZLib error %d in inflate().", err);

      size_t uncompressed_bytes = uncompressed.size() - zstream.avail_out;
      if (uncompressed_bytes > 0) {
        int i = 0;
        while (y < image->height()) {
          int n = std::min(uncompressed_bytes, scanline.size() - scanline_offset);
          if (n > 0) {
            // Fill the scanline buffer until it's completed
            std::copy(uncompressed.begin() + i,
                      uncompressed.begin() + i + n,
                      scanline.begin() + scanline_offset);
            uncompressed_bytes -= n;
            scanline_offset += n;
            i += n;
          }
          else if (scanline_offset < widthBytes) {
            // The scanline is not filled yet.
            break;
          }
          else {
            // Copy the whole scanline to the image
            pixel_io.read_scanline((typename ImageTraits::address_t)image->getPixelAddress(0, y),
                                   width,
                                   &scanline[0]);
            ++y;
            scanline_offset = 0;
            if (uncompressed_bytes == 0)
              break;
          }
        }
      }
    } while (zstream.avail_in != 0 && zstream.avail_out == 0);

    delegate->progress((float)f->tell() / (float)header->size);
  }

  err = inflateEnd(&zstream);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in inflateEnd().", err);
}

void read_compressed_image(FileInterface* f,
                           DecodeDelegate* delegate,
                           Image* image,
                           const AsepriteHeader* header,
                           const size_t chunk_end)
{
  // Try to read pixel data
  try {
    switch (image->pixelFormat()) {
      case IMAGE_RGB:
        read_compressed_image_templ<RgbTraits>(f, delegate, image, header, chunk_end);
        break;

      case IMAGE_GRAYSCALE:
        read_compressed_image_templ<GrayscaleTraits>(f, delegate, image, header, chunk_end);
        break;

      case IMAGE_INDEXED:
        read_compressed_image_templ<IndexedTraits>(f, delegate, image, header, chunk_end);
        break;

      case IMAGE_TILEMAP:
        read_compressed_image_templ<TilemapTraits>(f, delegate, image, header, chunk_end);
        break;
    }
  }
  // OK, in case of error we can show the problem, but continue
  // loading more cels.
  catch (const std::exception& e) {
    delegate->error(e.what());
  }
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Cel Chunk
//////////////////////////////////////////////////////////////////////

Cel* AsepriteDecoder::readCelChunk(frame_t frame,
                                   PixelFormat pixelFormat,
                                   const AsepriteHeader* header,
                                   const size_t chunk_end)
{
  // Read chunk data
  const layer_t layer_index = read16();
  int x = ((int16_t)read16());
  int y = ((int16_t)read16());
  int opacity = read8();
  int cel_type = read16();
  int zIndex = ((int16_t)read16());
  readPadding(5);

  if (cel_type == ASE_FILE_RAW_CEL || cel_type == ASE_FILE_COMPRESSED_CEL)
    m_celType = cel_type;

  Layer* layer = nullptr;
  if (layer_index >= 0 && layer_index < layer_t(m_allLayers.size()))
    layer = m_allLayers[layer_index];

  if (!layer) {
    delegate()->error(
      fmt::format("Frame {0} didn't found layer with index {1}", (int)frame, (int)layer_index));
    return nullptr;
  }
  if (!layer->isImage()) {
    delegate()->error(
      fmt::format("Invalid .ase file (frame {0} in layer {1} which does not contain images",
                  (int)frame,
                  (int)layer_index));
    return nullptr;
  }

  // Create the new frame.
  std::unique_ptr<Cel> cel;

  switch (cel_type) {
    case ASE_FILE_RAW_CEL: {
      // Read width and height
      int w = read16();
      int h = read16();

      if (w > 0 && h > 0) {
        // Read pixel data
        const ImageRef image(Image::create(pixelFormat, w, h));
        read_raw_image(f(), delegate(), image.get(), header);

        cel = std::make_unique<Cel>(frame, image);
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
        cel->setZIndex(zIndex);
      }
      break;
    }

    case ASE_FILE_LINK_CEL: {
      // Read link position
      const frame_t link_frame = frame_t(read16());
      Cel* link = layer->cel(link_frame);

      if (link) {
        // There were a beta version that allow to the user specify
        // different X, Y, or opacity per link, in that case we must
        // create a copy.
        if (link->x() == x && link->y() == y && link->opacity() == opacity) {
          cel.reset(Cel::MakeLink(frame, link));
        }
        else {
          cel.reset(Cel::MakeCopy(frame, link));
          cel->setPosition(x, y);
          cel->setOpacity(opacity);
        }
        cel->setZIndex(zIndex);
      }
      else {
        // Linked cel doesn't found
        return nullptr;
      }
      break;
    }

    case ASE_FILE_COMPRESSED_CEL: {
      // Read width and height
      int w = read16();
      int h = read16();

      if (w > 0 && h > 0) {
        const ImageRef image(Image::create(pixelFormat, w, h));
        read_compressed_image(f(), delegate(), image.get(), header, chunk_end);

        cel = std::make_unique<Cel>(frame, image);
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
        cel->setZIndex(zIndex);
      }
      break;
    }

    case ASE_FILE_COMPRESSED_TILEMAP: {
      // Read width and height
      int w = read16();
      int h = read16();
      int bitsPerTile = read16();
      uint32_t tileIDMask = read32();
      uint32_t tileIDShift = base::mask_shift(tileIDMask);
      uint32_t xflipMask = read32();
      uint32_t yflipMask = read32();
      uint32_t dflipMask = read32();
      uint32_t flagsMask = (xflipMask | yflipMask | dflipMask);
      readPadding(10);

      // We only support 32bpp at the moment
      // TODO add support for other bpp (8-bit, 16-bpp)
      if (bitsPerTile != 32) {
        delegate()->incompatibilityError(
          fmt::format("Unsupported tile format: {0} bits per tile", bitsPerTile));
        break;
      }

      if (w > 0 && h > 0) {
        ImageRef image(Image::create(IMAGE_TILEMAP, w, h));
        image->setMaskColor(notile);
        image->clear(notile);
        read_compressed_image(f(), delegate(), image.get(), header, chunk_end);

        // Check if the tileset of this tilemap has the
        // "ASE_TILESET_FLAG_ZERO_IS_NOTILE" we have to adjust all
        // tile references to the new format (where empty tile is
        // zero)
        Tileset* ts = static_cast<LayerTilemap*>(layer)->tileset();
        tileset_index tsi = static_cast<LayerTilemap*>(layer)->tilesetIndex();
        ASSERT(tsi >= 0 && tsi < m_tilesetFlags.size());
        if (tsi >= 0 && tsi < m_tilesetFlags.size() &&
            (m_tilesetFlags[tsi] & ASE_TILESET_FLAG_ZERO_IS_NOTILE) == 0) {
          fix_old_tilemap(image.get(), ts, tileIDMask, flagsMask);
        }

        // Convert the tile index and masks to a proper in-memory
        // representation for the doc-lib.
        transform_image<TilemapTraits>(
          image.get(),
          [ts, tileIDMask, tileIDShift, xflipMask, yflipMask, dflipMask](tile_t tile) {
            // Get the tile index.
            const tile_index ti = ((tile & tileIDMask) >> tileIDShift);

            // If the index is out of bounds from the tileset, we
            // allow to keep some small values in-memory, but if the
            // index is too big, we consider it as a broken file and
            // remove the tile (as an huge index bring some lag
            // problems in the remove_unused_tiles_from_tileset()
            // creating a big Remap structure).
            //
            // Related to https://github.com/aseprite/aseprite/issues/2877
            if (ti > ts->size() && ti > 0xffffff) {
              return notile;
            }

            // Convert read index to tile_i_mask, and flags to tile_f_mask
            tile = doc::tile(ti,
                             ((tile & xflipMask) == xflipMask ? tile_f_xflip : 0) |
                               ((tile & yflipMask) == yflipMask ? tile_f_yflip : 0) |
                               ((tile & dflipMask) == dflipMask ? tile_f_dflip : 0));

            return tile;
          });

        cel = std::make_unique<Cel>(frame, image);
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
        cel->setZIndex(zIndex);
      }
      break;
    }

    default:
      delegate()->incompatibilityError(fmt::format("Unknown cel type found: {0}", cel_type));
      break;
  }

  if (!cel)
    return nullptr;

  static_cast<LayerImage*>(layer)->addCel(cel.get());
  return cel.release();
}

void AsepriteDecoder::readCelExtraChunk(Cel* cel)
{
  // Read chunk data
  int flags = read32();
  if (flags & ASE_CEL_EXTRA_FLAG_PRECISE_BOUNDS) {
    fixmath::fixed x = read32();
    fixmath::fixed y = read32();
    fixmath::fixed w = read32();
    fixmath::fixed h = read32();
    if (w && h) {
      gfx::RectF bounds(fixmath::fixtof(x),
                        fixmath::fixtof(y),
                        fixmath::fixtof(w),
                        fixmath::fixtof(h));
      cel->setBoundsF(bounds);
    }
  }
}

void AsepriteDecoder::readColorProfile(Sprite* sprite)
{
  int type = read16();
  int flags = read16();
  fixmath::fixed gamma = read32();
  readPadding(8);

  // Without color space, like old Aseprite versions
  gfx::ColorSpaceRef cs(nullptr);

  switch (type) {
    case ASE_FILE_NO_COLOR_PROFILE:
      if (flags & ASE_COLOR_PROFILE_FLAG_GAMMA)
        cs = gfx::ColorSpace::MakeSRGBWithGamma(fixmath::fixtof(gamma));
      else
        cs = gfx::ColorSpace::MakeNone();
      break;

    case ASE_FILE_SRGB_COLOR_PROFILE:
      if (flags & ASE_COLOR_PROFILE_FLAG_GAMMA)
        cs = gfx::ColorSpace::MakeSRGBWithGamma(fixmath::fixtof(gamma));
      else
        cs = gfx::ColorSpace::MakeSRGB();
      break;

    case ASE_FILE_ICC_COLOR_PROFILE: {
      size_t length = read32();
      if (length > 0) {
        std::vector<uint8_t> data(length);
        readBytes(&data[0], length);
        cs = gfx::ColorSpace::MakeICC(std::move(data));
      }
      break;
    }

    default:
      delegate()->incompatibilityError(fmt::format("Unknown color profile type found: {0}", type));
      break;
  }

  sprite->setColorSpace(cs);
}

void AsepriteDecoder::readExternalFiles(AsepriteExternalFiles& extFiles)
{
  uint32_t n = read32();
  readPadding(8);
  for (uint32_t i = 0; i < n; ++i) {
    uint32_t id = read32();
    uint8_t type = read8();
    readPadding(7);
    std::string fn = readString();
    extFiles.insert(id, type, fn);
  }
}

Mask* AsepriteDecoder::readMaskChunk()
{
  int c, u, v, byte;
  Mask* mask;
  // Read chunk data
  int x = read16();
  int y = read16();
  int w = read16();
  int h = read16();

  readPadding(8);
  std::string name = readString();

  mask = new Mask();
  mask->setName(name.c_str());
  mask->replace(gfx::Rect(x, y, w, h));

  // Read image data
  for (v = 0; v < h; v++)
    for (u = 0; u < (w + 7) / 8; u++) {
      byte = read8();
      for (c = 0; c < 8; c++)
        put_pixel(mask->bitmap(), u * 8 + c, v, byte & (1 << (7 - c)));
    }

  return mask;
}

void AsepriteDecoder::readTagsChunk(Tags* tags)
{
  const size_t ntags = read16();

  read32(); // 8 reserved bytes
  read32();

  for (size_t c = 0; c < ntags; ++c) {
    const frame_t from = read16();
    const frame_t to = read16();
    int aniDir = read8();
    if (aniDir != int(AniDir::FORWARD) && aniDir != int(AniDir::REVERSE) &&
        aniDir != int(AniDir::PING_PONG) && aniDir != int(AniDir::PING_PONG_REVERSE)) {
      aniDir = int(AniDir::FORWARD);
    }

    const int repeat = read16(); // Number of times we repeat this tag
    read16();                    // 6 reserved bytes
    read32();

    int r = read8();
    int g = read8();
    int b = read8();
    read8(); // Skip

    std::string name = readString();

    auto* tag = new Tag(from, to);

    // We read the color field just in case that this is a .aseprite
    // file written by an old version of Aseprite (where the there are
    // not user data for tags).
    tag->setColor(rgba(r, g, b, 255));

    tag->setName(name);
    tag->setAniDir((AniDir)aniDir);
    tag->setRepeat(repeat);
    tags->add(tag);
  }
}

void AsepriteDecoder::readUserDataChunk(UserData* userData, const AsepriteExternalFiles& extFiles)
{
  size_t flags = read32();

  if (flags & ASE_USER_DATA_FLAG_HAS_TEXT) {
    std::string text = readString();
    userData->setText(text);
  }

  if (flags & ASE_USER_DATA_FLAG_HAS_COLOR) {
    int r = read8();
    int g = read8();
    int b = read8();
    int a = read8();
    userData->setColor(rgba(r, g, b, a));
  }

  if (flags & ASE_USER_DATA_FLAG_HAS_PROPERTIES) {
    readPropertiesMaps(userData->propertiesMaps(), extFiles);
  }
}

void AsepriteDecoder::readSlicesChunk(Slices& slices)
{
  size_t nslices = read32(); // Number of slices
  read32();                  // 8 bytes reserved
  read32();

  for (size_t i = 0; i < nslices; ++i) {
    Slice* slice = readSliceChunk(slices);
    // Set the user data
    if (slice) {
      // Default slice color
      slice->userData().setColor(delegate()->defaultSliceColor());
    }
  }
}

Slice* AsepriteDecoder::readSliceChunk(Slices& slices)
{
  const size_t nkeys = read32();   // Number of keys
  const int flags = read32();      // Flags
  read32();                        // 4 bytes reserved
  std::string name = readString(); // Name

  std::unique_ptr<Slice> slice(std::make_unique<Slice>());
  slice->setName(name);

  // For each key
  for (size_t j = 0; j < nkeys; ++j) {
    gfx::Rect bounds, center;
    gfx::Point pivot = SliceKey::NoPivot;
    const frame_t frame = read32();
    bounds.x = ((int32_t)read32());
    bounds.y = ((int32_t)read32());
    bounds.w = read32();
    bounds.h = read32();

    if (flags & ASE_SLICE_FLAG_HAS_CENTER_BOUNDS) {
      center.x = ((int32_t)read32());
      center.y = ((int32_t)read32());
      center.w = read32();
      center.h = read32();
    }

    if (flags & ASE_SLICE_FLAG_HAS_PIVOT_POINT) {
      pivot.x = ((int32_t)read32());
      pivot.y = ((int32_t)read32());
    }

    slice->insert(frame, SliceKey(bounds, center, pivot));
  }

  slices.add(slice.get());
  return slice.release();
}

Tileset* AsepriteDecoder::readTilesetChunk(Sprite* sprite,
                                           const AsepriteHeader* header,
                                           const AsepriteExternalFiles& extFiles)
{
  const tileset_index id = read32();
  const uint32_t flags = read32();
  const tile_index ntiles = read32();
  const int w = read16();
  const int h = read16();
  const int baseIndex = short(read16());
  readPadding(14);
  const std::string name = readString();

  // Errors
  if (ntiles < 0 || w < 1 || h < 1) {
    delegate()->error(
      fmt::format("Error: Invalid tileset (number of tiles={0}, tile size={1}x{2})", ntiles, w, h));
    return nullptr;
  }

  const Grid grid(gfx::Size(w, h));
  auto* tileset = new Tileset(sprite, grid, ntiles);
  tileset->setName(name);
  tileset->setBaseIndex(baseIndex);

  if (flags & ASE_TILESET_FLAG_EXTERNAL_FILE) {
    const uint32_t extFileId = read32();         // filename ID in the external files chunk
    const tileset_index extTilesetId = read32(); // tileset ID in the external file

    std::string fn;
    if (extFiles.getFilenameByID(extFileId, fn)) {
      tileset->setExternal(fn, extTilesetId);
    }
    else {
      delegate()->error(
        fmt::format("Error: Invalid external file reference (id={0} not found)", extFileId));
    }
  }

  if (flags & ASE_TILESET_FLAG_EMBEDDED) {
    if (ntiles > 0) {
      const size_t dataSize = read32(); // Size of compressed data
      const size_t dataBeg = tell();
      const size_t dataEnd = dataBeg + dataSize;

      base::buffer compressed;
      if (delegate()->cacheCompressedTilesets() && dataSize > 0) {
        compressed.resize(dataSize);
        f()->readBytes(&compressed[0], dataSize);
        seek(dataBeg);
      }

      const ImageRef alltiles(Image::create(sprite->pixelFormat(), w, h * ntiles));
      alltiles->setMaskColor(sprite->transparentColor());

      read_compressed_image(f(), delegate(), alltiles.get(), header, dataEnd);
      seek(dataEnd);

      for (tile_index i = 0; i < ntiles; ++i) {
        const ImageRef tile(crop_image(alltiles.get(), 0, i * h, w, h, alltiles->maskColor()));
        tileset->set(i, tile);
      }

      // If we are reading and old .aseprite file (where empty tile is not the zero]
      if ((flags & ASE_TILESET_FLAG_ZERO_IS_NOTILE) == 0)
        fix_old_tileset(tileset);

      if (!compressed.empty())
        tileset->setCompressedData(compressed);
    }
    sprite->tilesets()->set(id, tileset);
  }

  tileset->setMatchFlags((flags & ASE_TILESET_FLAG_MATCH_XFLIP ? tile_f_xflip : 0) |
                         (flags & ASE_TILESET_FLAG_MATCH_YFLIP ? tile_f_yflip : 0) |
                         (flags & ASE_TILESET_FLAG_MATCH_DFLIP ? tile_f_dflip : 0));

  if (id >= m_tilesetFlags.size())
    m_tilesetFlags.resize(id + 1, 0);
  m_tilesetFlags[id] = flags;

  return tileset;
}

void AsepriteDecoder::readPropertiesMaps(UserData::PropertiesMaps& propertiesMaps,
                                         const AsepriteExternalFiles& extFiles)
{
  auto startPos = tell();
  auto size = read32();
  auto numMaps = read32();
  try {
    for (int i = 0; i < numMaps; ++i) {
      auto id = read32();
      std::string extensionId; // extensionId = empty by default (when id == 0)
      if (id && !extFiles.getFilenameByID(id, extensionId)) {
        // This shouldn't happen, but if it does, we put the properties
        // in an artificial extensionId.
        extensionId = fmt::format("__missed__{}", id);
        delegate()->error(fmt::format("Error: Invalid extension ID (id={0} not found)", id));
      }
      auto properties = readPropertyValue(USER_DATA_PROPERTY_TYPE_PROPERTIES);
      propertiesMaps[extensionId] = get_value<UserData::Properties>(properties);
    }
  }
  catch (const base::Exception& e) {
    delegate()->incompatibilityError(fmt::format("Error reading custom properties: {0}", e.what()));
  }

  seek(startPos + size);
}

UserData::Variant AsepriteDecoder::readPropertyValue(uint16_t type)
{
  switch (type) {
    case USER_DATA_PROPERTY_TYPE_NULLPTR: {
      // This shouldn't exist in a .aseprite file
      ASSERT(false);
      return nullptr;
    }
    case USER_DATA_PROPERTY_TYPE_BOOL: {
      bool value = read8();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT8: {
      int8_t value = read8();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT8: {
      uint8_t value = read8();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT16: {
      int16_t value = read16();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT16: {
      uint16_t value = read16();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT32: {
      int32_t value = read32();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT32: {
      uint32_t value = read32();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_INT64: {
      int64_t value = read64();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UINT64: {
      uint64_t value = read64();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_FIXED: {
      int32_t value = read32();
      return UserData::Fixed{ value };
    }
    case USER_DATA_PROPERTY_TYPE_FLOAT: {
      float value = readFloat();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_DOUBLE: {
      double value = readDouble();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_STRING: {
      std::string value = readString();
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_POINT: {
      int32_t x = read32();
      int32_t y = read32();
      return gfx::Point(x, y);
    }
    case USER_DATA_PROPERTY_TYPE_SIZE: {
      int32_t w = read32();
      int32_t h = read32();
      return gfx::Size(w, h);
    }
    case USER_DATA_PROPERTY_TYPE_RECT: {
      int32_t x = read32();
      int32_t y = read32();
      int32_t w = read32();
      int32_t h = read32();
      return gfx::Rect(x, y, w, h);
    }
    case USER_DATA_PROPERTY_TYPE_VECTOR: {
      auto numElems = read32();
      auto elemsType = read16();
      auto elemType = elemsType;
      std::vector<UserData::Variant> value;
      for (int k = 0; k < numElems; ++k) {
        if (elemsType == 0) {
          elemType = read16();
        }
        value.push_back(readPropertyValue(elemType));
      }
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_PROPERTIES: {
      auto numProps = read32();
      UserData::Properties value;
      for (int j = 0; j < numProps; ++j) {
        auto name = readString();
        auto type = read16();
        value[name] = readPropertyValue(type);
      }
      return value;
    }
    case USER_DATA_PROPERTY_TYPE_UUID: {
      return readUuid();
    }
    default: {
      throw base::Exception(
        fmt::format("Unexpected property type '{0}' at file position {1}", type, tell()));
    }
  }

  return UserData::Variant{};
}

void AsepriteDecoder::readTilesData(Tileset* tileset, const AsepriteExternalFiles& extFiles)
{
  // Read as many user data chunks as tiles are in the tileset
  for (tile_index i = 0; i < tileset->size(); i++) {
    const size_t chunk_pos = tell();
    // Read chunk information
    const int chunk_size = read32();
    const int chunk_type = read16();
    if (chunk_type != ASE_FILE_CHUNK_USER_DATA) {
      // Something went wrong...
      delegate()->error(
        fmt::format("Warning: Unexpected chunk type {0} when reading tileset index {1}",
                    chunk_type,
                    i));
      seek(chunk_pos);
      return;
    }

    UserData tileData;
    readUserDataChunk(&tileData, extFiles);
    tileset->setTileData(i, tileData);
    seek(chunk_pos + chunk_size);
  }
}

base::Uuid AsepriteDecoder::readUuid()
{
  base::Uuid value;
  uint8_t* bytes = value.bytes();
  for (int i = 0; i < 16; ++i) {
    bytes[i] = read8();
  }
  return value;
}

} // namespace dio
