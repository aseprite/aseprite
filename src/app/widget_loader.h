// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WIDGET_LOADER_H_INCLUDED
#define APP_WIDGET_LOADER_H_INCLUDED
#pragma once

#include "app/i18n/xml_translator.h"
#include "app/widget_type_mismatch.h"

#include <map>
#include <string>

namespace tinyxml2 {
class XMLElement;
}

namespace ui {
class Widget;
class TooltipManager;
} // namespace ui

namespace app {

class WidgetLoader {
public:
  // Interface used to create customized widgets.
  class IWidgetTypeCreator {
  public:
    virtual ~IWidgetTypeCreator() {}
    virtual void dispose() = 0;
    virtual ui::Widget* createWidgetFromXml(const tinyxml2::XMLElement* xmlElem) = 0;
  };

  WidgetLoader();
  ~WidgetLoader();

  // Adds a new widget type that can be referenced in the .xml file
  // with an XML element. The "tagName" is the same name as in the
  // .xml should appear as <tagName>...</tagName>
  //
  // The "creator" will not be deleted automatically at the
  // WidgetLoader dtor.
  void addWidgetType(const char* tagName, IWidgetTypeCreator* creator);

  // Loads the specified widget from an .xml file.
  ui::Widget* loadWidget(const char* fileName, const char* widgetId, ui::Widget* widget = NULL);

  template<class T>
  T* loadWidgetT(const char* fileName, const char* widgetId, T* widget = NULL)
  {
    T* specificWidget = dynamic_cast<T*>(loadWidget(fileName, widgetId, widget));
    if (!specificWidget)
      throw WidgetTypeMismatch(widgetId);

    return specificWidget;
  }

private:
  ui::Widget* loadWidgetFromXmlFile(const std::string& xmlFilename,
                                    const std::string& widgetId,
                                    ui::Widget* widget);

  ui::Widget* convertXmlElementToWidget(const tinyxml2::XMLElement* elem,
                                        ui::Widget* root,
                                        ui::Widget* parent,
                                        ui::Widget* widget);
  void fillWidgetWithXmlElementAttributes(const tinyxml2::XMLElement* elem,
                                          ui::Widget* root,
                                          ui::Widget* widget);
  void fillWidgetWithXmlElementAttributesWithChildren(const tinyxml2::XMLElement* elem,
                                                      ui::Widget* root,
                                                      ui::Widget* widget);

  typedef std::map<std::string, IWidgetTypeCreator*> TypeCreatorsMap;

  TypeCreatorsMap m_typeCreators;
  ui::TooltipManager* m_tooltipManager;
  XmlTranslator m_xmlTranslator;
};

} // namespace app

#endif
