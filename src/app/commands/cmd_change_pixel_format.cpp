// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/flatten_layers.h"
#include "app/cmd/set_pixel_format.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/extensions.h"
#include "app/i18n/strings.h"
#include "app/load_matrix.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/sprite_job.h"
#include "app/transaction.h"
#include "app/ui/dithering_selector.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_render.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/thread.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "render/dithering.h"
#include "render/dithering_algorithm.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"
#include "render/task_delegate.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#include "color_mode.xml.h"
#include <string>

namespace app {

using namespace ui;

namespace {

rgba_to_graya_func get_gray_func(gen::ToGrayAlgorithm toGray) {
  switch (toGray) {
    case gen::ToGrayAlgorithm::LUMA: return &rgba_to_graya_using_luma;
    case gen::ToGrayAlgorithm::HSV: return &rgba_to_graya_using_hsv;
    case gen::ToGrayAlgorithm::HSL: return &rgba_to_graya_using_hsl;
  }
  return nullptr;
}

class ConversionItem : public ListItem {
public:
  ConversionItem(const doc::PixelFormat pixelFormat)
    : m_pixelFormat(pixelFormat) {
    switch (pixelFormat) {
      case IMAGE_RGB:
        setText("-> RGB");
        break;
      case IMAGE_GRAYSCALE:
        setText("-> Grayscale");
        break;
      case IMAGE_INDEXED:
        setText("-> Indexed");
        break;
    }
  }
  doc::PixelFormat pixelFormat() const { return m_pixelFormat; }
private:
  doc::PixelFormat m_pixelFormat;
};

class ConvertThread : public render::TaskDelegate {
public:
  ConvertThread(const doc::ImageRef& dstImage,
                const doc::Sprite* sprite,
                const doc::frame_t frame,
                const doc::PixelFormat pixelFormat,
                const render::Dithering& dithering,
                const gen::ToGrayAlgorithm toGray,
                const gfx::Point& pos,
                const bool newBlend)
    : m_image(dstImage)
    , m_pos(pos)
    , m_running(true)
    , m_stopFlag(false)
    , m_progress(0.0)
    , m_thread(
      [this,
       sprite, frame,
       pixelFormat,
       dithering,
       toGray,
       newBlend]() { // Copy the matrix
        run(sprite, frame,
            pixelFormat,
            dithering,
            toGray,
            newBlend);
      })
  {
  }

  void stop() {
    m_stopFlag = true;
    m_thread.join();
  }

  bool isRunning() const {
    return m_running;
  }

  double progress() const {
    return m_progress;
  }

private:
  void run(const Sprite* sprite,
           const doc::frame_t frame,
           const doc::PixelFormat pixelFormat,
           const render::Dithering& dithering,
           const gen::ToGrayAlgorithm toGray,
           const bool newBlend) {
    doc::ImageRef tmp(
      Image::create(sprite->pixelFormat(),
                    m_image->width(),
                    m_image->height()));

    render::Render render;
    render.setNewBlend(newBlend);
    render.renderSprite(
      tmp.get(), sprite, frame,
      gfx::Clip(0, 0,
                m_pos.x, m_pos.y,
                m_image->width(),
                m_image->height()));

    render::convert_pixel_format(
      tmp.get(),
      m_image.get(),
      pixelFormat,
      dithering,
      sprite->rgbMap(frame),
      sprite->palette(frame),
      (sprite->backgroundLayer() != nullptr),
      0,
      get_gray_func(toGray),
      this);

    m_running = false;
  }

private:
  // render::TaskDelegate impl
  bool continueTask() override {
    return !m_stopFlag;
  }

  void notifyTaskProgress(double progress) override {
    m_progress = progress;
  }

