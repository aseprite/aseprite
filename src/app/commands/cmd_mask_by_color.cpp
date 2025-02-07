// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
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
#include "app/commands/new_params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "app/ui/selection_mode_field.h"
#include "base/chrono.h"
#include "base/convert_to.h"
#include "base/scoped_value.h"
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
// #define SHOW_BOUNDARIES_GEN_PERFORMANCE

namespace app {

using namespace ui;

static const char* ConfigSection = "MaskColor";

struct MaskByColorParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<app::Color> color{ this, app::Color(), "color" };
  Param<int> tolerance{ this, 0, "tolerance" };
  Param<gen::SelectionMode> mode{ this, gen::SelectionMode::DEFAULT, "mode" };
};

class MaskByColorWindow : public ui::Window {
public:
  MaskByColorWindow(MaskByColorParams& params, const ContextReader& reader)
    : Window(Window::WithTitleBar, Strings::mask_by_color_title())
    , m_reader(&reader)
    // Save original mask visibility to process it correctly in
    // ADD/SUBTRACT/INTERSECT Selection Mode
    , m_isOrigMaskVisible(reader.document()->isMaskVisible())
  {
    TooltipManager* tooltipManager = new TooltipManager();
    addChild(tooltipManager);
    auto box1 = new Box(VERTICAL);
    auto box2 = new Box(HORIZONTAL);
    auto box3 = new Box(HORIZONTAL);
    auto box4 = new Box(HORIZONTAL | HOMOGENEOUS);
    auto label_color = new Label(Strings::mask_by_color_label_color());
    m_buttonColor =
      new ColorButton(params.color(), reader.sprite()->pixelFormat(), ColorButtonOptions());
    auto label_tolerance = new Label(Strings::mask_by_color_tolerance());
    m_sliderTolerance = new Slider(0, 255, params.tolerance());

    m_selMode = new SelModeField;
    m_selMode->setSelectionMode(params.mode());
    m_selMode->setupTooltips(tooltipManager);

    m_checkPreview = new CheckBox(Strings::mask_by_color_preview());
    m_buttonOk = new Button(Strings::mask_by_color_ok());
    auto button_cancel = new Button(Strings::mask_by_color_cancel());

    m_checkPreview->processMnemonicFromText();
    m_buttonOk->processMnemonicFromText();
    button_cancel->processMnemonicFromText();

    if (get_config_bool(ConfigSection, "Preview", true))
      m_checkPreview->setSelected(true);

    m_buttonOk->Click.connect([this] { closeWindow(m_buttonOk); });
    button_cancel->Click.connect([this, button_cancel] { closeWindow(button_cancel); });

    m_buttonColor->Change.connect([&] { maskPreview(); });
    m_sliderTolerance->Change.connect([&] { maskPreview(); });
    m_checkPreview->Click.connect([&] { maskPreview(); });
    m_selMode->ModeChange.connect([&] { maskPreview(); });

    m_buttonOk->setFocusMagnet(true);
    m_buttonColor->setExpansive(true);
    m_sliderTolerance->setExpansive(true);
    box2->setExpansive(true);

    addChild(box1);
    box1->addChild(m_selMode);
    box1->addChild(box2);
    box1->addChild(box3);
    box1->addChild(m_checkPreview);
    box1->addChild(box4);
    box2->addChild(label_color);
    box2->addChild(m_buttonColor);
    box3->addChild(label_tolerance);
    box3->addChild(m_sliderTolerance);
    box4->addChild(m_buttonOk);
    box4->addChild(button_cancel);

    // Default position
    remapWindow();
    centerWindow();

    // Mask first preview
    maskPreview();
  }

  bool accepted() const { return closer() == m_buttonOk; }

  app::Color getColor() const { return m_buttonColor->getColor(); }

  int getTolerance() const { return m_sliderTolerance->getValue(); }

  gen::SelectionMode getSelectionMode() const { return m_selMode->selectionMode(); }

  bool isPreviewChecked() const { return m_checkPreview->isSelected(); }

private:
  class SelModeField : public SelectionModeField {
  public:
    obs::signal<void()> ModeChange;

  protected:
    void onSelectionModeChange(gen::SelectionMode mode) override { ModeChange(); }
  };

  void maskPreview();

