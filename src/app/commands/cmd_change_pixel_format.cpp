// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/thread.h"
#include "doc/image.h"
#include "doc/sprite.h"
#include "render/dithering_algorithm.h"
#include "render/quantization.h"
#include "render/render.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#include "color_mode.xml.h"

namespace app {

using namespace ui;

namespace {

class ConversionItem : public ListItem {
public:
  ConversionItem(const doc::PixelFormat pixelFormat,
                 const render::DitheringAlgorithm dithering = render::DitheringAlgorithm::None)
    : m_pixelFormat(pixelFormat)
    , m_dithering(dithering) {
    switch (pixelFormat) {
      case IMAGE_RGB:
        setText("-> RGB");
        break;
      case IMAGE_GRAYSCALE:
        setText("-> Grayscale");
        break;
      case IMAGE_INDEXED:
        switch (m_dithering) {
          case render::DitheringAlgorithm::None:
            setText("-> Indexed");
            break;
          case render::DitheringAlgorithm::OldOrdered:
            setText("-> Indexed w/Old Ordered Dithering");
            break;
          case render::DitheringAlgorithm::Ordered:
            setText("-> Indexed w/Ordered Dithering");
            break;
        }
        break;
    }
  }

  doc::PixelFormat pixelFormat() const { return m_pixelFormat; }
  render::DitheringAlgorithm ditheringAlgorithm() const { return m_dithering; }

private:
  doc::PixelFormat m_pixelFormat;
  render::DitheringAlgorithm m_dithering;
};

class ConvertThread {
public:
  ConvertThread(const doc::ImageRef& dstImage,
                const doc::Sprite* sprite,
                const doc::frame_t frame,
                const doc::PixelFormat pixelFormat,
                const render::DitheringAlgorithm ditheringAlgorithm)
    : m_image(dstImage)
    , m_running(true)
    , m_stopFlag(false)
    , m_thread(
      [this,
       sprite, frame,
       pixelFormat,
       ditheringAlgorithm]() {
        run(sprite, frame,
            pixelFormat,
            ditheringAlgorithm);
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

private:
  void run(const Sprite* sprite,
           const doc::frame_t frame,
           const doc::PixelFormat pixelFormat,
           const render::DitheringAlgorithm ditheringAlgorithm) {
    doc::ImageRef tmp(Image::create(sprite->pixelFormat(),
                                    sprite->width(),
                                    sprite->height()));

    render::Render render;
    render.renderSprite(
      tmp.get(), sprite, frame);

    render::convert_pixel_format(
      tmp.get(),
      m_image.get(),
      pixelFormat,
      ditheringAlgorithm,
      sprite->rgbMap(frame),
      sprite->palette(frame),
      (sprite->backgroundLayer() != nullptr),
      0,
      &m_stopFlag);

    m_running = false;
  }

private:
  doc::ImageRef m_image;
  bool m_running;
  bool m_stopFlag;
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
      colorMode()->addChild(new ConversionItem(IMAGE_INDEXED, render::DitheringAlgorithm::Ordered));
      colorMode()->addChild(new ConversionItem(IMAGE_INDEXED, render::DitheringAlgorithm::OldOrdered));
    }
    if (from != IMAGE_GRAYSCALE)
      colorMode()->addChild(new ConversionItem(IMAGE_GRAYSCALE));

    colorModeView()->setMinSize(
      colorModeView()->sizeHint() +
      colorMode()->sizeHint());

    colorMode()->Change.connect(base::Bind<void>(&ColorModeWindow::onChangeColorMode, this));
    m_timer.Tick.connect(base::Bind<void>(&ColorModeWindow::onMonitorProgress, this));

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
    ASSERT(m_selectedItem);
    return m_selectedItem->ditheringAlgorithm();
  }

private:

  void stop() {
    m_timer.stop();
    if (m_bgThread) {
      m_bgThread->stop();
      m_bgThread.reset(nullptr);
    }
    m_editor->renderEngine().removePreviewImage();
    m_editor->invalidate();
  }

  void onChangeColorMode() {
    ConversionItem* item =
      static_cast<ConversionItem*>(colorMode()->getSelectedChild());
    if (item == m_selectedItem) // Avoid restarting the conversion process for the same option
      return;
    m_selectedItem = item;

    stop();

    m_image.reset(
      Image::create(item->pixelFormat(),
                    m_editor->sprite()->width(),
                    m_editor->sprite()->height(),
                    m_imageBuffer));

    m_editor->renderEngine().setPreviewImage(
      nullptr,
      m_editor->frame(),
      m_image.get(),
      gfx::Point(0, 0),
      doc::BlendMode::NORMAL);

    m_image->clear(0);
    m_editor->invalidate();

    m_bgThread.reset(
      new ConvertThread(
        m_image,
        m_editor->sprite(),
        m_editor->frame(),
        item->pixelFormat(),
        item->ditheringAlgorithm()));

    m_timer.start();
  }

  void onMonitorProgress() {
    ASSERT(m_bgThread);
    if (!m_bgThread)
      return;

    if (!m_bgThread->isRunning()) {
      m_timer.stop();
      m_bgThread->stop();
      m_bgThread.reset(nullptr);
    }

    m_editor->invalidate();
  }

  Timer m_timer;
  Editor* m_editor;
  doc::ImageRef m_image;
  doc::ImageBufferPtr m_imageBuffer;
  base::UniquePtr<ConvertThread> m_bgThread;
  ConversionItem* m_selectedItem;
};

} // anonymous namespace

class ChangePixelFormatCommand : public Command {
public:
  ChangePixelFormatCommand();
  Command* clone() const override { return new ChangePixelFormatCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;

private:
  bool m_useUI;
  doc::PixelFormat m_format;
  render::DitheringAlgorithm m_dithering;
};

ChangePixelFormatCommand::ChangePixelFormatCommand()
  : Command("ChangePixelFormat",
            "Change Pixel Format",
            CmdUIOnlyFlag)
{
  m_useUI = true;
  m_format = IMAGE_RGB;
  m_dithering = render::DitheringAlgorithm::None;
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
    m_dithering = render::DitheringAlgorithm::Ordered;
  else if (dithering == "old-ordered")
    m_dithering = render::DitheringAlgorithm::OldOrdered;
  else
    m_dithering = render::DitheringAlgorithm::None;
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
      m_dithering != render::DitheringAlgorithm::None)
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
      m_dithering != render::DitheringAlgorithm::None)
    return false;

  return
    (sprite &&
     sprite->pixelFormat() == m_format);
}

void ChangePixelFormatCommand::onExecute(Context* context)
{
  if (m_useUI) {
    ColorModeWindow window(current_editor);

    window.remapWindow();
    window.centerWindow();

    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    m_format = window.pixelFormat();
    m_dithering = window.ditheringAlgorithm();
  }

  {
    ContextWriter writer(context);
    Transaction transaction(writer.context(), "Color Mode Change");
    Document* document(writer.document());
    Sprite* sprite(writer.sprite());

    document->getApi(transaction).setPixelFormat(sprite, m_format, m_dithering);
    transaction.commit();
  }

  if (context->isUIAvailable())
    app_refresh_screen();
}

Command* CommandFactory::createChangePixelFormatCommand()
{
  return new ChangePixelFormatCommand;
}

} // namespace app
