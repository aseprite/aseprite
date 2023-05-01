// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/read_document.h"

#include "app/console.h"
#include "app/crash/doc_format.h"
#include "app/crash/internals.h"
#include "app/crash/log.h"
#include "app/doc.h"
#include "base/convert_to.h"
#include "base/exception.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/serialization.h"
#include "base/string.h"
#include "doc/cel.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/cels_range.h"
#include "doc/frame.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/palette.h"
#include "doc/palette_io.h"
#include "doc/slice.h"
#include "doc/slice_io.h"
#include "doc/sprite.h"
#include "doc/string_io.h"
#include "doc/subobjects_io.h"
#include "doc/tag.h"
#include "doc/tag_io.h"
#include "doc/tileset.h"
#include "doc/tileset_io.h"
#include "doc/tilesets.h"
#include "doc/user_data_io.h"
#include "doc/util.h"
#include "fixmath/fixmath.h"

#include <fstream>
#include <map>

namespace app {
namespace crash {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

namespace {

class Reader : public SubObjectsIO {
public:
  Reader(const std::string& dir,
         base::task_token* t)
    : m_docFormatVer(DOC_FORMAT_VERSION_0)
    , m_sprite(nullptr)
    , m_dir(dir)
    , m_docId(0)
    , m_docVersions(nullptr)
    , m_loadInfo(nullptr)
    , m_taskToken(t) {
    for (const auto& fn : base::list_files(dir)) {
      auto i = fn.find('-');
      if (i == std::string::npos)
        continue;               // Has no ID

      auto j = fn.find('.', ++i);
      if (j == std::string::npos)
        continue;               // Has no version

      ObjectId id = base::convert_to<int>(fn.substr(i, j - i));
      ObjectVersion ver = base::convert_to<int>(fn.substr(j+1));
      if (!id || !ver)
        continue;               // Error converting strings to ID/ver

      ObjVersions& versions = m_objVersions[id];
      versions.add(ver);

      if (fn.compare(0, 3, "doc") == 0) {
        if (!m_docId)
          m_docId = id;
        else {
          ASSERT(m_docId == id);
        }

        m_docVersions = &versions;
      }
    }
  }

  Doc* loadDocument() {
    Doc* doc = loadObject<Doc*>("doc", m_docId, &Reader::readDocument);
    if (doc)
      fixUndetectedDocumentIssues(doc);
    else
      Console().printf("Error recovering the document\n");
    return doc;
  }

  bool loadDocumentInfo(DocumentInfo& info) {
    m_loadInfo = &info;
    return
      loadObject<Doc*>("doc", m_docId, &Reader::readDocument)
        == (Doc*)1;
  }

private:

  const ObjectVersion docId() const {
    return m_docId;
  }

  const ObjVersions* docVersions() const {
    return m_docVersions;
  }

  Sprite* loadSprite(ObjectId sprId) {
    return loadObject<Sprite*>("spr", sprId, &Reader::readSprite);
  }

  ImageRef getImageRef(ObjectId imageId) {
    if (m_images.find(imageId) != m_images.end())
      return m_images[imageId];

    ImageRef image(loadObject<Image*>("img", imageId, &Reader::readImage));
    return m_images[imageId] = image;
  }

  CelDataRef getCelDataRef(ObjectId celdataId) {
    if (m_celdatas.find(celdataId) != m_celdatas.end())
      return m_celdatas[celdataId];

    CelDataRef celData(loadObject<CelData*>("celdata", celdataId, &Reader::readCelData));
    return m_celdatas[celdataId] = celData;
  }

