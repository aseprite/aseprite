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
#include "app/filename_formatter.h"
#include "app/i18n/strings.h"
#include "app/job.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/restore_visible_layers.h"
#include "app/task.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/navigate_state.h"
#include "app/ui/layer_frame_comboboxes.h"
#include "app/ui/optional_alert.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/string.h"
#include "base/thread.h"
#include "doc/layer.h"
#include "doc/tag.h"
#include "fmt/format.h"
#include "ui/system.h"

#include "export_sprite_sheet.xml.h"

#include <limits>
#include <sstream>

namespace app {

using namespace ui;

namespace {

struct ExportSpriteSheetParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<bool> askOverwrite { this, true, { "askOverwrite", "ask-overwrite" } };
  Param<app::SpriteSheetType> type { this, app::SpriteSheetType::None, "type" };
  Param<int> columns { this, 0, "columns" };
  Param<int> rows { this, 0, "rows" };
  Param<int> width { this, 0, "width" };
  Param<int> height { this, 0, "height" };
  Param<std::string> textureFilename { this, std::string(), "textureFilename" };
  Param<std::string> dataFilename { this, std::string(), "dataFilename" };
  Param<SpriteSheetDataFormat> dataFormat { this, SpriteSheetDataFormat::Default, "dataFormat" };
  Param<std::string> filenameFormat { this, std::string(), "filenameFormat" };
  Param<int> borderPadding { this, 0, "borderPadding" };
  Param<int> shapePadding { this, 0, "shapePadding" };
  Param<int> innerPadding { this, 0, "innerPadding" };
  Param<bool> trimSprite { this, false, "trimSprite" };
  Param<bool> trim { this, false, "trim" };
  Param<bool> trimByGrid { this, false, "trimByGrid" };
  Param<bool> extrude { this, false, "extrude" };
  Param<bool> openGenerated { this, false, "openGenerated" };
  Param<std::string> layer { this, std::string(), "layer" };
  Param<std::string> tag { this, std::string(), "tag" };
  Param<bool> splitLayers { this, false, "splitLayers" };
  Param<bool> splitTags { this, false, "splitTags" };
  Param<bool> listLayers { this, true, "listLayers" };
  Param<bool> listTags { this, true, "listTags" };
  Param<bool> listSlices { this, true, "listSlices" };
};

#ifdef ENABLE_UI

enum ConstraintType {
  kConstraintType_None,
  kConstraintType_Cols,
  kConstraintType_Rows,
  kConstraintType_Width,
  kConstraintType_Height,
  kConstraintType_Size,
};

// Special key value used in default preferences to know if by default
// the user wants to generate texture and/or files.
static const char* kSpecifiedFilename = "**filename**";

bool ask_overwrite(const bool askFilename, const std::string& filename,
                   const bool askDataname, const std::string& dataname)
{
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

    const int ret =
      OptionalAlert::show(
        Preferences::instance().spriteSheet.showOverwriteFilesAlert,
        1, // Yes is the default option when the alert dialog is disabled
        fmt::format(Strings::alerts_overwrite_files_on_export_sprite_sheet(),
                    text.str()));
    if (ret != 1)
      return false;
  }
  return true;
}

ConstraintType constraint_type_from_params(const ExportSpriteSheetParams& params)
{
  switch (params.type()) {
    case app::SpriteSheetType::Rows:
      if (params.width() > 0)
        return kConstraintType_Width;
      else if (params.columns() > 0)
        return kConstraintType_Cols;
      break;
    case app::SpriteSheetType::Columns:
      if (params.height() > 0)
        return kConstraintType_Height;
      else if (params.rows() > 0)
        return kConstraintType_Rows;
      break;
    case app::SpriteSheetType::Packed:
      if (params.width() > 0 && params.height() > 0)
        return kConstraintType_Size;
      else if (params.width() > 0)
        return kConstraintType_Width;
      else if (params.height() > 0)
        return kConstraintType_Height;
      break;
  }
  return kConstraintType_None;
}

#endif // ENABLE_UI

