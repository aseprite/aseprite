// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app_brushes.h"
#include "app/resource_finder.h"
#include "app/tools/ink_type.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/base64.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/serialization.h"
#include "doc/brush.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/image_impl.h"

#include "tinyxml2.h"

#include <fstream>

namespace app {

using namespace doc;
using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace tinyxml2;

namespace {

ImageRef load_xml_image(const XMLElement* imageElem)
{
  ImageRef image;
  int w, h;
  if (imageElem->QueryIntAttribute("width", &w) != XML_SUCCESS ||
      imageElem->QueryIntAttribute("height", &h) != XML_SUCCESS || w < 0 || w > 9999 || h < 0 ||
      h > 9999)
    return image;

  auto formatValue = imageElem->Attribute("format");
  if (!formatValue)
    return image;

  const char* pixels_base64 = imageElem->GetText();
  if (!pixels_base64)
    return image;

  base::buffer data;
  base::decode_base64(pixels_base64, data);
  auto it = data.begin(), end = data.end();

  std::string formatStr = formatValue;
  if (formatStr == "rgba") {
    image.reset(Image::create(IMAGE_RGB, w, h));
    LockImageBits<RgbTraits> pixels(image.get());
    for (auto& pixel : pixels) {
      if ((end - it) < 4)
        break;

      int r = *it;
      ++it;
      int g = *it;
      ++it;
      int b = *it;
      ++it;
      int a = *it;
      ++it;

      pixel = doc::rgba(r, g, b, a);
    }
  }
  else if (formatStr == "grayscale") {
    image.reset(Image::create(IMAGE_GRAYSCALE, w, h));
    LockImageBits<GrayscaleTraits> pixels(image.get());
    for (auto& pixel : pixels) {
      if ((end - it) < 2)
        break;

      int v = *it;
      ++it;
      int a = *it;
      ++it;

      pixel = doc::graya(v, a);
    }
  }
  else if (formatStr == "indexed") {
    image.reset(Image::create(IMAGE_INDEXED, w, h));
    LockImageBits<IndexedTraits> pixels(image.get());
    for (auto& pixel : pixels) {
      if (it == end)
        break;

      pixel = *it;
      ++it;
    }
  }
  else if (formatStr == "bitmap") {
    image.reset(Image::create(IMAGE_BITMAP, w, h));
    LockImageBits<BitmapTraits> pixels(image.get());
    for (auto& pixel : pixels) {
      if (it == end)
        break;

      pixel = *it;
      ++it;
    }
  }
  return image;
}

void save_xml_image(XMLElement* imageElem, const Image* image)
{
  int w = image->width();
  int h = image->height();
  imageElem->SetAttribute("width", w);
  imageElem->SetAttribute("height", h);

  std::string format;
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       format = "rgba"; break;
    case IMAGE_GRAYSCALE: format = "grayscale"; break;
    case IMAGE_INDEXED:   format = "indexed"; break;
    case IMAGE_BITMAP:    format = "bitmap"; break; // TODO add "bitmap" format
  }
  ASSERT(!format.empty());
  if (!format.empty())
    imageElem->SetAttribute("format", format.c_str());

  base::buffer data;
  data.reserve(h * image->widthBytes());
  switch (image->pixelFormat()) {
    case IMAGE_RGB: {
      const LockImageBits<RgbTraits> pixels(image);
      for (const auto& pixel : pixels) {
        data.push_back(doc::rgba_getr(pixel));
        data.push_back(doc::rgba_getg(pixel));
        data.push_back(doc::rgba_getb(pixel));
        data.push_back(doc::rgba_geta(pixel));
      }
      break;
    }
    case IMAGE_GRAYSCALE: {
      const LockImageBits<GrayscaleTraits> pixels(image);
      for (const auto& pixel : pixels) {
        data.push_back(doc::graya_getv(pixel));
        data.push_back(doc::graya_geta(pixel));
      }
      break;
    }
    case IMAGE_INDEXED: {
      const LockImageBits<IndexedTraits> pixels(image);
      for (const auto& pixel : pixels) {
        data.push_back(pixel);
      }
      break;
    }
    case IMAGE_BITMAP: {
      // Here we save bitmap format as indexed
      const LockImageBits<BitmapTraits> pixels(image);
      for (const auto& pixel : pixels) {
        data.push_back(pixel); // TODO save bitmap format as bitmap
      }
      break;
    }
  }

  std::string data_base64;
  base::encode_base64(data, data_base64);
  imageElem->InsertNewText(data_base64.c_str());
}

} // anonymous namespace

AppBrushes::AppBrushes()
{
  m_standard.push_back(BrushRef(new Brush(kCircleBrushType, 7, 0)));
  m_standard.push_back(BrushRef(new Brush(kSquareBrushType, 7, 0)));
  m_standard.push_back(BrushRef(new Brush(kLineBrushType, 7, 44)));

  std::string fn = userBrushesFilename();
  if (base::is_file(fn)) {
    try {
      load(fn);
    }
    catch (const std::exception& ex) {
      LOG(ERROR, "BRSH: Error loading user brushes from '%s': %s\n", fn.c_str(), ex.what());
    }
  }
  m_userBrushesFilename = fn;
}