  template<typename T>
  T loadObject(const char* prefix, ObjectId id, T (Reader::*readMember)(std::ifstream&)) {
    const ObjVersions& versions = m_objVersions[id];

    for (size_t i=0; i<versions.size(); ++i) {
      ObjectVersion ver = versions[i];
      if (!ver)
        continue;

      RECO_TRACE("RECO: Restoring %s #%d v%d\n", prefix, id, ver);

      std::string fn = prefix;
      fn.push_back('-');
      fn += base::convert_to<std::string>(id);
      fn.push_back('.');
      fn += base::convert_to<std::string>(ver);

      std::ifstream s(FSTREAM_PATH(base::join_path(m_dir, fn)), std::ifstream::binary);
      T obj = nullptr;
      if (read32(s) == MAGIC_NUMBER)
        obj = (this->*readMember)(s);

      if (obj) {
        RECO_TRACE("RECO: %s #%d v%d restored successfully\n", prefix, id, ver);
        return obj;
      }
      else {
        RECO_TRACE("RECO: %s #%d v%d was not restored\n", prefix, id, ver);
      }
    }

    // Show error only if we've failed to load all versions
    if (!m_loadInfo)
      Console().printf("Error loading object %s #%d\n", prefix, id);

    return nullptr;
  }

  Doc* readDocument(std::ifstream& s) {
    ObjectId sprId = read32(s);
    std::string filename = read_string(s);
    m_docFormatVer = read16(s);
    if (s.eof()) m_docFormatVer = DOC_FORMAT_VERSION_0;

    RECO_TRACE("RECO: internal format version=%d\n", m_docFormatVer);

    // Load DocumentInfo only
    if (m_loadInfo) {
      m_loadInfo->filename = filename;
      return (Doc*)loadSprite(sprId);
    }

    Sprite* spr = loadSprite(sprId);
    if (spr) {
      Doc* doc = new Doc(spr);
      doc->setFilename(filename);
      doc->impossibleToBackToSavedState();
      return doc;
    }
    else {
      Console().printf("Unable to load sprite #%d\n", sprId);
      return nullptr;
    }
  }

