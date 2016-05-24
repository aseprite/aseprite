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
#include "base/cfile.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "doc/doc.h"
#include "zlib.h"

#include <stdio.h>

#define ASE_FILE_MAGIC                      0xA5E0
#define ASE_FILE_FRAME_MAGIC                0xF1FA

#define ASE_FILE_FLAG_LAYER_WITH_OPACITY     1

#define ASE_FILE_CHUNK_FLI_COLOR2           4
#define ASE_FILE_CHUNK_FLI_COLOR            11
#define ASE_FILE_CHUNK_LAYER                0x2004
#define ASE_FILE_CHUNK_CEL                  0x2005
#define ASE_FILE_CHUNK_MASK                 0x2016
#define ASE_FILE_CHUNK_PATH                 0x2017
#define ASE_FILE_CHUNK_FRAME_TAGS           0x2018
#define ASE_FILE_CHUNK_PALETTE              0x2019
#define ASE_FILE_CHUNK_USER_DATA            0x2020

#define ASE_FILE_RAW_CEL                    0
#define ASE_FILE_LINK_CEL                   1
#define ASE_FILE_COMPRESSED_CEL             2

#define ASE_LAYER_FLAG_VISIBLE              1
#define ASE_LAYER_FLAG_EDITABLE             2
#define ASE_LAYER_FLAG_LOCK_MOVEMENT        4
#define ASE_LAYER_FLAG_BACKGROUND           8
#define ASE_LAYER_FLAG_PREFER_LINKED_CELS   16

#define ASE_PALETTE_FLAG_HAS_NAME           1

#define ASE_USER_DATA_FLAG_HAS_TEXT         1
#define ASE_USER_DATA_FLAG_HAS_COLOR        2

namespace app {

using namespace base;

struct ASE_Header {
  long pos;

  uint32_t size;
  uint16_t magic;
  uint16_t frames;
  uint16_t width;
  uint16_t height;
  uint16_t depth;
  uint32_t flags;
  uint16_t speed;       // Deprecated, use "duration" of FrameHeader
  uint32_t next;
  uint32_t frit;
  uint8_t transparent_index;
  uint8_t ignore[3];
  uint16_t ncolors;
  uint8_t pixel_width;
  uint8_t pixel_height;
};

struct ASE_FrameHeader {
  uint32_t size;
  uint16_t magic;
  uint16_t chunks;
  uint16_t duration;
};

struct ASE_Chunk {
  int type;
  int start;
};

static bool ase_file_read_header(FILE* f, ASE_Header* header);
static void ase_file_prepare_header(FILE* f, ASE_Header* header, const Sprite* sprite);
static void ase_file_write_header(FILE* f, ASE_Header* header);
static void ase_file_write_header_filesize(FILE* f, ASE_Header* header);

static void ase_file_read_frame_header(FILE* f, ASE_FrameHeader* frame_header);
static void ase_file_prepare_frame_header(FILE* f, ASE_FrameHeader* frame_header);
static void ase_file_write_frame_header(FILE* f, ASE_FrameHeader* frame_header);

static void ase_file_write_layers(FILE* f, ASE_FrameHeader* frame_header, const Layer* layer);
static void ase_file_write_cels(FILE* f, ASE_FrameHeader* frame_header, const Sprite* sprite, const Layer* layer, frame_t frame);

static void ase_file_read_padding(FILE* f, int bytes);
static void ase_file_write_padding(FILE* f, int bytes);
static std::string ase_file_read_string(FILE* f);
static void ase_file_write_string(FILE* f, const std::string& string);

static void ase_file_write_start_chunk(FILE* f, ASE_FrameHeader* frame_header, int type, ASE_Chunk* chunk);
static void ase_file_write_close_chunk(FILE* f, ASE_Chunk* chunk);

static Palette* ase_file_read_color_chunk(FILE* f, Palette* prevPal, frame_t frame);
static Palette* ase_file_read_color2_chunk(FILE* f, Palette* prevPal, frame_t frame);
static Palette* ase_file_read_palette_chunk(FILE* f, Palette* prevPal, frame_t frame);
static void ase_file_write_color2_chunk(FILE* f, ASE_FrameHeader* frame_header, const Palette* pal);
static void ase_file_write_palette_chunk(FILE* f, ASE_FrameHeader* frame_header, const Palette* pal, int from, int to);
static Layer* ase_file_read_layer_chunk(FILE* f, ASE_Header* header, Sprite* sprite, Layer** previous_layer, int* current_level);
static void ase_file_write_layer_chunk(FILE* f, ASE_FrameHeader* frame_header, const Layer* layer);
static Cel* ase_file_read_cel_chunk(FILE* f, Sprite* sprite, frame_t frame, PixelFormat pixelFormat, FileOp* fop, ASE_Header* header, size_t chunk_end);
static void ase_file_write_cel_chunk(FILE* f, ASE_FrameHeader* frame_header, const Cel* cel, const LayerImage* layer, const Sprite* sprite);
static Mask* ase_file_read_mask_chunk(FILE* f);
#if 0
static void ase_file_write_mask_chunk(FILE* f, ASE_FrameHeader* frame_header, Mask* mask);
#endif
static void ase_file_read_frame_tags_chunk(FILE* f, FrameTags* frameTags);
static void ase_file_write_frame_tags_chunk(FILE* f, ASE_FrameHeader* frame_header, const FrameTags* frameTags);
static void ase_file_read_user_data_chunk(FILE* f, UserData* userData);
static void ase_file_write_user_data_chunk(FILE* f, ASE_FrameHeader* frame_header, const UserData* userData);

class ChunkWriter {
public:
  ChunkWriter(FILE* f, ASE_FrameHeader* frame_header, int type) : m_file(f) {
    ase_file_write_start_chunk(m_file, frame_header, type, &m_chunk);
  }

