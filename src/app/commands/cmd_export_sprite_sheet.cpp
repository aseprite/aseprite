// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_exporter.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/pref/preferences.h"
#include "app/ui/status_bar.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/path.h"

#include "generated_export_sprite_sheet.h"

#include <limits>
#include <sstream>

namespace app {

using namespace ui;

namespace {

  struct Fit {
    int width;
    int height;
    int columns;
    int freearea;
    Fit() : width(0), height(0), columns(0), freearea(0) {
    }
    Fit(int width, int height, int columns, int freearea) :
      width(width), height(height), columns(columns), freearea(freearea) {
    }
  };

  // Calculate best size for the given sprite
  // TODO this function was programmed in ten minutes, please optimize it
  Fit best_fit(Sprite* sprite, int borderPadding, int shapePadding, int innerPadding) {
    int nframes = sprite->totalFrames();
    int framew = sprite->width()+2*innerPadding;
    int frameh = sprite->height()+2*innerPadding;
    Fit result(framew*nframes, frameh, nframes, std::numeric_limits<int>::max());
    int w, h;

    for (w=2; w < framew; w*=2)
      ;
    for (h=2; h < frameh; h*=2)
      ;

    int z = 0;
    bool fully_contained = false;
    while (!fully_contained) {  // TODO at this moment we're not
                                // getting the best fit for less
                                // freearea, just the first one.
      gfx::Rect rgnSize(w-2*borderPadding, h-2*borderPadding);
      gfx::Region rgn(rgnSize);
      int contained_frames = 0;

      for (int v=0; v+frameh <= rgnSize.h && !fully_contained; v+=frameh+shapePadding) {
        for (int u=0; u+framew <= rgnSize.w; u+=framew+shapePadding) {
          gfx::Rect framerc = gfx::Rect(u, v, framew, frameh);
          rgn.createSubtraction(rgn, gfx::Region(framerc));

          ++contained_frames;
          if (nframes == contained_frames) {
            fully_contained = true;
            break;
          }
        }
      }

      if (fully_contained) {
        // TODO convert this to a template function gfx::area()
        int freearea = 0;
        for (const gfx::Rect& rgnRect : rgn)
          freearea += rgnRect.w * rgnRect.h;

        Fit fit(w, h, (w / framew), freearea);
        if (fit.freearea < result.freearea)
          result = fit;
      }

      if ((++z) & 1) w *= 2;
      else h *= 2;
    }

    return result;
  }

  Fit calculate_sheet_size(Sprite* sprite, int columns, int borderPadding, int shapePadding, int innerPadding) {
    int nframes = sprite->totalFrames();

    columns = MID(1, columns, nframes);
    int rows = ((nframes/columns) + ((nframes%columns) > 0 ? 1: 0));

    return Fit(
      2*borderPadding + (sprite->width()+2*innerPadding)*columns + (columns-1)*shapePadding,
      2*borderPadding + (sprite->height()+2*innerPadding)*rows + (rows-1)*shapePadding,
      columns, 0);
  }