  doc::ImageRef m_image;
  gfx::Point m_pos;
  bool m_running;
  bool m_stopFlag;
  double m_progress;
  base::thread m_thread;
};

class ColorModeWindow : public app::gen::ColorMode {
public:
  ColorModeWindow(Editor* editor)
    : m_timer(100)
    , m_editor(editor)
    , m_image(nullptr)
    , m_imageBuffer(new doc::ImageBuffer)
    , m_selectedItem(nullptr)
    , m_ditheringSelector(nullptr)
    , m_imageJustCreated(true)
  {
    doc::PixelFormat from = m_editor->sprite()->pixelFormat();

    // Add the color mode in the window title
    switch (from) {
      case IMAGE_RGB: setText(text() + ": RGB"); break;
      case IMAGE_GRAYSCALE: setText(text() + ": Grayscale"); break;
      case IMAGE_INDEXED: setText(text() + ": Indexed"); break;
    }

    // Add conversion items
    if (from != IMAGE_RGB)
      colorMode()->addChild(new ConversionItem(IMAGE_RGB));
    if (from != IMAGE_INDEXED) {
      colorMode()->addChild(new ConversionItem(IMAGE_INDEXED));

      m_ditheringSelector = new DitheringSelector(DitheringSelector::SelectBoth);
      m_ditheringSelector->setExpansive(true);

      // Select default dithering method
      {
        int index = m_ditheringSelector->findItemIndex(
          Preferences::instance().quantization.ditheringAlgorithm());
        if (index >= 0)
          m_ditheringSelector->setSelectedItemIndex(index);
      }

      m_ditheringSelector->Change.connect(
        base::Bind<void>(&ColorModeWindow::onDithering, this));
      ditheringPlaceholder()->addChild(m_ditheringSelector);

      factor()->Change.connect(base::Bind<void>(&ColorModeWindow::onDithering, this));
    }
    else {
      amount()->setVisible(false);
    }
    if (from != IMAGE_GRAYSCALE) {
      colorMode()->addChild(new ConversionItem(IMAGE_GRAYSCALE));

      toGrayCombobox()->Change.connect(base::Bind<void>(&ColorModeWindow::onToGrayChange, this));
    }

    colorModeView()->setMinSize(
      colorModeView()->sizeHint() +
      colorMode()->sizeHint());

    colorMode()->Change.connect(base::Bind<void>(&ColorModeWindow::onChangeColorMode, this));
    m_timer.Tick.connect(base::Bind<void>(&ColorModeWindow::onMonitorProgress, this));

    progress()->setReadOnly(true);

    // Default dithering factor
    factor()->setValue(Preferences::instance().quantization.ditheringFactor());

    // Select first option
    colorMode()->selectIndex(0);
  }

  ~ColorModeWindow() {
    stop();
  }

  doc::PixelFormat pixelFormat() const {
    ASSERT(m_selectedItem);
    return m_selectedItem->pixelFormat();
  }

  render::Dithering dithering() const {
    render::Dithering d;
    if (m_ditheringSelector) {
      d.algorithm(m_ditheringSelector->ditheringAlgorithm());
      d.matrix(m_ditheringSelector->ditheringMatrix());
    }
    d.factor(double(factor()->getValue()) / 100.0);
    return d;
  }

  gen::ToGrayAlgorithm toGray() const {
    static_assert(
      int(gen::ToGrayAlgorithm::LUMA) == 0 &&
      int(gen::ToGrayAlgorithm::HSV) == 1 &&
      int(gen::ToGrayAlgorithm::HSL) == 2,
      "Check that 'to_gray_combobox' combobox items matches these indexes in color_mode.xml");
    return (gen::ToGrayAlgorithm)toGrayCombobox()->getSelectedItemIndex();
  }

  bool flattenEnabled() const {
    return flatten()->isSelected();
  }

  // Save the dithering method used for the future
  void saveDitheringOptions() {
    if (m_ditheringSelector) {
      if (auto item = m_ditheringSelector->getSelectedItem()) {
        Preferences::instance().quantization.ditheringAlgorithm(
          item->text());

        if (m_ditheringSelector->ditheringAlgorithm() ==
            render::DitheringAlgorithm::ErrorDiffusion) {
          Preferences::instance().quantization.ditheringFactor(
            factor()->getValue());
        }
      }
    }
  }

private:

  void stop() {
    m_editor->renderEngine().removePreviewImage();
    m_editor->invalidate();

    m_timer.stop();
    if (m_bgThread) {
      m_bgThread->stop();
      m_bgThread.reset(nullptr);
    }
  }