  ~ChunkWriter() {
    ase_file_write_close_chunk(m_file, &m_chunk);
  }

private:
  FILE* m_file;
  ASE_Chunk m_chunk;
};

class AseFormat : public FileFormat {
  const char* onGetName() const override { return "ase"; }
  const char* onGetExtensions() const override { return "ase,aseprite"; }
  int onGetFlags() const override {
    return
      FILE_SUPPORT_LOAD |
      FILE_SUPPORT_SAVE |
      FILE_SUPPORT_RGB |
      FILE_SUPPORT_RGBA |
      FILE_SUPPORT_GRAY |
      FILE_SUPPORT_GRAYA |
      FILE_SUPPORT_INDEXED |
      FILE_SUPPORT_LAYERS |
      FILE_SUPPORT_FRAMES |
      FILE_SUPPORT_PALETTES |
      FILE_SUPPORT_FRAME_TAGS |
      FILE_SUPPORT_BIG_PALETTES |
      FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
};

FileFormat* CreateAseFormat()
{
  return new AseFormat;
}

bool AseFormat::onLoad(FileOp* fop)
{
  FileHandle handle(open_file_with_exception(fop->filename(), "rb"));
  FILE* f = handle.get();
  bool ignore_old_color_chunks = false;

  ASE_Header header;
  if (!ase_file_read_header(f, &header)) {
    fop->setError("Error reading header\n");
    return false;
  }

  // Create the new sprite
  UniquePtr<Sprite> sprite(new Sprite(header.depth == 32 ? IMAGE_RGB:
      header.depth == 16 ? IMAGE_GRAYSCALE: IMAGE_INDEXED,
      header.width, header.height, header.ncolors));

  // Set frames and speed
  sprite->setTotalFrames(frame_t(header.frames));
  sprite->setDurationForAllFrames(header.speed);

  // Set transparent entry
  sprite->setTransparentColor(header.transparent_index);

  // Set pixel ratio
  sprite->setPixelRatio(PixelRatio(header.pixel_width, header.pixel_height));

  // Prepare variables for layer chunks
  Layer* last_layer = sprite->folder();
  WithUserData* last_object_with_user_data = nullptr;
  int current_level = -1;

  // Read frame by frame to end-of-file
  for (frame_t frame(0); frame<sprite->totalFrames(); ++frame) {
    // Start frame position
    int frame_pos = ftell(f);
    fop->setProgress((float)frame_pos / (float)header.size);

    // Read frame header
    ASE_FrameHeader frame_header;
    ase_file_read_frame_header(f, &frame_header);

    // Correct frame type
    if (frame_header.magic == ASE_FILE_FRAME_MAGIC) {
      // Use frame-duration field?
      if (frame_header.duration > 0)
        sprite->setFrameDuration(frame, frame_header.duration);

      // Read chunks
      for (int c=0; c<frame_header.chunks; c++) {
        /* start chunk position */
        int chunk_pos = ftell(f);
        fop->setProgress((float)chunk_pos / (float)header.size);

        // Read chunk information
        int chunk_size = fgetl(f);
        int chunk_type = fgetw(f);

        switch (chunk_type) {

          case ASE_FILE_CHUNK_FLI_COLOR:
          case ASE_FILE_CHUNK_FLI_COLOR2:
            if (!ignore_old_color_chunks) {
              Palette* prevPal = sprite->palette(frame);
              UniquePtr<Palette> pal(chunk_type == ASE_FILE_CHUNK_FLI_COLOR ?
                                     ase_file_read_color_chunk(f, prevPal, frame):
                                     ase_file_read_color2_chunk(f, prevPal, frame));

              if (prevPal->countDiff(pal.get(), NULL, NULL) > 0)
                sprite->setPalette(pal.get(), true);
            }
            break;

          case ASE_FILE_CHUNK_PALETTE: {
            Palette* prevPal = sprite->palette(frame);
            UniquePtr<Palette> pal(ase_file_read_palette_chunk(f, prevPal, frame));

            if (prevPal->countDiff(pal.get(), NULL, NULL) > 0)
              sprite->setPalette(pal.get(), true);

            ignore_old_color_chunks = true;
            break;
          }

          case ASE_FILE_CHUNK_LAYER: {
            last_object_with_user_data =
              ase_file_read_layer_chunk(f, &header, sprite,
                                        &last_layer,
                                        &current_level);
            break;
          }

          case ASE_FILE_CHUNK_CEL: {
            Cel* cel =
              ase_file_read_cel_chunk(f, sprite, frame,
                                      sprite->pixelFormat(), fop, &header,
                                      chunk_pos+chunk_size);
            if (cel) {
              last_object_with_user_data = cel->data();
            }
            break;
          }

          case ASE_FILE_CHUNK_MASK: {
            Mask* mask = ase_file_read_mask_chunk(f);
            if (mask)
              delete mask;      // TODO add the mask in some place?
            else
              fop->setError("Warning: error loading a mask chunk\n");
            break;
          }

          case ASE_FILE_CHUNK_PATH:
            // Ignore
            break;

          case ASE_FILE_CHUNK_FRAME_TAGS:
            ase_file_read_frame_tags_chunk(f, &sprite->frameTags());
            break;

          case ASE_FILE_CHUNK_USER_DATA: {
            UserData userData;
            ase_file_read_user_data_chunk(f, &userData);
            if (last_object_with_user_data)
              last_object_with_user_data->setUserData(userData);
            break;
          }

          default:
            fop->setError("Warning: Unsupported chunk type %d (skipping)\n", chunk_type);
            break;
        }

        // Skip chunk size
        fseek(f, chunk_pos+chunk_size, SEEK_SET);
      }
    }

    // Skip frame size
    fseek(f, frame_pos+frame_header.size, SEEK_SET);

    // Just one frame?
    if (fop->isOneFrame())
      break;

    if (fop->isStop())
      break;
  }

  fop->createDocument(sprite);
  sprite.release();

  if (ferror(f)) {
    fop->setError("Error reading file.\n");
    return false;
  }
  else {
    return true;
  }
}

#ifdef ENABLE_SAVE

bool AseFormat::onSave(FileOp* fop)
{
  const Sprite* sprite = fop->document()->sprite();
  FileHandle handle(open_file_with_exception(fop->filename(), "wb"));
  FILE* f = handle.get();

  // Write the header
  ASE_Header header;
  ase_file_prepare_header(f, &header, sprite);
  ase_file_write_header(f, &header);

  bool require_new_palette_chunk = false;
  for (Palette* pal : sprite->getPalettes()) {
    if (pal->size() != 256 || pal->hasAlpha()) {
      require_new_palette_chunk = true;
      break;
    }
  }

  // Write frames
  for (frame_t frame(0); frame<sprite->totalFrames(); ++frame) {
    // Prepare the frame header
    ASE_FrameHeader frame_header;
    ase_file_prepare_frame_header(f, &frame_header);

    // Frame duration
    frame_header.duration = sprite->frameDuration(frame);

    // is the first frame or did the palette change?
    Palette* pal = sprite->palette(frame);
    int palFrom = 0, palTo = pal->size()-1;
    if ((frame == 0 ||
         sprite->palette(frame-1)->countDiff(pal, &palFrom, &palTo) > 0)) {
      // Write new palette chunk
      if (require_new_palette_chunk) {
        ase_file_write_palette_chunk(f, &frame_header,
                                     pal, palFrom, palTo);
      }

      // Write color chunk for backward compatibility only
      ase_file_write_color2_chunk(f, &frame_header, pal);
    }

    // Write extra chunks in the first frame
    if (frame == 0) {
      LayerIterator it = sprite->folder()->getLayerBegin();
      LayerIterator end = sprite->folder()->getLayerEnd();

      // Write layer chunks
      for (; it != end; ++it)
        ase_file_write_layers(f, &frame_header, *it);

      // Writer frame tags
      if (sprite->frameTags().size() > 0)
        ase_file_write_frame_tags_chunk(f, &frame_header, &sprite->frameTags());
    }

    // Write cel chunks
    ase_file_write_cels(f, &frame_header, sprite, sprite->folder(), frame);

    // Write the frame header
    ase_file_write_frame_header(f, &frame_header);

    // Progress
    if (sprite->totalFrames() > 1)
      fop->setProgress(float(frame+1) / float(sprite->totalFrames()));

    if (fop->isStop())
      break;
  }

  // Write the missing field (filesize) of the header.
  ase_file_write_header_filesize(f, &header);

  if (ferror(f)) {
    fop->setError("Error writing file.\n");
    return false;
  }
  else {
    return true;
  }
}

#endif  // ENABLE_SAVE

static bool ase_file_read_header(FILE* f, ASE_Header* header)
{
  header->pos = ftell(f);

  header->size  = fgetl(f);
  header->magic = fgetw(f);
  if (header->magic != ASE_FILE_MAGIC)
    return false;

  header->frames     = fgetw(f);
  header->width      = fgetw(f);
  header->height     = fgetw(f);
  header->depth      = fgetw(f);
  header->flags      = fgetl(f);
  header->speed      = fgetw(f);
  header->next       = fgetl(f);
  header->frit       = fgetl(f);
  header->transparent_index = fgetc(f);
  header->ignore[0]  = fgetc(f);
  header->ignore[1]  = fgetc(f);
  header->ignore[2]  = fgetc(f);
  header->ncolors    = fgetw(f);
  header->pixel_width = fgetc(f);
  header->pixel_height = fgetc(f);

  if (header->ncolors == 0)     // 0 means 256 (old .ase files)
    header->ncolors = 256;

  if (header->pixel_width == 0 ||
      header->pixel_height == 0) {
    header->pixel_width = 1;
    header->pixel_height = 1;
  }

  fseek(f, header->pos+128, SEEK_SET);
  return true;
}

static void ase_file_prepare_header(FILE* f, ASE_Header* header, const Sprite* sprite)
{
  header->pos = ftell(f);

  header->size = 0;
  header->magic = ASE_FILE_MAGIC;
  header->frames = sprite->totalFrames();
  header->width = sprite->width();
  header->height = sprite->height();
  header->depth = (sprite->pixelFormat() == IMAGE_RGB ? 32:
                   sprite->pixelFormat() == IMAGE_GRAYSCALE ? 16:
                   sprite->pixelFormat() == IMAGE_INDEXED ? 8: 0);
  header->flags = ASE_FILE_FLAG_LAYER_WITH_OPACITY;
  header->speed = sprite->frameDuration(frame_t(0));
  header->next = 0;
  header->frit = 0;
  header->transparent_index = sprite->transparentColor();
  header->ignore[0] = 0;
  header->ignore[1] = 0;
  header->ignore[2] = 0;
  header->ncolors = sprite->palette(frame_t(0))->size();
  header->pixel_width = sprite->pixelRatio().w;
  header->pixel_height = sprite->pixelRatio().h;
}

static void ase_file_write_header(FILE* f, ASE_Header* header)
{
  fseek(f, header->pos, SEEK_SET);

  fputl(header->size, f);
  fputw(header->magic, f);
  fputw(header->frames, f);
  fputw(header->width, f);
  fputw(header->height, f);
  fputw(header->depth, f);
  fputl(header->flags, f);
  fputw(header->speed, f);
  fputl(header->next, f);
  fputl(header->frit, f);
  fputc(header->transparent_index, f);
  fputc(header->ignore[0], f);
  fputc(header->ignore[1], f);
  fputc(header->ignore[2], f);
  fputw(header->ncolors, f);
  fputc(header->pixel_width, f);
  fputc(header->pixel_height, f);

  fseek(f, header->pos+128, SEEK_SET);
}

static void ase_file_write_header_filesize(FILE* f, ASE_Header* header)
{
  header->size = ftell(f)-header->pos;

  fseek(f, header->pos, SEEK_SET);
  fputl(header->size, f);

  fseek(f, header->pos+header->size, SEEK_SET);
}

static void ase_file_read_frame_header(FILE* f, ASE_FrameHeader* frame_header)
{
  frame_header->size = fgetl(f);
  frame_header->magic = fgetw(f);
  frame_header->chunks = fgetw(f);
  frame_header->duration = fgetw(f);
  ase_file_read_padding(f, 6);
}

static void ase_file_prepare_frame_header(FILE* f, ASE_FrameHeader* frame_header)
{
  int pos = ftell(f);

  frame_header->size = pos;
  frame_header->magic = ASE_FILE_FRAME_MAGIC;
  frame_header->chunks = 0;
  frame_header->duration = 0;

  fseek(f, pos+16, SEEK_SET);
}

static void ase_file_write_frame_header(FILE* f, ASE_FrameHeader* frame_header)
{
  int pos = frame_header->size;
  int end = ftell(f);

  frame_header->size = end-pos;

  fseek(f, pos, SEEK_SET);

  fputl(frame_header->size, f);
  fputw(frame_header->magic, f);
  fputw(frame_header->chunks, f);
  fputw(frame_header->duration, f);
  ase_file_write_padding(f, 6);

  fseek(f, end, SEEK_SET);
}

static void ase_file_write_layers(FILE* f, ASE_FrameHeader* frame_header, const Layer* layer)
{
  ase_file_write_layer_chunk(f, frame_header, layer);
  if (!layer->userData().isEmpty())
    ase_file_write_user_data_chunk(f, frame_header, &layer->userData());

  if (layer->isFolder()) {
    auto it = static_cast<const LayerFolder*>(layer)->getLayerBegin(),
         end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

    for (; it != end; ++it)
      ase_file_write_layers(f, frame_header, *it);
  }
}

static void ase_file_write_cels(FILE* f, ASE_FrameHeader* frame_header, const Sprite* sprite, const Layer* layer, frame_t frame)
{
  if (layer->isImage()) {
    const Cel* cel = layer->cel(frame);
    if (cel) {
/*       fop->setError("New cel in frame %d, in layer %d\n", */
/*                   frame, sprite_layer2index(sprite, layer)); */

      ase_file_write_cel_chunk(f, frame_header, cel, static_cast<const LayerImage*>(layer), sprite);

      if (!cel->link() &&
          !cel->data()->userData().isEmpty()) {
        ase_file_write_user_data_chunk(f, frame_header,
                                       &cel->data()->userData());
      }
    }
  }

  if (layer->isFolder()) {
    auto it = static_cast<const LayerFolder*>(layer)->getLayerBegin(),
         end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

    for (; it != end; ++it)
      ase_file_write_cels(f, frame_header, sprite, *it, frame);
  }
}

static void ase_file_read_padding(FILE* f, int bytes)
{
  for (int c=0; c<bytes; c++)
    fgetc(f);
}

static void ase_file_write_padding(FILE* f, int bytes)
{
  for (int c=0; c<bytes; c++)
    fputc(0, f);
}

static std::string ase_file_read_string(FILE* f)
{
  int length = fgetw(f);
  if (length == EOF)
    return "";

  std::string string;
  string.reserve(length+1);

  for (int c=0; c<length; c++)
    string.push_back(fgetc(f));

  return string;
}

static void ase_file_write_string(FILE* f, const std::string& string)
{
  fputw(string.size(), f);

  for (size_t c=0; c<string.size(); ++c)
    fputc(string[c], f);
}

static void ase_file_write_start_chunk(FILE* f, ASE_FrameHeader* frame_header, int type, ASE_Chunk* chunk)
{
  frame_header->chunks++;

  chunk->type = type;
  chunk->start = ftell(f);

  fseek(f, chunk->start+6, SEEK_SET);
}

static void ase_file_write_close_chunk(FILE* f, ASE_Chunk* chunk)
{
  int chunk_end = ftell(f);
  int chunk_size = chunk_end - chunk->start;

  fseek(f, chunk->start, SEEK_SET);
  fputl(chunk_size, f);
  fputw(chunk->type, f);
  fseek(f, chunk_end, SEEK_SET);
}

static Palette* ase_file_read_color_chunk(FILE* f, Palette* prevPal, frame_t frame)
{
  int i, c, r, g, b, packets, skip, size;
  Palette* pal = new Palette(*prevPal);
  pal->setFrame(frame);

  packets = fgetw(f);   // Number of packets
  skip = 0;

  // Read all packets
  for (i=0; i<packets; i++) {
    skip += fgetc(f);
    size = fgetc(f);
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      pal->setEntry(c, rgba(scale_6bits_to_8bits(r),
                            scale_6bits_to_8bits(g),
                            scale_6bits_to_8bits(b), 255));
    }
  }

