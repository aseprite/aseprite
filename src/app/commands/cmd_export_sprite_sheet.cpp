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
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/path.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"

#include "export_sprite_sheet.xml.h"

#include <limits>
#include <sstream>

namespace app {

using namespace ui;

namespace {

  static const char* kAllLayers = "";
  static const char* kAllFrames = "";
  static const char* kSelectedLayers = "**selected-layers**";
  static const char* kSelectedFrames = "**selected-frames**";

  // Special key value used in default preferences to know if by default
  // the user wants to generate texture and/or files.
  static const char* kSpecifiedFilename = "**filename**";

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
                           int borderPadding, int shapePadding, int innerPadding) {
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

  bool ask_overwrite(bool askFilename, std::string filename,
                     bool askDataname, std::string dataname) {
    if ((askFilename &&
         !filename.empty() &&
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

  class SelectedFrameTag {
  public:
    static frame_t From() {
      // TODO the range of selected frames should be in doc::Site.
      DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
      if (range.enabled()) {
        return range.frameBegin();
      }
      else if (current_editor) {
        return current_editor->frame();
      }
      else
        return 0;
    }

    static frame_t To() {
      DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
      if (range.enabled()) {
        return range.frameEnd();
      }
      else if (current_editor) {
        return current_editor->frame();
      }
      else
        return 0;
    }

    SelectedFrameTag() : m_frameTag(nullptr) {
    }

    ~SelectedFrameTag() {
      if (m_frameTag) {
        m_frameTag->owner()->remove(m_frameTag);
        delete m_frameTag;
      }
    }

    FrameTag* create(Sprite* sprite) {
      m_frameTag = new FrameTag(From(), To());
      sprite->frameTags().add(m_frameTag);
      return m_frameTag;
    }

  private:
    FrameTag* m_frameTag;
  };

  class SelectedLayers {
  public:
    ~SelectedLayers() {
      for (auto item : m_restore)
        item.first->setVisible(item.second);
    }

    void showSelectedLayers(Sprite* sprite) {
      // TODO the range of selected frames should be in doc::Site.
      DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
      if (!range.enabled()) {
        if (current_editor) {
          ASSERT(current_editor->sprite() == sprite);
          range.startRange(sprite->layerToIndex(current_editor->layer()),
                           current_editor->frame(), DocumentRange::kCels);
          range.endRange(sprite->layerToIndex(current_editor->layer()),
                         current_editor->frame());
        }
        else
          return;
      }

      std::vector<Layer*> layers;
      sprite->getLayersList(layers);
      for (int i=0; i<int(layers.size()); ++i) {
        Layer* layer = layers[i];
        bool selected = range.inRange(LayerIndex(i));

        if (selected != layer->isVisible()) {
          m_restore.push_back(std::make_pair(layer, layer->isVisible()));
          layer->setVisible(selected);
        }
      }
    }

  private:
    std::vector<std::pair<Layer*, bool> > m_restore;
  };

}

class ExportSpriteSheetWindow : public app::gen::ExportSpriteSheet {
public:
  class LayerItem : public ListItem {
  public:
    LayerItem(Layer* layer)
      : ListItem("Layer: " + layer->name())
      , m_layer(layer) {
    }
    Layer* layer() const { return m_layer; }
  private:
    Layer* m_layer;
  };

  class TagItem : public ListItem {
  public:
    TagItem(FrameTag* tag)
      : ListItem("Tag: " + tag->name())
      , m_tag(tag) {
    }
    FrameTag* tag() const { return m_tag; }
  private:
    FrameTag* m_tag;
  };

  ExportSpriteSheetWindow(Document* doc, Sprite* sprite,
    DocumentPreferences& docPref)
    : m_sprite(sprite)
    , m_docPref(docPref)
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
    if (m_docPref.spriteSheet.type() != app::SpriteSheetType::None)
      sheetType()->setSelectedItemIndex((int)m_docPref.spriteSheet.type()-1);

    layers()->addItem("Visible layers");
    int i = layers()->addItem("Selected layers");
    if (m_docPref.spriteSheet.layer() == kSelectedLayers)
      layers()->setSelectedItemIndex(i);
    {
      std::vector<Layer*> layersList;
      m_sprite->getLayersList(layersList);
      for (Layer* layer : layersList) {
        i = layers()->addItem(new LayerItem(layer));
        if (m_docPref.spriteSheet.layer() == layer->name())
          layers()->setSelectedItemIndex(i);
      }
    }

    frames()->addItem("All frames");
    i = frames()->addItem("Selected frames");
    if (m_docPref.spriteSheet.frameTag() == kSelectedFrames)
      frames()->setSelectedItemIndex(i);
    for (FrameTag* tag : m_sprite->frameTags()) {
      i = frames()->addItem(new TagItem(tag));
      if (m_docPref.spriteSheet.frameTag() == tag->name())
        frames()->setSelectedItemIndex(i);
    }

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
      rows()->setTextf("%d", m_docPref.spriteSheet.rows());
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
    imageEnabled()->setSelected(!m_filename.empty());
    imageFilename()->setVisible(imageEnabled()->isSelected());

    m_dataFilename = m_docPref.spriteSheet.dataFilename();
    dataEnabled()->setSelected(!m_dataFilename.empty());
    dataFilename()->setVisible(dataEnabled()->isSelected());

    std::string base = doc->filename();
    base = base::join_path(base::get_file_path(base), base::get_file_title(base));

    if (m_filename.empty() ||
        m_filename == kSpecifiedFilename) {
      if (base::utf8_icmp(base::get_file_extension(doc->filename()), "png") == 0)
        m_filename = base + "-sheet.png";
      else
        m_filename = base + ".png";
    }

    if (m_dataFilename.empty() ||
        m_dataFilename == kSpecifiedFilename)
      m_dataFilename = base + ".json";

    exportButton()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onExport, this));
    sheetType()->Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    columns()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onColumnsChange, this));
    rows()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onRowsChange, this));
    fitWidth()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    fitHeight()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    bestFit()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onBestFit, this));
    borderPadding()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    shapePadding()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    innerPadding()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingChange, this));
    imageEnabled()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onImageEnabledChange, this));
    imageFilename()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onImageFilename, this));
    dataEnabled()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onDataEnabledChange, this));
    dataFilename()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onDataFilename, this));
    paddingEnabled()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onPaddingEnabledChange, this));
    frames()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onFramesChange, this));
    openGenerated()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onOpenGeneratedChange, this));

    onSheetTypeChange();
    onFileNamesChange();
    updateExportButton();
  }

  bool ok() const {
    return getKiller() == exportButton();
  }

  app::SpriteSheetType spriteSheetTypeValue() const {
    return (app::SpriteSheetType)(sheetType()->getSelectedItemIndex()+1);
  }

  int columnsValue() const {
    if (spriteSheetTypeValue() != SpriteSheetType::Columns)
      return columns()->getTextInt();
    else
      return 0;
  }

  int rowsValue() const {
    if (spriteSheetTypeValue() == SpriteSheetType::Columns)
      return rows()->getTextInt();
    else
      return 0;
  }

  int fitWidthValue() const {
    return fitWidth()->getEntryWidget()->getTextInt();
  }

  int fitHeightValue() const {
    return fitHeight()->getEntryWidget()->getTextInt();
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

  int borderPaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = borderPadding()->getTextInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  int shapePaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = shapePadding()->getTextInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  int innerPaddingValue() const {
    if (paddingEnabled()->isSelected()) {
      int value = innerPadding()->getTextInt();
      return MID(0, value, 100);
    }
    else
      return 0;
  }

  bool openGeneratedValue() const {
    return openGenerated()->isSelected();
  }

  std::string layerValue() const {
    if (LayerItem* item = dynamic_cast<LayerItem*>(layers()->getSelectedItem()))
      return item->layer()->name();
    else if (layers()->getSelectedItemIndex() == 1)
      return kSelectedLayers;
    else
      return kAllLayers;
  }

  std::string frameTagValue() const {
    if (TagItem* item = dynamic_cast<TagItem*>(frames()->getSelectedItem()))
      return item->tag()->name();
    else if (frames()->getSelectedItemIndex() == 1)
      return kSelectedFrames;
    else
      return kAllFrames;
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
    imageFilename()->setText("Select File: " + base::get_file_name(m_filename));
    dataFilename()->setText("Select File: " + base::get_file_name(m_dataFilename));
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
    std::string exts = get_writable_extensions();

    std::string newFilename = app::show_file_selector(
      "Save Sprite Sheet", m_filename, exts, FileSelectorType::Save);
    if (newFilename.empty())
      return;

    m_filename = newFilename;
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
    updateExportButton();
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

  void onFramesChange() {
    updateSizeFields();
  }

  void onOpenGeneratedChange() {
    updateExportButton();
  }

  void resize() {
    gfx::Size reqSize = getPreferredSize();
    moveWindow(gfx::Rect(getOrigin(), reqSize));
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
    int nframes = m_sprite->totalFrames();
    std::string tagName = frameTagValue();
    if (tagName == kSelectedFrames) {
      nframes = SelectedFrameTag::To() - SelectedFrameTag::From() + 1;
    }
    else {
      FrameTag* frameTag = m_sprite->frameTags().getByName(tagName);
      if (frameTag)
        nframes = frameTag->toFrame() - frameTag->fromFrame() + 1;
    }

    Fit fit;
    if (bestFit()->isSelected()) {
      fit = best_fit(m_sprite, nframes,
                     borderPaddingValue(), shapePaddingValue(), innerPaddingValue());
    }
    else {
      fit = calculate_sheet_size(
        m_sprite, nframes,
        columnsValue(),
        rowsValue(),
        borderPaddingValue(),
        shapePaddingValue(),
        innerPaddingValue());
    }

    columns()->setTextf("%d", fit.columns);
    rows()->setTextf("%d", fit.rows);
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
    docPref.spriteSheet.rows(window.rowsValue());
    docPref.spriteSheet.width(window.fitWidthValue());
    docPref.spriteSheet.height(window.fitHeightValue());
    docPref.spriteSheet.bestFit(window.bestFitValue());
    docPref.spriteSheet.textureFilename(window.filenameValue());
    docPref.spriteSheet.dataFilename(window.dataFilenameValue());
    docPref.spriteSheet.borderPadding(window.borderPaddingValue());
    docPref.spriteSheet.shapePadding(window.shapePaddingValue());
    docPref.spriteSheet.innerPadding(window.innerPaddingValue());
    docPref.spriteSheet.openGenerated(window.openGeneratedValue());
    docPref.spriteSheet.layer(window.layerValue());
    docPref.spriteSheet.frameTag(window.frameTagValue());

    // Default preferences for future sprites
    DocumentPreferences& defPref(Preferences::instance().document(nullptr));
    defPref.spriteSheet = docPref.spriteSheet;
    if (!defPref.spriteSheet.textureFilename().empty())
      defPref.spriteSheet.textureFilename.setValueAndDefault(kSpecifiedFilename);
    if (!defPref.spriteSheet.dataFilename().empty())
      defPref.spriteSheet.dataFilename.setValueAndDefault(kSpecifiedFilename);
    defPref.save();

    askOverwrite = false; // Already asked in the ExportSpriteSheetWindow
  }

  app::SpriteSheetType type = docPref.spriteSheet.type();
  int columns = docPref.spriteSheet.columns();
  int rows = docPref.spriteSheet.rows();
  int width = docPref.spriteSheet.width();
  int height = docPref.spriteSheet.height();
  bool bestFit = docPref.spriteSheet.bestFit();
  std::string filename = docPref.spriteSheet.textureFilename();
  std::string dataFilename = docPref.spriteSheet.dataFilename();
  std::string layerName = docPref.spriteSheet.layer();
  std::string frameTagName = docPref.spriteSheet.frameTag();
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

  // If the user want to export selected frames, we can create a
  // temporal frame tag for that.
  FrameTag* frameTag;
  bool isTemporalTag = false;
  SelectedFrameTag selectedFrameTag;
  if (frameTagName == kSelectedFrames) {
    frameTag = selectedFrameTag.create(sprite);
    isTemporalTag = true;
  }
  else if (frameTagName != kAllFrames)
    frameTag = sprite->frameTags().getByName(frameTagName);
  else
    frameTag = nullptr;

  // If the user choose to render selected layers only, we can
  // temporaly make them visible and hide the other ones.
  Layer* layer = nullptr;
  SelectedLayers layersVisibility;
  if (layerName == kSelectedLayers) {
    layersVisibility.showSelectedLayers(sprite);
  }
  else {
    // TODO add a getLayerByName
    std::vector<Layer*> layers;
    sprite->getLayersList(layers);
    for (Layer* l : layers) {
      if (l->name() == layerName) {
        layer = l;
        break;
      }
    }
  }

  int nframes = (frameTag ? frameTag->toFrame() - frameTag->fromFrame() + 1:
                            sprite->totalFrames());

  if (bestFit) {
    Fit fit = best_fit(sprite, nframes, borderPadding, shapePadding, innerPadding);
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
    borderPadding, shapePadding, innerPadding);
  if (sheet_w == 0) sheet_w = fit.width;
  if (sheet_h == 0) sheet_h = fit.height;

  DocumentExporter exporter;
  if (!filename.empty())
    exporter.setTextureFilename(filename);
  if (!dataFilename.empty())
    exporter.setDataFilename(dataFilename);
  exporter.setTextureWidth(sheet_w);
  exporter.setTextureHeight(sheet_h);
  exporter.setSpriteSheetType(type);
  exporter.setBorderPadding(borderPadding);
  exporter.setShapePadding(shapePadding);
  exporter.setInnerPadding(innerPadding);
  exporter.addDocument(document, layer, frameTag, isTemporalTag);

  base::UniquePtr<Document> newDocument(exporter.exportSheet());
  if (!newDocument)
    return;

  StatusBar* statusbar = StatusBar::instance();
  if (statusbar)
    statusbar->showTip(1000, "Sprite Sheet Generated");

  // Copy background and grid preferences
  {
    DocumentPreferences& newDocPref(Preferences::instance().document(newDocument));
    newDocPref.bg = docPref.bg;
    newDocPref.grid = docPref.grid;
    newDocPref.pixelGrid = docPref.pixelGrid;
    Preferences::instance().removeDocument(newDocument);
  }

  if (docPref.spriteSheet.openGenerated()) {
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
