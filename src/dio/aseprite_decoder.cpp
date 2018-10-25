// Aseprite Document IO Library
// Copyright (c) 2018 Igara Studio S.A.
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
#include "gfx/color_space.h"
#include "dio/aseprite_common.h"
#include "dio/decode_delegate.h"
#include "dio/file_interface.h"
#include "dio/pixel_io.h"
#include "doc/doc.h"
#include "fixmath/fixmath.h"
#include "fmt/format.h"
#include "zlib.h"

#include <cstdio>

namespace dio {

bool AsepriteDecoder::decode()
{
  bool ignore_old_color_chunks = false;

  AsepriteHeader header;
  if (!readHeader(&header)) {
    delegate()->error("Error reading header");
    return false;
  }

  // Create the new sprite
  std::unique_ptr<doc::Sprite> sprite(
    new doc::Sprite(doc::ImageSpec(header.depth == 32 ? doc::ColorMode::RGB:
                                   header.depth == 16 ? doc::ColorMode::GRAYSCALE:
                                                        doc::ColorMode::INDEXED,
                                   header.width,
                                   header.height),
                    header.ncolors));

  // Set frames and speed
  sprite->setTotalFrames(doc::frame_t(header.frames));
  sprite->setDurationForAllFrames(header.speed);

  // Set transparent entry
  sprite->setTransparentColor(header.transparent_index);

  // Set pixel ratio
  sprite->setPixelRatio(doc::PixelRatio(header.pixel_width, header.pixel_height));

  // Prepare variables for layer chunks
  doc::Layer* last_layer = sprite->root();
  doc::WithUserData* last_object_with_user_data = nullptr;
  doc::Cel* last_cel = nullptr;
  int current_level = -1;
  doc::LayerList allLayers;

  // Just one frame?
  doc::frame_t nframes = sprite->totalFrames();
  if (nframes > 1 && delegate()->decodeOneFrame())
    nframes = 1;

  // Read frame by frame to end-of-file
  for (doc::frame_t frame=0; frame<nframes; ++frame) {
    // Start frame position
    size_t frame_pos = f()->tell();
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
      for (int c=0; c<frame_header.chunks; c++) {
        // Start chunk position
        size_t chunk_pos = f()->tell();
        delegate()->progress((float)chunk_pos / (float)header.size);

        // Read chunk information
        int chunk_size = read32();
        int chunk_type = read16();

        switch (chunk_type) {

          case ASE_FILE_CHUNK_FLI_COLOR:
          case ASE_FILE_CHUNK_FLI_COLOR2:
            if (!ignore_old_color_chunks) {
              doc::Palette* prevPal = sprite->palette(frame);
              std::unique_ptr<doc::Palette> pal(
                chunk_type == ASE_FILE_CHUNK_FLI_COLOR ?
                readColorChunk(prevPal, frame):
                readColor2Chunk(prevPal, frame));

              if (prevPal->countDiff(pal.get(), NULL, NULL) > 0)
                sprite->setPalette(pal.get(), true);
            }
            break;

          case ASE_FILE_CHUNK_PALETTE: {
            doc::Palette* prevPal = sprite->palette(frame);
            std::unique_ptr<doc::Palette> pal(
              readPaletteChunk(prevPal, frame));

            if (prevPal->countDiff(pal.get(), NULL, NULL) > 0)
              sprite->setPalette(pal.get(), true);

            ignore_old_color_chunks = true;
            break;
          }

          case ASE_FILE_CHUNK_LAYER: {
            doc::Layer* newLayer =
              readLayerChunk(&header, sprite.get(),
                             &last_layer,
                             &current_level);
            if (newLayer) {
              allLayers.push_back(newLayer);
              last_object_with_user_data = newLayer;
            }
            break;
          }

          case ASE_FILE_CHUNK_CEL: {
            doc::Cel* cel =
              readCelChunk(sprite.get(), allLayers, frame,
                           sprite->pixelFormat(), &header,
                           chunk_pos+chunk_size);
            if (cel) {
              last_cel = cel;
              last_object_with_user_data = cel->data();
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

          case ASE_FILE_CHUNK_MASK: {
            doc::Mask* mask = readMaskChunk();
            if (mask)
              delete mask;      // TODO add the mask in some place?
            else
              delegate()->error("Warning: Cannot load a mask chunk");
            break;
          }

          case ASE_FILE_CHUNK_PATH:
            // Ignore
            break;

          case ASE_FILE_CHUNK_FRAME_TAGS:
            readFrameTagsChunk(&sprite->frameTags());
            break;

          case ASE_FILE_CHUNK_SLICES: {
            readSlicesChunk(sprite->slices());
            break;
          }

          case ASE_FILE_CHUNK_SLICE: {
            doc::Slice* slice = readSliceChunk(sprite->slices());
            if (slice)
              last_object_with_user_data = slice;
            break;
          }

          case ASE_FILE_CHUNK_USER_DATA: {
            doc::UserData userData;
            readUserDataChunk(&userData);
            if (last_object_with_user_data)
              last_object_with_user_data->setUserData(userData);
            break;
          }

          default:
            delegate()->error(
              fmt::format("Warning: Unsupported chunk type {0} (skipping)", chunk_type));
            break;
        }

        // Skip chunk size
        f()->seek(chunk_pos+chunk_size);
      }
    }

    // Skip frame size
    f()->seek(frame_pos+frame_header.size);

    if (delegate()->isCanceled())
      break;
  }

  delegate()->onSprite(sprite.release());
  return true;
}

bool AsepriteDecoder::readHeader(AsepriteHeader* header)
{
  size_t headerPos = f()->tell();

  header->size  = read32();
  header->magic = read16();

  // Developers can open any .ase file
#if !defined(ENABLE_DEVMODE)
  if (header->magic != ASE_FILE_MAGIC)
    return false;
#endif

  header->frames     = read16();
  header->width      = read16();
  header->height     = read16();
  header->depth      = read16();
  header->flags      = read32();
  header->speed      = read16();
  header->next       = read32();
  header->frit       = read32();
  header->transparent_index = read8();
  header->ignore[0]  = read8();
  header->ignore[1]  = read8();
  header->ignore[2]  = read8();
  header->ncolors    = read16();
  header->pixel_width = read8();
  header->pixel_height = read8();

  if (header->ncolors == 0)     // 0 means 256 (old .ase files)
    header->ncolors = 256;

  if (header->pixel_width == 0 ||
      header->pixel_height == 0) {
    header->pixel_width = 1;
    header->pixel_height = 1;
  }

#if defined(ENABLE_DEVMODE)
  // This is useful to read broken .ase files
  if (header->magic != ASE_FILE_MAGIC) {
    header->frames  = 256;  // Frames number might be not enought for some files
    header->width   = 1024; // Size doesn't matter, the sprite can be crop
    header->height  = 1024;
  }
#endif

  f()->seek(headerPos+128);
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

  if (frame_header->chunks == 0xFFFF &&
      frame_header->chunks < nchunks)
    frame_header->chunks = nchunks;
}

void AsepriteDecoder::readPadding(int bytes)
{
  for (int c=0; c<bytes; c++)
    read8();
}

std::string AsepriteDecoder::readString()
{
  int length = read16();
  if (length == EOF)
    return "";

  std::string string;
  string.reserve(length+1);

  for (int c=0; c<length; c++)
    string.push_back(read8());

  return string;
}

doc::Palette* AsepriteDecoder::readColorChunk(doc::Palette* prevPal,
                                              doc::frame_t frame)
{
  int i, c, r, g, b, packets, skip, size;
  doc::Palette* pal = new doc::Palette(*prevPal);
  pal->setFrame(frame);

  packets = read16();   // Number of packets
  skip = 0;

  // Read all packets
  for (i=0; i<packets; i++) {
    skip += read8();
    size = read8();
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      r = read8();
      g = read8();
      b = read8();
      pal->setEntry(c, doc::rgba(doc::scale_6bits_to_8bits(r),
                                 doc::scale_6bits_to_8bits(g),
                                 doc::scale_6bits_to_8bits(b), 255));
    }
  }

  return pal;
}

doc::Palette* AsepriteDecoder::readColor2Chunk(doc::Palette* prevPal,
                                               doc::frame_t frame)
{
  int i, c, r, g, b, packets, skip, size;
  doc::Palette* pal = new doc::Palette(*prevPal);
  pal->setFrame(frame);

  packets = read16();   // Number of packets
  skip = 0;

  // Read all packets
  for (i=0; i<packets; i++) {
    skip += read8();
    size = read8();
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      r = read8();
      g = read8();
      b = read8();
      pal->setEntry(c, doc::rgba(r, g, b, 255));
    }
  }

