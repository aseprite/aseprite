// Aseprite
// Copyright (C) 2018-2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/console.h"
#include "app/context.h"
#include "app/file/file.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "dio/aseprite_common.h"
#include "dio/aseprite_decoder.h"
#include "dio/aseprite_encoder.h"
#include "dio/decode_delegate.h"
#include "dio/encode_delegate.h"
#include "dio/file_interface.h"
#include "doc/doc.h"
#include "fmt/format.h"
#include "ui/alert.h"
#include "ver/info.h"

namespace app {

using namespace base;

namespace {

class DecodeDelegate : public dio::DecodeDelegate {
public:
  DecodeDelegate(FileOp* fop) : m_fop(fop), m_sprite(nullptr) {}
  ~DecodeDelegate() {}

  void error(const std::string& msg) override { m_fop->setError(msg.c_str()); }

  void incompatibilityError(const std::string& msg) override
  {
    m_fop->setIncompatibilityError(msg);
  }

  void progress(double fromZeroToOne) override { m_fop->setProgress(fromZeroToOne); }

  bool isCanceled() override { return m_fop->isStop(); }

  bool decodeOneFrame() override { return m_fop->isOneFrame(); }

  doc::color_t defaultSliceColor() override
  {
    auto color = m_fop->config().defaultSliceColor;
    return doc::rgba(color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
  }

  void onSprite(doc::Sprite* sprite) override { m_sprite = sprite; }

  doc::Sprite* sprite() { return m_sprite; }

  bool cacheCompressedTilesets() const override { return m_fop->config().cacheCompressedTilesets; }

private:
  FileOp* m_fop;
  doc::Sprite* m_sprite;
};

} // anonymous namespace

static bool ase_has_groups(LayerGroup* group);
static void ase_ungroup_all(LayerGroup* group);

class AseFormat : public FileFormat {
public:
  class AsepriteOptions : public FormatOptions {
  public:
    AsepriteOptions() : celType(ASE_FILE_COMPRESSED_CEL) {}

    int celType;
  };

private:
  const char* onGetName() const override { return "ase"; }

  void onGetExtensions(base::paths& exts) const override
  {
    exts.push_back("ase");
    exts.push_back("aseprite");
  }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::ASE_ANIMATION; }

  int onGetFlags() const override
  {
    return FILE_SUPPORT_LOAD | FILE_SUPPORT_SAVE | FILE_SUPPORT_RGB | FILE_SUPPORT_RGBA |
           FILE_SUPPORT_GRAY | FILE_SUPPORT_GRAYA | FILE_SUPPORT_INDEXED | FILE_SUPPORT_LAYERS |
           FILE_SUPPORT_FRAMES | FILE_SUPPORT_PALETTES | FILE_SUPPORT_TAGS |
           FILE_SUPPORT_BIG_PALETTES | FILE_SUPPORT_PALETTE_WITH_ALPHA |
           FILE_SUPPORT_GET_FORMAT_OPTIONS;
  }

  bool onLoad(FileOp* fop) override;
  bool onPostLoad(FileOp* fop) override;
#ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override;
#endif
  FormatOptionsPtr onAskUserForFormatOptions(FileOp* fop) override;
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

  // Assign RgbMap
  if (sprite->pixelFormat() == IMAGE_INDEXED)
    sprite->rgbMap(0,
                   Sprite::RgbMapFor(sprite->isOpaque()),
                   fop->config().rgbMapAlgorithm,
                   fop->config().fitCriteria);

  fop->createDocument(sprite);

  if (sprite->colorSpace() != nullptr && sprite->colorSpace()->type() != gfx::ColorSpace::None) {
    fop->setEmbeddedColorProfile();
  }

  // Sprite grid bounds will be set to empty (instead of
  // doc::Sprite::DefaultGridBounds()) if the file doesn't contain an
  // embedded grid bounds.
  if (!sprite->gridBounds().isEmpty())
    fop->setEmbeddedGridBounds();

  // Setup the file-data.
  if (!fop->formatOptions()) {
    auto aseOptions = std::make_shared<AsepriteOptions>();
    aseOptions->celType = decoder.celType();
    fop->setLoadedFormatOptions(aseOptions);
  }

