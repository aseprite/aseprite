// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "app/pref/preferences.h"
#include "base/cfile.h"
#include "base/clamp.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "dio/aseprite_common.h"
#include "dio/aseprite_decoder.h"
#include "dio/decode_delegate.h"
#include "dio/file_interface.h"
#include "doc/doc.h"
#include "fixmath/fixmath.h"
#include "fmt/format.h"
#include "ui/alert.h"
#include "ver/info.h"
#include "zlib.h"

#include <cstdio>

namespace app {

using namespace base;

namespace {

class DecodeDelegate : public dio::DecodeDelegate {
public:
  DecodeDelegate(FileOp* fop)
    : m_fop(fop)
    , m_sprite(nullptr) {
  }
  ~DecodeDelegate() { }

  void error(const std::string& msg) override {
    m_fop->setError(msg.c_str());
  }

  void progress(double fromZeroToOne) override {
    m_fop->setProgress(fromZeroToOne);
  }

  bool isCanceled() override {
    return m_fop->isStop();
  }

  bool decodeOneFrame() override {
    return m_fop->isOneFrame();
  }

  doc::color_t defaultSliceColor() override {
    auto color = Preferences::instance().slices.defaultColor();
    return doc::rgba(color.getRed(),
                     color.getGreen(),
                     color.getBlue(),
                     color.getAlpha());
  }

  void onSprite(doc::Sprite* sprite) override {
    m_sprite = sprite;
  }

  doc::Sprite* sprite() { return m_sprite; }

private:
  FileOp* m_fop;
  doc::Sprite* m_sprite;
};

} // anonymous namespace

static void ase_file_prepare_header(FILE* f, dio::AsepriteHeader* header, const Sprite* sprite,
                                    const frame_t firstFrame, const frame_t totalFrames);
static void ase_file_write_header(FILE* f, dio::AsepriteHeader* header);
static void ase_file_write_header_filesize(FILE* f, dio::AsepriteHeader* header);

static void ase_file_prepare_frame_header(FILE* f, dio::AsepriteFrameHeader* frame_header);
static void ase_file_write_frame_header(FILE* f, dio::AsepriteFrameHeader* frame_header);

static void ase_file_write_layers(FILE* f, dio::AsepriteFrameHeader* frame_header, const Layer* layer, int child_level);
static layer_t ase_file_write_cels(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                   const Sprite* sprite, const Layer* layer,
                                   layer_t layer_index,
                                   const frame_t frame,
                                   const frame_t firstFrame);

static void ase_file_write_padding(FILE* f, int bytes);
static void ase_file_write_string(FILE* f, const std::string& string);

static void ase_file_write_start_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, int type, dio::AsepriteChunk* chunk);
static void ase_file_write_close_chunk(FILE* f, dio::AsepriteChunk* chunk);

static void ase_file_write_color2_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Palette* pal);
static void ase_file_write_palette_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Palette* pal, int from, int to);
static void ase_file_write_layer_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Layer* layer, int child_level);
static void ase_file_write_cel_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                     const Cel* cel,
                                     const LayerImage* layer,
                                     const layer_t layer_index,
                                     const Sprite* sprite,
                                     const frame_t firstFrame);
static void ase_file_write_cel_extra_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                           const Cel* cel);
static void ase_file_write_color_profile(FILE* f,
                                         dio::AsepriteFrameHeader* frame_header,
                                         const doc::Sprite* sprite);
#if 0
static void ase_file_write_mask_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, Mask* mask);
#endif
static void ase_file_write_tags_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Tags* tags,
                                      const frame_t fromFrame, const frame_t toFrame);
static void ase_file_write_slice_chunks(FILE* f, dio::AsepriteFrameHeader* frame_header, const Slices& slices,
                                        const frame_t fromFrame, const frame_t toFrame);
static void ase_file_write_slice_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, Slice* slice,
                                       const frame_t fromFrame, const frame_t toFrame);
static void ase_file_write_user_data_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const UserData* userData);
static bool ase_has_groups(LayerGroup* group);
static void ase_ungroup_all(LayerGroup* group);

class ChunkWriter {
public:
  ChunkWriter(FILE* f, dio::AsepriteFrameHeader* frame_header, int type) : m_file(f) {
    ase_file_write_start_chunk(m_file, frame_header, type, &m_chunk);
  }