  return pal;
}

doc::Palette* AsepriteDecoder::readPaletteChunk(doc::Palette* prevPal,
                                                doc::frame_t frame)
{
  doc::Palette* pal = new doc::Palette(*prevPal);
  pal->setFrame(frame);

  int newSize = read32();
  int from = read32();
  int to = read32();
  readPadding(8);

  if (newSize > 0)
    pal->resize(newSize);

  for (int c=from; c<=to; ++c) {
    int flags = read16();
    int r = read8();
    int g = read8();
    int b = read8();
    int a = read8();
    pal->setEntry(c, doc::rgba(r, g, b, a));

    // Skip name
    if (flags & ASE_PALETTE_FLAG_HAS_NAME) {
      std::string name = readString();
      // Ignore color entry name
    }
  }

  return pal;
}

doc::Layer* AsepriteDecoder::readLayerChunk(AsepriteHeader* header,
                                            doc::Sprite* sprite,
                                            doc::Layer** previous_layer,
                                            int* current_level)
{
  // Read chunk data
  int flags = read16();
  int layer_type = read16();
  int child_level = read16();
  read16();                     // default width
  read16();                     // default height
  int blendmode = read16();     // blend mode
  int opacity = read8();       // opacity
  readPadding(3);
  std::string name = readString();

  doc::Layer* layer = nullptr;
  switch (layer_type) {

    case ASE_FILE_LAYER_IMAGE:
      layer = new doc::LayerImage(sprite);

      // Only transparent layers can have blend mode and opacity
      if (!(flags & int(doc::LayerFlags::Background))) {
        static_cast<doc::LayerImage*>(layer)->setBlendMode((doc::BlendMode)blendmode);
        if (header->flags & ASE_FILE_FLAG_LAYER_WITH_OPACITY)
          static_cast<doc::LayerImage*>(layer)->setOpacity(opacity);
      }
      break;

    case ASE_FILE_LAYER_GROUP:
      layer = new doc::LayerGroup(sprite);
      break;
  }

  if (layer) {
    // flags
    layer->setFlags(static_cast<doc::LayerFlags>(
                      flags &
                      static_cast<int>(doc::LayerFlags::PersistentFlagsMask)));

    // name
    layer->setName(name.c_str());

    // Child level
    if (child_level == *current_level)
      (*previous_layer)->parent()->addLayer(layer);
    else if (child_level > *current_level)
      static_cast<doc::LayerGroup*>(*previous_layer)->addLayer(layer);
    else if (child_level < *current_level) {
      doc::LayerGroup* parent = (*previous_layer)->parent();
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
  }

  return layer;
}

//////////////////////////////////////////////////////////////////////
// Raw Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
void read_raw_image(FileInterface* f,
                    DecodeDelegate* delegate,
                    doc::Image* image,
                    AsepriteHeader* header)
{
  PixelIO<ImageTraits> pixel_io;
  int x, y;
  int w = image->width();
  int h = image->height();

  for (y=0; y<h; ++y) {
    for (x=0; x<w; ++x) {
      doc::put_pixel_fast<ImageTraits>(image, x, y, pixel_io.read_pixel(f));
    }
    delegate->progress((float)f->tell() / (float)header->size);
  }
}

//////////////////////////////////////////////////////////////////////
// Compressed Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
void read_compressed_image(FileInterface* f,
                           DecodeDelegate* delegate,
                           doc::Image* image,
                           size_t chunk_end,
                           AsepriteHeader* header)
{
  PixelIO<ImageTraits> pixel_io;
  z_stream zstream;
  int y, err;

  zstream.zalloc = (alloc_func)0;
  zstream.zfree  = (free_func)0;
  zstream.opaque = (voidpf)0;

  err = inflateInit(&zstream);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in inflateInit().", err);

  std::vector<uint8_t> scanline(ImageTraits::getRowStrideBytes(image->width()));
  std::vector<uint8_t> uncompressed(image->height() * ImageTraits::getRowStrideBytes(image->width()));
  std::vector<uint8_t> compressed(4096);
  int uncompressed_offset = 0;

  while (true) {
    size_t input_bytes;

    if (f->tell()+compressed.size() > chunk_end) {
      input_bytes = chunk_end - f->tell(); // Remaining bytes
      ASSERT(input_bytes < compressed.size());

      if (input_bytes == 0)
        break;                  // Done, we consumed all chunk
    }
    else
      input_bytes = compressed.size();

    size_t bytes_read = f->readBytes(&compressed[0], input_bytes);
    zstream.next_in = (Bytef*)&compressed[0];
    zstream.avail_in = bytes_read;

    do {
      zstream.next_out = (Bytef*)&scanline[0];
      zstream.avail_out = scanline.size();

      err = inflate(&zstream, Z_NO_FLUSH);
      if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR)
        throw base::Exception("ZLib error %d in inflate().", err);

      size_t uncompressed_bytes = scanline.size() - zstream.avail_out;
      if (uncompressed_bytes > 0) {
        if (uncompressed_offset+uncompressed_bytes > uncompressed.size())
          throw base::Exception("Bad compressed image.");

        std::copy(scanline.begin(), scanline.begin()+uncompressed_bytes,
                  uncompressed.begin()+uncompressed_offset);

        uncompressed_offset += uncompressed_bytes;
      }
    } while (zstream.avail_out == 0);

    delegate->progress((float)f->tell() / (float)header->size);
  }

  uncompressed_offset = 0;
  for (y=0; y<image->height(); y++) {
    typename ImageTraits::address_t address =
      (typename ImageTraits::address_t)image->getPixelAddress(0, y);

    pixel_io.read_scanline(address, image->width(), &uncompressed[uncompressed_offset]);

    uncompressed_offset += ImageTraits::getRowStrideBytes(image->width());
  }

  err = inflateEnd(&zstream);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in inflateEnd().", err);
}

