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
#include "doc/slice.h"
#include "doc/sprite.h"
#include "psd/psd.h"

namespace app {

doc::PixelFormat psd_cmode_to_ase_format(const psd::ColorMode mode)
{
  switch (mode) {
    case psd::ColorMode::Grayscale: return doc::PixelFormat::IMAGE_GRAYSCALE;
    case psd::ColorMode::RGB:       return doc::PixelFormat::IMAGE_RGB;
    default:                        return doc::PixelFormat::IMAGE_INDEXED;
  }
}

doc::BlendMode psd_blendmode_to_ase(const psd::LayerBlendMode mode)
{
  switch (mode) {
    case psd::LayerBlendMode::Multiply:   return doc::BlendMode::MULTIPLY;
    case psd::LayerBlendMode::Darken:     return doc::BlendMode::DARKEN;
    case psd::LayerBlendMode::ColorBurn:  return doc::BlendMode::COLOR_BURN;
    case psd::LayerBlendMode::Lighten:    return doc::BlendMode::LIGHTEN;
    case psd::LayerBlendMode::Screen:     return doc::BlendMode::SCREEN;
    case psd::LayerBlendMode::ColorDodge: return doc::BlendMode::COLOR_DODGE;
    case psd::LayerBlendMode::Overlay:    return doc::BlendMode::OVERLAY;
    case psd::LayerBlendMode::SoftLight:  return doc::BlendMode::SOFT_LIGHT;
    case psd::LayerBlendMode::HardLight:  return doc::BlendMode::HARD_LIGHT;
    case psd::LayerBlendMode::Difference: return doc::BlendMode::DIFFERENCE;
    case psd::LayerBlendMode::Exclusion:  return doc::BlendMode::EXCLUSION;
    case psd::LayerBlendMode::Subtract:   return doc::BlendMode::SUBTRACT;
    case psd::LayerBlendMode::Divide:     return doc::BlendMode::DIVIDE;
    case psd::LayerBlendMode::Hue:        return doc::BlendMode::HSL_HUE;
    case psd::LayerBlendMode::Saturation: return doc::BlendMode::HSL_SATURATION;
    case psd::LayerBlendMode::Color:      return doc::BlendMode::HSL_COLOR;
    case psd::LayerBlendMode::Luminosity: return doc::BlendMode::HSL_LUMINOSITY;
    case psd::LayerBlendMode::Normal:
    default:                              return doc::BlendMode::NORMAL;
  }
}

class PsdFormat : public FileFormat {
  const char* onGetName() const override { return "psd"; }

  void onGetExtensions(base::paths& exts) const override
  {
    exts.push_back("psb");
    exts.push_back("psd");
  }

  dio::FileFormat onGetDioFormat() const override { return dio::FileFormat::PSD_IMAGE; }

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
    , m_layerGroup(nullptr)
    , m_sprite(nullptr)
    , m_activeFrameIndex(0)
    , m_pixelFormat(PixelFormat::IMAGE_INDEXED)
    , m_layerHasTransparentChannel(false)
  {
  }

  Sprite* getSprite() { return assembleDocument(); }

  void onFileHeader(const psd::FileHeader& header) override
  {
    m_pixelFormat = psd_cmode_to_ase_format(header.colorMode);
    m_sprite = new Sprite(ImageSpec(ColorMode(m_pixelFormat), header.width, header.width));
    m_layerHasTransparentChannel = hasTransparency(header.nchannels);
  }

  void onSlicesData(const psd::Slices& slices) override
  {
    auto& spriteSlices = m_sprite->slices();
    for (const auto& slice : slices.slices) {
      const gfx::Rect rect(slice.bound.left,
                           slice.bound.top,
                           slice.bound.right - slice.bound.left,
                           slice.bound.bottom - slice.bound.top);
      if (!rect.isEmpty() && slice.groupID > 0) {
        auto slice = new doc::Slice;
        slice->insert(0, doc::SliceKey(rect));

        // TO-DO: the slices color should come from the app preference
        slice->userData().setColor(doc::rgba(0, 0, 255, 255));
        spriteSlices.add(slice);
      }
    }
  }

  void onFramesData(const std::vector<psd::FrameInformation>& frameInfo,
                    const uint32_t activeFrameIndex) override
  {
    m_framesInfo = frameInfo;
    m_activeFrameIndex = activeFrameIndex;
    if (frameInfo.empty()) {
      m_sprite->setTotalFrames(frame_t(1));
      psd::FrameInformation frameInfo;
      frameInfo.id = 1;
      frameInfo.duration = 10;
      frameInfo.ga = 0;
      m_framesInfo.push_back(std::move(frameInfo));
    }
    else
      m_sprite->setTotalFrames(frame_t(frameInfo.size()));
  }

