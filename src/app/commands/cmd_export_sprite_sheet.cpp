// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_exporter.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/restore_visible_layers.h"
#include "app/ui/editor/editor.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "fmt/format.h"

#include "export_sprite_sheet.xml.h"

#include <limits>
#include <sstream>

namespace app {

using namespace ui;

namespace {

#ifdef ENABLE_UI
  // Special key value used in default preferences to know if by default
  // the user wants to generate texture and/or files.
  static const char* kSpecifiedFilename = "**filename**";
#endif

  struct Fit {
    int width;
    int height;
    int columns;
    int rows;
    int freearea;
    Fit() : width(0), height(0), columns(0), rows(0), freearea(0) {
    }
    Fit(int width, int height, int columns, int rows, int freearea) :
      width(width), height(height), columns(columns), rows(rows), freearea(freearea) {
    }
  };

  // Calculate best size for the given sprite
  // TODO this function was programmed in ten minutes, please optimize it
  Fit best_fit(Sprite* sprite, int nframes, int borderPadding, int shapePadding, int innerPadding) {
    int framew = sprite->width()+2*innerPadding;
    int frameh = sprite->height()+2*innerPadding;
    Fit result(framew*nframes, frameh, nframes, 1, std::numeric_limits<int>::max());
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

        Fit fit(w, h, (w / framew), (h / frameh), freearea);
        if (fit.freearea < result.freearea)
          result = fit;
      }

      if ((++z) & 1) w *= 2;
      else h *= 2;
    }

    return result;
  }

  Fit calculate_sheet_size(Sprite* sprite, int nframes,
                           int columns, int rows,
                           int borderPadding,
                           int shapePadding,
                           int innerPadding) {
    if (columns == 0) {
      rows = MID(1, rows, nframes);
      columns = ((nframes/rows) + ((nframes%rows) > 0 ? 1: 0));
    }
    else {
      columns = MID(1, columns, nframes);
      rows = ((nframes/columns) + ((nframes%columns) > 0 ? 1: 0));
    }

    return Fit(
      2*borderPadding + (sprite->width()+2*innerPadding)*columns + (columns-1)*shapePadding,
      2*borderPadding + (sprite->height()+2*innerPadding)*rows + (rows-1)*shapePadding,
      columns, rows, 0);
  }

#ifdef ENABLE_UI
  bool ask_overwrite(bool askFilename, std::string filename,
                     bool askDataname, std::string dataname) {
    if ((askFilename &&
         !filename.empty() &&
         base::is_file(filename)) ||
        (askDataname &&
         !dataname.empty() &&
         base::is_file(dataname))) {
      std::stringstream text;

      if (base::is_file(filename))
        text << "<<" << base::get_file_name(filename).c_str();

      if (base::is_file(dataname))
        text << "<<" << base::get_file_name(dataname).c_str();

      int ret = OptionalAlert::show(
        Preferences::instance().spriteSheet.showOverwriteFilesAlert,
        1, // Yes is the default option when the alert dialog is disabled
        fmt::format(Strings::alerts_overwrite_files_on_export_sprite_sheet(),
                    text.str()));
      if (ret != 1)
        return false;
    }
    return true;
  }
#endif // ENABLE_UI

}

struct ExportSpriteSheetParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<bool> askOverwrite { this, true, { "askOverwrite", "ask-overwrite" } };
  Param<app::SpriteSheetType> type { this, app::SpriteSheetType::None, "type" };
  Param<int> columns { this, 0, "columns" };
  Param<int> rows { this, 0, "rows" };
  Param<int> width { this, 0, "width" };
  Param<int> height { this, 0, "height" };
  Param<bool> bestFit { this, false, "bestFit" };
  Param<std::string> textureFilename { this, std::string(), "textureFilename" };
  Param<std::string> dataFilename { this, std::string(), "dataFilename" };
  Param<DocExporter::DataFormat> dataFormat { this, DocExporter::DefaultDataFormat, "dataFormat" };
  Param<int> borderPadding { this, 0, "borderPadding" };
  Param<int> shapePadding { this, 0, "shapePadding" };
  Param<int> innerPadding { this, 0, "innerPadding" };
  Param<bool> trim { this, false, "trim" };
  Param<bool> trimByGrid { this, false, "trimByGrid" };
  Param<bool> extrude { this, false, "extrude" };
  Param<bool> openGenerated { this, false, "openGenerated" };
  Param<std::string> layer { this, std::string(), "layer" };
  Param<std::string> tag { this, std::string(), "tag" };
  Param<bool> listLayers { this, true, "listLayers" };
  Param<bool> listTags { this, true, "listTags" };
  Param<bool> listSlices { this, true, "listSlices" };
};