//////////////////////////////////////////////////////////////////////
// Cel Chunk
//////////////////////////////////////////////////////////////////////

doc::Cel* AsepriteDecoder::readCelChunk(doc::Sprite* sprite,
                                        doc::LayerList& allLayers,
                                        doc::frame_t frame,
                                        doc::PixelFormat pixelFormat,
                                        AsepriteHeader* header,
                                        size_t chunk_end)
{
  // Read chunk data
  doc::layer_t layer_index = read16();
  int x = ((short)read16());
  int y = ((short)read16());
  int opacity = read8();
  int cel_type = read16();
  readPadding(7);

  doc::Layer* layer = nullptr;
  if (layer_index >= 0 && layer_index < doc::layer_t(allLayers.size()))
    layer = allLayers[layer_index];

  if (!layer) {
    delegate()->error(
      fmt::format("Frame {0} didn't found layer with index {1}",
                  (int)frame, (int)layer_index));
    return nullptr;
  }
  if (!layer->isImage()) {
    delegate()->error(
      fmt::format("Invalid .ase file (frame {0} in layer {1} which does not contain images",
                  (int)frame, (int)layer_index));
    return nullptr;
  }

  // Create the new frame.
  std::unique_ptr<doc::Cel> cel;

  switch (cel_type) {

    case ASE_FILE_RAW_CEL: {
      // Read width and height
      int w = read16();
      int h = read16();

      if (w > 0 && h > 0) {
        doc::ImageRef image(doc::Image::create(pixelFormat, w, h));

        // Read pixel data
        switch (image->pixelFormat()) {

          case doc::IMAGE_RGB:
            read_raw_image<doc::RgbTraits>(f(), delegate(), image.get(), header);
            break;

          case doc::IMAGE_GRAYSCALE:
            read_raw_image<doc::GrayscaleTraits>(f(), delegate(), image.get(), header);
            break;

          case doc::IMAGE_INDEXED:
            read_raw_image<doc::IndexedTraits>(f(), delegate(), image.get(), header);
            break;
        }

        cel.reset(new doc::Cel(frame, image));
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
      }
      break;
    }

    case ASE_FILE_LINK_CEL: {
      // Read link position
      doc::frame_t link_frame = doc::frame_t(read16());
      doc::Cel* link = layer->cel(link_frame);

      if (link) {
        // There were a beta version that allow to the user specify
        // different X, Y, or opacity per link, in that case we must
        // create a copy.
        if (link->x() == x && link->y() == y && link->opacity() == opacity) {
          cel.reset(doc::Cel::createLink(link));
          cel->setFrame(frame);
        }
        else {
          cel.reset(doc::Cel::createCopy(link));
          cel->setFrame(frame);
          cel->setPosition(x, y);
          cel->setOpacity(opacity);
        }
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
        doc::ImageRef image(doc::Image::create(pixelFormat, w, h));

        // Try to read pixel data
        try {
          switch (image->pixelFormat()) {

            case doc::IMAGE_RGB:
              read_compressed_image<doc::RgbTraits>(
                f(), delegate(), image.get(), chunk_end, header);
              break;

            case doc::IMAGE_GRAYSCALE:
              read_compressed_image<doc::GrayscaleTraits>(
                f(), delegate(), image.get(), chunk_end, header);
              break;

            case doc::IMAGE_INDEXED:
              read_compressed_image<doc::IndexedTraits>(
                f(), delegate(), image.get(), chunk_end, header);
              break;
          }
        }
        // OK, in case of error we can show the problem, but continue
        // loading more cels.
        catch (const std::exception& e) {
          delegate()->error(e.what());
        }

        cel.reset(new doc::Cel(frame, image));
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
      }
      break;
    }

  }

  if (!cel)
    return nullptr;

  static_cast<doc::LayerImage*>(layer)->addCel(cel.get());
  return cel.release();
}