  return pal;
}

static Palette* ase_file_read_color2_chunk(FILE* f, Palette* prevPal, frame_t frame)
{
  int i, c, r, g, b, packets, skip, size;
  Palette* pal = new Palette(*prevPal);
  pal->setFrame(frame);

  packets = fgetw(f);   // Number of packets
  skip = 0;

  // Read all packets
  for (i=0; i<packets; i++) {
    skip += fgetc(f);
    size = fgetc(f);
    if (!size) size = 256;

    for (c=skip; c<skip+size; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      pal->setEntry(c, rgba(r, g, b, 255));
    }
  }

  return pal;
}

static Palette* ase_file_read_palette_chunk(FILE* f, Palette* prevPal, frame_t frame)
{
  Palette* pal = new Palette(*prevPal);
  pal->setFrame(frame);

  int newSize = fgetl(f);
  int from = fgetl(f);
  int to = fgetl(f);
  ase_file_read_padding(f, 8);

  if (newSize > 0)
    pal->resize(newSize);

  for (int c=from; c<=to; ++c) {
    int flags = fgetw(f);
    int r = fgetc(f);
    int g = fgetc(f);
    int b = fgetc(f);
    int a = fgetc(f);
    pal->setEntry(c, rgba(r, g, b, a));

    // Skip name
    if (flags & ASE_PALETTE_FLAG_HAS_NAME) {
      std::string name = ase_file_read_string(f);
      // Ignore color entry name
    }
  }

  return pal;
}

static void ase_file_write_color2_chunk(FILE* f, ASE_FrameHeader* frame_header, const Palette* pal)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_FLI_COLOR2);
  int c, color;

  fputw(1, f);                  // Number of packets

  // First packet
  fputc(0, f);                                   // skip 0 colors
  fputc(pal->size() == 256 ? 0: pal->size(), f); // number of colors
  for (c=0; c<pal->size(); c++) {
    color = pal->getEntry(c);
    fputc(rgba_getr(color), f);
    fputc(rgba_getg(color), f);
    fputc(rgba_getb(color), f);
  }
}