void update_doc_exporter_from_params(DocExporter& exporter,
                                     const Site& site,
                                     const ExportSpriteSheetParams& params,
                                     const bool saveData)
{
  const app::SpriteSheetType type = params.type();
  const int columns = params.columns();
  const int rows = params.rows();
  const int width = params.width();
  const int height = params.height();
  const std::string filename = params.textureFilename();
  const std::string dataFilename = params.dataFilename();
  const SpriteSheetDataFormat dataFormat = params.dataFormat();
  const std::string filenameFormat = params.filenameFormat();
  const std::string layerName = params.layer();
  const std::string tagName = params.tag();
  const int borderPadding = base::clamp(params.borderPadding(), 0, 100);
  const int shapePadding = base::clamp(params.shapePadding(), 0, 100);
  const int innerPadding = base::clamp(params.innerPadding(), 0, 100);
  const bool trimSprite = params.trimSprite();
  const bool trimCels = params.trim();
  const bool trimByGrid = params.trimByGrid();
  const bool extrude = params.extrude();
  const bool splitLayers = params.splitLayers();
  const bool splitTags = params.splitTags();
  const bool listLayers = params.listLayers();
  const bool listTags = params.listTags();
  const bool listSlices = params.listSlices();

  SelectedFrames selFrames;
  Tag* tag = calculate_selected_frames(site, tagName, selFrames);
  frame_t nframes = selFrames.size();
  ASSERT(nframes > 0);

  Doc* doc = const_cast<Doc*>(site.document());
  const Sprite* sprite = site.sprite();

  // If the user choose to render selected layers only, we can
  // temporaly make them visible and hide the other ones.
  RestoreVisibleLayers layersVisibility;
  calculate_visible_layers(site, layerName, layersVisibility);

  SelectedLayers selLayers;
  if (layerName != kSelectedLayers) {
    // TODO add a getLayerByName
    for (const Layer* layer : sprite->allLayers()) {
      if (layer->name() == layerName) {
        selLayers.insert(const_cast<Layer*>(layer));
        break;
      }
    }
  }

  exporter.reset();
  exporter.addDocumentSamples(
    doc, tag, splitLayers, splitTags,
    !selLayers.empty() ? &selLayers: nullptr,
    !selFrames.empty() ? &selFrames: nullptr);

  if (!filename.empty())
    exporter.setTextureFilename(filename);
  if (!dataFilename.empty() && saveData) {
    exporter.setDataFilename(dataFilename);
    exporter.setDataFormat(dataFormat);
  }
  if (!filenameFormat.empty())
    exporter.setFilenameFormat(filenameFormat);

  exporter.setTextureWidth(width);
  exporter.setTextureHeight(height);
  exporter.setTextureColumns(columns);
  exporter.setTextureRows(rows);
  exporter.setSpriteSheetType(type);
  exporter.setBorderPadding(borderPadding);
  exporter.setShapePadding(shapePadding);
  exporter.setInnerPadding(innerPadding);
  exporter.setTrimSprite(trimSprite);
  exporter.setTrimCels(trimCels);
  exporter.setTrimByGrid(trimByGrid);
  exporter.setExtrude(extrude);
  if (listLayers) exporter.setListLayers(true);
  if (listTags) exporter.setListTags(true);
  if (listSlices) exporter.setListSlices(true);
}

std::unique_ptr<Doc> generate_sprite_sheet(
  DocExporter& exporter,
  Context* ctx,
  const Site& site,
  const ExportSpriteSheetParams& params,
  bool saveData,
  base::task_token& token)
{
  update_doc_exporter_from_params(exporter, site, params, saveData);
  std::unique_ptr<Doc> newDocument(
    exporter.exportSheet(ctx, token));
  if (newDocument) {
    // Setup a filename for the new document in case that user didn't
    // save the file/specified one output filename.
    if (params.textureFilename().empty()) {
      std::string fn = site.document()->filename();
      std::string ext = base::get_file_extension(fn);
      if (!ext.empty())
        ext.insert(0, 1, '.');

      newDocument->setFilename(
        base::join_path(base::get_file_path(fn),
                        base::get_file_title(fn) + "-Sheet") + ext);
    }
  }
  return newDocument;
}

