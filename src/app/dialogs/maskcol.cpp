/* ASEPRITE
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

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "app/undo_transaction.h"
#include "app/undoers/set_mask.h"
#include "app/util/misc.h"
#include "base/bind.h"
#include "base/unique_ptr.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/slider.h"
#include "ui/widget.h"
#include "ui/window.h"

namespace app {

using namespace ui;

static ColorButton* button_color;
static CheckBox* check_preview;
static Slider* slider_tolerance;

static Mask* gen_mask(const Sprite* sprite, const Image* image, int xpos, int ypos);
static void mask_preview(const ContextReader& reader);

void dialogs_mask_color(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  Box* box1, *box2, *box3, *box4;
  Widget* label_color;
  Widget* label_tolerance;
  Button* button_ok;
  Button* button_cancel;

  if (!App::instance()->isGui() || !sprite)
    return;

  int xpos, ypos;
  const Image* image = reader.image(&xpos, &ypos);
  if (!image)
    return;

  base::UniquePtr<Window> window(new Window(false, "Mask by Color"));
  box1 = new Box(JI_VERTICAL);
  box2 = new Box(JI_HORIZONTAL);
  box3 = new Box(JI_HORIZONTAL);
  box4 = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_color = new Label("Color:");
  button_color = new ColorButton
   (get_config_color("MaskColor", "Color",
                     ColorBar::instance()->getFgColor()),
    sprite->getPixelFormat());
  label_tolerance = new Label("Tolerance:");
  slider_tolerance = new Slider(0, 255, get_config_int("MaskColor", "Tolerance", 0));
  check_preview = new CheckBox("&Preview");
  button_ok = new Button("&OK");
  button_cancel = new Button("&Cancel");

  if (get_config_bool("MaskColor", "Preview", true))
    check_preview->setSelected(true);

  button_ok->Click.connect(Bind<void>(&Window::closeWindow, window.get(), button_ok));
  button_cancel->Click.connect(Bind<void>(&Window::closeWindow, window.get(), button_cancel));

  button_color->Change.connect(Bind<void>(&mask_preview, Ref(reader)));
  slider_tolerance->Change.connect(Bind<void>(&mask_preview, Ref(reader)));
  check_preview->Click.connect(Bind<void>(&mask_preview, Ref(reader)));

  button_ok->setFocusMagnet(true);
  button_color->setExpansive(true);
  slider_tolerance->setExpansive(true);
  box2->setExpansive(true);

  window->addChild(box1);
  box1->addChild(box2);
  box1->addChild(box3);
  box1->addChild(check_preview);
  box1->addChild(box4);
  box2->addChild(label_color);
  box2->addChild(button_color);
  box3->addChild(label_tolerance);
  box3->addChild(slider_tolerance);
  box4->addChild(button_ok);
  box4->addChild(button_cancel);

  // Default position
  window->remapWindow();
  window->centerWindow();

  // Mask first preview
  mask_preview(reader);

  // Load window configuration
  load_window_pos(window, "MaskColor");

  // Open the window
  window->openWindowInForeground();

  bool apply = (window->getKiller() == button_ok);

  ContextWriter writer(reader);
  Document* document(writer.document());

  if (apply) {
    UndoTransaction undo(writer.context(), "Mask by Color", undo::DoesntModifyDocument);

    if (undo.isEnabled())
      undo.pushUndoer(new undoers::SetMask(undo.getObjects(), document));

    // Change the mask
    {
      base::UniquePtr<Mask> mask(gen_mask(sprite, image, xpos, ypos));
      document->setMask(mask);
    }

    undo.commit();

    set_config_color("MaskColor", "Color", button_color->getColor());
    set_config_int("MaskColor", "Tolerance", slider_tolerance->getValue());
    set_config_bool("MaskColor", "Preview", check_preview->isSelected());
  }

  // Update boundaries and editors.
  document->generateMaskBoundaries();
  update_screen_for_document(document);

  // Save window configuration.
  save_window_pos(window, "MaskColor");
}

static Mask* gen_mask(const Sprite* sprite, const Image* image, int xpos, int ypos)
{
  int color, tolerance;

  color = color_utils::color_for_image(button_color->getColor(), sprite->getPixelFormat());
  tolerance = slider_tolerance->getValue();

  base::UniquePtr<Mask> mask(new Mask());
  mask->byColor(image, color, tolerance);
  mask->offsetOrigin(xpos, ypos);

  return mask.release();
}

static void mask_preview(const ContextReader& reader)
{
  if (check_preview->isSelected()) {
    int xpos, ypos;
    const Image* image = reader.image(&xpos, &ypos);
    base::UniquePtr<Mask> mask(gen_mask(reader.sprite(), image, xpos, ypos));
    {
      ContextWriter writer(reader);
      writer.document()->generateMaskBoundaries(mask);
      update_screen_for_document(writer.document());
    }
  }
}

} // namespace app