static void ase_file_write_palette_chunk(FILE* f, ASE_FrameHeader* frame_header, const Palette* pal, int from, int to)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_PALETTE);

  fputl(pal->size(), f);
  fputl(from, f);
  fputl(to, f);
  ase_file_write_padding(f, 8);

  for (int c=from; c<=to; ++c) {
    color_t color = pal->getEntry(c);
    fputw(0, f);                // Entry flags (without name)
    fputc(rgba_getr(color), f);
    fputc(rgba_getg(color), f);
    fputc(rgba_getb(color), f);
    fputc(rgba_geta(color), f);
  }
}

static Layer* ase_file_read_layer_chunk(FILE* f, ASE_Header* header, Sprite* sprite, Layer** previous_layer, int* current_level)
{
  std::string name;
  Layer* layer = NULL;
  /* read chunk data */
  int flags;
  int layer_type;
  int child_level;

  flags = fgetw(f);
  layer_type = fgetw(f);
  child_level = fgetw(f);
  fgetw(f);                     // default width
  fgetw(f);                     // default height
  int blendmode = fgetw(f);     // blend mode
  int opacity = fgetc(f);       // opacity

  ase_file_read_padding(f, 3);
  name = ase_file_read_string(f);

  // Image layer
  if (layer_type == 0) {
    layer = new LayerImage(sprite);

    // Only transparent layers can have blend mode and opacity
    if (!(flags & ASE_LAYER_FLAG_BACKGROUND)) {
      static_cast<LayerImage*>(layer)->setBlendMode((BlendMode)blendmode);
      if (header->flags & ASE_FILE_FLAG_LAYER_WITH_OPACITY)
        static_cast<LayerImage*>(layer)->setOpacity(opacity);
    }
  }
  // Layer set
  else if (layer_type == 1) {
    layer = new LayerFolder(sprite);
  }

  if (layer) {
    // flags
    layer->setFlags(static_cast<LayerFlags>(flags));

    // name
    layer->setName(name.c_str());

    // Child level
    if (child_level == *current_level)
      (*previous_layer)->parent()->addLayer(layer);
    else if (child_level > *current_level)
      static_cast<LayerFolder*>(*previous_layer)->addLayer(layer);
    else if (child_level < *current_level)
      (*previous_layer)->parent()->parent()->addLayer(layer);

    *previous_layer = layer;
    *current_level = child_level;
  }

  return layer;
}

