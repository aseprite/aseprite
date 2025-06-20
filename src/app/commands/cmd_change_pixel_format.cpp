// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
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
#include "app/commands/new_params.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/extensions.h"
#include "app/i18n/strings.h"
#include "app/load_matrix.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/sprite_job.h"
#include "app/transaction.h"
#include "app/ui/best_fit_criteria_selector.h"
#include "app/ui/dithering_selector.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_render.h"
#include "app/ui/rgbmap_algorithm_selector.h"
#include "app/ui/skin/skin_theme.h"
#include "doc/color_mode.h"
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
#include "ui/button.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#include "color_mode.xml.h"

#include <string>
#include <thread>

namespace app {

using namespace ui;

namespace {

rgba_to_graya_func get_gray_func(gen::ToGrayAlgorithm toGray)
{
  switch (toGray) {
    case gen::ToGrayAlgorithm::LUMA: return &rgba_to_graya_using_luma;
    case gen::ToGrayAlgorithm::HSV:  return &rgba_to_graya_using_hsv;
    case gen::ToGrayAlgorithm::HSL:  return &rgba_to_graya_using_hsl;
  }
  return nullptr;
}

class ConvertThread : public render::TaskDelegate {
public:
  ConvertThread(const doc::ImageRef& dstImage,
                const doc::Sprite* sprite,
                const doc::frame_t frame,
                const doc::ColorMode colorMode,
                const render::Dithering& dithering,
                const gen::ToGrayAlgorithm toGray,
                const gfx::Point& pos,
                const bool newBlend)
    : m_image(dstImage)
    , m_pos(pos)
    , m_running(true)
    , m_stopFlag(false)
    , m_progress(0.0)
    , m_thread([this,
                sprite,
                frame,
                colorMode,
                dithering,
                toGray,
                newBlend]() { // Copy the matrix
      run(sprite, frame, colorMode, dithering, toGray, newBlend);
    })
  {
  }

  void stop()
  {
    m_stopFlag = true;
    m_thread.join();
  }

  bool isRunning() const { return m_running; }