#if ENABLE_UI

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  ExportSpriteSheetWindow(Site& site, ExportSpriteSheetParams& params)
    : m_site(site)
    , m_sprite(site.sprite())
    , m_filenameAskOverwrite(true)
    , m_dataFilenameAskOverwrite(true)
  {
    static_assert(
      (int)app::SpriteSheetType::None == 0 &&
      (int)app::SpriteSheetType::Horizontal == 1 &&
      (int)app::SpriteSheetType::Vertical == 2 &&
      (int)app::SpriteSheetType::Rows == 3 &&
      (int)app::SpriteSheetType::Columns == 4,
      "SpriteSheetType enum changed");

    sheetType()->addItem("Horizontal Strip");
    sheetType()->addItem("Vertical Strip");
    sheetType()->addItem("By Rows");
    sheetType()->addItem("By Columns");
    if (params.type() != app::SpriteSheetType::None)
      sheetType()->setSelectedItemIndex((int)params.type()-1);

    fill_layers_combobox(
      m_sprite, layers(), params.layer());

    fill_frames_combobox(
      m_sprite, frames(), params.tag());

    openGenerated()->setSelected(params.openGenerated());
    trimEnabled()->setSelected(params.trim());
    trimContainer()->setVisible(trimEnabled()->isSelected());
    gridTrimEnabled()->setSelected(trimEnabled()->isSelected() && params.trimByGrid());
    extrudeEnabled()->setSelected(params.extrude());

    borderPadding()->setTextf("%d", params.borderPadding());
    shapePadding()->setTextf("%d", params.shapePadding());
    innerPadding()->setTextf("%d", params.innerPadding());
    paddingEnabled()->setSelected(
      params.borderPadding() ||
      params.shapePadding() ||
      params.innerPadding());
    paddingContainer()->setVisible(paddingEnabled()->isSelected());

    for (int i=2; i<=8192; i*=2) {
      std::string value = base::convert_to<std::string>(i);
      if (i >= m_sprite->width()) fitWidth()->addItem(value);
      if (i >= m_sprite->height()) fitHeight()->addItem(value);
    }

    if (params.bestFit()) {
      bestFit()->setSelected(true);
      onBestFit();
    }
    else {
      columns()->setTextf("%d", params.columns());
      rows()->setTextf("%d", params.rows());
      onColumnsChange();

      if (params.width() > 0 || params.height() > 0) {
        if (params.width() > 0)
          fitWidth()->getEntryWidget()->setTextf("%d", params.width());

        if (params.height() > 0)
          fitHeight()->getEntryWidget()->setTextf("%d", params.height());

        onSizeChange();
      }
    }

    m_filename = params.textureFilename();
    imageEnabled()->setSelected(!m_filename.empty());
    imageFilename()->setVisible(imageEnabled()->isSelected());

    m_dataFilename = params.dataFilename();
    dataEnabled()->setSelected(!m_dataFilename.empty());
    dataFormat()->setSelectedItemIndex(int(params.dataFormat()));
    listLayers()->setSelected(params.listLayers());
    listTags()->setSelected(params.listTags());
    listSlices()->setSelected(params.listSlices());
    updateDataFields();

    std::string base = site.document()->filename();
    base = base::join_path(base::get_file_path(base), base::get_file_title(base));

    if (m_filename.empty() ||
        m_filename == kSpecifiedFilename) {
      std::string defExt = Preferences::instance().spriteSheet.defaultExtension();

      if (base::utf8_icmp(base::get_file_extension(site.document()->filename()), defExt) == 0)
        m_filename = base + "-sheet." + defExt;
      else
        m_filename = base + "." + defExt;
    }

    if (m_dataFilename.empty() ||
        m_dataFilename == kSpecifiedFilename)
      m_dataFilename = base + ".json";

    exportButton()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onExport, this));
    sheetType()->Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    columns()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onColumnsChange, this));
    rows()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onRowsChange, this));
    fitWidth()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    fitHeight()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    bestFit()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onBestFit, this));
    borderPadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    shapePadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    innerPadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    extrudeEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onExtrudeChange, this));
    imageEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onImageEnabledChange, this));
    imageFilename()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onImageFilename, this));
    dataEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataEnabledChange, this));
    dataFilename()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataFilename, this));
    trimEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onTrimEnabledChange, this));
    paddingEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onPaddingEnabledChange, this));
    frames()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onFramesChange, this));
    openGenerated()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onOpenGeneratedChange, this));

    onSheetTypeChange();
    onFileNamesChange();
    updateExportButton();
  }

  bool ok() const {
    return closer() == exportButton();
  }

  app::SpriteSheetType spriteSheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  int columnsValue() const {
    if (spriteSheetTypeValue() != SpriteSheetType::Columns)
      return columns()->textInt();
    else
      return 0;
  }

  int rowsValue() const {
    if (spriteSheetTypeValue() == SpriteSheetType::Columns)
      return rows()->textInt();
    else
      return 0;
  }

  int fitWidthValue() const {
    return fitWidth()->getEntryWidget()->textInt();
  }

  int fitHeightValue() const {
    return fitHeight()->getEntryWidget()->textInt();
  }

  bool bestFitValue() const {
    return bestFit()->isSelected();
  }

  std::string filenameValue() const {
    if (imageEnabled()->isSelected())
      return m_filename;
    else
      return std::string();
  }

  std::string dataFilenameValue() const {
    if (dataEnabled()->isSelected())
      return m_dataFilename;
    else
      return std::string();
  }

  DocExporter::DataFormat dataFormatValue() const {
    if (dataEnabled()->isSelected())
      return DocExporter::DataFormat(dataFormat()->getSelectedItemIndex());
    else
      return DocExporter::DefaultDataFormat;
  }

  int borderPaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = borderPadding()->textInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  int shapePaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = shapePadding()->textInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  int innerPaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = innerPadding()->textInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  bool trimValue() const {
    return trimEnabled()->isSelected();
  }

  bool trimByGridValue() const {
    return gridTrimEnabled()->isSelected();
  }

  bool extrudeValue() const {
    return extrudeEnabled()->isSelected();
  }

  bool extrudePadding() const {
    return (extrudeValue() ? 1: 0);
  }

  bool openGeneratedValue() const {
    return openGenerated()->isSelected();
  }

  std::string layerValue() const {
    return layers()->getValue();
  }

  std::string frameTagValue() const {
    return frames()->getValue();
  }

  bool listLayersValue() const {
    return listLayers()->isSelected();
  }

  bool listFrameTagsValue() const {
    return listTags()->isSelected();
  }

  bool listSlicesValue() const {
    return listSlices()->isSelected();
  }