  ~ChunkWriter() {
    ase_file_write_close_chunk(m_file, &m_chunk);
  }

private:
  FILE* m_file;
  dio::AsepriteChunk m_chunk;
};

class AseFormat : public FileFormat {

  const char* onGetName() const override {
    return "ase";
  }

  void onGetExtensions(base::paths& exts) const override {
    exts.push_back("ase");
    exts.push_back("aseprite");
  }

  dio::FileFormat onGetDioFormat() const override {
    return dio::FileFormat::ASE_ANIMATION;
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
      FILE_SUPPORT_LAYERS |
      FILE_SUPPORT_FRAMES |
      FILE_SUPPORT_PALETTES |
      FILE_SUPPORT_TAGS |
      FILE_SUPPORT_BIG_PALETTES |
      FILE_SUPPORT_PALETTE_WITH_ALPHA;
  }

  bool onLoad(FileOp* fop) override;
  bool onPostLoad(FileOp* fop) override;
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
  dio::StdioFileInterface fileInterface(handle.get());

  DecodeDelegate delegate(fop);
  dio::AsepriteDecoder decoder;
  decoder.initialize(&delegate, &fileInterface);
  if (!decoder.decode())
    return false;

  Sprite* sprite = delegate.sprite();
  fop->createDocument(sprite);

  if (sprite->colorSpace() != nullptr &&
      sprite->colorSpace()->type() != gfx::ColorSpace::None) {
    fop->setEmbeddedColorProfile();
  }

  // Sprite grid bounds will be set to empty (instead of
  // doc::Sprite::DefaultGridBounds()) if the file doesn't contain an
  // embedded grid bounds.
  if (!sprite->gridBounds().isEmpty())
    fop->setEmbeddedGridBounds();

  return true;
}

bool AseFormat::onPostLoad(FileOp* fop)
{
  LayerGroup* group = fop->document()->sprite()->root();

  // Forward Compatibility: In 1.1 we convert a file with layer groups
  // (saved with 1.2) as top level layers
  std::string ver = get_app_version();
  bool flat = (ver[0] == '1' &&
               ver[1] == '.' &&
               ver[2] == '1');
  if (flat && ase_has_groups(group)) {
    if (fop->context() &&
        fop->context()->isUIAvailable() &&
        ui::Alert::show(
          fmt::format(
            // This message is not translated because is used only in the old v1.1 only
            "Warning"
            "<<The selected file \"{0}\" has layer groups."
            "<<Do you want to open it with \"{1} {2}\" anyway?"
            "<<"
            "<<Note: Layers inside groups will be converted to top level layers."
            "||&Yes||&No",
            base::get_file_name(fop->filename()),
            get_app_name(), ver)) != 1) {
      return false;
    }
    ase_ungroup_all(group);
  }
  return true;
}

#ifdef ENABLE_SAVE

