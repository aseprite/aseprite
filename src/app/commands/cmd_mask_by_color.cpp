// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
#include "app/ui/selection_mode_field.h"
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
#include "ui/tooltips.h"
#include "ui/widget.h"
#include "ui/window.h"

// Uncomment to see the performance of doc::MaskBoundaries ctor
//#define SHOW_BOUNDARIES_GEN_PERFORMANCE

namespace app {

using namespace ui;

class MaskByColorCommand : public Command {
public:
  MaskByColorCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  Mask* generateMask(const Mask& origMask,
                     const Sprite* sprite,
                     const Image* image,
                     int xpos, int ypos,
                     gen::SelectionMode mode);
  void maskPreview(const ContextReader& reader);

  class SelModeField : public SelectionModeField {
  public:
    obs::signal<void()> ModeChange;
  protected:
    void onSelectionModeChange(gen::SelectionMode mode) override {
      ModeChange();
    }
  };

  Window* m_window; // TODO we cannot use a std::unique_ptr because clone() needs a copy ctor
  ColorButton* m_buttonColor;
  CheckBox* m_checkPreview;
  Slider* m_sliderTolerance;
  SelModeField* m_selMode;
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

  if (!App::instance()->isGui() || !sprite)
    return;

  int xpos, ypos;
  const Image* image = reader.image(&xpos, &ypos);
  if (!image)
    return;

  m_window = new Window(Window::WithTitleBar, "Mask by Color");
  TooltipManager* tooltipManager = new TooltipManager();
  m_window->addChild(tooltipManager);
  auto box1 = new Box(VERTICAL);
  auto box2 = new Box(HORIZONTAL);
  auto box3 = new Box(HORIZONTAL);
  auto box4 = new Box(HORIZONTAL | HOMOGENEOUS);
  auto label_color = new Label("Color:");
  m_buttonColor = new ColorButton
    (get_config_color("MaskColor", "Color",
                      ColorBar::instance()->getFgColor()),
     sprite->pixelFormat(),
     ColorButtonOptions());
  auto label_tolerance = new Label("Tolerance:");
  m_sliderTolerance = new Slider(0, 255, get_config_int("MaskColor", "Tolerance", 0));

  m_selMode = new SelModeField;
  m_selMode->setupTooltips(tooltipManager);

  m_checkPreview = new CheckBox("&Preview");
  auto button_ok = new Button("&OK");
  auto button_cancel = new Button("&Cancel");

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
  m_selMode->ModeChange.connect(base::Bind<void>(&MaskByColorCommand::maskPreview, this, base::Ref(reader)));

  button_ok->setFocusMagnet(true);
  m_buttonColor->setExpansive(true);
  m_sliderTolerance->setExpansive(true);
  box2->setExpansive(true);

  m_window->addChild(box1);
  box1->addChild(m_selMode);
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
    std::unique_ptr<Mask> mask(generateMask(*document->mask(),
                                            sprite, image, xpos, ypos,
                                            m_selMode->selectionMode()));
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

Mask* MaskByColorCommand::generateMask(const Mask& origMask,
                                       const Sprite* sprite,
                                       const Image* image,
                                       int xpos, int ypos,
                                       gen::SelectionMode mode)
{
  int color = color_utils::color_for_image(m_buttonColor->getColor(), sprite->pixelFormat());
  int tolerance = m_sliderTolerance->getValue();

  std::unique_ptr<Mask> mask(new Mask());
  mask->byColor(image, color, tolerance);
  mask->offsetOrigin(xpos, ypos);

  if (!origMask.isEmpty()) {
    switch (mode) {
      case gen::SelectionMode::DEFAULT:
        break;
      case gen::SelectionMode::ADD:
        mask->add(origMask);
        break;
      case gen::SelectionMode::SUBTRACT: {
        if (!mask->isEmpty()) {
          Mask mask2(origMask);
          mask2.subtract(*mask);
          mask->replace(mask2);
        }
        else {
          mask->replace(origMask);
        }
        break;
      }
      case gen::SelectionMode::INTERSECT: {
        mask->intersect(origMask);
        break;
      }
    }
  }

  return mask.release();
}

void MaskByColorCommand::maskPreview(const ContextReader& reader)
{
  if (m_checkPreview->isSelected()) {
    int xpos, ypos;
    const Image* image = reader.image(&xpos, &ypos);
    std::unique_ptr<Mask> mask(generateMask(*reader.document()->mask(),
                                            reader.sprite(), image,
                                            xpos, ypos,
                                            m_selMode->selectionMode()));
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
