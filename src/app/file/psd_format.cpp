// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
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
    : m_currentLayer(nullptr)
    , m_currentImage(nullptr)
#ifdef _DEBUG
    , m_prevImage(nullptr)
#endif  // _DEBUG
    , parentSprite(nullptr)
    , m_pixelFormat(PixelFormat::IMAGE_INDEXED)
  { }

  Sprite* getSprite() { return assembleDocument(); }

  void onFileHeader(const psd::FileHeader& header) override
  {
    m_pixelFormat = psd_cmode_to_ase_format(header.colorMode);
    parentSprite = new Sprite(
      ImageSpec(ColorMode(m_pixelFormat), header.width, header.width));
    parentSprite->setTotalFrames(frame_t(1));
  }

  void onLayerSelected(const psd::LayerRecord& layerRecord) override
  {
    auto findIter = std::find_if(
      layers.begin(), layers.end(), [&layerRecord](doc::Layer* layer) {
        return layer->name() == layerRecord.name;
      });

    if (findIter == layers.end()) {
      m_currentImage.reset();
      createNewLayer(layerRecord.name);
    }
    else {
      m_currentLayer = *findIter;
      m_currentImage = m_currentLayer->cel(frame_t(0))->imageRef();
    }
  }

  void onColorModeData(const psd::ColorModeData& colorModeD) override
  {
    if (!colorModeD.colors.empty()) {
      for (int i = 0; i < colorModeD.colors.size(); ++i) {
        const psd::IndexColor iColor = colorModeD.colors[i];
        m_palette.setEntry(i, rgba(iColor.r, iColor.g, iColor.b, 255));
      }
    }
  }

  void onBeginImage(const psd::ImageData& imageData) override
  {
    // only occurs where there's an image with no layer
    if (layers.empty() && !m_currentImage) {
      createNewImage();
      createNewLayer("Layer Y");
      linkNewCel(m_currentLayer, m_currentImage);
    }

    else if (m_currentLayer && !m_currentImage) {
      createNewImage();
      linkNewCel(m_currentLayer, m_currentImage);
    }
  }

  // emitted when a new layer's image is about to be worked on
  void onLayersAndMask(const psd::LayersInformation& layersInfo) override
  {
    if (layersInfo.layers.size() == layers.size()) {
      const std::vector<uint8_t>& globalMasks = layersInfo.globalMaskData;
      for (int i = 0; i < layers.size(); ++i) {
        const psd::LayerRecord& layerRecord = layersInfo.layers[i];

        LayerImage* layer = static_cast<LayerImage*>(layers[i]);
        layer->setBlendMode(psd_blendmode_to_ase(layerRecord.blendMode));

        Cel* cel = layer->cel(frame_t(0));
        cel->setOpacity(layerRecord.opacity);
        // cel->setPosition(gfx::Point(layerRecord.left, layerRecord.top));
        if (globalMasks.size() > i)
          cel->image()->setMaskColor(globalMasks[i]);
      }
    }
  }
  // this is called on images stored in raw data
  void onImageScanline(const psd::ImageData& imgData,
                       const int y,
                       const psd::ChannelID chanID,
                       const std::vector<uint32_t>& data) override
  {
    ASSERT(m_currentImage);
    ASSERT(m_currentImage->width() >= data.size());

    for (int x = 0; x < data.size(); ++x) {
      const color_t c = m_currentImage->getPixel(x, y);
      putPixel(
        x, y, c, normalizeValue(data[x], imgData.depth), chanID, m_pixelFormat);
    }
  }

  // this method is called on images stored in RLE format
  void onImageScanline(const psd::ImageData& img,
                       const int y,
                       const psd::ChannelID chanID,
                       const uint8_t* data,
                       const int bytes) override
  {
    ASSERT(m_currentImage);
    if (!m_currentImage) {
      throw std::runtime_error("there was a problem in RLE image scanline");
    }

    if (y >= m_currentImage->height())
      return;

    // uint32_t* dst_address = (uint32_t*)m_currentImage->getPixelAddress(0, y);
    for (int x = 0; x < m_currentImage->width(); ++x) {
      const color_t c = m_currentImage->getPixel(x, y);
      putPixel(
        x, y, c, normalizeValue(data[x], img.depth), chanID, m_pixelFormat);
    }
  }

