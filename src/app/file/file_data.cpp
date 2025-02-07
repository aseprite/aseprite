// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/file/file_data.h"

#include "app/color.h"
#include "app/xml_document.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "doc/color.h"
#include "doc/document.h"
#include "doc/slice.h"
#include "fmt/format.h"
#include "gfx/color.h"

#include "tinyxml2.h"

#include <cstdlib>
#include <cstring>
#include <set>

namespace app {

using namespace base;
using namespace doc;
using namespace tinyxml2;

namespace {

std::string color_to_hex(doc::color_t color)
{
  std::string buf;
  if (doc::rgba_geta(color) == 255) {
    buf = fmt::format("#{:02x}{:02x}{:02x}",
                      doc::rgba_getr(color),
                      doc::rgba_getg(color),
                      doc::rgba_getb(color));
  }
  else {
    buf = fmt::format("#{:02x}{:02x}{:02x}{:02x}",
                      doc::rgba_getr(color),
                      doc::rgba_getg(color),
                      doc::rgba_getb(color),
                      doc::rgba_geta(color));
  }
  return buf;
}

doc::color_t color_from_hex(const char* str)
{
  if (*str == '#')
    ++str;

  if (std::strlen(str) == 3) { // #fff
    uint32_t value = std::strtol(str, nullptr, 16);
    int r = ((value & 0xf00) >> 8);
    int g = ((value & 0xf0) >> 4);
    int b = (value & 0xf);
    return gfx::rgba(r | (r << 4), g | (g << 4), b | (b << 4));
  }
  else if (std::strlen(str) == 6) { // #ffffff
    uint32_t value = std::strtol(str, nullptr, 16);
    return gfx::rgba((value & 0xff0000) >> 16, (value & 0xff00) >> 8, (value & 0xff));
  }
  else if (std::strlen(str) == 8) { // #ffffffff
    uint32_t value = std::strtol(str, nullptr, 16);
    return gfx::rgba((value & 0xff000000) >> 24,
                     (value & 0xff0000) >> 16,
                     (value & 0xff00) >> 8,
                     (value & 0xff));
  }
  else
    return gfx::rgba(0, 0, 0, 255);
}

template<typename Container, typename ChildNameGetterFunc, typename UpdateXmlChildFunc>
void update_xml_collection(const Container& container,
                           XMLElement* xmlParent,
                           const char* childElemName,
                           const char* idAttrName,
                           ChildNameGetterFunc childNameGetter,
                           UpdateXmlChildFunc updateXmlChild)
{
  if (!xmlParent)
    return;

  XMLElement* xmlNext = nullptr;
  std::set<std::string> existent;

  // Update existent children
  for (XMLElement* xmlChild = (xmlParent->FirstChildElement(childElemName) ?
                                 xmlParent->FirstChildElement(childElemName) :
                                 nullptr);
       xmlChild;
       xmlChild = xmlNext) {
    xmlNext = xmlChild->NextSiblingElement();

    const char* xmlChildName = xmlChild->Attribute(idAttrName);
    if (!xmlChildName)
      continue;

    bool found = false;
    for (const auto& child : container) {
      std::string thisChildName = childNameGetter(child);
      if (thisChildName == xmlChildName) {
        existent.insert(thisChildName);
        updateXmlChild(child, xmlChild);
        found = true;
        break;
      }
    }

    // Delete this <child> element (as the child was removed from the
    // original container)
    if (!found)
      xmlParent->DeleteChild(xmlChild);
  }

  // Add new children
  for (const auto& child : container) {
    std::string thisChildName = childNameGetter(child);
    if (existent.find(thisChildName) == existent.end()) {
      XMLElement* xmlChild = xmlParent->InsertNewChildElement(childElemName);
      xmlChild->SetAttribute(idAttrName, thisChildName.c_str());
      updateXmlChild(child, xmlChild);
    }
  }
}

void update_xml_part_from_slice_key(const doc::SliceKey* key, XMLElement* xmlPart)
{
  xmlPart->SetAttribute("x", key->bounds().x);
  xmlPart->SetAttribute("y", key->bounds().y);
  if (!key->hasCenter()) {
    xmlPart->SetAttribute("w", key->bounds().w);
    xmlPart->SetAttribute("h", key->bounds().h);
    if (xmlPart->Attribute("w1"))
      xmlPart->DeleteAttribute("w1");
    if (xmlPart->Attribute("w2"))
      xmlPart->DeleteAttribute("w2");
    if (xmlPart->Attribute("w3"))
      xmlPart->DeleteAttribute("w3");
    if (xmlPart->Attribute("h1"))
      xmlPart->DeleteAttribute("h1");
    if (xmlPart->Attribute("h2"))
      xmlPart->DeleteAttribute("h2");
    if (xmlPart->Attribute("h3"))
      xmlPart->DeleteAttribute("h3");
  }
  else {
    xmlPart->SetAttribute("w1", key->center().x);
    xmlPart->SetAttribute("w2", key->center().w);
    xmlPart->SetAttribute("w3", key->bounds().w - key->center().x2());
    xmlPart->SetAttribute("h1", key->center().y);
    xmlPart->SetAttribute("h2", key->center().h);
    xmlPart->SetAttribute("h3", key->bounds().h - key->center().y2());
    if (xmlPart->Attribute("w"))
      xmlPart->DeleteAttribute("w");
    if (xmlPart->Attribute("h"))
      xmlPart->DeleteAttribute("h");
  }

  if (key->hasPivot()) {
    xmlPart->SetAttribute("focusx", key->pivot().x);
    xmlPart->SetAttribute("focusy", key->pivot().y);
  }
  else {
    if (xmlPart->Attribute("focusx"))
      xmlPart->DeleteAttribute("focusx");
    if (xmlPart->Attribute("focusy"))
      xmlPart->DeleteAttribute("focusy");
  }
}

void update_xml_slice(const doc::Slice* slice, XMLElement* xmlSlice)
{
  if (!slice->userData().text().empty())
    xmlSlice->SetAttribute("text", slice->userData().text().c_str());
  else if (xmlSlice->Attribute("text"))
    xmlSlice->DeleteAttribute("text");
  xmlSlice->SetAttribute("color", color_to_hex(slice->userData().color()).c_str());

  // Update <key> elements
  update_xml_collection(
    *slice,
    xmlSlice,
    "key",
    "frame",
    [](const Keyframes<SliceKey>::Key& key) -> std::string {
      return base::convert_to<std::string>(key.frame());
    },
    [](const Keyframes<SliceKey>::Key& key, XMLElement* xmlKey) {
      SliceKey* sliceKey = key.value();

      xmlKey->SetAttribute("x", sliceKey->bounds().x);
      xmlKey->SetAttribute("y", sliceKey->bounds().y);
      xmlKey->SetAttribute("w", sliceKey->bounds().w);
      xmlKey->SetAttribute("h", sliceKey->bounds().h);

      if (sliceKey->hasCenter()) {
        xmlKey->SetAttribute("cx", sliceKey->center().x);
        xmlKey->SetAttribute("cy", sliceKey->center().y);
        xmlKey->SetAttribute("cw", sliceKey->center().w);
        xmlKey->SetAttribute("ch", sliceKey->center().h);
      }
      else {
        if (xmlKey->Attribute("cx"))
          xmlKey->DeleteAttribute("cx");
        if (xmlKey->Attribute("cy"))
          xmlKey->DeleteAttribute("cy");
        if (xmlKey->Attribute("cw"))
          xmlKey->DeleteAttribute("cw");
        if (xmlKey->Attribute("ch"))
          xmlKey->DeleteAttribute("ch");
      }

      if (sliceKey->hasPivot()) {
        xmlKey->SetAttribute("px", sliceKey->pivot().x);
        xmlKey->SetAttribute("py", sliceKey->pivot().y);
      }
      else {
        if (xmlKey->Attribute("px"))
          xmlKey->DeleteAttribute("px");
        if (xmlKey->Attribute("py"))
          xmlKey->DeleteAttribute("py");
      }
    });
}

} // anonymous namespace

void load_aseprite_data_file(const std::string& dataFilename,
                             doc::Document* doc,
                             app::Color& defaultSliceColor)
{
  XMLDocumentRef xmlDoc = open_xml(dataFilename);
  XMLHandle handle(xmlDoc.get());

  XMLElement* xmlSlices =
    handle.FirstChildElement("sprite").FirstChildElement("slices").ToElement();

  // Load slices/parts from theme.xml file
  if (xmlSlices && xmlSlices->Attribute("theme")) {
    std::string themeFileName = xmlSlices->Attribute("theme");

    // Open theme XML file
    XMLDocumentRef xmlThemeDoc = open_xml(
      base::join_path(base::get_file_path(dataFilename), themeFileName));
    XMLHandle themeHandle(xmlThemeDoc.get());
    for (XMLElement* xmlPart = themeHandle.FirstChildElement("theme")
                                 .FirstChildElement("parts")
                                 .FirstChildElement("part")
                                 .ToElement();
         xmlPart;
         xmlPart = xmlPart->NextSiblingElement()) {
      const char* partId = xmlPart->Attribute("id");
      if (!partId)
        continue;

      auto slice = new doc::Slice();
      slice->setName(partId);

      // Default slice color
      slice->userData().setColor(doc::rgba(defaultSliceColor.getRed(),
                                           defaultSliceColor.getGreen(),
                                           defaultSliceColor.getBlue(),
                                           defaultSliceColor.getAlpha()));

      doc::SliceKey key;

      int x = std::strtol(xmlPart->Attribute("x"), NULL, 10);
      int y = std::strtol(xmlPart->Attribute("y"), NULL, 10);

      if (xmlPart->Attribute("w1")) {
        int w1 = xmlPart->Attribute("w1") ? std::strtol(xmlPart->Attribute("w1"), NULL, 10) : 0;
        int w2 = xmlPart->Attribute("w2") ? std::strtol(xmlPart->Attribute("w2"), NULL, 10) : 0;
        int w3 = xmlPart->Attribute("w3") ? std::strtol(xmlPart->Attribute("w3"), NULL, 10) : 0;
        int h1 = xmlPart->Attribute("h1") ? std::strtol(xmlPart->Attribute("h1"), NULL, 10) : 0;
        int h2 = xmlPart->Attribute("h2") ? std::strtol(xmlPart->Attribute("h2"), NULL, 10) : 0;
        int h3 = xmlPart->Attribute("h3") ? std::strtol(xmlPart->Attribute("h3"), NULL, 10) : 0;

        key.setBounds(gfx::Rect(x, y, w1 + w2 + w3, h1 + h2 + h3));
        key.setCenter(gfx::Rect(w1, h1, w2, h2));
      }
      else if (xmlPart->Attribute("w")) {
        int w = xmlPart->Attribute("w") ? std::strtol(xmlPart->Attribute("w"), NULL, 10) : 0;
        int h = xmlPart->Attribute("h") ? std::strtol(xmlPart->Attribute("h"), NULL, 10) : 0;
        key.setBounds(gfx::Rect(x, y, w, h));
      }

      if (xmlPart->Attribute("focusx")) {
        int x = xmlPart->Attribute("focusx") ? std::strtol(xmlPart->Attribute("focusx"), NULL, 10) :
                                               0;
        int y = xmlPart->Attribute("focusy") ? std::strtol(xmlPart->Attribute("focusy"), NULL, 10) :
                                               0;
        key.setPivot(gfx::Point(x, y));
      }

      slice->insert(0, key);
      doc->sprite()->slices().add(slice);
    }
  }
  // Load slices from <slice> elements
  else if (xmlSlices) {
    for (XMLElement* xmlSlice = (xmlSlices->FirstChildElement("slice") ?
                                   xmlSlices->FirstChildElement("slice")->ToElement() :
                                   nullptr);
         xmlSlice;
         xmlSlice = xmlSlice->NextSiblingElement()) {
      const char* sliceId = xmlSlice->Attribute("id");
      if (!sliceId)
        continue;

      // If the document already contains a slice with this name, use the one from the document
      if (doc->sprite()->slices().getByName(sliceId))
        continue;

      auto slice = new doc::Slice();
      slice->setName(sliceId);

      // Slice text
      if (xmlSlice->Attribute("text"))
        slice->userData().setText(xmlSlice->Attribute("text"));

      // Slice color
      doc::color_t color;
      if (xmlSlice->Attribute("color")) {
        color = color_from_hex(xmlSlice->Attribute("color"));
      }
      else {
        color = doc::rgba(defaultSliceColor.getRed(),
                          defaultSliceColor.getGreen(),
                          defaultSliceColor.getBlue(),
                          defaultSliceColor.getAlpha());
      }
      slice->userData().setColor(color);

      for (XMLElement* xmlKey =
             (xmlSlice->FirstChildElement("key") ? xmlSlice->FirstChildElement("key")->ToElement() :
                                                   nullptr);
           xmlKey;
           xmlKey = xmlKey->NextSiblingElement()) {
        if (!xmlKey->Attribute("frame"))
          continue;

        doc::SliceKey key;
        doc::frame_t frame = std::strtol(xmlKey->Attribute("frame"), nullptr, 10);

        int x = std::strtol(xmlKey->Attribute("x"), nullptr, 10);
        int y = std::strtol(xmlKey->Attribute("y"), nullptr, 10);
        int w = std::strtol(xmlKey->Attribute("w"), nullptr, 10);
        int h = std::strtol(xmlKey->Attribute("h"), nullptr, 10);
        key.setBounds(gfx::Rect(x, y, w, h));

        if (xmlKey->Attribute("cx")) {
          int cx = std::strtol(xmlKey->Attribute("cx"), nullptr, 10);
          int cy = std::strtol(xmlKey->Attribute("cy"), nullptr, 10);
          int cw = std::strtol(xmlKey->Attribute("cw"), nullptr, 10);
          int ch = std::strtol(xmlKey->Attribute("ch"), nullptr, 10);
          key.setCenter(gfx::Rect(cx, cy, cw, ch));
        }

        if (xmlKey->Attribute("px")) {
          int px = std::strtol(xmlKey->Attribute("px"), nullptr, 10);
          int py = std::strtol(xmlKey->Attribute("py"), nullptr, 10);
          key.setPivot(gfx::Point(px, py));
        }

        slice->insert(frame, key);
      }

      doc->sprite()->slices().add(slice);
    }
  }
}

#ifdef ENABLE_SAVE
void save_aseprite_data_file(const std::string& dataFilename, const doc::Document* doc)
{
  XMLDocumentRef xmlDoc = open_xml(dataFilename);
  XMLHandle handle(xmlDoc.get());

  XMLElement* xmlSlices =
    handle.FirstChildElement("sprite").FirstChildElement("slices").ToElement();

  // Update theme.xml file
  if (xmlSlices && xmlSlices->Attribute("theme")) {
    // Open theme XML file
    std::string themeFileName = base::join_path(base::get_file_path(dataFilename),
                                                xmlSlices->Attribute("theme"));
    XMLDocumentRef xmlThemeDoc = open_xml(themeFileName);

    XMLHandle themeHandle(xmlThemeDoc.get());
    XMLElement* xmlParts =
      themeHandle.FirstChildElement("theme").FirstChildElement("parts").ToElement();

    update_xml_collection(
      doc->sprite()->slices(),
      xmlParts,
      "part",
      "id",
      [](const Slice* slice) -> std::string {
        if (slice->getByFrame(0))
          return slice->name();
        else
          return std::string();
      },
      [](Slice* slice, XMLElement* xmlSlice) {
        ASSERT(slice->getByFrame(0));
        update_xml_part_from_slice_key(slice->getByFrame(0), xmlSlice);
      });

    // Save theme.xml file
    save_xml(xmlThemeDoc.get(), themeFileName);
  }
  // <slices> without "theme" attribute
  else if (xmlSlices) {
    update_xml_collection(
      doc->sprite()->slices(),
      xmlSlices,
      "slice",
      "id",
      [](const Slice* slice) -> std::string { return slice->name(); },
      update_xml_slice);

    // Save .aseprite-data file
    save_xml(xmlDoc.get(), dataFilename);
  }
}
#endif

} // namespace app