  const ContextReader* m_reader = nullptr;
  bool m_isOrigMaskVisible;
  Button* m_buttonOk = nullptr;
  ColorButton* m_buttonColor = nullptr;
  CheckBox* m_checkPreview = nullptr;
  Slider* m_sliderTolerance = nullptr;
  SelModeField* m_selMode = nullptr;
};

static Mask* generateMask(const Mask& origMask,
                          bool isOrigMaskVisible,
                          const Image* image,
                          int xpos,
                          int ypos,
                          gen::SelectionMode mode,
                          int color,
                          int tolerance)
{
  std::unique_ptr<Mask> mask(new Mask());
  mask->byColor(image, color, tolerance);
  mask->offsetOrigin(xpos, ypos);

  if (!origMask.isEmpty() && isOrigMaskVisible) {
    switch (mode) {
      case gen::SelectionMode::DEFAULT:  break;
      case gen::SelectionMode::ADD:      mask->add(origMask); break;
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

class MaskByColorCommand : public CommandWithNewParams<MaskByColorParams> {
public:
  MaskByColorCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

MaskByColorCommand::MaskByColorCommand()
  : CommandWithNewParams(CommandId::MaskByColor(), CmdUIOnlyFlag)
{
}

bool MaskByColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite | ContextFlags::HasActiveImage);
}

void MaskByColorCommand::onExecute(Context* context)
{
  const bool ui = (params().ui() && context->isUIAvailable());
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();

  if (!sprite)
    return;

  int xpos, ypos;
  const Image* image = reader.image(&xpos, &ypos);
  if (!image)
    return;

  // Save original mask visibility to process it correctly in
  // ADD/SUBTRACT/INTERSECT Selection Mode
  bool isOrigMaskVisible = reader.document()->isMaskVisible();

  bool apply = true;
  auto& params = this->params();

  // If UI is available, set parameters default values from the UI/configuration
  if (context->isUIAvailable()) {
    if (!params.color.isSet())
      params.color(ColorBar::instance()->getFgColor());

    if (!params.tolerance.isSet())
      params.tolerance(get_config_int(ConfigSection, "Tolerance", 0));

    if (!params.mode.isSet())
      params.mode(Preferences::instance().selection.mode());
  }

  if (ui) {
    MaskByColorWindow window(params, reader);

    // Load window configuration
    load_window_pos(&window, ConfigSection);

    // Open the window
    window.openWindowInForeground();

    // Save window configuration.
    save_window_pos(&window, ConfigSection);

    apply = window.accepted();
    if (apply) {
      params.color(window.getColor());
      params.mode(window.getSelectionMode());
      params.tolerance(window.getTolerance());

      set_config_int(ConfigSection, "Tolerance", params.tolerance());
      set_config_bool(ConfigSection, "Preview", window.isPreviewChecked());
    }
  }

  ContextWriter writer(reader);
  Doc* document(writer.document());

  if (apply) {
    int color = color_utils::color_for_image(params.color(), sprite->pixelFormat());

    Tx tx(writer, "Mask by Color", DoesntModifyDocument);
    std::unique_ptr<Mask> mask(generateMask(*document->mask(),
                                            isOrigMaskVisible,
                                            image,
                                            xpos,
                                            ypos,
                                            params.mode(),
                                            color,
                                            params.tolerance()));
    tx(new cmd::SetMask(document, mask.get()));
    tx.commit();
  }
  else {
    document->generateMaskBoundaries();
  }

  // Update boundaries and editors.
  update_screen_for_document(document);
}

void MaskByColorWindow::maskPreview()
{
  if (isPreviewChecked()) {
    int xpos, ypos;
    const Image* image = m_reader->image(&xpos, &ypos);
    int color = color_utils::color_for_image(m_buttonColor->getColor(),
                                             m_reader->sprite()->pixelFormat());
    int tolerance = m_sliderTolerance->getValue();
    std::unique_ptr<Mask> mask(generateMask(*m_reader->document()->mask(),
                                            m_isOrigMaskVisible,
                                            image,
                                            xpos,
                                            ypos,
                                            m_selMode->selectionMode(),
                                            color,
                                            tolerance));

    ContextWriter writer(*m_reader);

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

Command* CommandFactory::createMaskByColorCommand()
{
  return new MaskByColorCommand;
}

} // namespace app