  Sprite* readSprite(std::ifstream& s) {
    // Header
    ColorMode mode = (ColorMode)read8(s);
    int w = read16(s);
    int h = read16(s);
    color_t transparentColor = read32(s);
    frame_t nframes = read32(s);

    if (mode != ColorMode::RGB &&
        mode != ColorMode::INDEXED &&
        mode != ColorMode::GRAYSCALE) {
      if (!m_loadInfo)
        Console().printf("Invalid sprite color mode #%d\n", (int)mode);
      return nullptr;
    }

    if (w < 1 || h < 1 || w > 0xfffff || h > 0xfffff) {
      if (!m_loadInfo)
        Console().printf("Invalid sprite dimension %dx%d\n", w, h);
      return nullptr;
    }

    if (m_loadInfo) {
      m_loadInfo->mode = mode;
      m_loadInfo->width = w;
      m_loadInfo->height = h;
      m_loadInfo->frames = nframes;
      return (Sprite*)1;        // TODO improve this
    }

    std::unique_ptr<Sprite> spr(new Sprite(ImageSpec(mode, w, h), 256));
    m_sprite = spr.get();
    spr->setTransparentColor(transparentColor);

    if (nframes >= 1) {
      spr->setTotalFrames(nframes);
      for (frame_t fr=0; fr<nframes; ++fr) {
        int msecs = read32(s);
        spr->setFrameDuration(fr, msecs);
      }
    }
    else {
      Console().printf("Invalid number of frames #%d\n", nframes);
    }

    // IDs of all tilesets
    if (m_docFormatVer >= DOC_FORMAT_VERSION_1) {
      int ntilesets = read32(s);
      if (ntilesets > 0 && ntilesets < 0xffffff) {
        for (int i=0; i<ntilesets; ++i) {
          ObjectId tilesetId = read32(s);
          Tileset* tileset = loadObject<Tileset*>("tset", tilesetId, &Reader::readTileset);
          if (tileset)
            spr->tilesets()->add(tileset);
          else
            spr->tilesets()->add(nullptr);
        }
      }
    }

    // Read layers
    int nlayers = read32(s);
    if (nlayers >= 1 && nlayers < 0xfffff) {
      std::map<ObjectId, LayerGroup*> layersMap;
      layersMap[0] = spr->root(); // parentId = 0 is the root level

      for (int i=0; i<nlayers; ++i) {
        if (canceled())
          return nullptr;

        ObjectId layId = read32(s);
        ObjectId parentId = read32(s);

        if (!layersMap[parentId]) {
          Console().printf("Inexistent parent #%d for layer #%d", parentId, layId);
          // Put this layer at the root level
          parentId = 0;
        }

        Layer* lay = loadObject<Layer*>("lay", layId, &Reader::readLayer);
        if (lay) {
          if (lay->isGroup())
            layersMap[layId] = static_cast<LayerGroup*>(lay);

          layersMap[parentId]->addLayer(lay);
        }
      }
    }
    else {
      Console().printf("Invalid number of layers #%d\n", nlayers);
    }

    // Read all cels
    for (size_t i=0; i<m_celsToLoad.size(); ++i) {
      if (canceled())
        return nullptr;

      const auto& pair = m_celsToLoad[i];
      LayerImage* lay = doc::get<LayerImage>(pair.first);
      if (!lay)
        continue;

      ObjectId celId = pair.second;

      Cel* cel = loadObject<Cel*>("cel", celId, &Reader::readCel);
      if (cel) {
        // Expand sprite size
        if (cel->frame() > m_sprite->lastFrame())
          m_sprite->setTotalFrames(cel->frame()+1);

        lay->addCel(cel);
      }

      if (m_taskToken) {
        m_taskToken->set_progress(float(i) / float(m_celsToLoad.size()));
      }
    }

    // Read palettes
    int npalettes = read32(s);
    if (npalettes >= 1 && npalettes < 0xfffff) {
      for (int i = 0; i < npalettes; ++i) {
        if (canceled())
          return nullptr;

        ObjectId palId = read32(s);
        std::unique_ptr<Palette> pal(
          loadObject<Palette*>("pal", palId, &Reader::readPalette));
        if (pal)
          spr->setPalette(pal.get(), true);
      }
    }

    // Read frame tags
    int nfrtags = read32(s);
    if (nfrtags >= 1 && nfrtags < 0xfffff) {
      for (int i = 0; i < nfrtags; ++i) {
        if (canceled())
          return nullptr;

        ObjectId tagId = read32(s);
        Tag* tag = loadObject<Tag*>("frtag", tagId, &Reader::readTag);
        if (tag)
          spr->tags().add(tag);
      }
    }

    // Read slices
    int nslices = read32(s);
    if (nslices >= 1 && nslices < 0xffffff) {
      for (int i = 0; i < nslices; ++i) {
        if (canceled())
          return nullptr;

        ObjectId sliceId = read32(s);
        Slice* slice = loadObject<Slice*>("slice", sliceId, &Reader::readSlice);
        if (slice)
          spr->slices().add(slice);
      }
    }

    // Read color space
    if (!s.eof()) {
      gfx::ColorSpaceRef colorSpace = readColorSpace(s);
      if (colorSpace)
        spr->setColorSpace(colorSpace);
    }

    // Read grid bounds
    if (!s.eof()) {
      gfx::Rect gridBounds = readGridBounds(s);
      if (!gridBounds.isEmpty())
        spr->setGridBounds(gridBounds);
    }

    // Read Sprite User Data
    if (!s.eof()) {
      UserData userData = read_user_data(s, m_docFormatVer);
      if (!userData.isEmpty())
        spr->setUserData(userData);
    }

    return spr.release();
  }

  gfx::ColorSpaceRef readColorSpace(std::ifstream& s) {
    const gfx::ColorSpace::Type type = (gfx::ColorSpace::Type)read16(s);
    const gfx::ColorSpace::Flag flags = (gfx::ColorSpace::Flag)read16(s);
    const double gamma = fixmath::fixtof(read32(s));
    const size_t n = read32(s);

    // If the color space file is to big, it's because the sprite file
    // is invalid or or from an old session without color spcae.
    if (n > 1024*1024*64) // 64 MB is too much for an ICC file
      return nullptr;

    std::vector<uint8_t> buf(n);
    if (n)
      s.read((char*)&buf[0], n);
    std::string name = read_string(s);

    auto colorSpace = base::make_ref<gfx::ColorSpace>(
      type, flags, gamma, std::move(buf));
    colorSpace->setName(name);
    return colorSpace;
  }

