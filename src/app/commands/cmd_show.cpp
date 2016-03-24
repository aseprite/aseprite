// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/pref/preferences.h"

namespace app {

class ShowExtrasCommand : public Command {
public:
  ShowExtrasCommand()
    : Command("ShowExtras",
              "Show Extras",
              CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new ShowExtrasCommand(*this); }

protected:
  bool onChecked(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.selectionEdges();
  }

  void onExecute(Context* ctx) override {
    DocumentPreferences& globPref = Preferences::instance().document(nullptr);
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    if (docPref.show.selectionEdges()) {
      globPref.show = docPref.show;
      docPref.show.selectionEdges(false);
      docPref.show.grid(false);
      docPref.show.pixelGrid(false);
    }
    else {
      docPref.show.selectionEdges(true);
      docPref.show.grid(
        docPref.show.grid() ||
        globPref.show.grid());
      docPref.show.pixelGrid(
        docPref.show.pixelGrid() ||
        globPref.show.pixelGrid());
    }
  }
};

class ShowGridCommand : public Command {
public:
  ShowGridCommand()
    : Command("ShowGrid",
              "Show Grid",
              CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new ShowGridCommand(*this); }

protected:
  bool onChecked(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.grid();
  }

  void onExecute(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.grid(!docPref.show.grid());
  }
};

class ShowPixelGridCommand : public Command {
public:
  ShowPixelGridCommand()
    : Command("ShowPixelGrid",
              "Show Pixel Grid",
              CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new ShowPixelGridCommand(*this); }

protected:
  bool onChecked(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.pixelGrid();
  }

  void onExecute(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.pixelGrid(!docPref.show.pixelGrid());
  }
};

class ShowSelectionEdgesCommand : public Command {
public:
  ShowSelectionEdgesCommand()
    : Command("ShowSelectionEdges",
              "Show Selection Edges",
              CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new ShowSelectionEdgesCommand(*this); }

protected:
  bool onChecked(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.show.selectionEdges();
  }

  void onExecute(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    docPref.show.selectionEdges(!docPref.show.selectionEdges());
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

Command* CommandFactory::createShowSelectionEdgesCommand()
{
  return new ShowSelectionEdgesCommand;
}

} // namespace app