  double progress() const { return m_progress; }

private:
  void run(const Sprite* sprite,
           const doc::frame_t frame,
           const doc::ColorMode colorMode,
           const render::Dithering& dithering,
           const gen::ToGrayAlgorithm toGray,
           const bool newBlend)
  {
    doc::ImageRef tmp(Image::create(sprite->pixelFormat(), m_image->width(), m_image->height()));

    render::Render render;
    render.setNewBlend(newBlend);
    render.renderSprite(tmp.get(),
                        sprite,
                        frame,
                        gfx::Clip(0, 0, m_pos.x, m_pos.y, m_image->width(), m_image->height()));

    render::convert_pixel_format(tmp.get(),
                                 m_image.get(),
                                 (PixelFormat)colorMode,
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
  bool continueTask() override { return !m_stopFlag; }

  void notifyTaskProgress(double progress) override { m_progress = progress; }

  doc::ImageRef m_image;
  gfx::Point m_pos;
  bool m_running;
  bool m_stopFlag;
  double m_progress;
  std::thread m_thread;
};

class ConversionItem : public ListItem {
public:
  ConversionItem(const doc::ColorMode colorMode) : m_colorMode(colorMode)
  {
    std::string toMode;
    switch (colorMode) {
      case doc::ColorMode::RGB: toMode = Strings::commands_ChangePixelFormat_RGB(); break;
      case doc::ColorMode::GRAYSCALE:
        toMode = Strings::commands_ChangePixelFormat_Grayscale();
        break;
      case doc::ColorMode::INDEXED: toMode = Strings::commands_ChangePixelFormat_Indexed(); break;
    }
    setText(fmt::format("-> {}", toMode));
  }
  doc::ColorMode colorMode() const { return m_colorMode; }

private:
  doc::ColorMode m_colorMode;
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
    , m_zigZagCheck(nullptr)
    , m_mapAlgorithmSelector(nullptr)
    , m_bestFitCriteriaSelector(nullptr)
    , m_imageJustCreated(true)
  {
    const auto& pref = Preferences::instance();
    const doc::ColorMode from = m_editor->sprite()->colorMode();

    // Add the color mode in the window title
    std::string fromMode;
    switch (from) {
      case doc::ColorMode::RGB: fromMode = Strings::commands_ChangePixelFormat_RGB(); break;
      case doc::ColorMode::GRAYSCALE:
        fromMode = Strings::commands_ChangePixelFormat_Grayscale();
        break;
      case doc::ColorMode::INDEXED: fromMode = Strings::commands_ChangePixelFormat_Indexed(); break;
    }
    setText(fmt::format("{}: {}", text(), fromMode));

    // Add conversion items
    if (from != doc::ColorMode::RGB)
      colorMode()->addChild(new ConversionItem(doc::ColorMode::RGB));
    if (from != doc::ColorMode::INDEXED) {
      colorMode()->addChild(new ConversionItem(doc::ColorMode::INDEXED));

      m_ditheringSelector = new DitheringSelector(DitheringSelector::SelectBoth);
      m_ditheringSelector->setExpansive(true);

      m_zigZagCheck = new CheckBox(Strings::commands_ChangePixelFormat_ZigZag());

      m_mapAlgorithmSelector = new RgbMapAlgorithmSelector;
      m_mapAlgorithmSelector->setExpansive(true);

      m_bestFitCriteriaSelector = new BestFitCriteriaSelector;
      m_bestFitCriteriaSelector->setExpansive(true);

      // Select default dithering method
      {
        int index = m_ditheringSelector->findItemIndex(pref.quantization.ditheringAlgorithm());
        if (index >= 0)
          m_ditheringSelector->setSelectedItemIndex(index);
      }

      // Select default zig zag
      m_zigZagCheck->setSelected(pref.quantization.zigZag());

      // Select default RgbMap algorithm
      m_mapAlgorithmSelector->algorithm(pref.quantization.rgbmapAlgorithm());

      // Select default best fit criteria
      m_bestFitCriteriaSelector->criteria(pref.quantization.fitCriteria());

      ditheringPlaceholder()->addChild(m_ditheringSelector);
      ditheringPlaceholder()->addChild(m_zigZagCheck);
      rgbmapAlgorithmPlaceholder()->addChild(m_mapAlgorithmSelector);
      bestFitCriteriaPlaceholder()->addChild(m_bestFitCriteriaSelector);

      const bool adv = pref.quantization.advanced();
      advancedCheck()->setSelected(adv);
      advanced()->setVisible(adv);

      // Signals
      m_ditheringSelector->Change.connect([this] { onIndexParamChange(); });
      m_zigZagCheck->Click.connect([this] { onIndexParamChange(); });
      m_mapAlgorithmSelector->Change.connect([this] { onIndexParamChange(); });
      m_bestFitCriteriaSelector->Change.connect([this] { onIndexParamChange(); });
      factor()->Change.connect([this] { onIndexParamChange(); });

      advancedCheck()->Click.connect([this]() {
        advanced()->setVisible(advancedCheck()->isSelected());
        expandWindow(sizeHint());
      });
    }
    else {
      amount()->setVisible(false);
      advancedCheck()->setVisible(false);
      advanced()->setVisible(false);
    }
    if (from != doc::ColorMode::GRAYSCALE) {
      colorMode()->addChild(new ConversionItem(doc::ColorMode::GRAYSCALE));

      toGrayCombobox()->Change.connect([this] { onToGrayChange(); });
    }

    colorModeView()->setMinSize(colorModeView()->sizeHint() + colorMode()->sizeHint());

    colorMode()->Change.connect([this] { onChangeColorMode(); });
    m_timer.Tick.connect([this] { onMonitorProgress(); });

    progress()->setReadOnly(true);

    // Default dithering factor
    factor()->setValue(pref.quantization.ditheringFactor());

    // Select first option
    colorMode()->selectIndex(0);
  }

  ~ColorModeWindow() { stop(); }

  doc::ColorMode selectedColorMode() const
  {
    ASSERT(m_selectedItem);
    return m_selectedItem->colorMode();
  }

  render::Dithering dithering() const
  {
    render::Dithering d;
    if (m_ditheringSelector) {
      d.algorithm(m_ditheringSelector->ditheringAlgorithm());
      d.matrix(m_ditheringSelector->ditheringMatrix());
      d.zigzag(m_zigZagCheck->isSelected());
    }
    d.factor(double(factor()->getValue()) / 100.0);
    return d;
  }

  doc::RgbMapAlgorithm rgbMapAlgorithm() const
  {
    if (m_mapAlgorithmSelector)
      return m_mapAlgorithmSelector->algorithm();
    else
      return doc::RgbMapAlgorithm::DEFAULT;
  }

  doc::FitCriteria fitCriteria() const
  {
    if (m_bestFitCriteriaSelector)
      return m_bestFitCriteriaSelector->criteria();
    else
      return doc::FitCriteria::DEFAULT;
  }

  gen::ToGrayAlgorithm toGray() const
  {
    static_assert(
      int(gen::ToGrayAlgorithm::LUMA) == 0 && int(gen::ToGrayAlgorithm::HSV) == 1 &&
        int(gen::ToGrayAlgorithm::HSL) == 2,
      "Check that 'to_gray_combobox' combobox items matches these indexes in color_mode.xml");
    return (gen::ToGrayAlgorithm)toGrayCombobox()->getSelectedItemIndex();
  }

  bool flattenEnabled() const { return flatten()->isSelected(); }

  void saveOptions()
  {
    auto& pref = Preferences::instance();

    // Save the dithering method and zig-zag setting used for the future
    if (m_ditheringSelector) {
      if (auto item = m_ditheringSelector->getSelectedItem()) {
        pref.quantization.ditheringAlgorithm(item->text());

        if (DitheringAlgorithmIsDiffusion(m_ditheringSelector->ditheringAlgorithm()))
          pref.quantization.ditheringFactor(factor()->getValue());
      }

      pref.quantization.zigZag(m_zigZagCheck->isEnabled());
    }

    if (m_mapAlgorithmSelector || m_bestFitCriteriaSelector)
      pref.quantization.advanced(advancedCheck()->isSelected());
  }

private:
  void stop()
  {
    m_editor->renderEngine().removePreviewImage();
    m_editor->invalidate();

    m_timer.stop();
    if (m_bgThread) {
      m_bgThread->stop();
      m_bgThread.reset(nullptr);
    }
  }

  void onChangeColorMode()
  {
    ConversionItem* item = static_cast<ConversionItem*>(colorMode()->getSelectedChild());
    if (item == m_selectedItem) // Avoid restarting the conversion process for the same option
      return;
    m_selectedItem = item;

    stop();

    gfx::Rect visibleBounds = m_editor->getVisibleSpriteBounds();
    if (visibleBounds.isEmpty())
      return;

    doc::ColorMode dstColorMode = item->colorMode();

    if (m_ditheringSelector) {
      const bool toIndexed = (dstColorMode == doc::ColorMode::INDEXED);
      m_ditheringSelector->setVisible(toIndexed);

      const bool errorDiff =
        (render::DitheringAlgorithmIsDiffusion(m_ditheringSelector->ditheringAlgorithm()));
      amount()->setVisible(toIndexed && errorDiff);
    }

    {
      const bool toGray = (dstColorMode == doc::ColorMode::GRAYSCALE);
      toGrayCombobox()->setVisible(toGray);
    }

    m_image.reset(
      Image::create(ImageSpec(dstColorMode, visibleBounds.w, visibleBounds.h, 0), m_imageBuffer));
    if (m_imageJustCreated) {
      m_imageJustCreated = false;
      m_image->clear(0);
    }

    m_editor->renderEngine().setPreviewImage(nullptr,
                                             m_editor->frame(),
                                             m_image.get(),
                                             nullptr,
                                             visibleBounds.origin(),
                                             doc::BlendMode::SRC);

    m_editor->sprite()->rgbMap(0,
                               m_editor->sprite()->rgbMapForSprite(),
                               rgbMapAlgorithm(),
                               fitCriteria());

    m_editor->invalidate();
    progress()->setValue(0);
    progress()->setVisible(false);
    layout();

    m_bgThread.reset(new ConvertThread(m_image,
                                       m_editor->sprite(),
                                       m_editor->frame(),
                                       dstColorMode,
                                       dithering(),
                                       toGray(),
                                       visibleBounds.origin(),
                                       Preferences::instance().experimental.newBlend()));

    m_timer.start();
  }

  void onIndexParamChange()
  {
    stop();
    m_selectedItem = nullptr;
    onChangeColorMode();
  }

  void onToGrayChange()
  {
    stop();
    m_selectedItem = nullptr;
    onChangeColorMode();
  }

  void onMonitorProgress()
  {
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
  CheckBox* m_zigZagCheck;
  RgbMapAlgorithmSelector* m_mapAlgorithmSelector;
  BestFitCriteriaSelector* m_bestFitCriteriaSelector;
  bool m_imageJustCreated;
};

} // anonymous namespace

struct ChangePixelFormatParams : public NewParams {
  Param<bool> ui{ this, false, "ui" };
  Param<ColorMode> colorMode{
    this,
    ColorMode::RGB,
    { "colorMode", "format" }
  };
  Param<render::DitheringAlgorithm> dithering{ this,
                                               render::DitheringAlgorithm::None,
                                               "dithering" };
  Param<std::string> matrix{
    this,
    "",
    { "ditheringMatrix", "dithering-matrix" }
  };
  Param<double> factor{
    this,
    1.0,
    { "ditheringFactor", "dithering-factor" }
  };
  Param<bool> zigZag{
    this,
    true,
    { "zigZag", "zig-zag" }
  };
  Param<doc::RgbMapAlgorithm> rgbmap{ this, RgbMapAlgorithm::DEFAULT, "rgbmap" };
  Param<gen::ToGrayAlgorithm> toGray{ this, gen::ToGrayAlgorithm::DEFAULT, "toGray" };
  Param<doc::FitCriteria> fitCriteria{ this, doc::FitCriteria::DEFAULT, "fitCriteria" };
};

class ChangePixelFormatCommand : public CommandWithNewParams<ChangePixelFormatParams> {
public:
  ChangePixelFormatCommand();

protected:
  bool onEnabled(Context* ctx) override;
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;

private:
  static render::DitheringMatrix getDitheringMatrix(const std::string& ditheringMatrixId);
};

ChangePixelFormatCommand::ChangePixelFormatCommand()
  : CommandWithNewParams(CommandId::ChangePixelFormat(), CmdUIOnlyFlag)
{
}

bool ChangePixelFormatCommand::onEnabled(Context* ctx)
{
  if (!ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveSprite))
    return false;

  const ContextReader reader(ctx);
  const Sprite* sprite(reader.sprite());

  if (!sprite)
    return false;

  if (params().ui())
    return true;

  if (sprite->pixelFormat() == IMAGE_INDEXED && params().colorMode() == ColorMode::INDEXED &&
      params().dithering() != render::DitheringAlgorithm::None)
    return false;

  return true;
}

bool ChangePixelFormatCommand::onChecked(Context* ctx)
{
  if (params().ui() || (!params().ui.isSet() && !params().colorMode.isSet()))
    return false;

  const ContextReader reader(ctx);
  const Sprite* sprite = reader.sprite();

  if (sprite && sprite->pixelFormat() == IMAGE_INDEXED &&
      params().colorMode() == ColorMode::INDEXED &&
      params().dithering() != render::DitheringAlgorithm::None)
    return false;

  return (sprite && sprite->colorMode() == params().colorMode());
}

void ChangePixelFormatCommand::onExecute(Context* ctx)
{
  const bool ui = ((params().ui() || (!params().ui.isSet() && !params().colorMode.isSet())) &&
                   ctx->isUIAvailable());
  bool flatten = false;
  render::DitheringMatrix matrix = getDitheringMatrix(params().matrix());

  if (ui) {
    ColorModeWindow window(Editor::activeEditor());

    window.remapWindow();
    window.centerWindow();

    load_window_pos(&window, "ChangePixelFormat");
    window.openWindowInForeground();
    save_window_pos(&window, "ChangePixelFormat");

    if (window.closer() != window.ok())
      return;

    const auto d = window.dithering();

    params().colorMode(window.selectedColorMode());
    params().dithering(d.algorithm());
    matrix = d.matrix();
    params().factor(d.factor());
    params().zigZag(d.zigzag());
    params().rgbmap(window.rgbMapAlgorithm());
    params().fitCriteria(window.fitCriteria());
    params().toGray(window.toGray());
    flatten = window.flattenEnabled();

    window.saveOptions();
  }
  else if (params().colorMode() == ColorMode::INDEXED) {
    if (!params().rgbmap.isSet())
      params().rgbmap(Preferences::instance().quantization.rgbmapAlgorithm());
    if (!params().fitCriteria.isSet())
      params().fitCriteria(Preferences::instance().quantization.fitCriteria());
  }

  // No conversion needed
  Doc* doc = ctx->activeDocument();
  if (doc->sprite()->pixelFormat() == (PixelFormat)params().colorMode())
    return;

  {
    SpriteJob job(ctx, doc, Strings::color_mode_title(), ui);
    Sprite* sprite(job.sprite());

    // TODO this was moved in the main UI thread because
    //      cmd::FlattenLayers() generates a EditorObserver::onAfterLayerChanged()
    //      event, and that event is an UI event.
    //      We should refactor the whole app to separate doc changes <-> UI changes,
    //      but that is for the future:
    //      https://github.com/aseprite/aseprite/issues/509
    //      https://github.com/aseprite/aseprite/issues/378
    if (flatten) {
      Tx tx(Tx::LockDoc, ctx, doc);
      const bool newBlend = Preferences::instance().experimental.newBlend();
      cmd::FlattenLayers::Options options;
      options.newBlendMethod = newBlend;

      SelectedLayers selLayers;
      for (auto layer : sprite->root()->layers())
        selLayers.insert(layer);
      tx(new cmd::FlattenLayers(sprite, selLayers, options));
    }

    job.startJobWithCallback([this, &job, sprite, &matrix](Tx& tx) {
      tx(new cmd::SetPixelFormat(
        sprite,
        (PixelFormat)params().colorMode(),
        render::Dithering(params().dithering(), matrix, params().zigZag(), params().factor()),
        params().rgbmap(),
        get_gray_func(params().toGray()),
        &job, // SpriteJob is a render::TaskDelegate
        params().fitCriteria()));
    });
    job.waitJob();
  }

  if (ctx->isUIAvailable())
    app_refresh_screen();
}

std::string ChangePixelFormatCommand::onGetFriendlyName() const
{
  std::string conversion;

  if (!params().ui()) {
    switch (params().colorMode()) {
      case ColorMode::RGB: conversion = Strings::commands_ChangePixelFormat_RGB(); break;
      case ColorMode::GRAYSCALE:
        conversion = Strings::commands_ChangePixelFormat_Grayscale();
        break;
      case ColorMode::INDEXED:
        switch (params().dithering()) {
          case render::DitheringAlgorithm::None:
            conversion = Strings::commands_ChangePixelFormat_Indexed();
            break;
          case render::DitheringAlgorithm::Ordered:
            conversion = Strings::commands_ChangePixelFormat_Indexed_OrderedDithering();
            break;
          case render::DitheringAlgorithm::Old:
            conversion = Strings::commands_ChangePixelFormat_Indexed_OldDithering();
            break;
          case render::DitheringAlgorithm::FloydSteinberg:
            conversion = Strings::commands_ChangePixelFormat_Indexed_FloydSteinberg();
            break;
          case render::DitheringAlgorithm::JarvisJudiceNinke:
            conversion = Strings::commands_ChangePixelFormat_Indexed_JarvisJudiceNinke();
            break;
          case render::DitheringAlgorithm::Stucki:
            conversion = Strings::commands_ChangePixelFormat_Indexed_Stucki();
            break;
          case render::DitheringAlgorithm::Atkinson:
            conversion = Strings::commands_ChangePixelFormat_Indexed_Atkinson();
            break;
          case render::DitheringAlgorithm::Burkes:
            conversion = Strings::commands_ChangePixelFormat_Indexed_Burkes();
            break;
          case render::DitheringAlgorithm::Sierra:
            conversion = Strings::commands_ChangePixelFormat_Indexed_Sierra();
            break;
        }
        break;
    }
  }
  else
    conversion = Strings::commands_ChangePixelFormat_MoreOptions();

  return Strings::commands_ChangePixelFormat(conversion);
}

render::DitheringMatrix ChangePixelFormatCommand::getDitheringMatrix(
  const std::string& ditheringMatrix)
{
  if (!ditheringMatrix.empty()) {
    // Try to get the matrix from the extensions
    const render::DitheringMatrix* knownMatrix = App::instance()->extensions().ditheringMatrix(
      ditheringMatrix);
    if (knownMatrix) {
      return *knownMatrix;
    }

    // If the matrix doesn't exist we try to load it from a file
    try {
      render::DitheringMatrix matrix;
      load_dithering_matrix_from_sprite(ditheringMatrix, matrix);
      return matrix;
    }
    catch (const std::exception& e) {
      LOG(ERROR, "%s\n", e.what());
      Console::showException(e);
    }
  }

  // Default dithering matrix is BayerMatrix(8)
  return render::BayerMatrix::make(8);
}

Command* CommandFactory::createChangePixelFormatCommand()
{
  return new ChangePixelFormatCommand;
}

} // namespace app
