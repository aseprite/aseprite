// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"

namespace app {

class ShowExtrasCommand : public Command {
public:
  ShowExtrasCommand() : Command(CommandId::ShowExtras(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.selectionEdges();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& globPref = Preferences::instance().document(nullptr);
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    if (docPref.show.selectionEdges()) {
      globPref.show = docPref.show;
      docPref.show.selectionEdges(false);
      docPref.show.layerEdges(false);
      docPref.show.grid(false);
      docPref.show.pixelGrid(false);
    }
    else {
      docPref.show.selectionEdges(true);
      docPref.show.layerEdges(docPref.show.layerEdges() || globPref.show.layerEdges());
      docPref.show.grid(docPref.show.grid() || globPref.show.grid());
      docPref.show.pixelGrid(docPref.show.pixelGrid() || globPref.show.pixelGrid());
    }
  }
};

class ShowLayerEdgesCommand : public Command {
public:
  ShowLayerEdgesCommand() : Command(CommandId::ShowLayerEdges(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.layerEdges();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.layerEdges(!docPref.show.layerEdges());
  }
};

class ShowGridCommand : public Command {
public:
  ShowGridCommand() : Command(CommandId::ShowGrid(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.grid();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.grid(!docPref.show.grid());
  }
};

class ShowPixelGridCommand : public Command {
public:
  ShowPixelGridCommand() : Command(CommandId::ShowPixelGrid(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.pixelGrid();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.pixelGrid(!docPref.show.pixelGrid());
  }
};

class ShowSelectionEdgesCommand : public Command {
public:
  ShowSelectionEdgesCommand() : Command(CommandId::ShowSelectionEdges(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.selectionEdges();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.selectionEdges(!docPref.show.selectionEdges());
  }
};

class ShowBrushPreviewCommand : public Command {
public:
  ShowBrushPreviewCommand() : Command(CommandId::ShowBrushPreview(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.brushPreview();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.brushPreview(!docPref.show.brushPreview());

    // TODO we shouldn't need this, but it happens to be that the
    // Preview editor isn't being updated correctly when we change the
    // brush preview state.
    update_screen_for_document(ctx->activeDocument());
  }
};

// This command works as a global preference (toogling the Brush
// Preview in the Preview window), not based on the active document.
class ShowBrushPreviewInPreviewCommand : public Command {
public:
  ShowBrushPreviewInPreviewCommand()
    : Command(CommandId::ShowBrushPreviewInPreview(), CmdUIOnlyFlag)
  {
  }

protected:
  bool onChecked(Context* ctx) override
  {
    Preferences& pref = Preferences::instance();
    return pref.cursor.brushPreviewInPreview();
  }

  void onExecute(Context* ctx) override
  {
    Preferences& pref = Preferences::instance();
    pref.cursor.brushPreviewInPreview(!pref.cursor.brushPreviewInPreview());
  }
};

class ShowAutoGuidesCommand : public Command {
public:
  ShowAutoGuidesCommand() : Command(CommandId::ShowAutoGuides(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.autoGuides();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.autoGuides(!docPref.show.autoGuides());
  }
};

class ShowSlicesCommand : public Command {
public:
  ShowSlicesCommand() : Command(CommandId::ShowSlices(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.slices();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.slices(!docPref.show.slices());
  }
};

class ShowTileNumbersCommand : public Command {
public:
  ShowTileNumbersCommand() : Command(CommandId::ShowTileNumbers(), CmdUIOnlyFlag) {}

protected:
  bool onChecked(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.tileNumbers();
  }

  void onExecute(Context* ctx) override
  {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.tileNumbers(!docPref.show.tileNumbers());
  }
};

Command* CommandFactory::createShowExtrasCommand()
{
  return new ShowExtrasCommand;
}

Command* CommandFactory::createShowGridCommand()
{
  return new ShowGridCommand;
}

Command* CommandFactory::createShowPixelGridCommand()
{
  return new ShowPixelGridCommand;
}

Command* CommandFactory::createShowLayerEdgesCommand()
{
  return new ShowLayerEdgesCommand;
}

Command* CommandFactory::createShowSelectionEdgesCommand()
{
  return new ShowSelectionEdgesCommand;
}

Command* CommandFactory::createShowBrushPreviewCommand()
{
  return new ShowBrushPreviewCommand;
}

Command* CommandFactory::createShowBrushPreviewInPreviewCommand()
{
  return new ShowBrushPreviewInPreviewCommand;
}

Command* CommandFactory::createShowAutoGuidesCommand()
{
  return new ShowAutoGuidesCommand;
}

Command* CommandFactory::createShowSlicesCommand()
{
  return new ShowSlicesCommand;
}

Command* CommandFactory::createShowTileNumbersCommand()
{
  return new ShowTileNumbersCommand;
}

} // namespace app