#if ENABLE_UI

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  ExportSpriteSheetWindow(DocExporter& exporter,
                          Site& site,
                          ExportSpriteSheetParams& params)
    : m_exporter(exporter)
    , m_frontBuffer(std::make_shared<doc::ImageBuffer>())
    , m_backBuffer(std::make_shared<doc::ImageBuffer>())
    , m_site(site)
    , m_sprite(site.sprite())
    , m_filenameAskOverwrite(true)
    , m_dataFilenameAskOverwrite(true)
    , m_editor(nullptr)
    , m_genTimer(100, nullptr)
    , m_executeFromUI(0)
    , m_filenameFormat(params.filenameFormat())
  {
    static_assert(
      (int)app::SpriteSheetType::None == 0 &&
      (int)app::SpriteSheetType::Horizontal == 1 &&
      (int)app::SpriteSheetType::Vertical == 2 &&
      (int)app::SpriteSheetType::Rows == 3 &&
      (int)app::SpriteSheetType::Columns == 4 &&
      (int)app::SpriteSheetType::Packed == 5,
      "SpriteSheetType enum changed");

    sheetType()->addItem(Strings::export_sprite_sheet_type_horz());
    sheetType()->addItem(Strings::export_sprite_sheet_type_vert());
    sheetType()->addItem(Strings::export_sprite_sheet_type_rows());
    sheetType()->addItem(Strings::export_sprite_sheet_type_cols());
    sheetType()->addItem(Strings::export_sprite_sheet_type_pack());
    if (params.type() != app::SpriteSheetType::None)
      sheetType()->setSelectedItemIndex((int)params.type()-1);

    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_none());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_cols());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_rows());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_width());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_height());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_size());
    constraintType()->setSelectedItemIndex(constraint_type_from_params(params));

    fill_layers_combobox(
      m_sprite, layers(), params.layer());

    fill_frames_combobox(
      m_sprite, frames(), params.tag());

    openGenerated()->setSelected(params.openGenerated());
    trimSpriteEnabled()->setSelected(params.trimSprite());
    trimEnabled()->setSelected(params.trim());
    trimContainer()->setVisible(trimSpriteEnabled()->isSelected() ||
                                trimEnabled()->isSelected());
    gridTrimEnabled()->setSelected((trimSpriteEnabled()->isSelected() ||
                                    trimEnabled()->isSelected()) &&
                                   params.trimByGrid());
    extrudeEnabled()->setSelected(params.extrude());

    borderPadding()->setTextf("%d", params.borderPadding());
    shapePadding()->setTextf("%d", params.shapePadding());
    innerPadding()->setTextf("%d", params.innerPadding());
    paddingEnabled()->setSelected(
      params.borderPadding() ||
      params.shapePadding() ||
      params.innerPadding());
    paddingContainer()->setVisible(paddingEnabled()->isSelected());

    m_filename = params.textureFilename();
    imageEnabled()->setSelected(!m_filename.empty());
    imageFilename()->setVisible(imageEnabled()->isSelected());

    m_dataFilename = params.dataFilename();
    dataEnabled()->setSelected(!m_dataFilename.empty());
    dataFormat()->setSelectedItemIndex(int(params.dataFormat()));
    splitLayers()->setSelected(params.splitLayers());
    splitTags()->setSelected(params.splitTags());
    listLayers()->setSelected(params.listLayers());
    listTags()->setSelected(params.listTags());
    listSlices()->setSelected(params.listSlices());

    updateDefaultDataFilenameFormat();
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
    showConstraints()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSheetTypeChange, this));
    constraintType()->Change.connect(&ExportSpriteSheetWindow::onConstraintTypeChange, this);
    widthConstraint()->Change.connect(&ExportSpriteSheetWindow::generatePreview, this);
    heightConstraint()->Change.connect(&ExportSpriteSheetWindow::generatePreview, this);
    borderPadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    shapePadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    innerPadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    extrudeEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    imageEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onImageEnabledChange, this));
    imageFilename()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onImageFilename, this));
    dataEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataEnabledChange, this));
    dataFilename()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataFilename, this));
    trimSpriteEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onTrimEnabledChange, this));
    trimEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onTrimEnabledChange, this));
    gridTrimEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    paddingEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onPaddingEnabledChange, this));
    layers()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    splitLayers()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSplitLayersOrFrames, this));
    splitTags()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSplitLayersOrFrames, this));
    frames()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    dataFilenameFormat()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataFilenameFormatChange, this));
    openGenerated()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onOpenGeneratedChange, this));
    preview()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    m_genTimer.Tick.connect(base::Bind<void>(&ExportSpriteSheetWindow::onGenTimerTick, this));

    onSheetTypeChange();
    onFileNamesChange();
    updateExportButton();

    preview()->setSelected(Preferences::instance().spriteSheet.preview());
    generatePreview();
  }

  ~ExportSpriteSheetWindow() {
    ASSERT(m_executeFromUI == 0);
    cancelGenTask();
    if (m_spriteSheet) {
      DocDestroyer destroyer(UIContext::instance(), m_spriteSheet.release(), 100);
      destroyer.destroyDocument();
    }
  }

  bool ok() const {
    return closer() == exportButton();
  }

  void updateParams(ExportSpriteSheetParams& params) {
    params.type            (spriteSheetTypeValue());
    params.columns         (columnsValue());
    params.rows            (rowsValue());
    params.width           (widthValue());
    params.height          (heightValue());
    params.textureFilename (filenameValue());
    params.dataFilename    (dataFilenameValue());
    params.dataFormat      (dataFormatValue());
    params.filenameFormat  (filenameFormatValue());
    params.borderPadding   (borderPaddingValue());
    params.shapePadding    (shapePaddingValue());
    params.innerPadding    (innerPaddingValue());
    params.trimSprite      (trimSpriteValue());
    params.trim            (trimValue());
    params.trimByGrid      (trimByGridValue());
    params.extrude         (extrudeValue());
    params.openGenerated   (openGeneratedValue());
    params.layer           (layerValue());
    params.tag             (tagValue());
    params.splitLayers     (splitLayersValue());
    params.splitTags       (splitTagsValue());
    params.listLayers      (listLayersValue());
    params.listTags        (listTagsValue());
    params.listSlices      (listSlicesValue());
  }