void AsepriteDecoder::readCelExtraChunk(doc::Cel* cel)
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

void AsepriteDecoder::readColorProfile(doc::Sprite* sprite)
{
  int type = read16();
  int flags = read16();
  fixmath::fixed gamma = read32();
  readPadding(8);

  // Without color space, like old Aseprite versions
  gfx::ColorSpacePtr cs(nullptr);

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
  }

  sprite->setColorSpace(cs);
}

doc::Mask* AsepriteDecoder::readMaskChunk()
{
  int c, u, v, byte;
  doc::Mask* mask;
  // Read chunk data
  int x = read16();
  int y = read16();
  int w = read16();
  int h = read16();

  readPadding(8);
  std::string name = readString();

  mask = new doc::Mask();
  mask->setName(name.c_str());
  mask->replace(gfx::Rect(x, y, w, h));

  // Read image data
  for (v=0; v<h; v++)
    for (u=0; u<(w+7)/8; u++) {
      byte = read8();
      for (c=0; c<8; c++)
        doc::put_pixel(mask->bitmap(), u*8+c, v, byte & (1<<(7-c)));
    }

  return mask;
}

void AsepriteDecoder::readFrameTagsChunk(doc::FrameTags* frameTags)
{
  size_t tags = read16();

  read32();                     // 8 reserved bytes
  read32();

  for (size_t c=0; c<tags; ++c) {
    doc::frame_t from = read16();
    doc::frame_t to = read16();
    int aniDir = read8();
    if (aniDir != int(doc::AniDir::FORWARD) &&
        aniDir != int(doc::AniDir::REVERSE) &&
        aniDir != int(doc::AniDir::PING_PONG)) {
      aniDir = int(doc::AniDir::FORWARD);
    }

    read32();                     // 8 reserved bytes
    read32();

    int r = read8();
    int g = read8();
    int b = read8();
    read8();                     // Skip

    std::string name = readString();

    doc::FrameTag* tag = new doc::FrameTag(from, to);
    tag->setColor(doc::rgba(r, g, b, 255));
    tag->setName(name);
    tag->setAniDir((doc::AniDir)aniDir);
    frameTags->add(tag);
  }
}