static void ase_file_write_layer_chunk(FILE* f, ASE_FrameHeader* frame_header, const Layer* layer)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_LAYER);

  // Flags
  fputw(static_cast<int>(layer->flags()), f);

  // Layer type
  fputw(layer->isImage() ? 0: (layer->isFolder() ? 1: -1), f);

  // Layer child level
  LayerFolder* parent = layer->parent();
  int child_level = -1;
  while (parent != NULL) {
    child_level++;
    parent = parent->parent();
  }
  fputw(child_level, f);

  // Default width & height, and blend mode
  fputw(0, f);
  fputw(0, f);
  fputw(layer->isImage() ? (int)static_cast<const LayerImage*>(layer)->blendMode(): 0, f);
  fputc(layer->isImage() ? (int)static_cast<const LayerImage*>(layer)->opacity(): 0, f);

  // padding
  ase_file_write_padding(f, 3);

  /* layer name */
  ase_file_write_string(f, layer->name());

  /* fop->setError("Layer name \"%s\" child level: %d\n", layer->name, child_level); */
}

//////////////////////////////////////////////////////////////////////
// Pixel I/O
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class PixelIO {
public:
  typename ImageTraits::pixel_t read_pixel(FILE* f);
  void write_pixel(FILE* f, typename ImageTraits::pixel_t c);
  void read_scanline(typename ImageTraits::address_t address, int w, uint8_t* buffer);
  void write_scanline(typename ImageTraits::address_t address, int w, uint8_t* buffer);
};

