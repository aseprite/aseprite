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
  PsdDecoderDelegate(FileOp* fop)
    : m_fop(fop)
    , m_currentLayer(nullptr)
    , m_currentImage(nullptr)
    , parentSprite(nullptr)
  { }

  Sprite* getSprite() { return assembleDocument(); }

  void onFileHeader(const psd::FileHeader& header) override
  {
    m_pixelFormat = psd_cmode_to_ase_format(header.colorMode);
    parentSprite = new Sprite(
      ImageSpec(ColorMode(m_pixelFormat), header.width, header.width), 256);
  }

  void onLayerSelected(const psd::LayerRecord& layerRecord) override
  {
    auto findIter = std::find_if(
      layers.begin(), layers.end(), [&layerRecord](doc::Layer* layer) {
        return layer->name() == layerRecord.name;
      });

    if (findIter == layers.end()) {
      if (m_currentLayer) {
        m_currentLayer = new LayerImage(parentSprite);
        m_currentLayer->setName(layerRecord.name);
        parentSprite->root()->addLayer(m_currentLayer);
        layers.push_back(m_currentLayer);
        m_currentImage = nullptr;
      }
      else {
        createSequenceImage(layerRecord.width(), layerRecord.height());
        m_currentLayer = new LayerImage(parentSprite);
        m_currentLayer->setName(layerRecord.name);
        parentSprite->root()->addLayer(m_currentLayer);
        static_cast<LayerImage*>(m_currentLayer)
            ->addCel(new Cel(frame_t(0), ImageRef(m_currentImage)));
        layers.push_back(m_currentLayer);
      }
    }
    else {
      m_currentLayer = *findIter;
      m_currentImage = m_currentLayer->cel(frame_t(0))->image();
    }
  }

  void onColorModeData(const psd::ColorModeData& colorModeD) override
  {
    if (!colorModeD.colors.empty()) {
      const uint8_t alpha = 255;
      if (m_pixelFormat == PixelFormat::IMAGE_INDEXED) {
        for (int i = 0; i < colorModeD.colors.size(); ++i) {
          const psd::IndexColor iColor = colorModeD.colors[i];
          const color_t color = rgba(iColor.r, iColor.g, iColor.b, alpha);
          m_palette.setEntry(i, color);
        }
      }
      else {
        for (int i = 0; i < 255; ++i) {
          const color_t color = rgba(i, i, i, alpha);
          m_palette.setEntry(i, color);
        }
      }
    }
  }

  void onBeginImage(const psd::ImageData& imageData) override
  {
    // just a simple image with no layer
    if (layers.empty() && !m_currentImage) {
      createSequenceImage(imageData.width, imageData.height);
      m_currentLayer = new LayerImage(parentSprite);
      parentSprite->root()->addLayer(m_currentLayer);
      static_cast<LayerImage*>(m_currentLayer)
        ->addCel(new Cel(frame_t(0), ImageRef(m_currentImage)));
      layers.push_back(m_currentLayer);
    }

    else if (m_currentLayer && !m_currentImage) {
      LayerImage* layerImage = static_cast<LayerImage*>(m_currentLayer);
      layerImage->addCel(
        new doc::Cel(frame_t(0),
                     doc::ImageRef(doc::Image::create(
                       m_pixelFormat, imageData.width, imageData.height))));
      m_currentImage = m_currentLayer->cel(0)->image();
    }
  }

  // this is called on images stored in raw data
  void onImageScanline(const psd::ImageData& imgData,
                       const int y,
                       const psd::ChannelID chanID,
                       const std::vector<uint32_t>& data)
  {
    ASSERT(m_currentImage);
    ASSERT(m_currentImage->width() == data.size());

    for (int x = 0; x < m_currentImage->width(); ++x) {
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
                       const int bytes)
  {
    ASSERT(m_currentImage);
    if (!m_currentImage) {
      m_fop->setError("there was a problem on the image scanline");
      throw std::runtime_error("there was a problem in RLE image scanline");
    }

    if (y >= m_currentImage->height())
      return;

    // uint32_t* dst_address = (uint32_t*)m_currentImage->getPixelAddress(0, y);
    for (int x = 0; x < m_currentImage->width() && x < bytes; ++x) {
      const color_t c = m_currentImage->getPixel(x, y);
      putPixel(x, y, c, normalizeValue(data[x], img.depth), chanID, m_pixelFormat);
    }
  }

private:
  // TODO: this is a sham, don't know what to do
  std::uint8_t normalizeValue(const uint32_t value, const int depth)
  {
    if (depth <= 8) {
      return static_cast<uint8_t>(value);
    }
    else if (depth == 16) {
      if (value > std::numeric_limits<uint8_t>::max()) {
        return ((value >> 0) & 0xff);
      }
      else
        return static_cast<uint8_t>(value);
    }
    else if (depth == 32) {
      if (value <= std::numeric_limits<uint8_t>::max()) {
        return 1;
      }
    }
    return 0;
  }

  Sprite* assembleDocument()
  {
    if (m_pixelFormat == doc::PixelFormat::IMAGE_INDEXED ||
        m_pixelFormat == doc::PixelFormat::IMAGE_GRAYSCALE) {
      parentSprite->setPalette(&m_palette, true);
    }
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
      } else if(chanID == psd::ChannelID::Alpha){
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

  void createSequenceImage(const int width, const int height)
  {
    m_currentImage = Image::create(m_pixelFormat, width, height);
    clear_image(m_currentImage, 0);
  }

  FileOp* m_fop;
  doc::Layer* m_currentLayer;
  doc::Image* m_currentImage;
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
  PsdDecoderDelegate pDelegate(fop);
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