  void onChangeColorMode() {
    ConversionItem* item =
      static_cast<ConversionItem*>(colorMode()->getSelectedChild());
    if (item == m_selectedItem) // Avoid restarting the conversion process for the same option
      return;
    m_selectedItem = item;

    stop();

    gfx::Rect visibleBounds = m_editor->getVisibleSpriteBounds();
    if (visibleBounds.isEmpty())
      return;

    doc::PixelFormat dstPixelFormat = item->pixelFormat();

    if (m_ditheringSelector) {
      const bool toIndexed = (dstPixelFormat == doc::IMAGE_INDEXED);
      m_ditheringSelector->setVisible(toIndexed);

      const bool errorDiff =
        (m_ditheringSelector->ditheringAlgorithm() ==
         render::DitheringAlgorithm::ErrorDiffusion);
      amount()->setVisible(toIndexed && errorDiff);
    }

    {
      const bool toGray = (dstPixelFormat == doc::IMAGE_GRAYSCALE);
      toGrayCombobox()->setVisible(toGray);
    }

    m_image.reset(
      Image::create(dstPixelFormat,
                    visibleBounds.w,
                    visibleBounds.h,
                    m_imageBuffer));
    if (m_imageJustCreated) {
      m_imageJustCreated = false;
      m_image->clear(0);
    }

    m_editor->renderEngine().setPreviewImage(
      nullptr,
      m_editor->frame(),
      m_image.get(),
      visibleBounds.origin(),
      doc::BlendMode::SRC);

    m_editor->invalidate();
    progress()->setValue(0);
    progress()->setVisible(false);
    layout();

    m_bgThread.reset(
      new ConvertThread(
        m_image,
        m_editor->sprite(),
        m_editor->frame(),
        dstPixelFormat,
        dithering(),
        toGray(),
        visibleBounds.origin(),
        Preferences::instance().experimental.newBlend()));

    m_timer.start();
  }

  void onDithering() {
    stop();
    m_selectedItem = nullptr;
    onChangeColorMode();
  }

  void onToGrayChange() {
    stop();
    m_selectedItem = nullptr;
    onChangeColorMode();
  }

  void onMonitorProgress() {
    ASSERT(m_bgThread);
    if (!m_bgThread)
      return;

    if (!m_bgThread->isRunning()) {
      m_timer.stop();
      m_bgThread->stop();
      m_bgThread.reset(nullptr);

      progress()->setVisible(false);
      layout();
    }
    else {
      int v = int(100 * m_bgThread->progress());
      if (v > 0) {
        progress()->setValue(v);
        if (!progress()->isVisible()) {
          progress()->setVisible(true);
          layout();
        }
      }
    }

    m_editor->invalidate();
  }

  Timer m_timer;
  Editor* m_editor;
  doc::ImageRef m_image;
  doc::ImageBufferPtr m_imageBuffer;
  std::unique_ptr<ConvertThread> m_bgThread;
  ConversionItem* m_selectedItem;
  DitheringSelector* m_ditheringSelector;
  bool m_imageJustCreated;
};

} // anonymous namespace

class ChangePixelFormatCommand : public Command {
public:
  ChangePixelFormatCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  bool m_useUI;
  doc::PixelFormat m_format;
  render::Dithering m_dithering;
  gen::ToGrayAlgorithm m_toGray;
};

ChangePixelFormatCommand::ChangePixelFormatCommand()
  : Command(CommandId::ChangePixelFormat(), CmdUIOnlyFlag)
{
  m_useUI = true;
  m_format = IMAGE_RGB;
  m_dithering = render::Dithering();
  m_toGray = gen::ToGrayAlgorithm::DEFAULT;
}

void ChangePixelFormatCommand::onLoadParams(const Params& params)
{
  m_useUI = false;

  std::string format = params.get("format");
  if (format == "rgb") m_format = IMAGE_RGB;
  else if (format == "grayscale" ||
           format == "gray") m_format = IMAGE_GRAYSCALE;
  else if (format == "indexed") m_format = IMAGE_INDEXED;
  else
    m_useUI = true;

  std::string dithering = params.get("dithering");
  if (dithering == "ordered")
    m_dithering.algorithm(render::DitheringAlgorithm::Ordered);
  else if (dithering == "old")
    m_dithering.algorithm(render::DitheringAlgorithm::Old);
  else if (dithering == "error-diffusion")
    m_dithering.algorithm(render::DitheringAlgorithm::ErrorDiffusion);
  else
    m_dithering.algorithm(render::DitheringAlgorithm::None);

  std::string matrix = params.get("dithering-matrix");
  if (!matrix.empty()) {
    // Try to get the matrix from the extensions
    const render::DitheringMatrix* knownMatrix =
      App::instance()->extensions().ditheringMatrix(matrix);
    if (knownMatrix) {
      m_dithering.matrix(*knownMatrix);
    }
    // Then, if the matrix doesn't exist we try to load it from a file
    else {
      render::DitheringMatrix ditMatrix;
      if (!load_dithering_matrix_from_sprite(matrix, ditMatrix))
        throw std::runtime_error("Invalid matrix name");
      m_dithering.matrix(ditMatrix);
    }
  }
  // Default dithering matrix is BayerMatrix(8)
  else {
    // TODO object slicing here (from BayerMatrix -> DitheringMatrix)
    m_dithering.matrix(render::BayerMatrix(8));
  }

  std::string toGray = params.get("toGray");
  if (toGray == "luma")
    m_toGray = gen::ToGrayAlgorithm::LUMA;
  else if (dithering == "hsv")
    m_toGray = gen::ToGrayAlgorithm::HSV;
  else if (dithering == "hsl")
    m_toGray = gen::ToGrayAlgorithm::HSL;
  else
    m_toGray = gen::ToGrayAlgorithm::DEFAULT;
}

bool ChangePixelFormatCommand::onEnabled(Context* context)
{
  if (!context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasActiveSprite))
    return false;

  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());

  if (!sprite)
    return false;

  if (m_useUI)
    return true;

  if (sprite->pixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_dithering.algorithm() != render::DitheringAlgorithm::None)
    return false;

  return true;
}