private:

  void onExport() {
    if (!ask_overwrite(m_filenameAskOverwrite, filenameValue(),
                       m_dataFilenameAskOverwrite, dataFilenameValue()))
      return;

    closeWindow(exportButton());
  }

  void onSheetTypeChange() {
    bool rowsState = false;
    bool colsState = false;
    bool matrixState = false;
    switch (spriteSheetTypeValue()) {
      case app::SpriteSheetType::Rows:
        colsState = true;
        matrixState = true;
        break;
      case app::SpriteSheetType::Columns:
        rowsState = true;
        matrixState = true;
        break;
    }

    columnsLabel()->setVisible(colsState);
    columns()->setVisible(colsState);

    rowsLabel()->setVisible(rowsState);
    rows()->setVisible(rowsState);

    fitWidthLabel()->setVisible(matrixState);
    fitWidth()->setVisible(matrixState);
    fitHeightLabel()->setVisible(matrixState);
    fitHeight()->setVisible(matrixState);
    bestFitFiller()->setVisible(matrixState);
    bestFit()->setVisible(matrixState);

    resize();
  }

  void onFileNamesChange() {
    imageFilename()->setText(base::get_file_name(m_filename));
    dataFilename()->setText(base::get_file_name(m_dataFilename));
    resize();
  }

  void onColumnsChange() {
    bestFit()->setSelected(false);
    updateSizeFields();
  }

  void onRowsChange() {
    bestFit()->setSelected(false);
    updateSizeFields();
  }

  void onSizeChange() {
    columns()->setTextf("%d", fitWidthValue() / m_sprite->width());
    rows()->setTextf("%d", fitHeightValue() / m_sprite->height());
    bestFit()->setSelected(false);
  }

  void onBestFit() {
    updateSizeFields();
  }

  void onImageFilename() {
    base::paths newFilename;
    if (!app::show_file_selector(
          "Save Sprite Sheet", m_filename,
          get_writable_extensions(),
          FileSelectorType::Save, newFilename))
      return;

    ASSERT(!newFilename.empty());

    m_filename = newFilename.front();
    m_filenameAskOverwrite = false; // Already asked in file selector
    onFileNamesChange();
  }

  void onImageEnabledChange() {
    m_filenameAskOverwrite = true;

    imageFilename()->setVisible(imageEnabled()->isSelected());
    updateExportButton();
    resize();
  }

  void onDataFilename() {
    // TODO hardcoded "json" extension
    base::paths exts = { "json" };
    base::paths newFilename;
    if (!app::show_file_selector(
          "Save JSON Data", m_dataFilename, exts,
          FileSelectorType::Save, newFilename))
      return;

    ASSERT(!newFilename.empty());

    m_dataFilename = newFilename.front();
    m_dataFilenameAskOverwrite = false; // Already asked in file selector
    onFileNamesChange();
  }

  void onDataEnabledChange() {
    m_dataFilenameAskOverwrite = true;

    updateDataFields();
    updateExportButton();
    resize();
  }

  void onTrimEnabledChange() {
      trimContainer()->setVisible(trimEnabled()->isSelected());
      resize();
      updateSizeFields();
  }

  void onPaddingEnabledChange() {
    paddingContainer()->setVisible(paddingEnabled()->isSelected());
    resize();
    updateSizeFields();
  }

  void onPaddingChange() {
    updateSizeFields();
  }

  void onExtrudeChange() {
    updateSizeFields();
  }

  void onFramesChange() {
    updateSizeFields();
  }

  void onOpenGeneratedChange() {
    updateExportButton();
  }

  void resize() {
    gfx::Size reqSize = sizeHint();
    moveWindow(gfx::Rect(origin(), reqSize));
    layout();
    invalidate();
  }

  void updateExportButton() {
    exportButton()->setEnabled(
      imageEnabled()->isSelected() ||
      dataEnabled()->isSelected() ||
      openGenerated()->isSelected());
  }

  void updateSizeFields() {
    SelectedFrames selFrames;
    calculate_selected_frames(m_site,
                              frameTagValue(),
                              selFrames);

    frame_t nframes = selFrames.size();

    Fit fit;
    if (bestFit()->isSelected()) {
      fit = best_fit(m_sprite, nframes,
                     borderPaddingValue(), shapePaddingValue(),
                     innerPaddingValue() + extrudePadding());
    }
    else {
      fit = calculate_sheet_size(
        m_sprite, nframes,
        columnsValue(),
        rowsValue(),
        borderPaddingValue(),
        shapePaddingValue(),
        innerPaddingValue() + extrudePadding());
    }

    columns()->setTextf("%d", fit.columns);
    rows()->setTextf("%d", fit.rows);
    fitWidth()->getEntryWidget()->setTextf("%d", fit.width);
    fitHeight()->getEntryWidget()->setTextf("%d", fit.height);
  }

  void updateDataFields() {
    bool state = dataEnabled()->isSelected();
    dataFilename()->setVisible(state);
    dataMeta()->setVisible(state);
  }

  Site& m_site;
  Sprite* m_sprite;
  std::string m_filename;
  std::string m_dataFilename;
  bool m_filenameAskOverwrite;
  bool m_dataFilenameAskOverwrite;
};