  gfx::Rect readGridBounds(std::ifstream& s) {
    gfx::Rect grid;
    grid.x = (int16_t)read16(s);
    grid.y = (int16_t)read16(s);
    grid.w = read16(s);
    grid.h = read16(s);
    return grid;
  }

  // TODO could we use doc::read_layer() here?
  Layer* readLayer(std::ifstream& s) {
    LayerFlags flags = (LayerFlags)read32(s);
    ObjectType type = (ObjectType)read16(s);
    ASSERT(type == ObjectType::LayerImage ||
           type == ObjectType::LayerGroup ||
           type == ObjectType::LayerTilemap);

    std::string name = read_string(s);
    std::unique_ptr<Layer> lay;

    switch (type) {

      case ObjectType::LayerImage:
      case ObjectType::LayerTilemap: {
        switch (type) {
          case ObjectType::LayerImage:
            lay.reset(new LayerImage(m_sprite));
            break;
          case ObjectType::LayerTilemap: {
            tileset_index tilesetIndex = read32(s);
            lay.reset(new LayerTilemap(m_sprite, tilesetIndex));
            break;
          }
        }

        lay->setName(name);
        lay->setFlags(flags);

        // Blend mode & opacity
        static_cast<LayerImage*>(lay.get())->setBlendMode((BlendMode)read16(s));
        static_cast<LayerImage*>(lay.get())->setOpacity(read8(s));

        // Cels
        int ncels = read32(s);
        for (int i=0; i<ncels; ++i) {
          if (canceled())
            return nullptr;

          // Add a new cel to load in the future after we load all layers
          ObjectId celId = read32(s);
          m_celsToLoad.push_back(std::make_pair(lay->id(), celId));
        }
        break;
      }

      case ObjectType::LayerGroup:
        lay.reset(new LayerGroup(m_sprite));
        lay->setName(name);
        lay->setFlags(flags);
        break;

      default:
        Console().printf("Unable to load layer named '%s', type #%d\n",
                         name.c_str(), (int)type);
        break;
    }

    if (lay) {
      UserData userData = read_user_data(s, m_docFormatVer);
      lay->setUserData(userData);
      return lay.release();
    }
    else
      return nullptr;
  }

  Cel* readCel(std::ifstream& s) {
    return read_cel(s, this, false);
  }

  CelData* readCelData(std::ifstream& s) {
    return read_celdata(s, this, false, m_docFormatVer);
  }

  Image* readImage(std::ifstream& s) {
    return read_image(s, false);
  }

  Palette* readPalette(std::ifstream& s) {
    return read_palette(s);
  }

  Tileset* readTileset(std::ifstream& s) {
    uint32_t tilesetVer;
    Tileset* tileset = read_tileset(s, m_sprite, false, &tilesetVer, m_docFormatVer);
    if (tileset && tilesetVer < TILESET_VER1)
      m_updateOldTilemapWithTileset.insert(tileset->id());
    return tileset;
  }

  Tag* readTag(std::ifstream& s) {
    return read_tag(s, false, m_docFormatVer);
  }

  Slice* readSlice(std::ifstream& s) {
    return read_slice(s, false);
  }