  return true;
}

bool AseFormat::onPostLoad(FileOp* fop)
{
  LayerGroup* group = fop->document()->sprite()->root();

  // Forward Compatibility: In 1.1 we convert a file with layer groups
  // (saved with 1.2) as top level layers
  std::string ver = get_app_version();
  bool flat = (ver[0] == '1' && ver[1] == '.' && ver[2] == '1');
  if (flat && ase_has_groups(group)) {
    if (fop->context() && fop->context()->isUIAvailable() &&
        ui::Alert::show(fmt::format(
          // This message is not translated because is used only in the old v1.1 only
          "Warning"
          "<<The selected file \"{0}\" has layer groups."
          "<<Do you want to open it with \"{1} {2}\" anyway?"
          "<<"
          "<<Note: Layers inside groups will be converted to top level layers."
          "||&Yes||&No",
          base::get_file_name(fop->filename()),
          get_app_name(),
          ver)) != 1) {
      return false;
    }
    ase_ungroup_all(group);
  }
  return true;
}

#ifdef ENABLE_SAVE

namespace {

class EncodeDelegate : public dio::EncodeDelegate {
public:
  EncodeDelegate(FileOp* fop) : m_fop(fop) {}
  ~EncodeDelegate() {}

  void error(const std::string& msg) override { m_fop->setError(msg.c_str()); }
  void progress(double fromZeroToOne) override { m_fop->setProgress(fromZeroToOne); }
  bool isCanceled() override { return m_fop->isStop(); }

  doc::Sprite* sprite() override { return m_fop->document()->sprite(); }

  const doc::FramesSequence& framesSequence() const override
  {
    return m_fop->roi().framesSequence();
  }

  bool composeGroups() override { return m_fop->config().composeGroups; }
  bool preserveColorProfile() override { return m_fop->preserveColorProfile(); }
  bool cacheCompressedTilesets() override { return m_fop->config().cacheCompressedTilesets; }

  int preferredCelType() override
  {
    if (const auto aseOptions = std::static_pointer_cast<AseFormat::AsepriteOptions>(
          m_fop->formatOptions()))
      return aseOptions->celType;
    return ASE_FILE_COMPRESSED_CEL;
  }

  int preferredTilemapCelType() override { return ASE_FILE_COMPRESSED_TILEMAP; }

private:
  FileOp* m_fop;
};

} // namespace

bool AseFormat::onSave(FileOp* fop)
{
  FileHandle handle(open_file_with_exception_sync_on_close(fop->filename(), "wb"));
  dio::StdioFileInterface fileInterface(handle.get());

  EncodeDelegate delegate(fop);
  dio::AsepriteEncoder encoder;
  encoder.initialize(&delegate, &fileInterface);

  return encoder.encode();
}

#endif // ENABLE_SAVE

FormatOptionsPtr AseFormat::onAskUserForFormatOptions(FileOp* fop)
{
  auto opts = fop->formatOptionsOfDocument<AsepriteOptions>();
  if (fop->context() && fop->context()->isUIAvailable()) {
    try {
      auto& pref = Preferences::instance();
      if (pref.isSet(pref.asepriteFormat.celFormat)) {
        switch (pref.asepriteFormat.celFormat()) {
          case app::gen::CelContentFormat::COMPRESSED:
            opts->celType = ASE_FILE_COMPRESSED_CEL;
            break;
          case app::gen::CelContentFormat::RAW_IMAGE: opts->celType = ASE_FILE_RAW_CEL; break;
        }
      }
    }
    catch (std::exception& e) {
      Console::showException(e);
      return std::shared_ptr<AsepriteOptions>(nullptr);
    }
  }
  return opts;
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
      auto* childGroup = static_cast<LayerGroup*>(child);
      ase_ungroup_all(childGroup);
      group->removeLayer(childGroup);

      ASSERT(childGroup->layersCount() == 0);
      delete childGroup;
    }
    else if (group != root) {
      // Create a new name adding all group layer names
      {
        std::string name;
        for (Layer* layer = child; layer != root; layer = layer->parent()) {
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
}

} // namespace app