template<>
class PixelIO<RgbTraits> {
  int r, g, b, a;
public:
  RgbTraits::pixel_t read_pixel(FILE* f) {
    r = fgetc(f);
    g = fgetc(f);
    b = fgetc(f);
    a = fgetc(f);
    return rgba(r, g, b, a);
  }
  void write_pixel(FILE* f, RgbTraits::pixel_t c) {
    fputc(rgba_getr(c), f);
    fputc(rgba_getg(c), f);
    fputc(rgba_getb(c), f);
    fputc(rgba_geta(c), f);
  }
  void read_scanline(RgbTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x=0; x<w; ++x) {
      r = *(buffer++);
      g = *(buffer++);
      b = *(buffer++);
      a = *(buffer++);
      *(address++) = rgba(r, g, b, a);
    }
  }
  void write_scanline(RgbTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x=0; x<w; ++x) {
      *(buffer++) = rgba_getr(*address);
      *(buffer++) = rgba_getg(*address);
      *(buffer++) = rgba_getb(*address);
      *(buffer++) = rgba_geta(*address);
      ++address;
    }
  }
};

template<>
class PixelIO<GrayscaleTraits> {
  int k, a;
public:
  GrayscaleTraits::pixel_t read_pixel(FILE* f) {
    k = fgetc(f);
    a = fgetc(f);
    return graya(k, a);
  }
  void write_pixel(FILE* f, GrayscaleTraits::pixel_t c) {
    fputc(graya_getv(c), f);
    fputc(graya_geta(c), f);
  }
  void read_scanline(GrayscaleTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x=0; x<w; ++x) {
      k = *(buffer++);
      a = *(buffer++);
      *(address++) = graya(k, a);
    }
  }
  void write_scanline(GrayscaleTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x=0; x<w; ++x) {
      *(buffer++) = graya_getv(*address);
      *(buffer++) = graya_geta(*address);
      ++address;
    }
  }
};

template<>
class PixelIO<IndexedTraits> {
public:
  IndexedTraits::pixel_t read_pixel(FILE* f) {
    return fgetc(f);
  }
  void write_pixel(FILE* f, IndexedTraits::pixel_t c) {
    fputc(c, f);
  }
  void read_scanline(IndexedTraits::address_t address, int w, uint8_t* buffer)
  {
    memcpy(address, buffer, w);
  }
  void write_scanline(IndexedTraits::address_t address, int w, uint8_t* buffer)
  {
    memcpy(buffer, address, w);
  }
};

//////////////////////////////////////////////////////////////////////
// Raw Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
static void read_raw_image(FILE* f, Image* image, FileOp* fop, ASE_Header* header)
{
  PixelIO<ImageTraits> pixel_io;
  int x, y;

  for (y=0; y<image->height(); y++) {
    for (x=0; x<image->width(); x++)
      put_pixel_fast<ImageTraits>(image, x, y, pixel_io.read_pixel(f));

    fop->setProgress((float)ftell(f) / (float)header->size);
  }
}

template<typename ImageTraits>
static void write_raw_image(FILE* f, const Image* image)
{
  PixelIO<ImageTraits> pixel_io;
  int x, y;

  for (y=0; y<image->height(); y++)
    for (x=0; x<image->width(); x++)
      pixel_io.write_pixel(f, get_pixel_fast<ImageTraits>(image, x, y));
}

//////////////////////////////////////////////////////////////////////
// Compressed Image
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
static void read_compressed_image(FILE* f, Image* image, size_t chunk_end, FileOp* fop, ASE_Header* header)
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

    if (ftell(f)+compressed.size() > chunk_end) {
      input_bytes = chunk_end - ftell(f); // Remaining bytes
      ASSERT(input_bytes < compressed.size());

      if (input_bytes == 0)
        break;                  // Done, we consumed all chunk
    }
    else
      input_bytes = compressed.size();

    size_t bytes_read = fread(&compressed[0], 1, input_bytes, f);
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

    fop->setProgress((float)ftell(f) / (float)header->size);
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

template<typename ImageTraits>
static void write_compressed_image(FILE* f, const Image* image)
{
  PixelIO<ImageTraits> pixel_io;
  z_stream zstream;
  int y, err;

  zstream.zalloc = (alloc_func)0;
  zstream.zfree  = (free_func)0;
  zstream.opaque = (voidpf)0;
  err = deflateInit(&zstream, Z_DEFAULT_COMPRESSION);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in deflateInit().", err);

  std::vector<uint8_t> scanline(ImageTraits::getRowStrideBytes(image->width()));
  std::vector<uint8_t> compressed(4096);

  for (y=0; y<image->height(); y++) {
    typename ImageTraits::address_t address =
      (typename ImageTraits::address_t)image->getPixelAddress(0, y);

    pixel_io.write_scanline(address, image->width(), &scanline[0]);

    zstream.next_in = (Bytef*)&scanline[0];
    zstream.avail_in = scanline.size();
    int flush = (y == image->height()-1 ? Z_FINISH: Z_NO_FLUSH);

    do {
      zstream.next_out = (Bytef*)&compressed[0];
      zstream.avail_out = compressed.size();

      // Compress
      err = deflate(&zstream, flush);
      if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR)
        throw base::Exception("ZLib error %d in deflate().", err);

      int output_bytes = compressed.size() - zstream.avail_out;
      if (output_bytes > 0) {
        if ((fwrite(&compressed[0], 1, output_bytes, f) != (size_t)output_bytes)
            || ferror(f))
          throw base::Exception("Error writing compressed image pixels.\n");
      }
    } while (zstream.avail_out == 0);
  }

  err = deflateEnd(&zstream);
  if (err != Z_OK)
    throw base::Exception("ZLib error %d in deflateEnd().", err);
}

