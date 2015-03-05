// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_export_sprite_sheet.h"

#include "app/commands/cmd_save_file.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_undo.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/transaction.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/ui.h"

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
  typedef ExportSpriteSheetCommand::SpriteSheetType SpriteSheetType;
  typedef ExportSpriteSheetCommand::ExportAction ExportAction;

  ExportSpriteSheetWindow(Document* doc, Sprite* sprite)
    : m_sprite(sprite)
  {
    doc::ExportDataPtr data = doc->exportData();

    sheetType()->addItem("Horizontal Strip");
    sheetType()->addItem("Vertical Strip");
    sheetType()->addItem("Matrix");
    if (data)
      sheetType()->setSelectedItemIndex((int)data->type()-1); // TODO harcoded -1

    exportAction()->addItem("Save Copy As...");
    exportAction()->addItem("Save As...");
    exportAction()->addItem("Save");
    exportAction()->addItem("Do Not Save");

    for (int i=2; i<=8192; i*=2) {
      std::string value = base::convert_to<std::string>(i);
      if (i >= m_sprite->width()) fitWidth()->addItem(value);
      if (i >= m_sprite->height()) fitHeight()->addItem(value);
    }

    if (!data || data->bestFit()) {
      bestFit()->setSelected(true);
      onBestFit();
    }
    else if (data) {
      columns()->setTextf("%d", data->columns());
      onColumnsChange();

      if (data->width() > 0 || data->height() > 0) {
        if (data->width() > 0) fitWidth()->getEntryWidget()->setTextf("%d", data->width());
        if (data->height() > 0) fitHeight()->getEntryWidget()->setTextf("%d", data->height());
        onSizeChange();
      }
    }
    else {
      columns()->setText("4");
      onColumnsChange();
    }

    sheetType()->Change.connect(&ExportSpriteSheetWindow::onSheetTypeChange, this);
    columns()->EntryChange.connect(Bind<void>(&ExportSpriteSheetWindow::onColumnsChange, this));
    fitWidth()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    fitHeight()->Change.connect(Bind<void>(&ExportSpriteSheetWindow::onSizeChange, this));
    bestFit()->Click.connect(Bind<void>(&ExportSpriteSheetWindow::onBestFit, this));

    onSheetTypeChange();
  }

  bool ok() {
    return getKiller() == exportButton();
  }

  SpriteSheetType spriteSheetTypeValue() {
    return (SpriteSheetType)sheetType()->getSelectedItemIndex();
  }

  ExportAction exportActionValue() {
    return (ExportAction)exportAction()->getSelectedItemIndex();
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

protected:

  void onSheetTypeChange() {
    bool state = false;
    switch (sheetType()->getSelectedItemIndex()) {
      case ExportSpriteSheetCommand::Matrix:
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

    gfx::Size reqSize = getPreferredSize();
    moveWindow(gfx::Rect(getOrigin(), reqSize));

    invalidate();
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

private:
  Sprite* m_sprite;
};

ExportSpriteSheetCommand::ExportSpriteSheetCommand()
  : Command("ExportSpriteSheet",
            "Export Sprite Sheet",
            CmdRecordableFlag)
  , m_useUI(true)
{
}

bool ExportSpriteSheetCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ExportSpriteSheetCommand::onExecute(Context* context)
{
  Document* document(context->activeDocument());
  Sprite* sprite = document->sprite();

  if (m_useUI) {
    ExportSpriteSheetWindow window(document, sprite);
    window.openWindowInForeground();
    if (!window.ok())
      return;

    m_type = window.spriteSheetTypeValue();
    m_action = window.exportActionValue();
    m_columns = window.columnsValue();
    m_width = window.fitWidthValue();
    m_height= window.fitHeightValue();
    m_bestFit = window.bestFitValue();
  }
  else if (m_bestFit) {
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
    case HorizontalStrip:
      columns = nframes;
      break;
    case VerticalStrip:
      columns = 1;
      break;
    case Matrix:
      columns = m_columns;
      if (m_width > 0) sheet_w = m_width;
      if (m_height > 0) sheet_h = m_height;
      break;
  }

  columns = MID(1, columns, nframes);
  if (sheet_w == 0) sheet_w = sprite->width()*columns;
  if (sheet_h == 0) sheet_h = sprite->height()*((nframes/columns)+((nframes%columns)>0?1:0));
  columns = sheet_w / sprite->width();

  ImageRef resultImage(Image::create(sprite->pixelFormat(), sheet_w, sheet_h));
  doc::clear_image(resultImage, 0);

  render::Render render;

  int column = 0, row = 0;
  for (frame_t frame = 0; frame<nframes; ++frame) {
    render.renderSprite(resultImage, sprite, frame,
      gfx::Clip(column*sprite->width(), row*sprite->height(),
        sprite->bounds()));

    if (++column >= columns) {
      column = 0;
      ++row;
    }
  }

  // Store the frame in the current editor so we can restore it
  // after change and restore the setTotalFrames() number.
  frame_t oldSelectedFrame = (current_editor ?
    current_editor->frame():
    frame_t(0));

  {
    // The following steps modify the sprite, so we wrap all
    // operations in a undo-transaction.
    ContextWriter writer(context);
    Transaction transaction(writer.context(), "Export Sprite Sheet", ModifyDocument);
    DocumentApi api = document->getApi(transaction);

    // Add the layer in the sprite.
    LayerImage* resultLayer = api.newLayer(sprite);

    // Create the cel.
    base::UniquePtr<Cel> resultCel(new Cel(frame_t(0), resultImage));

    // Add the cel in the layer.
    api.addCel(resultLayer, resultCel);
    resultCel.release();

    // Copy the list of layers (because we will modify it in the iteration).
    LayerList layers = sprite->folder()->getLayersList();

    // Remove all other layers
    for (LayerIterator it=layers.begin(), end=layers.end(); it!=end; ++it) {
      if (*it != resultLayer)
        api.removeLayer(*it);
    }

    // Change the number of frames (just one, the sprite sheet). As
    // we are using the observable API, all DocumentView will change
    // its current frame to frame 1. We'll try to restore the
    // selected frame for the current_editor later.
    api.setTotalFrames(sprite, frame_t(1));

    // Set the size of the sprite to the tile size.
    api.setSpriteSize(sprite, sheet_w, sheet_h);

    transaction.commit();

    // Draw the document with the new dimensions in the screen.
    update_screen_for_document(document);
  }

  // This flag indicates if we've to undo the last action (so we
  // back to the original sprite dimensions).
  bool undo = false;

  Params params;
  if (!m_filename.empty())
    params.set("filename", m_filename.c_str());

  SaveFileBaseCommand* command = NULL;

  // Do the "Export Action"
  switch (m_action) {

    case SaveCopyAs:
      command = static_cast<SaveFileBaseCommand*>(CommandsModule::instance()
        ->getCommandByName(CommandId::SaveFileCopyAs));

      context->executeCommand(command, &params);

      // Always go back, as we are using "Save Copy As", so the user
      // wants to continue editing the original sprite.
      undo = true;
      break;

    case SaveAs:
      command = static_cast<SaveFileBaseCommand*>(CommandsModule::instance()
        ->getCommandByName(CommandId::SaveFileAs));

      context->executeCommand(command, &params);

      // If the command was cancelled, we go back to the original
      // state, if the sprite sheet was saved then we don't undo
      // because the user wants to edit the sprite sheet.
      undo = (document->isModified());
      break;

    case Save:
      command = static_cast<SaveFileBaseCommand*>(CommandsModule::instance()
        ->getCommandByName(CommandId::SaveFile));

      context->executeCommand(command, &params);

      // Same case as "Save As"
      undo = (document->isModified());
      break;

    case DoNotSave:
      // Do not undo as the user wants to edit the sprite sheet
      // before to save the file.
      undo = false;
      break;
  }

  // Set export data
  if (command != NULL) {
    doc::ExportData::Type type;
    switch (m_type) {
      case ExportSpriteSheetCommand::HorizontalStrip:
        type = doc::ExportData::HorizontalStrip;
        break;
      case ExportSpriteSheetCommand::VerticalStrip:
        type = doc::ExportData::VerticalStrip;
        break;
      case ExportSpriteSheetCommand::Matrix:
        type = doc::ExportData::Matrix;
        break;
    }
    doc::ExportDataPtr data(new doc::ExportData);
    data->setType(type);
    data->setColumns(columns);
    data->setWidth(m_width);
    data->setHeight(m_height);
    data->setBestFit(m_bestFit);
    data->setFilename(command->selectedFilename());
    document->setExportData(data);
  }

  // Undo the sprite sheet conversion
  if (undo) {
    if (document->undoHistory()->canUndo()) {
      document->undoHistory()->undo();

      // We've to restore the previously selected frame. As we've
      // called setTotalFrames(), all document observers
      // (current_editor included) have changed its current frame to
      // the first one (to a visible/editable frame). The "undo"
      // action doesn't restore the previously selected frame in
      // observers, so at least we can restore the current_editor's
      // frame.
      if (current_editor)
        current_editor->setFrame(oldSelectedFrame);
    }

    document->generateMaskBoundaries();
    document->destroyExtraCel(); // Regenerate extras

    // Redraw the sprite.
    update_screen_for_document(document);
  }
}

void ExportSpriteSheetCommand::setExportData(doc::ExportDataPtr data)
{
  ExportSpriteSheetCommand::SpriteSheetType type;
  switch (data->type()) {
    case doc::ExportData::None: return;
    case doc::ExportData::HorizontalStrip:
      type = ExportSpriteSheetCommand::HorizontalStrip;
      break;
    case doc::ExportData::VerticalStrip:
      type = ExportSpriteSheetCommand::VerticalStrip;
      break;
    case doc::ExportData::Matrix:
      type = ExportSpriteSheetCommand::Matrix;
      break;
  }

  m_type = type;
  m_action = ExportSpriteSheetCommand::SaveCopyAs;
  m_columns = data->columns();
  m_width = data->width();
  m_height = data->height();
  m_bestFit = data->bestFit();
  m_filename = data->filename();
}

Command* CommandFactory::createExportSpriteSheetCommand()
{
  return new ExportSpriteSheetCommand;
}

} // namespace app
