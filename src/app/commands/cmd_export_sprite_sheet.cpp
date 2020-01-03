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
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
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
#include "ui/message.h"
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
  Param<bool> ignoreEmpty { this, false, "ignoreEmpty" };
  Param<bool> mergeDuplicates { this, false, "mergeDuplicates" };
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

enum Section {
  kSectionLayout,
  kSectionSprite,
  kSectionBorders,
  kSectionOutput,
};

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

Doc* generate_sprite_sheet_from_params(
  DocExporter& exporter,
  Context* ctx,
  const Site& site,
  const ExportSpriteSheetParams& params,
  const bool saveData,
  base::task_token& token)
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
  const bool ignoreEmpty = params.ignoreEmpty();
  const bool mergeDuplicates = params.mergeDuplicates();
  const bool splitLayers = params.splitLayers();
  const bool splitTags = params.splitTags();
  const bool listLayers = params.listLayers();
  const bool listTags = params.listTags();
  const bool listSlices = params.listSlices();

  SelectedFrames selFrames;
  Tag* tag = calculate_selected_frames(site, tagName, selFrames);

#ifdef _DEBUG
  frame_t nframes = selFrames.size();
  ASSERT(nframes > 0);
#endif

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

  if (saveData) {
    if (!filename.empty())
      exporter.setTextureFilename(filename);
    if (!dataFilename.empty()) {
      exporter.setDataFilename(dataFilename);
      exporter.setDataFormat(dataFormat);
    }
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
  exporter.setSplitLayers(splitLayers);
  exporter.setSplitTags(splitTags);
  exporter.setIgnoreEmptyCels(ignoreEmpty);
  exporter.setMergeDuplicates(mergeDuplicates);
  if (listLayers) exporter.setListLayers(true);
  if (listTags) exporter.setListTags(true);
  if (listSlices) exporter.setListSlices(true);

  // We have to call exportSheet() while RestoreVisibleLayers is still
  // alive. In this way we can export selected layers correctly if
  // that option (kSelectedLayers) is selected.
  return exporter.exportSheet(ctx, token);
}

std::unique_ptr<Doc> generate_sprite_sheet(
  DocExporter& exporter,
  Context* ctx,
  const Site& site,
  const ExportSpriteSheetParams& params,
  bool saveData,
  base::task_token& token)
{
  std::unique_ptr<Doc> newDocument(
    generate_sprite_sheet_from_params(exporter, ctx, site, params, saveData, token));
  if (!newDocument)
    return nullptr;

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
  return newDocument;
}