void AsepriteDecoder::readUserDataChunk(doc::UserData* userData)
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
    userData->setColor(doc::rgba(r, g, b, a));
  }
}

void AsepriteDecoder::readSlicesChunk(doc::Slices& slices)
{
  size_t nslices = read32();    // Number of slices
  read32();                     // 8 bytes reserved
  read32();

  for (size_t i=0; i<nslices; ++i) {
    doc::Slice* slice = readSliceChunk(slices);
    // Set the user data
    if (slice) {
      // Default slice color
      slice->userData().setColor(delegate()->defaultSliceColor());
    }
  }
}

doc::Slice* AsepriteDecoder::readSliceChunk(doc::Slices& slices)
{
  const size_t nkeys = read32();   // Number of keys
  const int flags = read32();      // Flags
  read32();                        // 4 bytes reserved
  std::string name = readString(); // Name

  std::unique_ptr<doc::Slice> slice(new doc::Slice);
  slice->setName(name);

  // For each key
  for (size_t j=0; j<nkeys; ++j) {
    gfx::Rect bounds, center;
    gfx::Point pivot = doc::SliceKey::NoPivot;
    doc::frame_t frame = read32();
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

    slice->insert(frame, doc::SliceKey(bounds, center, pivot));
  }

  slices.add(slice.get());
  return slice.release();
}

} // namespace dio