private:

  void onBroadcastMouseMessage(WidgetsList& targets) override {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

  app::SpriteSheetType spriteSheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  int columnsValue() const {
    if (showConstraints()->isSelected() &&
        spriteSheetTypeValue() == app::SpriteSheetType::Rows &&
        constraintType()->getSelectedItemIndex() == (int)kConstraintType_Cols) {
      return widthConstraint()->textInt();
    }
    else
      return 0;
  }

  int rowsValue() const {
    if (showConstraints()->isSelected() &&
        spriteSheetTypeValue() == app::SpriteSheetType::Columns &&
        constraintType()->getSelectedItemIndex() == (int)kConstraintType_Rows) {
      return heightConstraint()->textInt();
    }
    else
      return 0;
  }

  int widthValue() const {
    if (showConstraints()->isSelected() &&
        (spriteSheetTypeValue() == app::SpriteSheetType::Rows ||
         spriteSheetTypeValue() == app::SpriteSheetType::Packed) &&
        (constraintType()->getSelectedItemIndex() == (int)kConstraintType_Width ||
         constraintType()->getSelectedItemIndex() == (int)kConstraintType_Size)) {
      return widthConstraint()->textInt();
    }
    else
      return 0;
  }

  int heightValue() const {
    if (showConstraints()->isSelected() &&
        (spriteSheetTypeValue() == app::SpriteSheetType::Columns ||
         spriteSheetTypeValue() == app::SpriteSheetType::Packed) &&
        (constraintType()->getSelectedItemIndex() == (int)kConstraintType_Height ||
         constraintType()->getSelectedItemIndex() == (int)kConstraintType_Size)) {
      return heightConstraint()->textInt();
    }
    else
      return 0;
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

  std::string filenameFormatValue() const {
    if (!m_filenameFormat.empty() &&
        m_filenameFormat != m_filenameFormatDefault)
      return m_filenameFormat;
    else
      return std::string();
  }

  SpriteSheetDataFormat dataFormatValue() const {
    if (dataEnabled()->isSelected())
      return SpriteSheetDataFormat(dataFormat()->getSelectedItemIndex());
    else
      return SpriteSheetDataFormat::Default;
  }

  int borderPaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = borderPadding()->textInt();
      return base::clamp(value, 0, 100);
    }
    else
      return 0;
  }

  int shapePaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = shapePadding()->textInt();
      return base::clamp(value, 0, 100);
    }
    else
      return 0;
  }

  int innerPaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = innerPadding()->textInt();
      return base::clamp(value, 0, 100);
    }
    else
      return 0;
  }

  bool trimSpriteValue() const {
    return trimSpriteEnabled()->isSelected();
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

  std::string tagValue() const {
    return frames()->getValue();
  }

  bool splitLayersValue() const {
    return splitLayers()->isSelected();
  }

  bool splitTagsValue() const {
    return splitTags()->isSelected();
  }

  bool listLayersValue() const {
    return listLayers()->isSelected();
  }

  bool listTagsValue() const {
    return listTags()->isSelected();
  }

  bool listSlicesValue() const {
    return listSlices()->isSelected();
  }

  void onExport() {
    if (!ask_overwrite(m_filenameAskOverwrite, filenameValue(),
                       m_dataFilenameAskOverwrite, dataFilenameValue()))
      return;

    closeWindow(exportButton());
  }

  void onSheetTypeChange() {
    for (int i=0; i<constraintType()->getItemCount(); ++i)
      constraintType()->getItem(i)->setVisible(false);

    const ConstraintType selectConstraint =
      (ConstraintType)constraintType()->getSelectedItemIndex();
    bool constraints = true;
    switch (spriteSheetTypeValue()) {
      case app::SpriteSheetType::Horizontal:
      case app::SpriteSheetType::Vertical:
        constraints = false;
        constraintType()->setSelectedItemIndex(kConstraintType_None);
        break;
      case app::SpriteSheetType::Rows:
        constraintType()->getItem(kConstraintType_Cols)->setVisible(true);
        constraintType()->getItem(kConstraintType_Width)->setVisible(true);
        if (selectConstraint != kConstraintType_Cols &&
            selectConstraint != kConstraintType_Width)
          constraintType()->setSelectedItemIndex((int)kConstraintType_Cols);
        break;
      case app::SpriteSheetType::Columns:
        constraintType()->getItem(kConstraintType_Rows)->setVisible(true);
        constraintType()->getItem(kConstraintType_Height)->setVisible(true);
        if (selectConstraint != kConstraintType_Rows &&
            selectConstraint != kConstraintType_Height)
          constraintType()->setSelectedItemIndex((int)kConstraintType_Rows);
        break;
      case app::SpriteSheetType::Packed:
        constraintType()->getItem(kConstraintType_Width)->setVisible(true);
        constraintType()->getItem(kConstraintType_Height)->setVisible(true);
        constraintType()->getItem(kConstraintType_Size)->setVisible(true);
        if (selectConstraint != kConstraintType_Width &&
            selectConstraint != kConstraintType_Height &&
            selectConstraint != kConstraintType_Size) {
          constraintType()->setSelectedItemIndex((int)kConstraintType_Size);
        }
        break;
    }

    showConstraints()->setVisible(constraints);
    constraints = (constraints &&
                   showConstraints()->isSelected());

    constraintsLabel()->setVisible(constraints);
    constraintsPlaceholder()->setVisible(constraints);

    if (constraints)
      onConstraintTypeChange();
    else {
      resize();
      generatePreview();
    }
  }

  void onConstraintTypeChange() {
    bool withWidth = false;
    bool withHeight = false;
    switch ((ConstraintType)constraintType()->getSelectedItemIndex()) {
      case kConstraintType_Cols:
        withWidth = true;
        widthConstraint()->setSuffix("");
        break;
      case kConstraintType_Rows:
        withHeight = true;
        heightConstraint()->setSuffix("");
        break;
      case kConstraintType_Width:
        withWidth = true;
        widthConstraint()->setSuffix("px");
        break;
      case kConstraintType_Height:
        withHeight = true;
        heightConstraint()->setSuffix("px");
        break;
      case kConstraintType_Size:
        withWidth = true;
        withHeight = true;
        widthConstraint()->setSuffix("px");
        heightConstraint()->setSuffix("px");
        break;
    }
    widthConstraint()->setVisible(withWidth);
    heightConstraint()->setVisible(withHeight);
    resize();
    generatePreview();
  }

  void onFileNamesChange() {
    imageFilename()->setText(base::get_file_name(m_filename));
    dataFilename()->setText(base::get_file_name(m_dataFilename));
    resize();
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
    trimContainer()->setVisible(
      trimSpriteEnabled()->isSelected() ||
      trimEnabled()->isSelected());
    resize();
    generatePreview();
  }

  void onPaddingEnabledChange() {
    paddingContainer()->setVisible(paddingEnabled()->isSelected());
    resize();
    generatePreview();
  }

  void onSplitLayersOrFrames() {
    updateDefaultDataFilenameFormat();
    generatePreview();
  }

  void onDataFilenameFormatChange() {
    m_filenameFormat = dataFilenameFormat()->text();
    if (m_filenameFormat.empty())
      updateDefaultDataFilenameFormat();
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

  void updateDefaultDataFilenameFormat() {
    m_filenameFormatDefault =
      get_default_filename_format_for_sheet(
        m_site.document()->filename(),
        m_site.document()->sprite()->totalFrames() > 0,
        splitLayersValue(),
        splitTagsValue());

    if (m_filenameFormat.empty()) {
      dataFilenameFormat()->setText(m_filenameFormatDefault);
    }
    else {
      dataFilenameFormat()->setText(m_filenameFormat);
    }
  }

  void updateDataFields() {
    bool state = dataEnabled()->isSelected();
    dataFilename()->setVisible(state);
    dataMeta()->setVisible(state);
    dataFilenameFormatPlaceholder()->setVisible(state);
  }

  void onGenTimerTick() {
    if (!m_genTask) {
      m_genTimer.stop();
      setText(Strings::export_sprite_sheet_title());
      return;
    }
    setText(
      fmt::format(
        "{} ({} {}%)",
        Strings::export_sprite_sheet_title(),
        Strings::export_sprite_sheet_preview(),
        int(100.0f * m_genTask->progress())));
  }

  void generatePreview() {
    cancelGenTask();

    if (!preview()->isSelected()) {
      if (m_spriteSheet) {
        DocDestroyer destroyer(UIContext::instance(), m_spriteSheet.release(), 100);
        destroyer.destroyDocument();
        m_editor = nullptr;
      }
      return;
    }

    ExportSpriteSheetParams params;
    updateParams(params);

    std::unique_ptr<Task> task(new Task);
    task->run(
      [this, params](base::task_token& token){
        generateSpriteSheetOnBackground(params, token);
      });
    m_genTask = std::move(task);
    m_genTimer.start();
    onGenTimerTick();
  }

  void generateSpriteSheetOnBackground(const ExportSpriteSheetParams& params,
                                       base::task_token& token) {
    m_exporter.setDocImageBuffer(m_backBuffer);

    auto context = UIContext::instance();
    Doc* newDocument =
      generate_sprite_sheet(
        m_exporter, context, m_site, params, false, token)
      .release();
    if (!newDocument)
      return;

    if (token.canceled()) {
      DocDestroyer destroyer(context, newDocument, 100);
      destroyer.destroyDocument();
      return;
    }

    ++m_executeFromUI;
    ui::execute_from_ui_thread(
      [this, newDocument]{
        openGeneratedSpriteSheet(newDocument);
      });
  }

  void openGeneratedSpriteSheet(Doc* newDocument) {
    --m_executeFromUI;

    auto context = UIContext::instance();

    if (!isVisible()) {
      DocDestroyer destroyer(context, newDocument, 100);
      destroyer.destroyDocument();
      return;
    }

    // Was the preview unselected when we were generating the preview?
    if (!preview()->isSelected())
      return;

    // Now the "m_frontBuffer" is the current "m_backBuffer" which was
    // used by the generator to create the "newDocument", in the next
    // iteration we'll use the "m_backBuffer" to re-generate the
    // sprite sheet (while the document being displayed in the Editor
    // will use the m_frontBuffer).
    std::swap(m_frontBuffer, m_backBuffer);

    if (!m_spriteSheet) {
      m_spriteSheet.reset(newDocument);
      m_spriteSheet->setContext(context);
      m_spriteSheet->setInhibitBackup(true);

      m_editor = context->getEditorFor(m_spriteSheet.get());
      if (m_editor) {
        m_editor->setState(EditorStatePtr(new NavigateState));
        m_editor->setDefaultScroll();
      }
    }
    else {
      // Replace old cel with the new one
      auto spriteSheetLay = static_cast<LayerImage*>(m_spriteSheet->sprite()->root()->firstLayer());
      auto newDocLay = static_cast<LayerImage*>(newDocument->sprite()->root()->firstLayer());
      auto oldCel = m_spriteSheet->sprite()->firstLayer()->cel(0);
      auto newCel = newDocument->sprite()->firstLayer()->cel(0);

      spriteSheetLay->removeCel(oldCel);
      delete oldCel;

      newDocLay->removeCel(newCel);
      spriteSheetLay->addCel(newCel);

      // Update sprite sheet size
      m_spriteSheet->sprite()->setSize(
        newDocument->sprite()->width(),
        newDocument->sprite()->height());

      m_spriteSheet->notifyGeneralUpdate();

      DocDestroyer destroyer(context, newDocument, 100);
      destroyer.destroyDocument();
    }

    waitGenTaskAndDelete();
  }

  void cancelGenTask() {
    if (m_genTask) {
      m_genTask->cancel();
      waitGenTaskAndDelete();
    }
  }

  void waitGenTaskAndDelete() {
    if (m_genTask) {
      if (!m_genTask->completed()) {
        while (!m_genTask->completed())
          base::this_thread::sleep_for(0.01);
      }
      m_genTask.reset();
    }
  }

  DocExporter& m_exporter;
  doc::ImageBufferPtr m_frontBuffer; // ImageBuffer in the preview ImageBuffer
  doc::ImageBufferPtr m_backBuffer; // ImageBuffer in the generator
  Site& m_site;
  Sprite* m_sprite;
  std::string m_filename;
  std::string m_dataFilename;
  bool m_filenameAskOverwrite;
  bool m_dataFilenameAskOverwrite;
  std::unique_ptr<Doc> m_spriteSheet;
  Editor* m_editor;
  std::unique_ptr<Task> m_genTask;
  ui::Timer m_genTimer;
  int m_executeFromUI;
  std::string m_filenameFormat;
  std::string m_filenameFormatDefault;
};

class ExportSpriteSheetJob : public Job {
public:
  ExportSpriteSheetJob(
    DocExporter& exporter,
    Context* context,
    const Site& site,
    const ExportSpriteSheetParams& params)
    : Job(Strings::export_sprite_sheet_generating().c_str())
    , m_exporter(exporter)
    , m_context(context)
    , m_site(site)
    , m_params(params) { }

  std::unique_ptr<Doc> releaseDoc() { return std::move(m_doc); }

private:
  void onJob() override {
    m_doc = generate_sprite_sheet(
      m_exporter, m_context, m_site, m_params, true, m_token);
  }

  void onMonitoringTick() override {
    Job::onMonitoringTick();
    if (isCanceled())
      m_token.cancel();
    else {
      jobProgress(m_token.progress());
    }
  }

  DocExporter& m_exporter;
  base::task_token m_token;
  Context* m_context;
  const Site& m_site;
  const ExportSpriteSheetParams& m_params;
  std::unique_ptr<Doc> m_doc;
};

#endif // ENABLE_UI

} // anonymous namespace

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
  auto& params = this->params();
  DocExporter exporter;