#if ENABLE_UI

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  ExportSpriteSheetWindow(DocExporter& exporter,
                          Site& site,
                          ExportSpriteSheetParams& params,
                          Preferences& pref)
    : m_exporter(exporter)
    , m_frontBuffer(std::make_shared<doc::ImageBuffer>())
    , m_backBuffer(std::make_shared<doc::ImageBuffer>())
    , m_site(site)
    , m_sprite(site.sprite())
    , m_filenameAskOverwrite(true)
    , m_dataFilenameAskOverwrite(true)
    , m_editor(nullptr)
    , m_genTimer(100, nullptr)
    , m_executionID(0)
    , m_filenameFormat(params.filenameFormat())
  {
    sectionTabs()->ItemChange.connect(base::Bind<void>(&ExportSpriteSheetWindow::onChangeSection, this));
    expandSections()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onExpandSections, this));
    closeSpriteSection()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onCloseSection, this, kSectionSprite));
    closeBordersSection()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onCloseSection, this, kSectionBorders));
    closeOutputSection()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onCloseSection, this, kSectionOutput));

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
    {
      int i;
      if (params.type() != app::SpriteSheetType::None)
        i = (int)params.type()-1;
      else
        i = ((int)app::SpriteSheetType::Rows)-1;
      sheetType()->setSelectedItemIndex(i);
    }

    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_none());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_cols());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_rows());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_width());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_height());
    constraintType()->addItem(Strings::export_sprite_sheet_constraint_fixed_size());

    auto constraint = constraint_type_from_params(params);
    constraintType()->setSelectedItemIndex(constraint);
    switch (constraint) {
      case kConstraintType_Cols:
        widthConstraint()->setTextf("%d", params.columns());
        break;
      case kConstraintType_Rows:
        heightConstraint()->setTextf("%d", params.rows());
        break;
      case kConstraintType_Width:
        widthConstraint()->setTextf("%d", params.width());
        break;
      case kConstraintType_Height:
        heightConstraint()->setTextf("%d", params.height());
        break;
      case kConstraintType_Size:
        widthConstraint()->setTextf("%d", params.width());
        heightConstraint()->setTextf("%d", params.height());
        break;
    }

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
    mergeDups()->setSelected(params.mergeDuplicates());
    ignoreEmpty()->setSelected(params.ignoreEmpty());

    borderPadding()->setTextf("%d", params.borderPadding());
    shapePadding()->setTextf("%d", params.shapePadding());
    innerPadding()->setTextf("%d", params.innerPadding());

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
      std::string defExt = pref.spriteSheet.defaultExtension();

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
    constraintType()->Change.connect(&ExportSpriteSheetWindow::onConstraintTypeChange, this);
    widthConstraint()->Change.connect(&ExportSpriteSheetWindow::generatePreview, this);
    heightConstraint()->Change.connect(&ExportSpriteSheetWindow::generatePreview, this);
    borderPadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    shapePadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    innerPadding()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    extrudeEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    mergeDups()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    ignoreEmpty()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    imageEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onImageEnabledChange, this));
    imageFilename()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onImageFilename, this));
    dataEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataEnabledChange, this));
    dataFilename()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataFilename, this));
    trimSpriteEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onTrimEnabledChange, this));
    trimEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onTrimEnabledChange, this));
    gridTrimEnabled()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    layers()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    splitLayers()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSplitLayersOrFrames, this));
    splitTags()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onSplitLayersOrFrames, this));
    frames()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    dataFilenameFormat()->Change.connect(base::Bind<void>(&ExportSpriteSheetWindow::onDataFilenameFormatChange, this));
    openGenerated()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::onOpenGeneratedChange, this));
    preview()->Click.connect(base::Bind<void>(&ExportSpriteSheetWindow::generatePreview, this));
    m_genTimer.Tick.connect(base::Bind<void>(&ExportSpriteSheetWindow::onGenTimerTick, this));

    // Select tabs
    {
      const std::string s = pref.spriteSheet.sections();
      const bool layout = (s.find("layout") != std::string::npos);
      const bool sprite = (s.find("sprite") != std::string::npos);
      const bool borders = (s.find("borders") != std::string::npos);
      const bool output = (s.find("output") != std::string::npos);
      sectionTabs()->getItem(kSectionLayout)->setSelected(layout || (!sprite & !borders && !output));
      sectionTabs()->getItem(kSectionSprite)->setSelected(sprite);
      sectionTabs()->getItem(kSectionBorders)->setSelected(borders);
      sectionTabs()->getItem(kSectionOutput)->setSelected(output);
    }

    onChangeSection();
    onSheetTypeChange();
    onFileNamesChange();
    updateExportButton();

    preview()->setSelected(pref.spriteSheet.preview());
    generatePreview();

    remapWindow();
    centerWindow();
    load_window_pos(this, "ExportSpriteSheet");
  }

  ~ExportSpriteSheetWindow() {
    cancelGenTask();
    if (m_spriteSheet) {
      auto ctx = UIContext::instance();
      ctx->setActiveDocument(m_site.document());

      DocDestroyer destroyer(ctx, m_spriteSheet.release(), 100);
      destroyer.destroyDocument();
    }
  }

  std::string selectedSectionsString() const {
    const bool layout = sectionTabs()->getItem(kSectionLayout)->isSelected();
    const bool sprite = sectionTabs()->getItem(kSectionSprite)->isSelected();
    const bool borders = sectionTabs()->getItem(kSectionBorders)->isSelected();
    const bool output = sectionTabs()->getItem(kSectionOutput)->isSelected();
    return
      fmt::format("{} {} {} {}",
                  (layout ? "layout": ""),
                  (sprite ? "sprite": ""),
                  (borders ? "borders": ""),
                  (output ? "output": ""));
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
    params.mergeDuplicates (mergeDupsValue());
    params.ignoreEmpty     (ignoreEmptyValue());
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

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case kCloseMessage:
        save_window_pos(this, "ExportSpriteSheet");
        break;
    }
    return Window::onProcessMessage(msg);
  }

  void onBroadcastMouseMessage(WidgetsList& targets) override {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    if (m_editor)
      targets.push_back(View::getView(m_editor));
  }

  void onChangeSection() {
    panel()->showAllChildren();

    const bool layout = sectionTabs()->getItem(kSectionLayout)->isSelected();
    const bool sprite = sectionTabs()->getItem(kSectionSprite)->isSelected();
    const bool borders = sectionTabs()->getItem(kSectionBorders)->isSelected();
    const bool output = sectionTabs()->getItem(kSectionOutput)->isSelected();

    sectionLayout()->setVisible(layout);
    sectionSpriteSeparator()->setVisible(sprite && layout);
    sectionSprite()->setVisible(sprite);
    sectionBordersSeparator()->setVisible(borders && (layout || sprite));
    sectionBorders()->setVisible(borders);
    sectionOutputSeparator()->setVisible(output && (layout || sprite || borders));
    sectionOutput()->setVisible(output);

    resize();
  }

  void onExpandSections() {
    sectionTabs()->getItem(kSectionLayout)->setSelected(true);
    sectionTabs()->getItem(kSectionSprite)->setSelected(true);
    sectionTabs()->getItem(kSectionBorders)->setSelected(true);
    sectionTabs()->getItem(kSectionOutput)->setSelected(true);
    onChangeSection();
  }

  void onCloseSection(const Section section) {
    if (sectionTabs()->countSelectedItems() > 1)
      sectionTabs()->getItem(section)->setSelected(false);
    onChangeSection();
  }

  app::SpriteSheetType spriteSheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  int columnsValue() const {
    if (spriteSheetTypeValue() == app::SpriteSheetType::Rows &&
        constraintType()->getSelectedItemIndex() == (int)kConstraintType_Cols) {
      return widthConstraint()->textInt();
    }
    else
      return 0;
  }

  int rowsValue() const {
    if (spriteSheetTypeValue() == app::SpriteSheetType::Columns &&
        constraintType()->getSelectedItemIndex() == (int)kConstraintType_Rows) {
      return heightConstraint()->textInt();
    }
    else
      return 0;
  }

  int widthValue() const {
    if ((spriteSheetTypeValue() == app::SpriteSheetType::Rows ||
         spriteSheetTypeValue() == app::SpriteSheetType::Packed) &&
        (constraintType()->getSelectedItemIndex() == (int)kConstraintType_Width ||
         constraintType()->getSelectedItemIndex() == (int)kConstraintType_Size)) {
      return widthConstraint()->textInt();
    }
    else
      return 0;
  }

  int heightValue() const {
    if ((spriteSheetTypeValue() == app::SpriteSheetType::Columns ||
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
    int value = borderPadding()->textInt();
    return base::clamp(value, 0, 100);
  }

  int shapePaddingValue() const {
    int value = shapePadding()->textInt();
    return base::clamp(value, 0, 100);
  }

  int innerPaddingValue() const {
    int value = innerPadding()->textInt();
    return base::clamp(value, 0, 100);
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

  bool mergeDupsValue() const {
    return mergeDups()->isSelected();
  }

  bool ignoreEmptyValue() const {
    return ignoreEmpty()->isSelected();
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
    for (int i=1; i<constraintType()->getItemCount(); ++i)
      constraintType()->getItem(i)->setVisible(false);

    mergeDups()->setEnabled(true);

    const ConstraintType selectConstraint =
      (ConstraintType)constraintType()->getSelectedItemIndex();
    switch (spriteSheetTypeValue()) {
      case app::SpriteSheetType::Horizontal:
      case app::SpriteSheetType::Vertical:
        constraintType()->setSelectedItemIndex(kConstraintType_None);
        break;
      case app::SpriteSheetType::Rows:
        constraintType()->getItem(kConstraintType_Cols)->setVisible(true);
        constraintType()->getItem(kConstraintType_Width)->setVisible(true);
        if (selectConstraint != kConstraintType_None &&
            selectConstraint != kConstraintType_Cols &&
            selectConstraint != kConstraintType_Width)
          constraintType()->setSelectedItemIndex(kConstraintType_None);
        break;
      case app::SpriteSheetType::Columns:
        constraintType()->getItem(kConstraintType_Rows)->setVisible(true);
        constraintType()->getItem(kConstraintType_Height)->setVisible(true);
        if (selectConstraint != kConstraintType_None &&
            selectConstraint != kConstraintType_Rows &&
            selectConstraint != kConstraintType_Height)
          constraintType()->setSelectedItemIndex(kConstraintType_None);
        break;
      case app::SpriteSheetType::Packed:
        constraintType()->getItem(kConstraintType_Width)->setVisible(true);
        constraintType()->getItem(kConstraintType_Height)->setVisible(true);
        constraintType()->getItem(kConstraintType_Size)->setVisible(true);
        if (selectConstraint != kConstraintType_None &&
            selectConstraint != kConstraintType_Width &&
            selectConstraint != kConstraintType_Height &&
            selectConstraint != kConstraintType_Size) {
          constraintType()->setSelectedItemIndex(kConstraintType_None);
        }
        mergeDups()->setSelected(true);
        mergeDups()->setEnabled(false);
        break;
    }
    onConstraintTypeChange();
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
        auto ctx = UIContext::instance();
        ctx->setActiveDocument(m_site.document());

        DocDestroyer destroyer(ctx, m_spriteSheet.release(), 100);
        destroyer.destroyDocument();
        m_editor = nullptr;
      }
      return;
    }

    ASSERT(m_genTask == nullptr);

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
    // Sometimes (more often on Linux) the back buffer is still being
    // used by the new document after
    // generateSpriteSheetOnBackground() and before
    // openGeneratedSpriteSheet(). In this case the use counter is 3
    // which means that 2 or more openGeneratedSpriteSheet() are
    // queued in the laf-os events queue. In this case we just create
    // a new back buffer and the old one will be discarded by
    // openGeneratedSpriteSheet() when m_executionID != executionID.
    if (m_backBuffer.use_count() > 2) {
      auto ptr = std::make_shared<doc::ImageBuffer>();
      m_backBuffer.swap(ptr);
    }
    m_exporter.setDocImageBuffer(m_backBuffer);

    ASSERT(m_backBuffer.use_count() == 2);

    // Create a non-UI context to avoid showing UI dialogs for
    // GifOptions or JpegOptions from the background thread.
    Context tmpCtx;

    Doc* newDocument =
      generate_sprite_sheet(
        m_exporter, &tmpCtx, m_site, params, false, token)
      .release();
    if (!newDocument)
      return;

    if (token.canceled()) {
      DocDestroyer destroyer(&tmpCtx, newDocument, 100);
      destroyer.destroyDocument();
      return;
    }

    ++m_executionID;
    int executionID = m_executionID;

    tmpCtx.documents().remove(newDocument);

    ui::execute_from_ui_thread(
      [this, newDocument, executionID]{
        openGeneratedSpriteSheet(newDocument, executionID);
      });
  }

  void openGeneratedSpriteSheet(Doc* newDocument, int executionID) {
    auto context = UIContext::instance();

    if (!isVisible() ||
        // Other openGeneratedSpriteSheet() is queued and we are the
        // old one. IN this case the newDocument contains a back
        // buffer (ImageBufferPtr) that will be discarded.
        m_executionID != executionID) {
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
    m_frontBuffer.swap(m_backBuffer);

    if (!m_spriteSheet) {
      m_spriteSheet.reset(newDocument);
      m_spriteSheet->setInhibitBackup(true);
      m_spriteSheet->setContext(context);

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
      Cel* oldCel = m_spriteSheet->sprite()->firstLayer()->cel(0);
      Cel* newCel = newDocument->sprite()->firstLayer()->cel(0);

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
  int m_executionID;
  std::string m_filenameFormat;
  std::string m_filenameFormatDefault;
};

class ExportSpriteSheetJob : public Job {
public:
  ExportSpriteSheetJob(
    DocExporter& exporter,
    const Site& site,
    const ExportSpriteSheetParams& params)
    : Job(Strings::export_sprite_sheet_generating().c_str())
    , m_exporter(exporter)
    , m_site(site)
    , m_params(params) { }

  std::unique_ptr<Doc> releaseDoc() { return std::move(m_doc); }

private:
  void onJob() override {
    // Create a non-UI context to avoid showing UI dialogs for
    // GifOptions or JpegOptions from the background thread.
    Context tmpCtx;

    m_doc = generate_sprite_sheet(
      m_exporter, &tmpCtx, m_site, m_params, true, m_token);

    if (m_doc)
      tmpCtx.documents().remove(m_doc.get());
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
  Doc* document = site.document();
  DocumentPreferences& docPref(Preferences::instance().document(document));

  // Show UI if the user specified it explicitly (params.ui=true) or
  // the sprite sheet type wasn't specified.
  const bool showUI = (context->isUIAvailable() && params.ui() &&
                       (params.ui.isSet() || !params.type.isSet()));

  // Copy document preferences to undefined params
  {
    auto& defPref = (docPref.spriteSheet.defined() ? docPref: Preferences::instance().document(nullptr));
    if (!params.type.isSet()) {
      params.type(defPref.spriteSheet.type());
      if (!params.columns.isSet())          params.columns(         defPref.spriteSheet.columns());
      if (!params.rows.isSet())             params.rows(            defPref.spriteSheet.rows());
      if (!params.width.isSet())            params.width(           defPref.spriteSheet.width());
      if (!params.height.isSet())           params.height(          defPref.spriteSheet.height());
      if (!params.textureFilename.isSet())  params.textureFilename( defPref.spriteSheet.textureFilename());
      if (!params.dataFilename.isSet())     params.dataFilename(    defPref.spriteSheet.dataFilename());
      if (!params.dataFormat.isSet())       params.dataFormat(      defPref.spriteSheet.dataFormat());
      if (!params.filenameFormat.isSet())   params.filenameFormat(  defPref.spriteSheet.filenameFormat());
      if (!params.borderPadding.isSet())    params.borderPadding(   defPref.spriteSheet.borderPadding());
      if (!params.shapePadding.isSet())     params.shapePadding(    defPref.spriteSheet.shapePadding());
      if (!params.innerPadding.isSet())     params.innerPadding(    defPref.spriteSheet.innerPadding());
      if (!params.trimSprite.isSet())       params.trimSprite(      defPref.spriteSheet.trimSprite());
      if (!params.trim.isSet())             params.trim(            defPref.spriteSheet.trim());
      if (!params.trimByGrid.isSet())       params.trimByGrid(      defPref.spriteSheet.trimByGrid());
      if (!params.extrude.isSet())          params.extrude(         defPref.spriteSheet.extrude());
      if (!params.mergeDuplicates.isSet())  params.mergeDuplicates( defPref.spriteSheet.mergeDuplicates());
      if (!params.ignoreEmpty.isSet())      params.ignoreEmpty(     defPref.spriteSheet.ignoreEmpty());
      if (!params.openGenerated.isSet())    params.openGenerated(   defPref.spriteSheet.openGenerated());
      if (!params.layer.isSet())            params.layer(           defPref.spriteSheet.layer());
      if (!params.tag.isSet())              params.tag(             defPref.spriteSheet.frameTag());
      if (!params.splitLayers.isSet())      params.splitLayers(     defPref.spriteSheet.splitLayers());
      if (!params.splitTags.isSet())        params.splitTags(       defPref.spriteSheet.splitTags());
      if (!params.listLayers.isSet())       params.listLayers(      defPref.spriteSheet.listLayers());
      if (!params.listTags.isSet())         params.listTags(        defPref.spriteSheet.listFrameTags());
      if (!params.listSlices.isSet())       params.listSlices(      defPref.spriteSheet.listSlices());
    }
  }

  bool askOverwrite = params.askOverwrite();
  if (showUI) {
    auto& pref = Preferences::instance();

    ExportSpriteSheetWindow window(exporter, site, params, pref);
    window.openWindowInForeground();

    // Save global sprite sheet generation settings anyway (even if
    // the user cancel the dialog, the global settings are stored).
    pref.spriteSheet.preview(window.preview()->isSelected());
    pref.spriteSheet.sections(window.selectedSectionsString());

    if (!window.ok())
      return;

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
    docPref.spriteSheet.mergeDuplicates (params.mergeDuplicates());
    docPref.spriteSheet.ignoreEmpty     (params.ignoreEmpty());
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
    ExportSpriteSheetJob job(exporter, site, params);
    job.startJob();
    job.waitJob();

    newDocument = job.releaseDoc();
    if (!newDocument)
      return;

    StatusBar* statusbar = StatusBar::instance();
    if (statusbar)
      statusbar->showTip(1000, "Sprite Sheet Generated");

    // Save the exported sprite sheet as a recent file
    if (newDocument->isAssociatedToFile())
      App::instance()->recentFiles()->addRecentFile(newDocument->filename());

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

  ASSERT(newDocument);

  if (params.openGenerated()) {
    newDocument->setContext(context);
    newDocument.release();
  }
  else {
    DocDestroyer destroyer(context, newDocument.release(), 100);
    destroyer.destroyDocument();
  }
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
