// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/file/file.h"
#include "app/file/file_format.h"
#include "base/file_handle.h"
#include "doc/blend_mode.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "psd/psd.h"

namespace app {

doc::PixelFormat psd_cmode_to_ase_format(const psd::ColorMode mode)
{
  switch (mode) {
    case psd::ColorMode::Grayscale:
      return doc::PixelFormat::IMAGE_GRAYSCALE;
    case psd::ColorMode::RGB:
      return doc::PixelFormat::IMAGE_RGB;
    default:
      return doc::PixelFormat::IMAGE_INDEXED;
  }
}

doc::BlendMode psd_blendmode_to_ase(const psd::LayerBlendMode mode)
{
  switch (mode) {
    case psd::LayerBlendMode::Multiply:
      return doc::BlendMode::MULTIPLY;
    case psd::LayerBlendMode::Darken:
      return doc::BlendMode::DARKEN;
    case psd::LayerBlendMode::ColorBurn:
      return doc::BlendMode::COLOR_BURN;
    case psd::LayerBlendMode::Lighten:
      return doc::BlendMode::LIGHTEN;
    case psd::LayerBlendMode::Screen:
      return doc::BlendMode::SCREEN;
    case psd::LayerBlendMode::ColorDodge:
      return doc::BlendMode::COLOR_DODGE;
    case psd::LayerBlendMode::Overlay:
      return doc::BlendMode::OVERLAY;
    case psd::LayerBlendMode::SoftLight:
      return doc::BlendMode::SOFT_LIGHT;
    case psd::LayerBlendMode::HardLight:
      return doc::BlendMode::HARD_LIGHT;
    case psd::LayerBlendMode::Difference:
      return doc::BlendMode::DIFFERENCE;
    case psd::LayerBlendMode::Exclusion:
      return doc::BlendMode::EXCLUSION;
    case psd::LayerBlendMode::Subtract:
      return doc::BlendMode::SUBTRACT;
    case psd::LayerBlendMode::Divide:
      return doc::BlendMode::DIVIDE;
    case psd::LayerBlendMode::Hue:
      return doc::BlendMode::HSL_HUE;
    case psd::LayerBlendMode::Saturation:
      return doc::BlendMode::HSL_SATURATION;
    case psd::LayerBlendMode::Color:
      return doc::BlendMode::HSL_COLOR;
    case psd::LayerBlendMode::Luminosity:
      return doc::BlendMode::HSL_LUMINOSITY;
    case psd::LayerBlendMode::Normal:
    default:
      return doc::BlendMode::NORMAL;
  }
}

class PsdFormat : public FileFormat {
  const char* onGetName() const override { return "psd"; }

  void onGetExtensions(base::paths& exts) const override
  {
    exts.push_back("psb");
    exts.push_back("psd");
  }

  dio::FileFormat onGetDioFormat() const override
  {
    return dio::FileFormat::PSD_IMAGE;
  }

  int onGetFlags() const override { return FILE_SUPPORT_LOAD; }

  bool onLoad(FileOp* fop) override;
  bool onSave(FileOp* fop) override;
};

FileFormat* CreatePsdFormat()
{
  return new PsdFormat();
}

class PsdDecoderDelegate : public psd::DecoderDelegate {
public:
  PsdDecoderDelegate()
    : m_currentImage(nullptr)
    , m_currentLayer(nullptr)
    , m_sprite(nullptr)
    , m_pixelFormat(PixelFormat::IMAGE_INDEXED)
    , m_layerHasTransparentChannel(false)
  { }

  Sprite* getSprite() { return assembleDocument(); }

  void onFileHeader(const psd::FileHeader& header) override
  {
    m_pixelFormat = psd_cmode_to_ase_format(header.colorMode);
    m_sprite = new Sprite(
      ImageSpec(ColorMode(m_pixelFormat), header.width, header.width));
    m_sprite->setTotalFrames(frame_t(1));
    m_layerHasTransparentChannel = hasTransparency(header.nchannels);
  }

