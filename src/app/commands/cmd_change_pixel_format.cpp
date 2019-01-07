// Aseprite
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
#include "render/dithering_algorithm.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"
#include "render/task_delegate.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#include "color_mode.xml.h"

namespace app {

using namespace ui;

namespace {

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
                const render::DitheringAlgorithm ditheringAlgorithm,
                const render::DitheringMatrix& ditheringMatrix,
                const gfx::Point& pos)
    : m_image(dstImage)
    , m_pos(pos)
    , m_running(true)
    , m_stopFlag(false)
    , m_progress(0.0)
    , m_thread(
      [this,
       sprite, frame,
       pixelFormat,
       ditheringAlgorithm,
       ditheringMatrix]() { // Copy the matrix
        run(sprite, frame,
            pixelFormat,
            ditheringAlgorithm,
            ditheringMatrix);
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
           const render::DitheringAlgorithm ditheringAlgorithm,
           const render::DitheringMatrix& ditheringMatrix) {
    doc::ImageRef tmp(
      Image::create(sprite->pixelFormat(),
                    m_image->width(),
                    m_image->height()));

    render::Render render;
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
      ditheringAlgorithm,
      ditheringMatrix,
      sprite->rgbMap(frame),
      sprite->palette(frame),
      (sprite->backgroundLayer() != nullptr),
      0,
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
      m_ditheringSelector->Change.connect(
        base::Bind<void>(&ColorModeWindow::onDithering, this));
      ditheringPlaceholder()->addChild(m_ditheringSelector);
    }
    if (from != IMAGE_GRAYSCALE)
      colorMode()->addChild(new ConversionItem(IMAGE_GRAYSCALE));

    colorModeView()->setMinSize(
      colorModeView()->sizeHint() +
      colorMode()->sizeHint());

    colorMode()->Change.connect(base::Bind<void>(&ColorModeWindow::onChangeColorMode, this));
    m_timer.Tick.connect(base::Bind<void>(&ColorModeWindow::onMonitorProgress, this));

    progress()->setReadOnly(true);

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

  render::DitheringAlgorithm ditheringAlgorithm() const {
    return (m_ditheringSelector ? m_ditheringSelector->ditheringAlgorithm():
                                  render::DitheringAlgorithm::None);
  }

  render::DitheringMatrix ditheringMatrix() const {
    return (m_ditheringSelector ? m_ditheringSelector->ditheringMatrix():
                                  render::BayerMatrix(8));
  }

  bool flattenEnabled() const {
    return flatten()->isSelected();
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

    if (m_ditheringSelector)
      m_ditheringSelector->setVisible(dstPixelFormat == doc::IMAGE_INDEXED);

    m_image.reset(
      Image::create(dstPixelFormat,
                    visibleBounds.w,
                    visibleBounds.h,
                    m_imageBuffer));

    m_editor->renderEngine().setPreviewImage(
      nullptr,
      m_editor->frame(),
      m_image.get(),
      visibleBounds.origin(),
      doc::BlendMode::SRC);

    m_image->clear(0);
    m_editor->invalidate();
    progress()->setValue(0);
    progress()->setVisible(true);
    layout();

    m_bgThread.reset(
      new ConvertThread(
        m_image,
        m_editor->sprite(),
        m_editor->frame(),
        dstPixelFormat,
        ditheringAlgorithm(),
        ditheringMatrix(),
        visibleBounds.origin()));

    m_timer.start();
  }

  void onDithering() {
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
    else
      progress()->setValue(100 * m_bgThread->progress());

    m_editor->invalidate();
  }

  Timer m_timer;
  Editor* m_editor;
  doc::ImageRef m_image;
  doc::ImageBufferPtr m_imageBuffer;
  std::unique_ptr<ConvertThread> m_bgThread;
  ConversionItem* m_selectedItem;
  DitheringSelector* m_ditheringSelector;
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
  render::DitheringAlgorithm m_ditheringAlgorithm;
  render::DitheringMatrix m_ditheringMatrix;
};

ChangePixelFormatCommand::ChangePixelFormatCommand()
  : Command(CommandId::ChangePixelFormat(), CmdUIOnlyFlag)
{
  m_useUI = true;
  m_format = IMAGE_RGB;
  m_ditheringAlgorithm = render::DitheringAlgorithm::None;
}

void ChangePixelFormatCommand::onLoadParams(const Params& params)
{
  m_useUI = false;

  std::string format = params.get("format");
  if (format == "rgb") m_format = IMAGE_RGB;
  else if (format == "grayscale") m_format = IMAGE_GRAYSCALE;
  else if (format == "indexed") m_format = IMAGE_INDEXED;
  else
    m_useUI = true;

  std::string dithering = params.get("dithering");
  if (dithering == "ordered")
    m_ditheringAlgorithm = render::DitheringAlgorithm::Ordered;
  else if (dithering == "old")
    m_ditheringAlgorithm = render::DitheringAlgorithm::Old;
  else
    m_ditheringAlgorithm = render::DitheringAlgorithm::None;

  std::string matrix = params.get("dithering-matrix");
  if (!matrix.empty()) {
    // Try to get the matrix from the extensions
    const render::DitheringMatrix* knownMatrix =
      App::instance()->extensions().ditheringMatrix(matrix);
    if (knownMatrix) {
      m_ditheringMatrix = *knownMatrix;
    }
    // Then, if the matrix doesn't exist we try to load it from a file
    else if (!load_dithering_matrix_from_sprite(matrix, m_ditheringMatrix)) {
      throw std::runtime_error("Invalid matrix name");
    }
  }
  // Default dithering matrix is BayerMatrix(8)
  else {
    // TODO object slicing here (from BayerMatrix -> DitheringMatrix)
    m_ditheringMatrix = render::BayerMatrix(8);
  }
}

bool ChangePixelFormatCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Sprite* sprite(writer.sprite());

  if (!sprite)
    return false;

  if (m_useUI)
    return true;

  if (sprite->pixelFormat() == IMAGE_INDEXED &&
      m_format == IMAGE_INDEXED &&
      m_ditheringAlgorithm != render::DitheringAlgorithm::None)
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
      m_ditheringAlgorithm != render::DitheringAlgorithm::None)
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
    m_ditheringAlgorithm = window.ditheringAlgorithm();
    m_ditheringMatrix = window.ditheringMatrix();
    flatten = window.flattenEnabled();
  }
#endif // ENABLE_UI

  // No conversion needed
  if (context->activeDocument()->sprite()->pixelFormat() == m_format)
    return;

  {
    const ContextReader reader(context);
    SpriteJob job(reader, "Color Mode Change");
    job.startJobWithCallback(
      [this, &job, flatten] {
        Sprite* sprite(job.sprite());

        if (flatten) {
          SelectedLayers selLayers;
          for (auto layer : sprite->root()->layers())
            selLayers.insert(layer);
          job.tx()(new cmd::FlattenLayers(sprite, selLayers));
        }

        job.tx()(
          new cmd::SetPixelFormat(
            sprite, m_format,
            m_ditheringAlgorithm,
            m_ditheringMatrix,
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
        switch (m_ditheringAlgorithm) {
          case render::DitheringAlgorithm::None:
            conversion = Strings::commands_ChangePixelFormat_Indexed();
            break;
          case render::DitheringAlgorithm::Ordered:
            conversion = Strings::commands_ChangePixelFormat_Indexed_OrderedDithering();
            break;
          case render::DitheringAlgorithm::Old:
            conversion = Strings::commands_ChangePixelFormat_Indexed_OldDithering();
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