// TODO move the encoder to the dio library
bool AseFormat::onSave(FileOp* fop)
{
  const Sprite* sprite = fop->document()->sprite();
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  FILE* f = handle.get();

  // Write the header
  dio::AsepriteHeader header;
  ase_file_prepare_header(f, &header, sprite,
                          fop->roi().fromFrame(),
                          fop->roi().frames());
  ase_file_write_header(f, &header);

  bool require_new_palette_chunk = false;
  for (Palette* pal : sprite->getPalettes()) {
    if (pal->size() != 256 || pal->hasAlpha()) {
      require_new_palette_chunk = true;
      break;
    }
  }

  // Write frames
  int outputFrame = 0;
  for (frame_t frame : fop->roi().selectedFrames()) {
    // Prepare the frame header
    dio::AsepriteFrameHeader frame_header;
    ase_file_prepare_frame_header(f, &frame_header);

    // Frame duration
    frame_header.duration = sprite->frameDuration(frame);

    // Save color profile in first frame
    if (outputFrame == 0 && fop->preserveColorProfile())
      ase_file_write_color_profile(f, &frame_header, sprite);

    // is the first frame or did the palette change?
    Palette* pal = sprite->palette(frame);
    int palFrom = 0, palTo = pal->size()-1;
    if (// First frame or..
         (frame == fop->roi().fromFrame() ||
         // This palette is different from the previous frame palette
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
    if (frame == fop->roi().fromFrame()) {
      // Write layer chunks
      for (Layer* child : sprite->root()->layers())
        ase_file_write_layers(f, &frame_header, child, 0);

      // Writer frame tags
      if (sprite->tags().size() > 0)
        ase_file_write_tags_chunk(f, &frame_header, &sprite->tags(),
                                  fop->roi().fromFrame(),
                                  fop->roi().toFrame());

      // Writer slice chunks
      ase_file_write_slice_chunks(f, &frame_header,
                                  sprite->slices(),
                                  fop->roi().fromFrame(),
                                  fop->roi().toFrame());
    }

    // Write cel chunks
    ase_file_write_cels(f, &frame_header,
                        sprite, sprite->root(),
                        0, frame, fop->roi().fromFrame());

    // Write the frame header
    ase_file_write_frame_header(f, &frame_header);

    // Progress
    if (fop->roi().frames() > 1)
      fop->setProgress(float(outputFrame+1) / float(fop->roi().frames()));
    ++outputFrame;

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

static void ase_file_prepare_header(FILE* f, dio::AsepriteHeader* header, const Sprite* sprite,
                                    const frame_t firstFrame, const frame_t totalFrames)
{
  header->pos = ftell(f);

  header->size = 0;
  header->magic = ASE_FILE_MAGIC;
  header->frames = totalFrames;
  header->width = sprite->width();
  header->height = sprite->height();
  header->depth = (sprite->pixelFormat() == IMAGE_RGB ? 32:
                   sprite->pixelFormat() == IMAGE_GRAYSCALE ? 16:
                   sprite->pixelFormat() == IMAGE_INDEXED ? 8: 0);
  header->flags = ASE_FILE_FLAG_LAYER_WITH_OPACITY;
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
  header->grid_x       = sprite->gridBounds().x;
  header->grid_y       = sprite->gridBounds().y;
  header->grid_width   = sprite->gridBounds().w;
  header->grid_height  = sprite->gridBounds().h;
}

static void ase_file_write_header(FILE* f, dio::AsepriteHeader* header)
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
  fputw(header->grid_x, f);
  fputw(header->grid_y, f);
  fputw(header->grid_width, f);
  fputw(header->grid_height, f);

  fseek(f, header->pos+128, SEEK_SET);
}

static void ase_file_write_header_filesize(FILE* f, dio::AsepriteHeader* header)
{
  header->size = ftell(f)-header->pos;

  fseek(f, header->pos, SEEK_SET);
  fputl(header->size, f);

  fseek(f, header->pos+header->size, SEEK_SET);
}

static void ase_file_prepare_frame_header(FILE* f, dio::AsepriteFrameHeader* frame_header)
{
  int pos = ftell(f);

  frame_header->size = pos;
  frame_header->magic = ASE_FILE_FRAME_MAGIC;
  frame_header->chunks = 0;
  frame_header->duration = 0;

  fseek(f, pos+16, SEEK_SET);
}

static void ase_file_write_frame_header(FILE* f, dio::AsepriteFrameHeader* frame_header)
{
  int pos = frame_header->size;
  int end = ftell(f);

  frame_header->size = end-pos;

  fseek(f, pos, SEEK_SET);

  fputl(frame_header->size, f);
  fputw(frame_header->magic, f);
  fputw(frame_header->chunks < 0xFFFF ? frame_header->chunks: 0xFFFF, f);
  fputw(frame_header->duration, f);
  ase_file_write_padding(f, 2);
  fputl(frame_header->chunks, f);

  fseek(f, end, SEEK_SET);
}

static void ase_file_write_layers(FILE* f, dio::AsepriteFrameHeader* frame_header, const Layer* layer, int child_index)
{
  ase_file_write_layer_chunk(f, frame_header, layer, child_index);
  if (!layer->userData().isEmpty())
    ase_file_write_user_data_chunk(f, frame_header, &layer->userData());

  if (layer->isGroup()) {
    for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers())
      ase_file_write_layers(f, frame_header, child, child_index+1);
  }
}

static layer_t ase_file_write_cels(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                   const Sprite* sprite, const Layer* layer,
                                   layer_t layer_index,
                                   const frame_t frame,
                                   const frame_t firstFrame)
{
  if (layer->isImage()) {
    const Cel* cel = layer->cel(frame);
    if (cel) {
      ase_file_write_cel_chunk(f, frame_header, cel,
                               static_cast<const LayerImage*>(layer),
                               layer_index, sprite, firstFrame);

      if (layer->isReference())
        ase_file_write_cel_extra_chunk(f, frame_header, cel);

      if (!cel->link() &&
          !cel->data()->userData().isEmpty()) {
        ase_file_write_user_data_chunk(f, frame_header,
                                       &cel->data()->userData());
      }
    }
  }

  if (layer != sprite->root())
    ++layer_index;

  if (layer->isGroup()) {
    for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers()) {
      layer_index =
        ase_file_write_cels(f, frame_header, sprite, child,
                            layer_index, frame, firstFrame);
    }
  }

  return layer_index;
}

static void ase_file_write_padding(FILE* f, int bytes)
{
  for (int c=0; c<bytes; c++)
    fputc(0, f);
}

static void ase_file_write_string(FILE* f, const std::string& string)
{
  fputw(string.size(), f);

  for (size_t c=0; c<string.size(); ++c)
    fputc(string[c], f);
}

static void ase_file_write_start_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, int type, dio::AsepriteChunk* chunk)
{
  frame_header->chunks++;

  chunk->type = type;
  chunk->start = ftell(f);

  fseek(f, chunk->start+6, SEEK_SET);
}