//////////////////////////////////////////////////////////////////////
// Cel Chunk
//////////////////////////////////////////////////////////////////////

static Cel* ase_file_read_cel_chunk(FILE* f, Sprite* sprite, frame_t frame,
                                    PixelFormat pixelFormat,
                                    FileOp* fop, ASE_Header* header, size_t chunk_end)
{
  /* read chunk data */
  LayerIndex layer_index = LayerIndex(fgetw(f));
  int x = ((short)fgetw(f));
  int y = ((short)fgetw(f));
  int opacity = fgetc(f);
  int cel_type = fgetw(f);
  Layer* layer;

  ase_file_read_padding(f, 7);

  layer = sprite->indexToLayer(layer_index);
  if (!layer) {
    fop->setError("Frame %d didn't found layer with index %d\n",
                  (int)frame, (int)layer_index);
    return NULL;
  }
  if (!layer->isImage()) {
    fop->setError("Invalid .ase file (frame %d in layer %d which does not contain images\n",
                  (int)frame, (int)layer_index);
    return NULL;
  }

  // Create the new frame.
  base::UniquePtr<Cel> cel;

  switch (cel_type) {

    case ASE_FILE_RAW_CEL: {
      // Read width and height
      int w = fgetw(f);
      int h = fgetw(f);

      if (w > 0 && h > 0) {
        ImageRef image(Image::create(pixelFormat, w, h));

        // Read pixel data
        switch (image->pixelFormat()) {

          case IMAGE_RGB:
            read_raw_image<RgbTraits>(f, image.get(), fop, header);
            break;

          case IMAGE_GRAYSCALE:
            read_raw_image<GrayscaleTraits>(f, image.get(), fop, header);
            break;

          case IMAGE_INDEXED:
            read_raw_image<IndexedTraits>(f, image.get(), fop, header);
            break;
        }

        cel.reset(new Cel(frame, image));
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
      }
      break;
    }

    case ASE_FILE_LINK_CEL: {
      // Read link position
      frame_t link_frame = frame_t(fgetw(f));
      Cel* link = layer->cel(link_frame);

      if (link) {
        // There were a beta version that allow to the user specify
        // different X, Y, or opacity per link, in that case we must
        // create a copy.
        if (link->x() == x && link->y() == y && link->opacity() == opacity) {
          cel.reset(Cel::createLink(link));
          cel->setFrame(frame);
        }
        else {
          cel.reset(Cel::createCopy(link));
          cel->setFrame(frame);
          cel->setPosition(x, y);
          cel->setOpacity(opacity);
        }
      }
      else {
        // Linked cel doesn't found
        return NULL;
      }
      break;
    }

    case ASE_FILE_COMPRESSED_CEL: {
      // Read width and height
      int w = fgetw(f);
      int h = fgetw(f);

      if (w > 0 && h > 0) {
        ImageRef image(Image::create(pixelFormat, w, h));

        // Try to read pixel data
        try {
          switch (image->pixelFormat()) {

            case IMAGE_RGB:
              read_compressed_image<RgbTraits>(f, image.get(), chunk_end, fop, header);
              break;

            case IMAGE_GRAYSCALE:
              read_compressed_image<GrayscaleTraits>(f, image.get(), chunk_end, fop, header);
              break;

            case IMAGE_INDEXED:
              read_compressed_image<IndexedTraits>(f, image.get(), chunk_end, fop, header);
              break;
          }
        }
        // OK, in case of error we can show the problem, but continue
        // loading more cels.
        catch (const std::exception& e) {
          fop->setError(e.what());
        }

        cel.reset(new Cel(frame, image));
        cel->setPosition(x, y);
        cel->setOpacity(opacity);
      }
      break;
    }

  }

  if (!cel)
    return nullptr;

  static_cast<LayerImage*>(layer)->addCel(cel);
  return cel.release();
}

static void ase_file_write_cel_chunk(FILE* f, ASE_FrameHeader* frame_header,
                                     const Cel* cel, const LayerImage* layer, const Sprite* sprite)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_CEL);

  int layer_index = sprite->layerToIndex(layer);
  const Cel* link = cel->link();
  int cel_type = (link ? ASE_FILE_LINK_CEL: ASE_FILE_COMPRESSED_CEL);

  fputw(layer_index, f);
  fputw(cel->x(), f);
  fputw(cel->y(), f);
  fputc(cel->opacity(), f);
  fputw(cel_type, f);
  ase_file_write_padding(f, 7);

  switch (cel_type) {

    case ASE_FILE_RAW_CEL: {
      const Image* image = cel->image();

      if (image) {
        // Width and height
        fputw(image->width(), f);
        fputw(image->height(), f);

        // Pixel data
        switch (image->pixelFormat()) {

          case IMAGE_RGB:
            write_raw_image<RgbTraits>(f, image);
            break;

          case IMAGE_GRAYSCALE:
            write_raw_image<GrayscaleTraits>(f, image);
            break;

          case IMAGE_INDEXED:
            write_raw_image<IndexedTraits>(f, image);
            break;
        }
      }
      else {
        // Width and height
        fputw(0, f);
        fputw(0, f);
      }
      break;
    }

    case ASE_FILE_LINK_CEL:
      // Linked cel to another frame
      fputw(link->frame(), f);
      break;

    case ASE_FILE_COMPRESSED_CEL: {
      const Image* image = cel->image();

      if (image) {
        // Width and height
        fputw(image->width(), f);
        fputw(image->height(), f);

        // Pixel data
        switch (image->pixelFormat()) {

          case IMAGE_RGB:
            write_compressed_image<RgbTraits>(f, image);
            break;

          case IMAGE_GRAYSCALE:
            write_compressed_image<GrayscaleTraits>(f, image);
            break;

          case IMAGE_INDEXED:
            write_compressed_image<IndexedTraits>(f, image);
            break;
        }
      }
      else {
        // Width and height
        fputw(0, f);
        fputw(0, f);
      }
      break;
    }
  }
}