  // Emitted when a new layer has been chosen and its channel image data
  // is about to be read
  void onBeginLayer(const psd::LayerRecord& layerRecord) override
  {
    auto findIter = std::find_if(
      m_layers.begin(), m_layers.end(), [&layerRecord](doc::Layer* layer) {
        return layer->name() == layerRecord.name;
      });
    if (findIter == m_layers.end()) {
      createNewLayer(layerRecord.name);
      m_currentLayer->setVisible(layerRecord.isVisible());
      m_layerHasTransparentChannel =
        hasTransparency(layerRecord.channels.size());
    }
    else {
      m_currentLayer = *findIter;
      m_currentImage = m_currentLayer->cel(frame_t(0))->imageRef();
    }
  }

  void onEndLayer(const psd::LayerRecord& layerRecord) override
  {
    ASSERT(m_currentLayer);
    ASSERT(m_currentImage);

    m_currentImage.reset();
    m_currentLayer = nullptr;
    m_layerHasTransparentChannel = false;
  }

  // Emitted only if there's a palette in an image
  void onColorModeData(const psd::ColorModeData& colorModeD) override
  {
    if (!colorModeD.colors.empty()) {
      for (int i = 0; i < colorModeD.colors.size(); ++i) {
        const psd::IndexColor iColor = colorModeD.colors[i];
        m_palette.setEntry(i, rgba(iColor.r, iColor.g, iColor.b, 255));
      }
    }
  }

  // Emitted when an image data is about to be transmitted
  void onBeginImage(const psd::ImageData& imageData) override
  {
    if (!m_currentImage) {
      // only occurs where there's an image with no layer
      if (m_layers.empty()) {
        createNewLayer("Layer 1");
        m_layerHasTransparentChannel =
          hasTransparency(imageData.channels.size());
      }
      if (m_currentLayer) {
        createNewImage(imageData.width, imageData.height);
        linkNewCel(m_currentLayer, m_currentImage);
      }
    }
  }

  // Emitted when all layers and their masks have been processed
  void onLayersAndMask(const psd::LayersInformation& layersInfo) override
  {
    if (layersInfo.layers.size() == m_layers.size()) {
      for (int i = 0; i < m_layers.size(); ++i) {
        const psd::LayerRecord& layerRecord = layersInfo.layers[i];

        LayerImage* layer = static_cast<LayerImage*>(m_layers[i]);
        layer->setBlendMode(psd_blendmode_to_ase(layerRecord.blendMode));

        Cel* cel = layer->cel(frame_t(0));
        cel->setOpacity(layerRecord.opacity);
        cel->setPosition(gfx::Point(layerRecord.left, layerRecord.top));
      }
    }
  }

  // This method is called on images stored in RLE format
  void onImageScanline(const psd::ImageData& img,
                       const int y,
                       const psd::ChannelID chanID,
                       const uint8_t* data,
                       const int bytes) override
  {
    if (!m_currentImage || y >= m_currentImage->height())
      return;

    const int dataCount = bytes / (img.depth >= 8 ? (img.depth / 8) : 1);
    for (int x = 0; x < dataCount && x < m_currentImage->width(); ++x) {
      const color_t c = m_currentImage->getPixel(x, y);
      const uint32_t pixel = getValue(data, img.depth);
      putPixel(x, y, c, pixel, chanID, m_pixelFormat);
    }
  }

private:
  // TODO: This is meant to convert values of a channel based on its depth
  std::uint32_t normalizeValue(const uint32_t value, const int depth)
  {
    return value;
  }

  inline bool hasTransparency(const size_t nchannels)
  {
    // RGBA or grayscale image with alpha channel
    return nchannels == 4 || nchannels == 2;
  }

  void linkNewCel(Layer* layer, doc::ImageRef image)
  {
    std::unique_ptr<Cel> cel(new doc::Cel(frame_t(0), image));
    cel->setPosition(0, 0);
    static_cast<LayerImage*>(layer)->addCel(cel.release());
  }

  void createNewLayer(const std::string& layerName)
  {
    m_currentLayer = new LayerImage(m_sprite);
    m_sprite->root()->addLayer(m_currentLayer);
    m_layers.push_back(m_currentLayer);
    m_currentLayer->setName(layerName);
  }

  Sprite* assembleDocument()
  {
    if (m_palette.getModifications() > 1)
      m_sprite->setPalette(&m_palette, true);

    return m_sprite;
  }