static void ase_file_write_close_chunk(FILE* f, dio::AsepriteChunk* chunk)
{
  int chunk_end = ftell(f);
  int chunk_size = chunk_end - chunk->start;

  fseek(f, chunk->start, SEEK_SET);
  fputl(chunk_size, f);
  fputw(chunk->type, f);
  fseek(f, chunk_end, SEEK_SET);
}

static void ase_file_write_color2_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Palette* pal)
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

static void ase_file_write_palette_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Palette* pal, int from, int to)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_PALETTE);

  fputl(pal->size(), f);
  fputl(from, f);
  fputl(to, f);
  ase_file_write_padding(f, 8);

  for (int c=from; c<=to; ++c) {
    color_t color = pal->getEntry(c);
    // TODO add support to save palette entry name
    fputw(0, f);                // Entry flags (without name)
    fputc(rgba_getr(color), f);
    fputc(rgba_getg(color), f);
    fputc(rgba_getb(color), f);
    fputc(rgba_geta(color), f);
  }
}

static void ase_file_write_layer_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const Layer* layer, int child_level)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_LAYER);

  // Flags
  fputw(static_cast<int>(layer->flags()) &
        static_cast<int>(doc::LayerFlags::PersistentFlagsMask), f);

  // Layer type
  fputw((layer->isImage() ? ASE_FILE_LAYER_IMAGE:
         (layer->isGroup() ? ASE_FILE_LAYER_GROUP: -1)), f);

  // Layer child level
  fputw(child_level, f);

  // Default width & height, and blend mode
  fputw(0, f);
  fputw(0, f);
  fputw(layer->isImage() ? (int)static_cast<const LayerImage*>(layer)->blendMode(): 0, f);
  fputc(layer->isImage() ? (int)static_cast<const LayerImage*>(layer)->opacity(): 0, f);

  // Padding
  ase_file_write_padding(f, 3);

  // Layer name
  ase_file_write_string(f, layer->name());
}

//////////////////////////////////////////////////////////////////////
// Pixel I/O
//////////////////////////////////////////////////////////////////////

template<typename ImageTraits>
class PixelIO {
public:
  void write_pixel(FILE* f, typename ImageTraits::pixel_t c);
  void write_scanline(typename ImageTraits::address_t address, int w, uint8_t* buffer);
};

template<>
class PixelIO<RgbTraits> {
public:
  void write_pixel(FILE* f, RgbTraits::pixel_t c) {
    fputc(rgba_getr(c), f);
    fputc(rgba_getg(c), f);
    fputc(rgba_getb(c), f);
    fputc(rgba_geta(c), f);
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
public:
  void write_pixel(FILE* f, GrayscaleTraits::pixel_t c) {
    fputc(graya_getv(c), f);
    fputc(graya_geta(c), f);
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
  void write_pixel(FILE* f, IndexedTraits::pixel_t c) {
    fputc(c, f);
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

static void ase_file_write_cel_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                     const Cel* cel,
                                     const LayerImage* layer,
                                     const layer_t layer_index,
                                     const Sprite* sprite,
                                     const frame_t firstFrame)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_CEL);

  const Cel* link = cel->link();

  // In case the original link is outside the ROI, we've to find the
  // first linked cel that is inside the ROI.
  if (link && link->frame() < firstFrame) {
    link = nullptr;
    for (frame_t i=firstFrame; i<=cel->frame(); ++i) {
      link = layer->cel(i);
      if (link && link->image()->id() == cel->image()->id())
        break;
    }
    if (link == cel)
      link = nullptr;
  }

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
      fputw(link->frame()-firstFrame, f);
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

static void ase_file_write_cel_extra_chunk(FILE* f,
                                           dio::AsepriteFrameHeader* frame_header,
                                           const Cel* cel)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_CEL_EXTRA);