AppBrushes::~AppBrushes()
{
  if (!m_userBrushesFilename.empty()) {
    try {
      save(m_userBrushesFilename);
    }
    // We cannot throw exceptions from a destructor
    catch (const std::exception& ex) {
      LOG(ERROR,
          "BRSH: Error saving user brushes to '%s': %s\n",
          m_userBrushesFilename.c_str(),
          ex.what());
    }
  }
}

AppBrushes::slot_id AppBrushes::addBrushSlot(const BrushSlot& brush)
{
  // Use an empty slot
  for (size_t i = 0; i < m_slots.size(); ++i) {
    if (!m_slots[i].locked() || m_slots[i].isEmpty()) {
      m_slots[i] = brush;
      return i + 1;
    }
  }

  m_slots.push_back(brush);
  ItemsChange();
  return slot_id(m_slots.size()); // Returns the slot
}

void AppBrushes::removeBrushSlot(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size()) {
    m_slots[slot] = BrushSlot();

    // Erase empty trailing slots
    while (!m_slots.empty() && m_slots[m_slots.size() - 1].isEmpty())
      m_slots.erase(--m_slots.end());

    ItemsChange();
  }
}

void AppBrushes::removeAllBrushSlots()
{
  while (!m_slots.empty())
    m_slots.erase(--m_slots.end());

  ItemsChange();
}

bool AppBrushes::hasBrushSlot(slot_id slot) const
{
  --slot;
  return (slot >= 0 && slot < (int)m_slots.size() && !m_slots[slot].isEmpty());
}

BrushSlot AppBrushes::getBrushSlot(slot_id slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size())
    return m_slots[slot];
  else
    return BrushSlot();
}

void AppBrushes::setBrushSlot(slot_id slot, const BrushSlot& brush)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size()) {
    m_slots[slot] = brush;
    ItemsChange();
  }
}

void AppBrushes::lockBrushSlot(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() && !m_slots[slot].isEmpty()) {
    m_slots[slot].setLocked(true);
  }
}

void AppBrushes::unlockBrushSlot(slot_id slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() && !m_slots[slot].isEmpty()) {
    m_slots[slot].setLocked(false);
  }
}

bool AppBrushes::isBrushSlotLocked(slot_id slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_slots.size() && !m_slots[slot].isEmpty()) {
    return m_slots[slot].locked();
  }
  else
    return false;
}

static const int kBrushFlags = int(BrushSlot::Flags::BrushType) | int(BrushSlot::Flags::BrushSize) |
                               int(BrushSlot::Flags::BrushAngle);

void AppBrushes::load(const std::string& filename)
{
  XMLDocumentRef doc = app::open_xml(filename);
  XMLHandle handle(doc.get());
  XMLElement* brushElem =
    handle.FirstChildElement("brushes").FirstChildElement("brush").ToElement();

  while (brushElem) {
    // flags
    int flags = 0;
    BrushRef brush;
    app::Color fgColor;
    app::Color bgColor;
    tools::InkType inkType = tools::InkType::DEFAULT;
    int inkOpacity = 255;
    Shade shade;
    bool pixelPerfect = false;

    // Brush
    const char* type = brushElem->Attribute("type");
    const char* size = brushElem->Attribute("size");
    const char* angle = brushElem->Attribute("angle");
    if (type || size || angle) {
      if (type)
        flags |= int(BrushSlot::Flags::BrushType);
      if (size)
        flags |= int(BrushSlot::Flags::BrushSize);
      if (angle)
        flags |= int(BrushSlot::Flags::BrushAngle);
      brush.reset(new Brush((type ? string_id_to_brush_type(type) : kFirstBrushType),
                            (size ? base::convert_to<int>(std::string(size)) : 1),
                            (angle ? base::convert_to<int>(std::string(angle)) : 0)));
    }

    // Brush image
    ImageRef image, mask;
    if (XMLElement* imageElem = brushElem->FirstChildElement("image"))
      image = load_xml_image(imageElem);
    if (XMLElement* maskElem = brushElem->FirstChildElement("mask"))
      mask = load_xml_image(maskElem);

    if (image) {
      if (!brush)
        brush.reset(new Brush());
      brush->setImage(image.get(), mask.get());
    }

    // Colors
    if (XMLElement* fgcolorElem = brushElem->FirstChildElement("fgcolor")) {
      if (auto value = fgcolorElem->Attribute("value")) {
        fgColor = app::Color::fromString(value);
        flags |= int(BrushSlot::Flags::FgColor);
      }
    }

    if (XMLElement* bgcolorElem = brushElem->FirstChildElement("bgcolor")) {
      if (auto value = bgcolorElem->Attribute("value")) {
        bgColor = app::Color::fromString(value);
        flags |= int(BrushSlot::Flags::BgColor);
      }
    }

    // Ink
    if (XMLElement* inkTypeElem = brushElem->FirstChildElement("inktype")) {
      if (auto value = inkTypeElem->Attribute("value")) {
        inkType = app::tools::string_id_to_ink_type(value);
        flags |= int(BrushSlot::Flags::InkType);
      }
    }

    if (XMLElement* inkOpacityElem = brushElem->FirstChildElement("inkopacity")) {
      if (auto value = inkOpacityElem->Attribute("value")) {
        inkOpacity = base::convert_to<int>(std::string(value));
        flags |= int(BrushSlot::Flags::InkOpacity);
      }
    }

    // Shade
    if (XMLElement* shadeElem = brushElem->FirstChildElement("shade")) {
      if (auto value = shadeElem->Attribute("value")) {
        shade = shade_from_string(value);
        flags |= int(BrushSlot::Flags::Shade);
      }
    }

    // Pixel-perfect
    if (XMLElement* pixelPerfectElem = brushElem->FirstChildElement("pixelperfect")) {
      pixelPerfect = bool_attr(pixelPerfectElem, "value", false);
      flags |= int(BrushSlot::Flags::PixelPerfect);
    }

    // Image color (enabled by default for backward compatibility)
    if (!brushElem->Attribute("imagecolor") || bool_attr(brushElem, "imagecolor", false))
      flags |= int(BrushSlot::Flags::ImageColor);

    if (flags != 0)
      flags |= int(BrushSlot::Flags::Locked);

    BrushSlot brushSlot(BrushSlot::Flags(flags),
                        brush,
                        fgColor,
                        bgColor,
                        inkType,
                        inkOpacity,
                        shade,
                        pixelPerfect);
    m_slots.push_back(brushSlot);

    brushElem = brushElem->NextSiblingElement();
  }
}