  // Emitted when a new layer has been chosen and its channel image data
  // is about to be read
  void onBeginLayer(const psd::LayerRecord& layerRecord) override
  {
    if (layerRecord.isOpenGroup()) {
      LayerGroup* layerGroup = new LayerGroup(m_sprite);
      if (m_groups.empty())
        m_sprite->root()->addLayer(layerGroup);
      else
        m_layerGroup->addLayer(layerGroup);

      m_layerGroup = layerGroup;
      m_groups.push_back(layerGroup);
    }
    else if (layerRecord.isCloseGroup()) {
      if (!m_layerGroup)
        throw std::runtime_error("unexpected end of a group layer");

      m_layerGroup->setName(layerRecord.name);
      if (!m_groups.empty())
        m_groups.pop_back();

      if (m_groups.empty())
        m_layerGroup = m_sprite->root();
      else
        m_layerGroup = m_groups.back();
    }
    else {
      auto findIter = std::find_if(
        m_layers.begin(),
        m_layers.end(),
        [&layerRecord](doc::Layer* layer) { return layer->name() == layerRecord.name; });
      if (findIter == m_layers.end()) {
        if (!m_layerGroup) // In this case, there are no layer groups
          m_layerGroup = m_sprite->root();

        createNewLayer(layerRecord.name);
        // m_currentLayer->setVisible(layerRecord.isVisible());
        m_layerHasTransparentChannel = hasTransparency(layerRecord.channels.size());
      }
      else {
        m_currentLayer = *findIter;
        m_currentImage = m_currentLayer->cel(frame_t(0))->imageRef();
      }
    }
  }

  void onEndLayer(const psd::LayerRecord& layerRecord) override
  {
    if (!m_framesInfo.empty() && (layerRecord.inFrames.size() == m_framesInfo.size()) &&
        m_currentImage) {
      std::unique_ptr<Cel> layerCel(m_currentLayer->cel(frame_t(0)));
      LayerImage* imageLayer = static_cast<LayerImage*>(m_currentLayer);
      imageLayer->removeCel(layerCel.get());

      for (const auto& inFrame : layerRecord.inFrames) {
        if (inFrame.isVisibleInFrame) {
          const auto findIter = std::find_if(
            m_framesInfo.cbegin(),
            m_framesInfo.cend(),
            [id = inFrame.frameID](const psd::FrameInformation& frameInfo) {
              return id == frameInfo.id;
            });
          if (findIter != m_framesInfo.cend()) {
            const size_t index = std::distance(m_framesInfo.cbegin(), findIter);
            auto newCel = Cel::MakeCopy(index, layerCel.get());
            imageLayer->addCel(newCel);
          }
        }
      }
    }

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
      // Only occurs where there's an image with no layer
      if (m_layers.empty()) {
        m_layerGroup = m_sprite->root();
        createNewLayer("Layer 1");
        m_layerHasTransparentChannel = hasTransparency(imageData.channels.size());
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

        for (size_t i = 0; i < m_sprite->totalFrames(); ++i) {
          Cel* cel = layer->cel(frame_t(i));
          if (cel) {
            cel->setOpacity(layerRecord.opacity);
            cel->setPosition(gfx::Point(layerRecord.left, layerRecord.top));
          }
        }
      }
    }
  }

  void onImageScanline(const psd::ImageData& img,
                       const int y,
                       const psd::ChannelID chanID,
                       const uint8_t* data,
                       const int bytes) override
  {
    if (!m_currentImage || y >= m_currentImage->height())
      return;

    const int dataCount = bytes / (img.depth >= 8 ? (img.depth / 8) : 1);
    uint8_t* dstGenericAddress = m_currentImage->getPixelAddress(0, y);

    if (m_pixelFormat == doc::PixelFormat::IMAGE_INDEXED) {
      IndexedTraits::address_t dstAddress = (IndexedTraits::address_t)dstGenericAddress;
      for (int x = 0; x < dataCount && x < m_currentImage->width(); ++x) {
        *(dstAddress)++ = getNormalizedPixelValue(data, img.depth);
      }
    }
    else if (m_pixelFormat == doc::PixelFormat::IMAGE_GRAYSCALE) {
      GrayscaleTraits::address_t dstAddress = (GrayscaleTraits::address_t)dstGenericAddress;
      uint8_t v = 0, a = 0;
      for (int x = 0; x < dataCount && x < m_currentImage->width(); ++x) {
        const GrayscaleTraits::pixel_t pixel = *dstAddress;
        const uint8_t newPixelValue = getNormalizedPixelValue(data, img.depth);
        if (chanID == psd::ChannelID::Red) {
          v = newPixelValue;
          a = m_layerHasTransparentChannel ? graya_geta(pixel) : 255;
        }
        else if (chanID == psd::ChannelID::Alpha || chanID == psd::ChannelID::TransparencyMask) {
          a = newPixelValue;
          v = graya_getv(pixel);
        }
        *(dstAddress++) = graya(v, a);
      }
    }
    else if (m_pixelFormat == doc::PixelFormat::IMAGE_RGB) {
      RgbTraits::address_t dstAddress = (RgbTraits::address_t)dstGenericAddress;
      uint8_t r, g, b, a;
      for (int x = 0; x < dataCount && x < m_currentImage->width(); ++x) {
        const uint8_t newPixelValue = getNormalizedPixelValue(data, img.depth);
        const color_t c = *(dstAddress);
        r = rgba_getr(c);
        g = rgba_getg(c);
        b = rgba_getb(c);
        a = m_layerHasTransparentChannel ? rgba_geta(c) : 255;
        if (chanID == psd::ChannelID::Red) {
          r = newPixelValue;
        }
        else if (chanID == psd::ChannelID::Green) {
          g = newPixelValue;
        }
        else if (chanID == psd::ChannelID::Blue) {
          b = newPixelValue;
        }
        else if (chanID == psd::ChannelID::Alpha || chanID == psd::ChannelID::TransparencyMask) {
          a = newPixelValue;
        }
        *(dstAddress++) = rgba(r, g, b, a);
      }
    }
  }

private:
  inline bool hasTransparency(const size_t nchannels)
  {
    // RGBA or grayscale image with alpha channel
    return nchannels == 4 || nchannels == 2;
  }