  ASSERT(cel->layer()->isReference());

  gfx::RectF bounds = cel->boundsF();

  fputl(ASE_CEL_EXTRA_FLAG_PRECISE_BOUNDS, f);
  fputl(fixmath::ftofix(bounds.x), f);
  fputl(fixmath::ftofix(bounds.y), f);
  fputl(fixmath::ftofix(bounds.w), f);
  fputl(fixmath::ftofix(bounds.h), f);
  ase_file_write_padding(f, 16);
}

static void ase_file_write_color_profile(FILE* f,
                                         dio::AsepriteFrameHeader* frame_header,
                                         const doc::Sprite* sprite)
{
  const gfx::ColorSpacePtr& cs = sprite->colorSpace();
  if (!cs)                      // No color
    return;

  int type = ASE_FILE_NO_COLOR_PROFILE;
  switch (cs->type()) {

    case gfx::ColorSpace::None:
      return; // Without color profile, don't write this chunk.

    case gfx::ColorSpace::sRGB:
      type = ASE_FILE_SRGB_COLOR_PROFILE;
      break;
    case gfx::ColorSpace::ICC:
      type = ASE_FILE_ICC_COLOR_PROFILE;
      break;
    default:
      ASSERT(false);            // Unknown color profile
      return;
  }

  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_COLOR_PROFILE);
  fputw(type, f);
  fputw(cs->hasGamma() ? ASE_COLOR_PROFILE_FLAG_GAMMA: 0, f);

  fixmath::fixed gamma = 0;
  if (cs->hasGamma())
    gamma = fixmath::ftofix(cs->gamma());
  fputl(gamma, f);
  ase_file_write_padding(f, 8);

  if (cs->type() == gfx::ColorSpace::ICC) {
    const size_t size = cs->iccSize();
    const void* data = cs->iccData();
    fputl(size, f);
    if (size && data)
      fwrite(data, 1, size, f);
  }
}

#if 0
static void ase_file_write_mask_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, Mask* mask)
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
  ase_file_write_string(f, mask->name());

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

static void ase_file_write_tags_chunk(FILE* f,
                                      dio::AsepriteFrameHeader* frame_header,
                                      const Tags* tags,
                                      const frame_t fromFrame,
                                      const frame_t toFrame)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_TAGS);

  int ntags = 0;
  for (const Tag* tag : *tags) {
    // Skip tags that are outside of the given ROI
    if (tag->fromFrame() > toFrame ||
        tag->toFrame() < fromFrame)
      continue;
    ++ntags;
  }

  fputw(ntags, f);
  fputl(0, f);  // 8 reserved bytes
  fputl(0, f);

  for (const Tag* tag : *tags) {
    if (tag->fromFrame() > toFrame ||
        tag->toFrame() < fromFrame)
      continue;

    frame_t from = base::clamp(tag->fromFrame()-fromFrame, 0, toFrame-fromFrame);
    frame_t to = base::clamp(tag->toFrame()-fromFrame, from, toFrame-fromFrame);

    fputw(from, f);
    fputw(to, f);
    fputc((int)tag->aniDir(), f);

    fputl(0, f);  // 8 reserved bytes
    fputl(0, f);

    fputc(doc::rgba_getr(tag->color()), f);
    fputc(doc::rgba_getg(tag->color()), f);
    fputc(doc::rgba_getb(tag->color()), f);
    fputc(0, f);

    ase_file_write_string(f, tag->name());
  }
}