void AppBrushes::save(const std::string& filename) const
{
  auto doc = std::make_unique<XMLDocument>();
  XMLElement* brushesElem = doc->NewElement("brushes");

  doc->InsertEndChild(doc->NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\""));
  doc->InsertEndChild(brushesElem);
  for (const auto& slot : m_slots) {
    XMLElement* brushElem = brushesElem->InsertNewChildElement("brush");
    if (slot.locked()) {
      // Flags
      int flags = int(slot.flags());

      // This slot might not have a brush. (E.g. a slot that changes
      // the pixel perfect mode only.)
      if (!slot.hasBrush())
        flags &= ~kBrushFlags;

      // Brush type
      if (slot.hasBrush()) {
        ASSERT(slot.brush());

        if (flags & int(BrushSlot::Flags::BrushType)) {
          brushElem->SetAttribute("type", brush_type_to_string_id(slot.brush()->type()).c_str());
        }

        if (flags & int(BrushSlot::Flags::BrushSize)) {
          brushElem->SetAttribute("size", slot.brush()->size());
        }

        if (flags & int(BrushSlot::Flags::BrushAngle)) {
          brushElem->SetAttribute("angle", slot.brush()->angle());
        }

        if (slot.brush()->type() == kImageBrushType && slot.brush()->originalImage()) {
          XMLElement* elem = brushElem->InsertNewChildElement("image");
          save_xml_image(elem, slot.brush()->originalImage());

          if (slot.brush()->maskBitmap()) {
            XMLElement* maskElem = brushElem->InsertNewChildElement("mask");
            save_xml_image(maskElem, slot.brush()->maskBitmap());
          }

          // Image color
          brushElem->SetAttribute("imagecolor",
                                  (flags & int(BrushSlot::Flags::ImageColor)) ? "true" : "false");
        }
      }

      // Colors
      if (flags & int(BrushSlot::Flags::FgColor)) {
        XMLElement* elem = brushElem->InsertNewChildElement("fgcolor");
        elem->SetAttribute("value", slot.fgColor().toString().c_str());
      }

      if (flags & int(BrushSlot::Flags::BgColor)) {
        XMLElement* elem = brushElem->InsertNewChildElement("bgcolor");
        elem->SetAttribute("value", slot.bgColor().toString().c_str());
      }

      // Ink
      if (flags & int(BrushSlot::Flags::InkType)) {
        XMLElement* elem = brushElem->InsertNewChildElement("inktype");
        elem->SetAttribute("value", app::tools::ink_type_to_string_id(slot.inkType()).c_str());
      }

      if (flags & int(BrushSlot::Flags::InkOpacity)) {
        XMLElement* elem = brushElem->InsertNewChildElement("inkopacity");
        elem->SetAttribute("value", slot.inkOpacity());
      }

      // Shade
      if (flags & int(BrushSlot::Flags::Shade)) {
        XMLElement* elem = brushElem->InsertNewChildElement("shade");
        elem->SetAttribute("value", shade_to_string(slot.shade()).c_str());
      }

      // Pixel-perfect
      if (flags & int(BrushSlot::Flags::PixelPerfect)) {
        XMLElement* elem = brushElem->InsertNewChildElement("pixelperfect");
        elem->SetAttribute("value", slot.pixelPerfect() ? "true" : "false");
      }
    }
  }

  save_xml(doc.get(), filename);
}

// static
std::string AppBrushes::userBrushesFilename()
{
  ResourceFinder rf;
  rf.includeUserDir("user.aseprite-brushes");
  return rf.getFirstOrCreateDefault();
}

} // namespace app
