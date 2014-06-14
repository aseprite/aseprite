/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/widget_loader.h"

#include "app/app.h"
#include "app/modules/gui.h"
#include "app/xml_document.h"
#include "app/resource_finder.h"
#include "app/ui/color_button.h"
#include "app/widget_not_found.h"
#include "app/xml_exception.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/memory.h"
#include "ui/ui.h"

#include "tinyxml.h"

#include <allegro.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define TRANSLATE_ATTR(a) a

namespace app {

using namespace ui;

static int convert_align_value_to_flags(const char *value);
static bool bool_attr_is_true(const TiXmlElement* elem, const char* attribute_name);

WidgetLoader::WidgetLoader()
  : m_tooltipManager(NULL)
{
}

WidgetLoader::~WidgetLoader()
{
  for (TypeCreatorsMap::iterator
         it=m_typeCreators.begin(), end=m_typeCreators.end(); it != end; ++it)
    it->second->dispose();
}

void WidgetLoader::addWidgetType(const char* tagName, IWidgetTypeCreator* creator)
{
  m_typeCreators[tagName] = creator;
}

Widget* WidgetLoader::loadWidget(const char* fileName, const char* widgetId)
{
  Widget* widget;
  std::string buf;

  ResourceFinder rf;
  rf.addPath(fileName);

  buf = "widgets/";
  buf += fileName;
  rf.includeDataDir(buf.c_str());

  if (!rf.findFirst())
    throw WidgetNotFound(widgetId);

  widget = loadWidgetFromXmlFile(rf.filename(), widgetId);
  if (!widget)
    throw WidgetNotFound(widgetId);

  return widget;
}

Widget* WidgetLoader::loadWidgetFromXmlFile(const std::string& xmlFilename,
                                            const std::string& widgetId)
{
  Widget* widget = NULL;
  m_tooltipManager = NULL;

  XmlDocumentRef doc(open_xml(xmlFilename));
  TiXmlHandle handle(doc);

  // Search the requested widget.
  TiXmlElement* xmlElement = handle
    .FirstChild("gui")
    .FirstChildElement().ToElement();

  while (xmlElement) {
    const char* nodename = xmlElement->Attribute("id");

    if (nodename && nodename == widgetId) {
      widget = convertXmlElementToWidget(xmlElement, NULL);
      break;
    }

    xmlElement = xmlElement->NextSiblingElement();
  }

  return widget;
}

Widget* WidgetLoader::convertXmlElementToWidget(const TiXmlElement* elem, Widget* root)
{
  const std::string elem_name = elem->Value();
  Widget* widget = NULL;
  Widget* child;

  // TODO error handling: add a message if the widget is bad specified

  // Try to use one of the creators.
  TypeCreatorsMap::iterator it = m_typeCreators.find(elem_name);

  if (it != m_typeCreators.end()) {
    widget = it->second->createWidgetFromXml(elem);
  }
  // Boxes
  else if (elem_name == "box") {
    bool horizontal  = bool_attr_is_true(elem, "horizontal");
    bool vertical    = bool_attr_is_true(elem, "vertical");
    bool homogeneous = bool_attr_is_true(elem, "homogeneous");

    widget = new Box((horizontal ? JI_HORIZONTAL:
                      vertical ? JI_VERTICAL: 0) |
                     (homogeneous ? JI_HOMOGENEOUS: 0));
  }
  else if (elem_name == "vbox") {
    bool homogeneous = bool_attr_is_true(elem, "homogeneous");

    widget = new VBox();
    if (homogeneous)
      widget->setAlign(widget->getAlign() | JI_HOMOGENEOUS);
  }
  else if (elem_name == "hbox") {
    bool homogeneous = bool_attr_is_true(elem, "homogeneous");

    widget = new HBox();
    if (homogeneous)
      widget->setAlign(widget->getAlign() | JI_HOMOGENEOUS);
  }
  else if (elem_name == "boxfiller") {
    widget = new BoxFiller();
  }
  // Button
  else if (elem_name == "button") {
    const char *text = elem->Attribute("text");

    widget = new Button(text ? TRANSLATE_ATTR(text): "");
    if (widget) {
      bool left   = bool_attr_is_true(elem, "left");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");
      bool closewindow = bool_attr_is_true(elem, "closewindow");
      const char *_bevel = elem->Attribute("bevel");

      widget->setAlign((left ? JI_LEFT: (right ? JI_RIGHT: JI_CENTER)) |
                       (top ? JI_TOP: (bottom ? JI_BOTTOM: JI_MIDDLE)));

      if (_bevel != NULL) {
        char* bevel = base_strdup(_bevel);
        int c, b[4];
        char *tok;

        for (c=0; c<4; ++c)
          b[c] = 0;

        for (tok=ustrtok(bevel, " "), c=0;
             tok;
             tok=ustrtok(NULL, " "), ++c) {
          if (c < 4)
            b[c] = ustrtol(tok, NULL, 10);
        }
        base_free(bevel);

        setup_bevels(widget, b[0], b[1], b[2], b[3]);
      }

      if (closewindow) {
        static_cast<Button*>(widget)
          ->Click.connect(Bind<void>(&Widget::closeWindow, widget));
      }
    }
  }
  // Check
  else if (elem_name == "check") {
    const char *text = elem->Attribute("text");
    const char *looklike = elem->Attribute("looklike");

    text = (text ? TRANSLATE_ATTR(text): "");

    if (looklike != NULL && strcmp(looklike, "button") == 0) {
      widget = new CheckBox(text, kButtonWidget);
    }
    else {
      widget = new CheckBox(text);
    }

    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign((center ? JI_CENTER:
                        (right ? JI_RIGHT: JI_LEFT)) |
                       (top    ? JI_TOP:
                        (bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* combobox */
  else if (elem_name == "combobox") {
    widget = new ComboBox();
  }
  /* entry */
  else if (elem_name == "entry") {
    const char* maxsize = elem->Attribute("maxsize");
    const char* text = elem->Attribute("text");
    const char* suffix = elem->Attribute("suffix");

    if (maxsize != NULL) {
      bool readonly = bool_attr_is_true(elem, "readonly");

      widget = new Entry(strtol(maxsize, NULL, 10),
                         text ? TRANSLATE_ATTR(text): "");

      if (readonly)
        ((Entry*)widget)->setReadOnly(true);

      if (suffix)
        ((Entry*)widget)->setSuffix(suffix);
    }
    else
      throw std::runtime_error("<entry> element found without 'maxsize' attribute");
  }
  /* grid */
  else if (elem_name == "grid") {
    const char *columns = elem->Attribute("columns");
    bool same_width_columns = bool_attr_is_true(elem, "same_width_columns");

    if (columns != NULL) {
      widget = new Grid(strtol(columns, NULL, 10),
                        same_width_columns);
    }
  }
  /* label */
  else if (elem_name == "label") {
    const char *text = elem->Attribute("text");

    widget = new Label(text ? TRANSLATE_ATTR(text): "");
    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign((center ? JI_CENTER:
                        (right ? JI_RIGHT: JI_LEFT)) |
                       (top    ? JI_TOP:
                        (bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* link */
  else if (elem_name == "link") {
    const char* text = elem->Attribute("text");
    const char* url = elem->Attribute("url");

    widget = new LinkLabel(url ? url: "", text ? TRANSLATE_ATTR(text): "");
    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign(
        (center ? JI_CENTER: (right ? JI_RIGHT: JI_LEFT)) |
        (top    ? JI_TOP: (bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* listbox */
  else if (elem_name == "listbox") {
    widget = new ListBox();
  }
  /* listitem */
  else if (elem_name == "listitem") {
    const char *text = elem->Attribute("text");

    widget = new ListItem(text ? TRANSLATE_ATTR(text): "");
  }
  /* splitter */
  else if (elem_name == "splitter") {
    bool horizontal = bool_attr_is_true(elem, "horizontal");
    bool vertical = bool_attr_is_true(elem, "vertical");
    const char* by = elem->Attribute("by");
    const char* position = elem->Attribute("position");
    Splitter::Type type = (by && strcmp(by, "pixel") == 0 ?
                           Splitter::ByPixel:
                           Splitter::ByPercentage);

    Splitter* splitter = new Splitter(type,
                                      horizontal ? JI_HORIZONTAL:
                                      vertical ? JI_VERTICAL: 0);
    if (position) {
      splitter->setPosition(strtod(position, NULL)
        * (type == Splitter::ByPixel ? jguiscale(): 1));
    }
    widget = splitter;
  }
  /* radio */
  else if (elem_name == "radio") {
    const char* text = elem->Attribute("text");
    const char* group = elem->Attribute("group");
    const char *looklike = elem->Attribute("looklike");

    text = (text ? TRANSLATE_ATTR(text): "");
    int radio_group = (group ? strtol(group, NULL, 10): 1);

    if (looklike != NULL && strcmp(looklike, "button") == 0) {
      widget = new RadioButton(text, radio_group, kButtonWidget);
    }
    else {
      widget = new RadioButton(text, radio_group);
    }

    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign((center ? JI_CENTER:
                        (right ? JI_RIGHT: JI_LEFT)) |
                       (top    ? JI_TOP:
                        (bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* separator */
  else if (elem_name == "separator") {
    const char *text = elem->Attribute("text");
    bool center      = bool_attr_is_true(elem, "center");
    bool right       = bool_attr_is_true(elem, "right");
    bool middle      = bool_attr_is_true(elem, "middle");
    bool bottom      = bool_attr_is_true(elem, "bottom");
    bool horizontal  = bool_attr_is_true(elem, "horizontal");
    bool vertical    = bool_attr_is_true(elem, "vertical");

    widget = new Separator(text ? TRANSLATE_ATTR(text): "",
                           (horizontal ? JI_HORIZONTAL: 0) |
                           (vertical ? JI_VERTICAL: 0) |
                           (center ? JI_CENTER:
                            (right ? JI_RIGHT: JI_LEFT)) |
                           (middle ? JI_MIDDLE:
                            (bottom ? JI_BOTTOM: JI_TOP)));
  }
  /* slider */
  else if (elem_name == "slider") {
    const char *min = elem->Attribute("min");
    const char *max = elem->Attribute("max");
    int min_value = min != NULL ? strtol(min, NULL, 10): 0;
    int max_value = max != NULL ? strtol(max, NULL, 10): 0;

    widget = new Slider(min_value, max_value, min_value);
  }
  /* textbox */
  else if (elem_name == "textbox") {
    bool wordwrap = bool_attr_is_true(elem, "wordwrap");

    widget = new TextBox(elem->GetText(), wordwrap ? JI_WORDWRAP: 0);
  }
  /* view */
  else if (elem_name == "view") {
    widget = new View();
  }
  /* window */
  else if (elem_name == "window") {
    const char *text = elem->Attribute("text");
    bool desktop = bool_attr_is_true(elem, "desktop");

    if (desktop)
      widget = new Window(Window::DesktopWindow);
    else if (text)
      widget = new Window(Window::WithTitleBar, TRANSLATE_ATTR(text));
    else
      widget = new Window(Window::WithoutTitleBar);
  }
  /* colorpicker */
  else if (elem_name == "colorpicker") {
    widget = new ColorButton(Color::fromMask(), app_get_current_pixel_format());
  }

  // Was the widget created?
  if (widget) {
    const char* id        = elem->Attribute("id");
    const char* tooltip   = elem->Attribute("tooltip");
    bool selected         = bool_attr_is_true(elem, "selected");
    bool disabled         = bool_attr_is_true(elem, "disabled");
    bool expansive        = bool_attr_is_true(elem, "expansive");
    bool magnet           = bool_attr_is_true(elem, "magnet");
    bool noborders        = bool_attr_is_true(elem, "noborders");
    const char* width     = elem->Attribute("width");
    const char* height    = elem->Attribute("height");
    const char* minwidth  = elem->Attribute("minwidth");
    const char* minheight = elem->Attribute("minheight");
    const char* maxwidth  = elem->Attribute("maxwidth");
    const char* maxheight = elem->Attribute("maxheight");
    const char* childspacing = elem->Attribute("childspacing");

    if (width) {
      if (!minwidth) minwidth = width;
      if (!maxwidth) maxwidth = width;
    }
    if (height) {
      if (!minheight) minheight = height;
      if (!maxheight) maxheight = height;
    }

    if (id != NULL)
      widget->setId(id);

    if (tooltip != NULL && root != NULL) {
      if (!m_tooltipManager) {
        m_tooltipManager = new ui::TooltipManager();
        root->addChild(m_tooltipManager);
      }
      m_tooltipManager->addTooltipFor(widget, tooltip, JI_LEFT);
    }

    if (selected)
      widget->setSelected(selected);

    if (disabled)
      widget->setEnabled(false);

    if (expansive)
      widget->setExpansive(true);

    if (magnet)
      widget->setFocusMagnet(true);

    if (noborders)
      widget->noBorderNoChildSpacing();

    if (childspacing)
      widget->child_spacing = strtol(childspacing, NULL, 10);

    gfx::Size reqSize = widget->getPreferredSize();

    if (minwidth || minheight) {
      int w = (minwidth ? jguiscale()*strtol(minwidth, NULL, 10): reqSize.w);
      int h = (minheight ? jguiscale()*strtol(minheight, NULL, 10): reqSize.h);
      widget->setMinSize(gfx::Size(w, h));
    }

    if (maxwidth || maxheight) {
      int w = (maxwidth ? jguiscale()*strtol(maxwidth, NULL, 10): INT_MAX);
      int h = (maxheight ? jguiscale()*strtol(maxheight, NULL, 10): INT_MAX);
      widget->setMaxSize(gfx::Size(w, h));
    }

    if (!root)
      root = widget;

    // Children
    const TiXmlElement* childElem = elem->FirstChildElement();
    while (childElem) {
      child = convertXmlElementToWidget(childElem, root);
      if (child) {
        // Attach the child in the view
        if (widget->type == kViewWidget) {
          static_cast<View*>(widget)->attachToView(child);
          break;
        }
        // Add the child in the grid
        else if (widget->type == kGridWidget) {
          const char* cell_hspan = childElem->Attribute("cell_hspan");
          const char* cell_vspan = childElem->Attribute("cell_vspan");
          const char* cell_align = childElem->Attribute("cell_align");
          int hspan = cell_hspan ? strtol(cell_hspan, NULL, 10): 1;
          int vspan = cell_vspan ? strtol(cell_vspan, NULL, 10): 1;
          int align = cell_align ? convert_align_value_to_flags(cell_align): 0;
          Grid* grid = dynamic_cast<Grid*>(widget);
          ASSERT(grid != NULL);

          grid->addChildInCell(child, hspan, vspan, align);
        }
        // Just add the child in any other kind of widget
        else
          widget->addChild(child);
      }
      childElem = childElem->NextSiblingElement();
    }

    if (widget->type == kViewWidget) {
      bool maxsize = bool_attr_is_true(elem, "maxsize");
      if (maxsize)
        static_cast<View*>(widget)->makeVisibleAllScrollableArea();
    }
  }

  return widget;
}

static int convert_align_value_to_flags(const char *value)
{
  char *tok, *ptr = base_strdup(value);
  int flags = 0;

  for (tok=ustrtok(ptr, " ");
       tok != NULL;
       tok=ustrtok(NULL, " ")) {
    if (ustrcmp(tok, "horizontal") == 0) {
      flags |= JI_HORIZONTAL;
    }
    else if (ustrcmp(tok, "vertical") == 0) {
      flags |= JI_VERTICAL;
    }
    else if (ustrcmp(tok, "left") == 0) {
      flags |= JI_LEFT;
    }
    else if (ustrcmp(tok, "center") == 0) {
      flags |= JI_CENTER;
    }
    else if (ustrcmp(tok, "right") == 0) {
      flags |= JI_RIGHT;
    }
    else if (ustrcmp(tok, "top") == 0) {
      flags |= JI_TOP;
    }
    else if (ustrcmp(tok, "middle") == 0) {
      flags |= JI_MIDDLE;
    }
    else if (ustrcmp(tok, "bottom") == 0) {
      flags |= JI_BOTTOM;
    }
    else if (ustrcmp(tok, "homogeneous") == 0) {
      flags |= JI_HOMOGENEOUS;
    }
  }

  base_free(ptr);
  return flags;
}

static bool bool_attr_is_true(const TiXmlElement* elem, const char* attribute_name)
{
  const char* value = elem->Attribute(attribute_name);

  return (value != NULL) && (strcmp(value, "true") == 0);
}

} // namespace app