bool ChangePixelFormatCommand::onChecked(Context* context)
{
  if (m_useUI)
    return false;

  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();

  if (sprite &&
      sprite->pixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_dithering.algorithm() != render::DitheringAlgorithm::None)
    return false;

  return
    (sprite &&
     sprite->pixelFormat() == m_format);
}

void ChangePixelFormatCommand::onExecute(Context* context)
{
  bool flatten = false;

#ifdef ENABLE_UI
  if (m_useUI) {
    ColorModeWindow window(current_editor);

    window.remapWindow();
    window.centerWindow();

    load_window_pos(&window, "ChangePixelFormat");
    window.openWindowInForeground();
    save_window_pos(&window, "ChangePixelFormat");

    if (window.closer() != window.ok())
      return;

    m_format = window.pixelFormat();
    m_dithering = window.dithering();
    m_toGray = window.toGray();
    flatten = window.flattenEnabled();

    window.saveDitheringOptions();
  }
#endif // ENABLE_UI

  // No conversion needed
  if (context->activeDocument()->sprite()->pixelFormat() == m_format)
    return;

  {
    const ContextReader reader(context);
    SpriteJob job(reader, "Color Mode Change");
    Sprite* sprite(job.sprite());

    // TODO this was moved in the main UI thread because
    //      cmd::FlattenLayers() generates a EditorObserver::onAfterLayerChanged()
    //      event, and that event is an UI event.
    //      We should refactor the whole app to separate doc changes <-> UI changes,
    //      but that is for the future:
    //      https://github.com/aseprite/aseprite/issues/509
    //      https://github.com/aseprite/aseprite/issues/378
    if (flatten) {
      const bool newBlend = Preferences::instance().experimental.newBlend();
      SelectedLayers selLayers;
      for (auto layer : sprite->root()->layers())
        selLayers.insert(layer);
      job.tx()(new cmd::FlattenLayers(sprite, selLayers, newBlend));
    }

    job.startJobWithCallback(
      [this, &job, sprite] {
        job.tx()(
          new cmd::SetPixelFormat(
            sprite, m_format,
            m_dithering,
            get_gray_func(m_toGray),
            &job));             // SpriteJob is a render::TaskDelegate
      });
    job.waitJob();
  }

  if (context->isUIAvailable())
    app_refresh_screen();
}

std::string ChangePixelFormatCommand::onGetFriendlyName() const
{
  std::string conversion;

  if (!m_useUI) {
    switch (m_format) {
      case IMAGE_RGB:
        conversion = Strings::commands_ChangePixelFormat_RGB();
        break;
      case IMAGE_GRAYSCALE:
        conversion = Strings::commands_ChangePixelFormat_Grayscale();
        break;
      case IMAGE_INDEXED:
        switch (m_dithering.algorithm()) {
          case render::DitheringAlgorithm::None:
            conversion = Strings::commands_ChangePixelFormat_Indexed();
            break;
          case render::DitheringAlgorithm::Ordered:
            conversion = Strings::commands_ChangePixelFormat_Indexed_OrderedDithering();
            break;
          case render::DitheringAlgorithm::Old:
            conversion = Strings::commands_ChangePixelFormat_Indexed_OldDithering();
            break;
          case render::DitheringAlgorithm::ErrorDiffusion:
            conversion = Strings::commands_ChangePixelFormat_Indexed_ErrorDifussion();
            break;
        }
        break;
    }
  }
  else
    conversion = Strings::commands_ChangePixelFormat_MoreOptions();

  return fmt::format(getBaseFriendlyName(), conversion);
}

Command* CommandFactory::createChangePixelFormatCommand()
{
  return new ChangePixelFormatCommand;
}

} // namespace app