  void linkNewCel(Layer* layer, doc::ImageRef image)
  {
    if (!image)
      return;
    std::unique_ptr<Cel> cel(new doc::Cel(frame_t(0), image));
    cel->setPosition(0, 0);
    static_cast<LayerImage*>(layer)->addCel(cel.release());
  }

  void createNewLayer(const std::string& layerName)
  {
    m_currentLayer = new LayerImage(m_sprite);
    m_layerGroup->addLayer(m_currentLayer);
    m_layers.push_back(m_currentLayer);
    m_currentLayer->setName(layerName);
  }

  Sprite* assembleDocument()
  {
    if (m_palette.getModifications() > 1)
      m_sprite->setPalette(&m_palette, true);

    for (size_t i = 0; i < m_framesInfo.size(); ++i) {
      const psd::FrameInformation& frameInfo = m_framesInfo[i];
      const int timeMs = frameInfo.duration * 10;
      m_sprite->setFrameDuration(frame_t(i), timeMs);
    }
    // at this point, we're supposed to be able to set the activeFrame
    return m_sprite;
  }

  std::uint8_t getNormalizedPixelValue(const std::uint8_t*& data, const int depth)
  {
    if (depth == 1 || depth == 8) {
      return *(data++);
    }
    else if (depth == 16) {
      const uint16_t value = int(data[0]) | int(data[1] << 8);
      data += 2;
      return value >> 8;
    }
    else if (depth == 32) {
      const uint32_t value = int(data[0] << 24) | int(data[1] << 16) | int(data[2] << 8) |
                             int(data[3]);
      data += 4;
      return value >> 24;
    }
    else
      throw std::runtime_error("invalid image depth");
  }

  void createNewImage(const int width, const int height)
  {
    if (width <= 0 || height <= 0)
      return; // throw std::runtime_error("invalid image width/height");

    m_currentImage.reset(Image::create(m_pixelFormat, width, height));
    clear_image(m_currentImage.get(), 0);
  }

  doc::ImageRef m_currentImage;
  doc::Layer* m_currentLayer;
  doc::LayerGroup* m_layerGroup;
  Sprite* m_sprite;
  uint32_t m_activeFrameIndex;
  PixelFormat m_pixelFormat;
  std::vector<doc::Layer*> m_layers;
  std::vector<doc::LayerGroup*> m_groups;
  std::vector<psd::FrameInformation> m_framesInfo;
  Palette m_palette;
  bool m_layerHasTransparentChannel;
};

bool PsdFormat::onLoad(FileOp* fop)
{
  base::FileHandle fileHandle = base::open_file_with_exception(fop->filename(), "rb");
  FILE* f = fileHandle.get();
  psd::StdioFileInterface fileInterface(f);
  PsdDecoderDelegate pDelegate;
  psd::Decoder decoder(&fileInterface, &pDelegate);

  if (!decoder.readFileHeader()) {
    fop->setError("The file doesn't have a valid PSD header\n");
    return false;
  }

  const psd::FileHeader header = decoder.fileHeader();

  if (header.colorMode != psd::ColorMode::RGB && header.colorMode != psd::ColorMode::Indexed &&
      header.colorMode != psd::ColorMode::Grayscale) {
    fop->setError("This preliminary work only supports "
                  "RGB, Grayscale & Indexed images\n");
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

} // namespace app