#endif // ENABLE_UI

class ExportSpriteSheetCommand : public CommandWithNewParams<ExportSpriteSheetParams> {
public:
  ExportSpriteSheetCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ExportSpriteSheetCommand::ExportSpriteSheetCommand()
  : CommandWithNewParams(CommandId::ExportSpriteSheet(), CmdRecordableFlag)
{
}

bool ExportSpriteSheetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ExportSpriteSheetCommand::onExecute(Context* context)
{
  Site site = context->activeSite();
  Doc* document = site.document();
  Sprite* sprite = site.sprite();
  auto& params = this->params();

#ifdef ENABLE_UI
  // TODO if we use this line when !ENABLE_UI,
  // Preferences::~Preferences() crashes on Linux when it wants to
  // save the document preferences. It looks like
  // Preferences::onRemoveDocument() is not called for some documents
  // and when the Preferences::m_docs collection is iterated to save
  // all DocumentPreferences, it accesses an invalid Doc* pointer (an
  // already removed/deleted document).
  DocumentPreferences& docPref(Preferences::instance().document(document));

  bool askOverwrite = params.askOverwrite();
  // Show UI if the user specified it explicitly or the sprite sheet type wasn't specified.
  if (context->isUIAvailable() && params.ui() &&
      (params.ui.isSet() || !params.type.isSet())) {
    // Copy document preferences to undefined params
    if (docPref.spriteSheet.defined(true) &&
        !params.type.isSet()) {
      params.type(docPref.spriteSheet.type());
      if (!params.columns.isSet())          params.columns(         docPref.spriteSheet.columns());
      if (!params.rows.isSet())             params.rows(            docPref.spriteSheet.rows());
      if (!params.width.isSet())            params.width(           docPref.spriteSheet.width());
      if (!params.height.isSet())           params.height(          docPref.spriteSheet.height());
      if (!params.bestFit.isSet())          params.bestFit(         docPref.spriteSheet.bestFit());
      if (!params.textureFilename.isSet())  params.textureFilename( docPref.spriteSheet.textureFilename());
      if (!params.dataFilename.isSet())     params.dataFilename(    docPref.spriteSheet.dataFilename());
      if (!params.dataFormat.isSet())       params.dataFormat(      docPref.spriteSheet.dataFormat());
      if (!params.borderPadding.isSet())    params.borderPadding(   docPref.spriteSheet.borderPadding());
      if (!params.shapePadding.isSet())     params.shapePadding(    docPref.spriteSheet.shapePadding());
      if (!params.innerPadding.isSet())     params.innerPadding(    docPref.spriteSheet.innerPadding());
      if (!params.trim.isSet())             params.trim(            docPref.spriteSheet.trim());
      if (!params.trimByGrid.isSet())       params.trimByGrid(      docPref.spriteSheet.trimByGrid());
      if (!params.extrude.isSet())          params.extrude(         docPref.spriteSheet.extrude());
      if (!params.openGenerated.isSet())    params.openGenerated(   docPref.spriteSheet.openGenerated());
      if (!params.layer.isSet())            params.layer(           docPref.spriteSheet.layer());
      if (!params.tag.isSet())              params.tag(             docPref.spriteSheet.frameTag());
      if (!params.listLayers.isSet())       params.listLayers(      docPref.spriteSheet.listLayers());
      if (!params.listTags.isSet())         params.listTags(        docPref.spriteSheet.listFrameTags());
      if (!params.listSlices.isSet())       params.listSlices(      docPref.spriteSheet.listSlices());
    }

    ExportSpriteSheetWindow window(site, params);
    window.openWindowInForeground();
    if (!window.ok())
      return;

    docPref.spriteSheet.defined(true);
    docPref.spriteSheet.type            (params.type            (window.spriteSheetTypeValue()));
    docPref.spriteSheet.columns         (params.columns         (window.columnsValue()));
    docPref.spriteSheet.rows            (params.rows            (window.rowsValue()));
    docPref.spriteSheet.width           (params.width           (window.fitWidthValue()));
    docPref.spriteSheet.height          (params.height          (window.fitHeightValue()));
    docPref.spriteSheet.bestFit         (params.bestFit         (window.bestFitValue()));
    docPref.spriteSheet.textureFilename (params.textureFilename (window.filenameValue()));
    docPref.spriteSheet.dataFilename    (params.dataFilename    (window.dataFilenameValue()));
    docPref.spriteSheet.dataFormat      (params.dataFormat      (window.dataFormatValue()));
    docPref.spriteSheet.borderPadding   (params.borderPadding   (window.borderPaddingValue()));
    docPref.spriteSheet.shapePadding    (params.shapePadding    (window.shapePaddingValue()));
    docPref.spriteSheet.innerPadding    (params.innerPadding    (window.innerPaddingValue()));
    docPref.spriteSheet.trim            (params.trim            (window.trimValue()));
    docPref.spriteSheet.trimByGrid      (params.trimByGrid      (window.trimByGridValue()));
    docPref.spriteSheet.extrude         (params.extrude         (window.extrudeValue()));
    docPref.spriteSheet.openGenerated   (params.openGenerated   (window.openGeneratedValue()));
    docPref.spriteSheet.layer           (params.layer           (window.layerValue()));
    docPref.spriteSheet.frameTag        (params.tag             (window.frameTagValue()));
    docPref.spriteSheet.listLayers      (params.listLayers      (window.listLayersValue()));
    docPref.spriteSheet.listFrameTags   (params.listTags        (window.listFrameTagsValue()));
    docPref.spriteSheet.listSlices      (params.listSlices      (window.listSlicesValue()));

    // Default preferences for future sprites
    DocumentPreferences& defPref(Preferences::instance().document(nullptr));
    defPref.spriteSheet = docPref.spriteSheet;
    defPref.spriteSheet.defined(false);
    if (!defPref.spriteSheet.textureFilename().empty())
      defPref.spriteSheet.textureFilename.setValueAndDefault(kSpecifiedFilename);
    if (!defPref.spriteSheet.dataFilename().empty())
      defPref.spriteSheet.dataFilename.setValueAndDefault(kSpecifiedFilename);
    defPref.save();

    askOverwrite = false; // Already asked in the ExportSpriteSheetWindow
  }
#endif // ENABLE_UI

  const app::SpriteSheetType type = params.type();
  int columns = params.columns();
  int rows = params.rows();
  int width = params.width();
  int height = params.height();
  const bool bestFit = params.bestFit();
  const std::string filename = params.textureFilename();
  const std::string dataFilename = params.dataFilename();
  const DocExporter::DataFormat dataFormat = params.dataFormat();
  const std::string layerName = params.layer();
  const std::string tagName = params.tag();
  const int borderPadding = base::clamp(params.borderPadding(), 0, 100);
  const int shapePadding = base::clamp(params.shapePadding(), 0, 100);
  const int innerPadding = base::clamp(params.innerPadding(), 0, 100);
  const bool trimCels = params.trim();
  const bool trimByGrid = params.trimByGrid();
  const bool extrude = params.extrude();
  const int extrudePadding = (extrude ? 1: 0);
  const bool listLayers = params.listLayers();
  const bool listTags = params.listTags();
  const bool listSlices = params.listSlices();

#ifdef ENABLE_UI
  if (context->isUIAvailable() && askOverwrite) {
    if (!ask_overwrite(true, filename,
                       true, dataFilename))
      return;                   // Do not overwrite
  }
#endif

  SelectedFrames selFrames;
  FrameTag* frameTag =
    calculate_selected_frames(site, tagName, selFrames);

  frame_t nframes = selFrames.size();
  ASSERT(nframes > 0);

  // If the user choose to render selected layers only, we can
  // temporaly make them visible and hide the other ones.
  RestoreVisibleLayers layersVisibility;
  calculate_visible_layers(site, layerName, layersVisibility);

  SelectedLayers selLayers;
  if (layerName != kSelectedLayers) {
    // TODO add a getLayerByName
    for (Layer* layer : sprite->allLayers()) {
      if (layer->name() == layerName) {
        selLayers.insert(layer);
        break;
      }
    }
  }

  if (bestFit) {
    Fit fit = best_fit(sprite, nframes, borderPadding, shapePadding,
                       innerPadding + extrudePadding);
    columns = fit.columns;
    rows = fit.rows;
    width = fit.width;
    height = fit.height;
  }

  int sheet_w = 0;
  int sheet_h = 0;

  switch (type) {
    case app::SpriteSheetType::Horizontal:
      columns = sprite->totalFrames();
      rows = 1;
      break;
    case app::SpriteSheetType::Vertical:
      columns = 1;
      rows = nframes;
      break;
    case app::SpriteSheetType::Rows:
    case app::SpriteSheetType::Columns:
      if (width > 0) sheet_w = width;
      if (height > 0) sheet_h = height;
      break;
  }

  Fit fit = calculate_sheet_size(
    sprite, nframes,
    columns, rows,
    borderPadding, shapePadding, innerPadding + extrudePadding);
  if (sheet_w == 0) sheet_w = fit.width;
  if (sheet_h == 0) sheet_h = fit.height;

  DocExporter exporter;
  if (!filename.empty())
    exporter.setTextureFilename(filename);
  if (!dataFilename.empty()) {
    exporter.setDataFilename(dataFilename);
    exporter.setDataFormat(dataFormat);
  }
  exporter.setTextureWidth(sheet_w);
  exporter.setTextureHeight(sheet_h);
  exporter.setSpriteSheetType(type);
  exporter.setBorderPadding(borderPadding);
  exporter.setShapePadding(shapePadding);
  exporter.setInnerPadding(innerPadding);
  exporter.setTrimCels(trimCels);
  exporter.setTrimByGrid(trimByGrid);
  exporter.setExtrude(extrude);
  if (listLayers) exporter.setListLayers(true);
  if (listTags) exporter.setListFrameTags(true);
  if (listSlices) exporter.setListSlices(true);
  exporter.addDocument(document, frameTag,
                       (!selLayers.empty() ? &selLayers: nullptr),
                       (!selFrames.empty() ? &selFrames: nullptr));

  std::unique_ptr<Doc> newDocument(exporter.exportSheet(context));
  if (!newDocument)
    return;

#ifdef ENABLE_UI
  if (context->isUIAvailable()) {
    StatusBar* statusbar = StatusBar::instance();
    if (statusbar)
      statusbar->showTip(1000, "Sprite Sheet Generated");

    // Copy background and grid preferences
    DocumentPreferences& newDocPref(
      Preferences::instance().document(newDocument.get()));
    newDocPref.bg = docPref.bg;
    newDocPref.grid = docPref.grid;
    newDocPref.pixelGrid = docPref.pixelGrid;
    Preferences::instance().removeDocument(newDocument.get());
  }
#endif

  if (params.openGenerated()) {
    // Setup a filename for the new document in case that user didn't
    // save the file/specified one output filename.
    if (filename.empty()) {
      std::string fn = document->filename();
      std::string ext = base::get_file_extension(fn);
      if (!ext.empty())
        ext.insert(0, 1, '.');

      newDocument->setFilename(
        base::join_path(base::get_file_path(fn),
                        base::get_file_title(fn) + "-Sheet") + ext);
    }

    newDocument->setContext(context);
    newDocument.release();
  }
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
