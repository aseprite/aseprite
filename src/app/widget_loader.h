/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_WIDGET_LOADER_H_INCLUDED
#define APP_WIDGET_LOADER_H_INCLUDED

#include "app/widget_type_mismatch.h"

#include <map>
#include <string>

class TiXmlElement;

namespace ui {
  class Widget;
  class TooltipManager;
}

namespace app {

  class WidgetLoader
  {
  public:
    // Interface used to create customized widgets.
    class IWidgetTypeCreator {
    public:
      virtual ~IWidgetTypeCreator() { }
      virtual void dispose() = 0;
      virtual ui::Widget* createWidgetFromXml(const TiXmlElement* xmlElem) = 0;
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
    ui::Widget* loadWidget(const char* fileName, const char* widgetId);

    template<class T>
    T* loadWidgetT(const char* fileName, const char* widgetId) {
      ui::Widget* widget = loadWidget(fileName, widgetId);

      T* specificWidget = dynamic_cast<T*>(widget);
      if (!specificWidget)
        throw WidgetTypeMismatch(widgetId);

      return specificWidget;
    }

  private:
    ui::Widget* loadWidgetFromXmlFile(const char* xmlFilename, const char* widgetId);
    ui::Widget* convertXmlElementToWidget(const TiXmlElement* elem, ui::Widget* root);

    typedef std::map<std::string, IWidgetTypeCreator*> TypeCreatorsMap;

    TypeCreatorsMap m_typeCreators;
    ui::TooltipManager* m_tooltipManager;
  };

} // namespace app

#endif