  bool ask_overwrite(bool askFilename, std::string filename,
                     bool askDataname, std::string dataname) {
    if ((askFilename &&
         base::is_file(filename)) ||
        (askDataname &&
         !dataname.empty() &&
         base::is_file(dataname))) {
      std::stringstream text;
      text << "Export Sprite Sheet Warning<<Do you want to overwrite the following file(s)?";

      if (base::is_file(filename))
        text << "<<" << base::get_file_name(filename).c_str();

      if (base::is_file(dataname))
        text << "<<" << base::get_file_name(dataname).c_str();

      text << "||&Yes||&No";
      if (Alert::show(text.str().c_str()) != 1)
        return false;
    }
    return true;
  }

}

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  ExportSpriteSheetWindow(Document* doc, Sprite* sprite,
    DocumentPreferences& docPref)
    : m_sprite(sprite)
    , m_docPref(docPref)
    , m_filenameAskOverwrite(true)
    , m_dataFilenameAskOverwrite(true)
  {
    static_assert(
      (int)app::gen::SpriteSheetType::NONE == 0 &&
      (int)app::gen::SpriteSheetType::HORIZONTAL_STRIP == 1 &&
      (int)app::gen::SpriteSheetType::VERTICAL_STRIP == 2 &&
      (int)app::gen::SpriteSheetType::MATRIX == 3,
      "ExportType enum changed");

    sheetType()->addItem("Horizontal Strip");
    sheetType()->addItem("Vertical Strip");
    sheetType()->addItem("Matrix");
    if (m_docPref.spriteSheet.type() != app::gen::SpriteSheetType::NONE)
      sheetType()->setSelectedItemIndex((int)m_docPref.spriteSheet.type()-1);

    openGenerated()->setSelected(m_docPref.spriteSheet.openGenerated());

    borderPadding()->setTextf("%d", m_docPref.spriteSheet.borderPadding());
    shapePadding()->setTextf("%d", m_docPref.spriteSheet.shapePadding());
    innerPadding()->setTextf("%d", m_docPref.spriteSheet.innerPadding());
    paddingEnabled()->setSelected(
      m_docPref.spriteSheet.borderPadding() ||
      m_docPref.spriteSheet.shapePadding() ||
      m_docPref.spriteSheet.innerPadding());
    paddingContainer()->setVisible(paddingEnabled()->isSelected());

    for (int i=2; i<=8192; i*=2) {
      std::string value = base::convert_to<std::string>(i);
      if (i >= m_sprite->width()) fitWidth()->addItem(value);
      if (i >= m_sprite->height()) fitHeight()->addItem(value);
    }

    if (m_docPref.spriteSheet.bestFit()) {
      bestFit()->setSelected(true);
      onBestFit();
    }
    else {
      columns()->setTextf("%d", m_docPref.spriteSheet.columns());
      onColumnsChange();

      if (m_docPref.spriteSheet.width() > 0 || m_docPref.spriteSheet.height() > 0) {
        if (m_docPref.spriteSheet.width() > 0)
          fitWidth()->getEntryWidget()->setTextf("%d", m_docPref.spriteSheet.width());

        if (m_docPref.spriteSheet.height() > 0)
          fitHeight()->getEntryWidget()->setTextf("%d", m_docPref.spriteSheet.height());

        onSizeChange();
      }
    }

    m_filename = m_docPref.spriteSheet.textureFilename();
    m_dataFilename = m_docPref.spriteSheet.dataFilename();
    dataEnabled()->setSelected(!m_dataFilename.empty());
    dataFilename()->setVisible(dataEnabled()->isSelected());

    std::string base = doc->filename();
    base = base::join_path(base::get_file_path(base), base::get_file_title(base));
    if (m_filename.empty()) {
      if (base::utf8_icmp(base::get_file_extension(doc->filename()), "png") == 0)
        m_filename = base + "-sheet.png";
      else
        m_filename = base + ".png";
    }
    if (m_dataFilename.empty())
      m_dataFilename = base + ".json";

    exportButton()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onExport, this));
    sheetType()->Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    columns()->EntryChange.connect(Bind<void>(&ExportSpriteSheetWindow::onColumnsChange, this));
    fitWidth()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    fitHeight()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    bestFit()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onBestFit, this));
    borderPadding()->EntryChange.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    shapePadding()->EntryChange.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    innerPadding()->EntryChange.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    imageFilename()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onImageFilename, this));
    dataEnabled()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onDataEnabledChange, this));
    dataFilename()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onDataFilename, this));
    paddingEnabled()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingEnabledChange, this));

    onSheetTypeChange();
    onFileNamesChange();
  }

  bool ok() const {
    return getKiller() == exportButton();
  }

  app::gen::SpriteSheetType spriteSheetTypeValue() {
    return (app::gen::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  int columnsValue() {
    return columns()->getTextInt();
  }

  int fitWidthValue() {
    return fitWidth()->getEntryWidget()->getTextInt();
  }

  int fitHeightValue() {
    return fitHeight()->getEntryWidget()->getTextInt();
  }

  bool bestFitValue() {
    return bestFit()->isSelected();
  }

  std::string filenameValue() {
    return m_filename;
  }

  std::string dataFilenameValue() {
    if (dataEnabled()->isSelected())
      return m_dataFilename;
    else
      return std::string();
  }

  int borderPaddingValue() {
    if (paddingEnabled()->isSelected()) {
      int value = borderPadding()->getTextInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  int shapePaddingValue() {
    if (paddingEnabled()->isSelected()) {
      int value = shapePadding()->getTextInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  int innerPaddingValue() {
    if (paddingEnabled()->isSelected()) {
      int value = innerPadding()->getTextInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  bool openGeneratedValue() {
    return openGenerated()->isSelected();
  }

protected:

  void onExport() {
    if (!ask_overwrite(m_filenameAskOverwrite, filenameValue(),
                       m_dataFilenameAskOverwrite, dataFilenameValue()))
      return;

    closeWindow(exportButton());
  }

  void onSheetTypeChange() {
    bool state = false;
    switch (spriteSheetTypeValue()) {
      case app::gen::SpriteSheetType::MATRIX:
        state = true;
        break;
    }

    columnsLabel()->setVisible(state);
    columns()->setVisible(state);
    fitWidthLabel()->setVisible(state);
    fitWidth()->setVisible(state);
    fitHeightLabel()->setVisible(state);
    fitHeight()->setVisible(state);
    bestFitFiller()->setVisible(state);
    bestFit()->setVisible(state);
    resize();
  }

  void onFileNamesChange() {
    imageFilename()->setText("Select File: " + base::get_file_name(m_filename));
    dataFilename()->setText("Select File: " + base::get_file_name(m_dataFilename));
    resize();
  }

  void onColumnsChange() {
    bestFit()->setSelected(false);
    updateSizeFields();
  }

  void onSizeChange() {
    columns()->setTextf("%d", fitWidthValue() / m_sprite->width());
    bestFit()->setSelected(false);
  }

  void onBestFit() {
    updateSizeFields();
  }

  void onImageFilename() {
    std::string exts = get_writable_extensions();

    std::string newFilename = app::show_file_selector(
      "Save Sprite Sheet", m_filename, exts, FileSelectorType::Save);
    if (newFilename.empty())
      return;

    m_filename = newFilename;
    m_filenameAskOverwrite = false; // Already asked in file selector
    onFileNamesChange();
  }

  void onDataFilename() {
    // TODO hardcoded "json" extension
    std::string newFilename = app::show_file_selector(
      "Save JSON Data", m_dataFilename, "json", FileSelectorType::Save);
    if (newFilename.empty())
      return;

    m_dataFilename = newFilename;
    m_dataFilenameAskOverwrite = false; // Already asked in file selector
    onFileNamesChange();
  }

  void onDataEnabledChange() {
    m_dataFilenameAskOverwrite = true;

    dataFilename()->setVisible(dataEnabled()->isSelected());
    resize();
  }

  void onPaddingEnabledChange() {
    paddingContainer()->setVisible(paddingEnabled()->isSelected());
    resize();
    updateSizeFields();
  }

  void onPaddingChange() {
    updateSizeFields();
  }

private:

  void resize() {
    gfx::Size reqSize = getPreferredSize();
    moveWindow(gfx::Rect(getOrigin(), reqSize));
    invalidate();
  }

  void updateSizeFields() {
    Fit fit;

    if (bestFit()->isSelected()) {
      fit = best_fit(m_sprite,
        borderPaddingValue(), shapePaddingValue(), innerPaddingValue());
    }
    else {
      fit = calculate_sheet_size(
        m_sprite, columnsValue(),
        borderPaddingValue(),
        shapePaddingValue(),
        innerPaddingValue());
    }

    columns()->setTextf("%d", fit.columns);
    fitWidth()->getEntryWidget()->setTextf("%d", fit.width);
    fitHeight()->getEntryWidget()->setTextf("%d", fit.height);
  }

  Sprite* m_sprite;
  DocumentPreferences& m_docPref;
  std::string m_filename;
  std::string m_dataFilename;
  bool m_filenameAskOverwrite;
  bool m_dataFilenameAskOverwrite;
};

class ExportSpriteSheetCommand : public Command {
public:
  ExportSpriteSheetCommand();
  Command* clone() const override { return new ExportSpriteSheetCommand(*this); }

  void setUseUI(bool useUI) { m_useUI = useUI; }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  bool m_useUI;
  bool m_askOverwrite;
};

ExportSpriteSheetCommand::ExportSpriteSheetCommand()
  : Command("ExportSpriteSheet",
            "Export Sprite Sheet",
            CmdRecordableFlag)
  , m_useUI(true)
  , m_askOverwrite(true)
{
}

void ExportSpriteSheetCommand::onLoadParams(const Params& params)
{
  if (params.has_param("ui"))
    m_useUI = params.get_as<bool>("ui");
  else
    m_useUI = true;

  if (params.has_param("ask-overwrite"))
    m_askOverwrite = params.get_as<bool>("ask-overwrite");
  else
    m_askOverwrite = true;
}

bool ExportSpriteSheetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ExportSpriteSheetCommand::onExecute(Context* context)
{
  Document* document(context->activeDocument());
  Sprite* sprite = document->sprite();
  DocumentPreferences& docPref(Preferences::instance().document(document));
  bool askOverwrite = m_askOverwrite;

  if (m_useUI && context->isUIAvailable()) {
    ExportSpriteSheetWindow window(document, sprite, docPref);
    window.openWindowInForeground();
    if (!window.ok())
      return;

    docPref.spriteSheet.type(window.spriteSheetTypeValue());
    docPref.spriteSheet.columns(window.columnsValue());
    docPref.spriteSheet.width(window.fitWidthValue());
    docPref.spriteSheet.height(window.fitHeightValue());
    docPref.spriteSheet.bestFit(window.bestFitValue());
    docPref.spriteSheet.textureFilename(window.filenameValue());
    docPref.spriteSheet.dataFilename(window.dataFilenameValue());
    docPref.spriteSheet.borderPadding(window.borderPaddingValue());
    docPref.spriteSheet.shapePadding(window.shapePaddingValue());
    docPref.spriteSheet.innerPadding(window.innerPaddingValue());
    docPref.spriteSheet.openGenerated(window.openGeneratedValue());

    // Default preferences for future sprites
    DocumentPreferences& defPref(Preferences::instance().document(nullptr));
    defPref.spriteSheet = docPref.spriteSheet;
    defPref.spriteSheet.textureFilename("");
    defPref.spriteSheet.dataFilename("");

    askOverwrite = false; // Already asked in the ExportSpriteSheetWindow
  }

  app::gen::SpriteSheetType type = docPref.spriteSheet.type();
  int columns = docPref.spriteSheet.columns();
  int width = docPref.spriteSheet.width();
  int height = docPref.spriteSheet.height();
  bool bestFit = docPref.spriteSheet.bestFit();
  std::string filename = docPref.spriteSheet.textureFilename();
  std::string dataFilename = docPref.spriteSheet.dataFilename();
  int borderPadding = docPref.spriteSheet.borderPadding();
  int shapePadding = docPref.spriteSheet.shapePadding();
  int innerPadding = docPref.spriteSheet.innerPadding();
  borderPadding = MID(0, borderPadding, 100);
  shapePadding = MID(0, shapePadding, 100);
  innerPadding = MID(0, innerPadding, 100);

  if (context->isUIAvailable() && askOverwrite) {
    if (!ask_overwrite(true, filename,
                       true, dataFilename))
      return;                   // Do not overwrite
  }

  if (bestFit) {
    Fit fit = best_fit(sprite, borderPadding, shapePadding, innerPadding);
    columns = fit.columns;
    width = fit.width;
    height = fit.height;
  }

  frame_t nframes = sprite->totalFrames();
  int sheet_w = 0;
  int sheet_h = 0;

  switch (type) {
    case app::gen::SpriteSheetType::HORIZONTAL_STRIP:
      columns = nframes;
      break;
    case app::gen::SpriteSheetType::VERTICAL_STRIP:
      columns = 1;
      break;
    case app::gen::SpriteSheetType::MATRIX:
      if (width > 0) sheet_w = width;
      if (height > 0) sheet_h = height;
      break;
  }

  Fit fit = calculate_sheet_size(sprite, columns,
    borderPadding, shapePadding, innerPadding);
  columns = fit.columns;
  if (sheet_w == 0) sheet_w = fit.width;
  if (sheet_h == 0) sheet_h = fit.height;

  DocumentExporter exporter;
  exporter.setTextureFilename(filename);
  if (!dataFilename.empty())
    exporter.setDataFilename(dataFilename);
  exporter.setTextureWidth(sheet_w);
  exporter.setTextureHeight(sheet_h);
  exporter.setTexturePack(false);
  exporter.setBorderPadding(borderPadding);
  exporter.setShapePadding(shapePadding);
  exporter.setInnerPadding(innerPadding);
  exporter.addDocument(document);

  base::UniquePtr<Document> newDocument(exporter.exportSheet());
  if (!newDocument)
    return;

  StatusBar* statusbar = StatusBar::instance();
  if (statusbar)
    statusbar->showTip(1000, "Sprite Sheet Generated");

  if (docPref.spriteSheet.openGenerated()) {
    newDocument->setContext(context);
    newDocument.release();
  }
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
