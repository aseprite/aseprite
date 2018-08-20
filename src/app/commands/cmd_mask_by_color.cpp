// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_mask.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "base/bind.h"
#include "base/chrono.h"
#include "base/convert_to.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/slider.h"
#include "ui/widget.h"
#include "ui/window.h"

// Uncomment to see the performance of doc::MaskBoundaries ctor
//#define SHOW_BOUNDARIES_GEN_PERFORMANCE

namespace app {

using namespace ui;

class MaskByColorCommand : public Command {
public:
  MaskByColorCommand();
  Command* clone() const override { return new MaskByColorCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  Mask* generateMask(const Sprite* sprite, const Image* image, int xpos, int ypos);
  void maskPreview(const ContextReader& reader);

  Window* m_window; // TODO we cannot use a std::unique_ptr because clone() needs a copy ctor
  ColorButton* m_buttonColor;
  CheckBox* m_checkPreview;
  Slider* m_sliderTolerance;
};

MaskByColorCommand::MaskByColorCommand()
  : Command(CommandId::MaskByColor(), CmdUIOnlyFlag)
{
}

bool MaskByColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveImage);
}

void MaskByColorCommand::onExecute(Context* context)
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

  m_window = new Window(Window::WithTitleBar, "Mask by Color");
  box1 = new Box(VERTICAL);
  box2 = new Box(HORIZONTAL);
  box3 = new Box(HORIZONTAL);
  box4 = new Box(HORIZONTAL | HOMOGENEOUS);
  label_color = new Label("Color:");
  m_buttonColor = new ColorButton
    (get_config_color("MaskColor", "Color",
                      ColorBar::instance()->getFgColor()),
     sprite->pixelFormat(),
     ColorButtonOptions());
  label_tolerance = new Label("Tolerance:");
  m_sliderTolerance = new Slider(0, 255, get_config_int("MaskColor", "Tolerance", 0));
  m_checkPreview = new CheckBox("&Preview");
  button_ok = new Button("&OK");
  button_cancel = new Button("&Cancel");

  m_checkPreview->processMnemonicFromText();
  button_ok->processMnemonicFromText();
  button_cancel->processMnemonicFromText();

  if (get_config_bool("MaskColor", "Preview", true))
    m_checkPreview->setSelected(true);

  button_ok->Click.connect(base::Bind<void>(&Window::closeWindow, m_window, button_ok));
  button_cancel->Click.connect(base::Bind<void>(&Window::closeWindow, m_window, button_cancel));


  m_buttonColor->Change.connect(base::Bind<void>(&MaskByColorCommand::maskPreview, this, base::Ref(reader)));
  m_sliderTolerance->Change.connect(base::Bind<void>(&MaskByColorCommand::maskPreview, this, base::Ref(reader)));
  m_checkPreview->Click.connect(base::Bind<void>(&MaskByColorCommand::maskPreview, this, base::Ref(reader)));

  button_ok->setFocusMagnet(true);
  m_buttonColor->setExpansive(true);
  m_sliderTolerance->setExpansive(true);
  box2->setExpansive(true);

  m_window->addChild(box1);
  box1->addChild(box2);
  box1->addChild(box3);
  box1->addChild(m_checkPreview);
  box1->addChild(box4);
  box2->addChild(label_color);
  box2->addChild(m_buttonColor);
  box3->addChild(label_tolerance);
  box3->addChild(m_sliderTolerance);
  box4->addChild(button_ok);
  box4->addChild(button_cancel);

  // Default position
  m_window->remapWindow();
  m_window->centerWindow();

  // Mask first preview
  maskPreview(reader);

  // Load window configuration
  load_window_pos(m_window, "MaskColor");

  // Open the window
  m_window->openWindowInForeground();

  bool apply = (m_window->closer() == button_ok);

  ContextWriter writer(reader);
  Doc* document(writer.document());

  if (apply) {
    Tx tx(writer.context(), "Mask by Color", DoesntModifyDocument);
    std::unique_ptr<Mask> mask(generateMask(sprite, image, xpos, ypos));
    tx(new cmd::SetMask(document, mask.get()));
    tx.commit();

    set_config_color("MaskColor", "Color", m_buttonColor->getColor());
    set_config_int("MaskColor", "Tolerance", m_sliderTolerance->getValue());
    set_config_bool("MaskColor", "Preview", m_checkPreview->isSelected());
  }

  // Update boundaries and editors.
  document->generateMaskBoundaries();
  update_screen_for_document(document);

  // Save window configuration.
  save_window_pos(m_window, "MaskColor");
  delete m_window;
}

Mask* MaskByColorCommand::generateMask(const Sprite* sprite, const Image* image, int xpos, int ypos)
{
  int color, tolerance;

  color = color_utils::color_for_image(m_buttonColor->getColor(), sprite->pixelFormat());
  tolerance = m_sliderTolerance->getValue();

  std::unique_ptr<Mask> mask(new Mask());
  mask->byColor(image, color, tolerance);
  mask->offsetOrigin(xpos, ypos);

  return mask.release();
}

void MaskByColorCommand::maskPreview(const ContextReader& reader)
{
  if (m_checkPreview->isSelected()) {
    int xpos, ypos;
    const Image* image = reader.image(&xpos, &ypos);
    std::unique_ptr<Mask> mask(generateMask(reader.sprite(), image, xpos, ypos));
    {
      ContextWriter writer(reader);

#ifdef SHOW_BOUNDARIES_GEN_PERFORMANCE
      base::Chrono chrono;
#endif

      writer.document()->generateMaskBoundaries(mask.get());

#ifdef SHOW_BOUNDARIES_GEN_PERFORMANCE
      double time = chrono.elapsed();
      m_window->setText("Mask by Color (" + base::convert_to<std::string>(time) + ")");
#endif

      update_screen_for_document(writer.document());
    }
  }
}

Command* CommandFactory::createMaskByColorCommand()
{
  return new MaskByColorCommand;
}

} // namespace app