static void ase_file_write_user_data_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header, const UserData* userData)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_USER_DATA);

  int flags = 0;
  if (!userData->text().empty())
    flags |= ASE_USER_DATA_FLAG_HAS_TEXT;
  if (doc::rgba_geta(userData->color()))
    flags |= ASE_USER_DATA_FLAG_HAS_COLOR;
  fputl(flags, f);

  if (flags & ASE_USER_DATA_FLAG_HAS_TEXT)
    ase_file_write_string(f, userData->text());

  if (flags & ASE_USER_DATA_FLAG_HAS_COLOR) {
    fputc(doc::rgba_getr(userData->color()), f);
    fputc(doc::rgba_getg(userData->color()), f);
    fputc(doc::rgba_getb(userData->color()), f);
    fputc(doc::rgba_geta(userData->color()), f);
  }
}

static void ase_file_write_slice_chunks(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                        const Slices& slices,
                                        const frame_t fromFrame,
                                        const frame_t toFrame)
{
  for (Slice* slice : slices) {
    // Skip slices that are outside of the given ROI
    if (slice->range(fromFrame, toFrame).empty())
      continue;

    ase_file_write_slice_chunk(f, frame_header, slice,
                               fromFrame, toFrame);

    if (!slice->userData().isEmpty())
      ase_file_write_user_data_chunk(f, frame_header, &slice->userData());
  }
}

static void ase_file_write_slice_chunk(FILE* f, dio::AsepriteFrameHeader* frame_header,
                                       Slice* slice,
                                       const frame_t fromFrame,
                                       const frame_t toFrame)
{
  ChunkWriter chunk(f, frame_header, ASE_FILE_CHUNK_SLICE);

  auto range = slice->range(fromFrame, toFrame);
  ASSERT(!range.empty());

  int flags = 0;
  for (auto key : range) {
    if (key) {
      if (key->hasCenter()) flags |= ASE_SLICE_FLAG_HAS_CENTER_BOUNDS;
      if (key->hasPivot()) flags |= ASE_SLICE_FLAG_HAS_PIVOT_POINT;
    }
  }

  fputl(range.countKeys(), f);             // number of keys
  fputl(flags, f);                         // flags
  fputl(0, f);                             // 4 bytes reserved
  ase_file_write_string(f, slice->name()); // slice name

  frame_t frame = fromFrame;
  const SliceKey* oldKey = nullptr;
  for (auto key : range) {
    if (frame == fromFrame || key != oldKey) {
      fputl(frame, f);
      fputl((int32_t)(key ? key->bounds().x: 0), f);
      fputl((int32_t)(key ? key->bounds().y: 0), f);
      fputl(key ? key->bounds().w: 0, f);
      fputl(key ? key->bounds().h: 0, f);

      if (flags & ASE_SLICE_FLAG_HAS_CENTER_BOUNDS) {
        if (key && key->hasCenter()) {
          fputl((int32_t)key->center().x, f);
          fputl((int32_t)key->center().y, f);
          fputl(key->center().w, f);
          fputl(key->center().h, f);
        }
        else {
          fputl(0, f);
          fputl(0, f);
          fputl(0, f);
          fputl(0, f);
        }
      }

      if (flags & ASE_SLICE_FLAG_HAS_PIVOT_POINT) {
        if (key && key->hasPivot()) {
          fputl((int32_t)key->pivot().x, f);
          fputl((int32_t)key->pivot().y, f);
        }
        else {
          fputl(0, f);
          fputl(0, f);
        }
      }

      oldKey = key;
    }
    ++frame;
  }
}

static bool ase_has_groups(LayerGroup* group)
{
  for (Layer* child : group->layers()) {
    if (child->isGroup())
      return true;
  }
  return false;
}

static void ase_ungroup_all(LayerGroup* group)
{
  LayerGroup* root = group->sprite()->root();
  LayerList list = group->layers();

  for (Layer* child : list) {
    if (child->isGroup()) {
      ase_ungroup_all(static_cast<LayerGroup*>(child));
      group->removeLayer(child);
    }
    else if (group != root) {
      // Create a new name adding all group layer names
      {
        std::string name;
        for (Layer* layer=child; layer!=root; layer=layer->parent()) {
          if (!name.empty())
            name.insert(0, "-");
          name.insert(0, layer->name());
        }
        child->setName(name);
      }

      group->removeLayer(child);
      root->addLayer(child);
    }
  }

  if (group != root) {
    ASSERT(group->layersCount() == 0);
    delete group;
  }
}

} // namespace app