private:
  // TODO: this is meant to convert values of a channel based on its depth 
  std::uint32_t normalizeValue(const uint32_t value, const int depth)
  {
    return value;
  }

  void linkNewCel(Layer* layer, doc::ImageRef image)
  {
    std::unique_ptr<Cel> cel(new doc::Cel(frame_t(0), image));
    cel->setPosition(0, 0);
    static_cast<LayerImage*>(layer)->addCel(cel.release());
  }

  void createNewLayer(const std::string& layerName)
  {
    m_currentLayer = new LayerImage(parentSprite);
    parentSprite->root()->addLayer(m_currentLayer);
    layers.push_back(m_currentLayer);
    m_currentLayer->setName(layerName);
  }

  Sprite* assembleDocument()
  {
    if (m_pixelFormat == doc::PixelFormat::IMAGE_INDEXED ||
        m_pixelFormat == doc::PixelFormat::IMAGE_GRAYSCALE) {
      parentSprite->setPalette(&m_palette, true);
    }
#ifdef _DEBUG
    ASSERT(layers.size() >= 1);
    for (int i = 0; i < layers.size(); ++i) {
      LayerImage* currLayer = static_cast<LayerImage*>(layers[i]);
      ASSERT(currLayer);
      ASSERT(currLayer->getCelsCount() >= 1);
      ASSERT(currLayer->cel(frame_t(0))->data());
    }
#endif  // _DEBUG

    return parentSprite;
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
      int a = rgba_geta(prevPixelValue);
      if (chanID == psd::ChannelID::Red) {
        r = pixelValue;
      }
      else if (chanID == psd::ChannelID::Green) {
        g = pixelValue;
      }
      else if (chanID == psd::ChannelID::Blue) {
        b = pixelValue;
      }
      else if (chanID == psd::ChannelID::Alpha) {
        a = pixelValue;
      }
      m_currentImage->putPixel(x, y, rgba(r, g, b, a));
    }
    else if (pixelFormat == doc::PixelFormat::IMAGE_GRAYSCALE) {
      int a = graya_geta(prevPixelValue);
      int v = graya_getv(prevPixelValue);
      if (chanID == psd::ChannelID::TransparencyMask) {
        a = pixelValue;
      }
      else if (chanID == psd::ChannelID::Red) {
        v = pixelValue;
      }
      else {
        ASSERT(false);
        throw std::runtime_error("Invalid channel ID encountered");
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

  void createNewImage()
  {
    m_currentImage.reset(Image::create(
      m_pixelFormat, parentSprite->width(), parentSprite->height()));
    clear_image(m_currentImage.get(), 0);
#ifdef _DEBUG
    if (!m_prevImage) {
      m_prevImage = m_currentImage;
      return;
    }
    ASSERT(m_currentImage != m_prevImage);
    m_prevImage = m_currentImage;
#endif  // _DEBUG
  }

  doc::Layer* m_currentLayer;
  doc::ImageRef m_currentImage;
#ifdef _DEBUG
  doc::ImageRef m_prevImage;
#endif  // _DEBUG

  Sprite* parentSprite;
  PixelFormat m_pixelFormat;
  std::vector<doc::Layer*> layers;
  Palette m_palette;
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
    fop->setError("this preliminary work only supports "
                  "RGB, Grayscale && Indexed\n");
    return false;
  }

  //this would be removed when support for 32bit per channel is supported
  if (header.depth >= 32) {
    fop->setError("support for 32bit per channel isn't supported yet");
    return false;
  }

  if (!decoder.readColorModeData()) {
    fop->setError("unable to read color mode of the file\n");
    return false;
  }

  if (!decoder.readImageResources()) {
    fop->setError("unable to read the image resources\n");
    return false;
  }

  if (!decoder.readLayersAndMask()) {
    fop->setError("there was a problem reading the "
                  "layers information\n");
    return false;
  }

  if (!decoder.readImageData()) {
    fop->setError("unable to decode the image data section\n");
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