static Mask* ase_file_read_mask_chunk(FILE* f)
{
  int c, u, v, byte;
  Mask* mask;
  // Read chunk data
  int x = fgetw(f);
  int y = fgetw(f);
  int w = fgetw(f);
  int h = fgetw(f);

  ase_file_read_padding(f, 8);
  std::string name = ase_file_read_string(f);

  mask = new Mask();
  mask->setName(name.c_str());
  mask->replace(gfx::Rect(x, y, w, h));

  // Read image data
  for (v=0; v<h; v++)
    for (u=0; u<(w+7)/8; u++) {
      byte = fgetc(f);
      for (c=0; c<8; c++)
        put_pixel(mask->bitmap(), u*8+c, v, byte & (1<<(7-c)));
    }

  return mask;
}

#if 0
static void ase_file_write_mask_chunk(FILE* f, ASE_FrameHeader* frame_header, Mask* mask)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_MASK);

  int c, u, v, byte;
  const gfx::Rect& bounds(mask->bounds());

  fputw(bounds.x, f);
  fputw(bounds.y, f);
  fputw(bounds.w, f);
  fputw(bounds.h, f);
  ase_file_write_padding(f, 8);

  // Name
  ase_file_write_string(f, mask->name().c_str());

  // Bitmap
  for (v=0; v<bounds.h; v++)
    for (u=0; u<(bounds.w+7)/8; u++) {
      byte = 0;
      for (c=0; c<8; c++)
        if (get_pixel(mask->bitmap(), u*8+c, v))
          byte |= (1<<(7-c));
      fputc(byte, f);
    }
}
#endif

static void ase_file_read_frame_tags_chunk(FILE* f, FrameTags* frameTags)
{
  size_t tags = fgetw(f);

  fgetl(f);                     // 8 reserved bytes
  fgetl(f);

  for (size_t c=0; c<tags; ++c) {
    frame_t from = fgetw(f);
    frame_t to = fgetw(f);
    int aniDir = fgetc(f);
    if (aniDir != int(AniDir::FORWARD) &&
        aniDir != int(AniDir::REVERSE) &&
        aniDir != int(AniDir::PING_PONG)) {
      aniDir = int(AniDir::FORWARD);
    }

    fgetl(f);                     // 8 reserved bytes
    fgetl(f);

    int r = fgetc(f);
    int g = fgetc(f);
    int b = fgetc(f);
    fgetc(f);                     // Skip

    std::string name = ase_file_read_string(f);

    FrameTag* tag = new FrameTag(from, to);
    tag->setColor(doc::rgba(r, g, b, 255));
    tag->setName(name);
    tag->setAniDir((AniDir)aniDir);
    frameTags->add(tag);
  }
}

static void ase_file_write_frame_tags_chunk(FILE* f, ASE_FrameHeader* frame_header, const FrameTags* frameTags)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_FRAME_TAGS);

  fputw(frameTags->size(), f);

  fputl(0, f);  // 8 reserved bytes
  fputl(0, f);

  for (const FrameTag* tag : *frameTags) {
    fputw(tag->fromFrame(), f);
    fputw(tag->toFrame(), f);
    fputc((int)tag->aniDir(), f);

    fputl(0, f);  // 8 reserved bytes
    fputl(0, f);

    fputc(doc::rgba_getr(tag->color()), f);
    fputc(doc::rgba_getg(tag->color()), f);
    fputc(doc::rgba_getb(tag->color()), f);
    fputc(0, f);

    ase_file_write_string(f, tag->name().c_str());
  }
}

static void ase_file_read_user_data_chunk(FILE* f, UserData* userData)
{
  size_t flags = fgetl(f);

  if (flags & ASE_USER_DATA_FLAG_HAS_TEXT) {
    std::string text = ase_file_read_string(f);
    userData->setText(text);
  }

  if (flags & ASE_USER_DATA_FLAG_HAS_COLOR) {
    int r = fgetc(f);
    int g = fgetc(f);
    int b = fgetc(f);
    int a = fgetc(f);
    userData->setColor(doc::rgba(r, g, b, a));
  }
}

static void ase_file_write_user_data_chunk(FILE* f, ASE_FrameHeader* frame_header, const UserData* userData)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_USER_DATA);

  int flags = 0;
  if (!userData->text().empty())
    flags |= ASE_USER_DATA_FLAG_HAS_TEXT;
  if (doc::rgba_geta(userData->color()))
    flags |= ASE_USER_DATA_FLAG_HAS_COLOR;
  fputl(flags, f);

  if (flags & ASE_USER_DATA_FLAG_HAS_TEXT)
    ase_file_write_string(f, userData->text().c_str());

  if (flags & ASE_USER_DATA_FLAG_HAS_COLOR) {
    fputc(doc::rgba_getr(userData->color()), f);
    fputc(doc::rgba_getg(userData->color()), f);
    fputc(doc::rgba_getb(userData->color()), f);
    fputc(doc::rgba_geta(userData->color()), f);
  }
}

} // namespace app
