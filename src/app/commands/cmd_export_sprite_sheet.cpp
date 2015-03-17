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
#include "base/path.h"

#include "generated_export_sprite_sheet.h"

#include <limits>

namespace app {

using namespace ui;

namespace {

  struct Fit {
    int width;
    int height;
    int columns;
    int freearea;

    Fit(int width, int height, int columns, int freearea) :
      width(width), height(height), columns(columns), freearea(freearea) {
    }
  };

  // Calculate best size for the given sprite
  // TODO this function was programmed in ten minutes, please optimize it
  Fit best_fit(Sprite* sprite) {
    int nframes = sprite->totalFrames();
    int framew = sprite->width();
    int frameh = sprite->height();
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
      gfx::Region rgn(gfx::Rect(w, h));
      int contained_frames = 0;

      for (int v=0; v+frameh <= h && !fully_contained; v+=frameh) {
        for (int u=0; u+framew <= w; u+=framew) {
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
        for (gfx::Region::iterator it=rgn.begin(), end=rgn.end();
             it != end; ++it) {
          freearea += (*it).w * (*it).h;
        }

        Fit fit(w, h, (w / framew), freearea);
        if (fit.freearea < result.freearea)
          result = fit;
      }

      if ((++z) & 1) w *= 2;
      else h *= 2;
    }

    return result;
  }

}

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  ExportSpriteSheetWindow(Document* doc, Sprite* sprite,
    DocumentPreferences& docPref)
    : m_sprite(sprite)
    , m_docPref(docPref)
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

    sheetType()->Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    columns()->EntryChange.connect(Bind<void>(&ExportSpriteSheetWindow::onColumnsChange, this));
    fitWidth()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    fitHeight()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    bestFit()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onBestFit, this));
    imageFilename()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onImageFilename, this));
    dataEnabled()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onDataEnabledChange, this));
    dataFilename()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onDataFilename, this));

    onSheetTypeChange();
    onFileNamesChange();
    onDataEnabledChange();
  }

  bool ok() {
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

protected:

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
    int nframes = m_sprite->totalFrames();
    int columns = columnsValue();
    columns = MID(1, columns, nframes);
    int sheet_w = m_sprite->width()*columns;
    int sheet_h = m_sprite->height()*((nframes/columns)+((nframes%columns)>0?1:0));

    fitWidth()->getEntryWidget()->setTextf("%d", sheet_w);
    fitHeight()->getEntryWidget()->setTextf("%d", sheet_h);
    bestFit()->setSelected(false);
  }

  void onSizeChange() {
    columns()->setTextf("%d", fitWidthValue() / m_sprite->width());
    bestFit()->setSelected(false);
  }

  void onBestFit() {
    if (!bestFit()->isSelected())
      return;

    Fit fit = best_fit(m_sprite);
    columns()->setTextf("%d", fit.columns);
    fitWidth()->getEntryWidget()->setTextf("%d", fit.width);
    fitHeight()->getEntryWidget()->setTextf("%d", fit.height);
  }

  void onImageFilename() {
    char exts[4096];
    get_writable_extensions(exts, sizeof(exts));

    std::string newFilename = app::show_file_selector(
      "Save Sprite Sheet", m_filename, exts, FileSelectorType::Save);
    if (newFilename.empty())
      return;

    m_filename = newFilename;
    onFileNamesChange();
  }

  void onDataFilename() {
    // TODO hardcoded "json" extension
    std::string newFilename = app::show_file_selector(
      "Save JSON Data", m_dataFilename, "json", FileSelectorType::Save);
    if (newFilename.empty())
      return;

    m_dataFilename = newFilename;
    onFileNamesChange();
  }

  void onDataEnabledChange() {
    dataFilename()->setVisible(dataEnabled()->isSelected());
    resize();
  }

private:

  void resize() {
    gfx::Size reqSize = getPreferredSize();
    moveWindow(gfx::Rect(getOrigin(), reqSize));
    invalidate();
  }

  Sprite* m_sprite;
  DocumentPreferences& m_docPref;
  std::string m_filename;
  std::string m_dataFilename;
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
  app::gen::SpriteSheetType m_type;
  int m_columns;
  int m_width;
  int m_height;
  bool m_bestFit;
  std::string m_filename;
  std::string m_dataFilename;
};

ExportSpriteSheetCommand::ExportSpriteSheetCommand()
  : Command("ExportSpriteSheet",
            "Export Sprite Sheet",
            CmdRecordableFlag)
  , m_useUI(true)
{
}

void ExportSpriteSheetCommand::onLoadParams(const Params& params)
{
  if (params.has_param("ui"))
    m_useUI = params.get_as<bool>("ui");
  else
    m_useUI = true;
}

bool ExportSpriteSheetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ExportSpriteSheetCommand::onExecute(Context* context)
{
  Document* document(context->activeDocument());
  Sprite* sprite = document->sprite();
  DocumentPreferences& docPref(App::instance()->preferences().document(document));

  if (m_useUI && context->isUiAvailable()) {
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
  }

  m_type = docPref.spriteSheet.type();
  m_columns = docPref.spriteSheet.columns();
  m_width = docPref.spriteSheet.width();
  m_height = docPref.spriteSheet.height();
  m_bestFit = docPref.spriteSheet.bestFit();
  m_filename = docPref.spriteSheet.textureFilename();
  m_dataFilename = docPref.spriteSheet.dataFilename();

  if (m_bestFit) {
    Fit fit = best_fit(sprite);
    m_columns = fit.columns;
    m_width = fit.width;
    m_height = fit.height;
  }

  frame_t nframes = sprite->totalFrames();
  int columns;
  int sheet_w = 0;
  int sheet_h = 0;

  switch (m_type) {
    case app::gen::SpriteSheetType::HORIZONTAL_STRIP:
      columns = nframes;
      break;
    case app::gen::SpriteSheetType::VERTICAL_STRIP:
      columns = 1;
      break;
    case app::gen::SpriteSheetType::MATRIX:
      columns = m_columns;
      if (m_width > 0) sheet_w = m_width;
      if (m_height > 0) sheet_h = m_height;
      break;
  }

  columns = MID(1, columns, nframes);
  if (sheet_w == 0) sheet_w = sprite->width()*columns;
  if (sheet_h == 0) sheet_h = sprite->height()*((nframes/columns)+((nframes%columns)>0?1:0));
  columns = sheet_w / sprite->width();

  DocumentExporter exporter;
  exporter.setTextureFilename(m_filename);
  if (!m_dataFilename.empty())
    exporter.setDataFilename(m_dataFilename);
  exporter.setTextureWidth(sheet_w);
  exporter.setTextureHeight(sheet_h);
  exporter.setTexturePack(true);
  exporter.addDocument(document);
  exporter.exportSheet();

  StatusBar* statusbar = StatusBar::instance();
  if (statusbar)
    statusbar->showTip(1000, "Sprite Sheet Generated");
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