#ifdef ENABLE_UI
  // TODO if we use this line when !ENABLE_UI,
  // Preferences::~Preferences() crashes on Linux when it wants to
  // save the document preferences. It looks like
  // Preferences::onRemoveDocument() is not called for some documents
  // and when the Preferences::m_docs collection is iterated to save
  // all DocumentPreferences, it accesses an invalid Doc* pointer (an
  // already removed/deleted document).
  DocumentPreferences& docPref(Preferences::instance().document(document));

  // Show UI if the user specified it explicitly (params.ui=true) or
  // the sprite sheet type wasn't specified.
  const bool showUI = (context->isUIAvailable() && params.ui() &&
                       (params.ui.isSet() || !params.type.isSet()));

  // Copy document preferences to undefined params
  if (docPref.spriteSheet.defined(true) &&
      !params.type.isSet()) {
    params.type(docPref.spriteSheet.type());
    if (!params.columns.isSet())          params.columns(         docPref.spriteSheet.columns());
    if (!params.rows.isSet())             params.rows(            docPref.spriteSheet.rows());
    if (!params.width.isSet())            params.width(           docPref.spriteSheet.width());
    if (!params.height.isSet())           params.height(          docPref.spriteSheet.height());
    if (!params.textureFilename.isSet())  params.textureFilename( docPref.spriteSheet.textureFilename());
    if (!params.dataFilename.isSet())     params.dataFilename(    docPref.spriteSheet.dataFilename());
    if (!params.dataFormat.isSet())       params.dataFormat(      docPref.spriteSheet.dataFormat());
    if (!params.filenameFormat.isSet())   params.filenameFormat(  docPref.spriteSheet.filenameFormat());
    if (!params.borderPadding.isSet())    params.borderPadding(   docPref.spriteSheet.borderPadding());
    if (!params.shapePadding.isSet())     params.shapePadding(    docPref.spriteSheet.shapePadding());
    if (!params.innerPadding.isSet())     params.innerPadding(    docPref.spriteSheet.innerPadding());
    if (!params.trimSprite.isSet())       params.trimSprite(      docPref.spriteSheet.trimSprite());
    if (!params.trim.isSet())             params.trim(            docPref.spriteSheet.trim());
    if (!params.trimByGrid.isSet())       params.trimByGrid(      docPref.spriteSheet.trimByGrid());
    if (!params.extrude.isSet())          params.extrude(         docPref.spriteSheet.extrude());
    if (!params.openGenerated.isSet())    params.openGenerated(   docPref.spriteSheet.openGenerated());
    if (!params.layer.isSet())            params.layer(           docPref.spriteSheet.layer());
    if (!params.tag.isSet())              params.tag(             docPref.spriteSheet.frameTag());
    if (!params.splitLayers.isSet())      params.splitLayers(     docPref.spriteSheet.splitLayers());
    if (!params.splitTags.isSet())        params.splitTags(       docPref.spriteSheet.splitTags());
    if (!params.listLayers.isSet())       params.listLayers(      docPref.spriteSheet.listLayers());
    if (!params.listTags.isSet())         params.listTags(        docPref.spriteSheet.listFrameTags());
    if (!params.listSlices.isSet())       params.listSlices(      docPref.spriteSheet.listSlices());
  }

  bool askOverwrite = params.askOverwrite();
  if (showUI) {
    ExportSpriteSheetWindow window(exporter, site, params);
    window.openWindowInForeground();
    if (!window.ok())
      return;

    // Preview sprite sheet generation
    Preferences::instance().spriteSheet.preview(window.preview()->isSelected());

    window.updateParams(params);
    docPref.spriteSheet.defined(true);
    docPref.spriteSheet.type            (params.type());
    docPref.spriteSheet.columns         (params.columns());
    docPref.spriteSheet.rows            (params.rows());
    docPref.spriteSheet.width           (params.width());
    docPref.spriteSheet.height          (params.height());
    docPref.spriteSheet.textureFilename (params.textureFilename());
    docPref.spriteSheet.dataFilename    (params.dataFilename());
    docPref.spriteSheet.dataFormat      (params.dataFormat());
    docPref.spriteSheet.filenameFormat  (params.filenameFormat());
    docPref.spriteSheet.borderPadding   (params.borderPadding());
    docPref.spriteSheet.shapePadding    (params.shapePadding());
    docPref.spriteSheet.innerPadding    (params.innerPadding());
    docPref.spriteSheet.trimSprite      (params.trimSprite());
    docPref.spriteSheet.trim            (params.trim());
    docPref.spriteSheet.trimByGrid      (params.trimByGrid());
    docPref.spriteSheet.extrude         (params.extrude());
    docPref.spriteSheet.openGenerated   (params.openGenerated());
    docPref.spriteSheet.layer           (params.layer());
    docPref.spriteSheet.frameTag        (params.tag());
    docPref.spriteSheet.splitLayers     (params.splitLayers());
    docPref.spriteSheet.splitTags       (params.splitTags());
    docPref.spriteSheet.listLayers      (params.listLayers());
    docPref.spriteSheet.listFrameTags   (params.listTags());
    docPref.spriteSheet.listSlices      (params.listSlices());

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

  if (context->isUIAvailable() && askOverwrite) {
    if (!ask_overwrite(true, params.textureFilename(),
                       true, params.dataFilename()))
      return;                   // Do not overwrite
  }
#endif

  exporter.setDocImageBuffer(std::make_shared<doc::ImageBuffer>());
  std::unique_ptr<Doc> newDocument;
#ifdef ENABLE_UI
  if (context->isUIAvailable()) {
    ExportSpriteSheetJob job(exporter, context, site, params);
    job.startJob();
    job.waitJob();

    newDocument = job.releaseDoc();

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
  else
#endif
  {
    base::task_token token;
    newDocument = generate_sprite_sheet(
      exporter, context, site, params, true, token);
    if (!newDocument)
      return;
  }

  if (params.openGenerated()) {
    newDocument->setContext(context);
    newDocument.release();
  }
  else if (newDocument) {
    DocDestroyer destroyer(context, newDocument.release(), 100);
    destroyer.destroyDocument();
  }
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