  std::uint32_t getValue(const std::uint8_t*& data, const int depth)
  {
    uint32_t value = 0;
    switch (depth) {
      case 1:
      case 8:
        value = data[0];
        ++data;
        return value;
      case 16:
        value = int(data[0]) | int(data[1] << 8);
        data += 2;
        return value;
      case 32:
        value = int(data[0] << 24) | int(data[1] << 16) | int(data[2] << 8) |
                int(data[3]);
        data += 4;
        return value;
      default:
        throw std::runtime_error("invalid image depth");
    }
  }

  template<typename NewPixel>
  void putPixel(const int x,
                const int y,
                const color_t prevPixelValue,
                const NewPixel pixelValue,
                const psd::ChannelID chanID,
                const PixelFormat pixelFormat)
  {
    if (pixelFormat == doc::PixelFormat::IMAGE_RGB) {
      int r = rgba_getr(prevPixelValue);
      int g = rgba_getg(prevPixelValue);
      int b = rgba_getb(prevPixelValue);
      int a = m_layerHasTransparentChannel ? rgba_geta(prevPixelValue) : 255;

      if (chanID == psd::ChannelID::Red) {
        r = pixelValue;
      }
      else if (chanID == psd::ChannelID::Green) {
        g = pixelValue;
      }
      else if (chanID == psd::ChannelID::Blue) {
        b = pixelValue;
      }
      else if (chanID == psd::ChannelID::Alpha ||
               chanID == psd::ChannelID::TransparencyMask) {
        a = pixelValue;
      }
      m_currentImage->putPixel(x, y, rgba(r, g, b, a));
    }
    else if (pixelFormat == doc::PixelFormat::IMAGE_GRAYSCALE) {
      int v = graya_getv(prevPixelValue);
      int a = m_layerHasTransparentChannel ? graya_geta(prevPixelValue) : 255;
      if (chanID == psd::ChannelID::Red) {
        v = pixelValue;
      }
      else if (chanID == psd::ChannelID::Alpha ||
               chanID == psd::ChannelID::TransparencyMask) {
        a = pixelValue;
      }
      m_currentImage->putPixel(x, y, graya(v, a));
    }
    else if (pixelFormat == doc::PixelFormat::IMAGE_INDEXED) {
      m_currentImage->putPixel(x, y, (color_t)pixelValue);
    }
    else {
      throw std::runtime_error(
        "Only RGB/Grayscale/Indexed format is supported");
    }
  }

  void createNewImage(const int width, const int height)
  {
    if (width <= 0 || height <= 0)
      throw std::runtime_error("invalid image width/height");

    m_currentImage.reset(Image::create(m_pixelFormat, width, height));
    clear_image(m_currentImage.get(), 0);
  }

  doc::ImageRef m_currentImage;
  doc::Layer* m_currentLayer;
  Sprite* m_sprite;
  PixelFormat m_pixelFormat;
  std::vector<doc::Layer*> m_layers;
  Palette m_palette;
  bool m_layerHasTransparentChannel;
};

bool PsdFormat::onLoad(FileOp* fop)
{
  base::FileHandle fileHandle =
    base::open_file_with_exception(fop->filename(), "rb");
  FILE* f = fileHandle.get();
  psd::StdioFileInterface fileInterface(f);
  PsdDecoderDelegate pDelegate;
  psd::Decoder decoder(&fileInterface, &pDelegate);

  if (!decoder.readFileHeader()) {
    fop->setError("The file doesn't have a valid PSD header\n");
    return false;
  }

  const psd::FileHeader header = decoder.fileHeader();

  if (header.colorMode != psd::ColorMode::RGB &&
      header.colorMode != psd::ColorMode::Indexed &&
      header.colorMode != psd::ColorMode::Grayscale) {
    fop->setError("This preliminary work only supports "
                  "RGB, Grayscale & Indexed\n");
    return false;
  }

  // This would be removed when support for 32bit per channel is supported
  if (header.depth >= 32) {
    fop->setError("Support for 32bit per channel isn't supported yet");
    return false;
  }

  try {
    decoder.readColorModeData();
    decoder.readImageResources();
    decoder.readLayersAndMask();
    decoder.readImageData();
  }
  catch (const std::runtime_error& e) {
    fop->setError(e.what());
    return false;
  }

  fop->createDocument(pDelegate.getSprite());
  return true;
}

bool PsdFormat::onSave(FileOp* fop)
{
  return false;
}

}  // namespace app