  // Fix issues that the restoration process could produce.
  void fixUndetectedDocumentIssues(Doc* doc) {
    Sprite* spr = doc->sprite();
    ASSERT(spr);
    if (!spr)
      return;                   // TODO create an empty sprite

    // Fill the background layer with empty cels if they are missing
    if (LayerImage* bg = spr->backgroundLayer()) {
      for (frame_t fr=0; fr<spr->totalFrames(); ++fr) {
        Cel* cel = bg->cel(fr);
        if (!cel) {
          ImageRef image(Image::create(spr->pixelFormat(),
                                       spr->width(),
                                       spr->height()));
          image->clear(spr->transparentColor());
          cel = new Cel(fr, image);
          bg->addCel(cel);
        }
      }
    }

    // Fix tilemaps using old tilesets
    if (!m_updateOldTilemapWithTileset.empty()) {
      for (Tileset* tileset : *spr->tilesets()) {
        if (!tileset)
          continue;

        if (m_updateOldTilemapWithTileset.find(tileset->id()) == m_updateOldTilemapWithTileset.end())
          continue;

        for (Cel* cel : spr->uniqueCels()) {
          if (cel->image()->pixelFormat() == IMAGE_TILEMAP &&
              static_cast<LayerTilemap*>(cel->layer())->tileset() == tileset) {
            doc::fix_old_tilemap(cel->image(), tileset,
                                 tile_i_mask, tile_f_mask);
          }
        }
      }
    }
  }

  bool canceled() const {
    if (m_taskToken)
      return m_taskToken->canceled();
    else
      return false;
  }

  int m_docFormatVer;
  Sprite* m_sprite;    // Used to pass the sprite in LayerImage() ctor
  std::string m_dir;
  ObjectVersion m_docId;
  ObjVersionsMap m_objVersions;
  ObjVersions* m_docVersions;
  DocumentInfo* m_loadInfo;
  std::vector<std::pair<ObjectId, ObjectId> > m_celsToLoad;
  std::map<ObjectId, ImageRef> m_images;
  std::map<ObjectId, CelDataRef> m_celdatas;
  // Each ObjectId is a tileset ID that didn't contain the empty tile
  // as the first tile (this was an old format used in internal betas)
  std::set<ObjectId> m_updateOldTilemapWithTileset;
  base::task_token* m_taskToken;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

bool read_document_info(const std::string& dir, DocumentInfo& info)
{
  return Reader(dir, nullptr).loadDocumentInfo(info);
}

Doc* read_document(const std::string& dir,
                   base::task_token* t)
{
  return Reader(dir, t).loadDocument();
}

Doc* read_document_with_raw_images(const std::string& dir,
                                   RawImagesAs as,
                                   base::task_token* t)
{
  Reader reader(dir, t);

  DocumentInfo info;
  if (!reader.loadDocumentInfo(info)) {
    info.mode = ColorMode::RGB;
    info.width = 256;
    info.height = 256;
    info.filename = "Unknown";
  }
  info.width = std::clamp(info.width, 1, 99999);
  info.height = std::clamp(info.height, 1, 99999);
  Sprite* spr = new Sprite(ImageSpec(info.mode, info.width, info.height), 256);

  // Load each image as a new frame
  auto lay = new LayerImage(spr);
  spr->root()->addLayer(lay);

  int i = 0;
  frame_t frame = 0;
  auto fns = base::list_files(dir);
  for (const auto& fn : fns) {
    if (t)
      t->set_progress((i++) / fns.size());

    if (fn.compare(0, 3, "img") != 0)
      continue;

    std::ifstream s(FSTREAM_PATH(base::join_path(dir, fn)), std::ifstream::binary);
    if (!s)
      continue;

    ImageRef img;
    if (read32(s) == MAGIC_NUMBER)
      img.reset(read_image(s, false));

    if (img) {
      lay->addCel(new Cel(frame, img));
    }

    switch (as) {
      case RawImagesAs::kFrames:
        ++frame;
        break;
      case RawImagesAs::kLayers:
        lay = new LayerImage(spr);
        spr->root()->addLayer(lay);
        break;
    }
  }
  if (as == RawImagesAs::kFrames) {
    if (frame > 1)
      spr->setTotalFrames(frame);
  }

  Doc* doc = new Doc(spr);
  doc->setFilename(info.filename);
  doc->impossibleToBackToSavedState();
  return doc;
}

} // namespace crash
} // namespace app
