// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/script_input_chain.h"

#include "app/app.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/remap_colors.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/site.h"
#include "app/tx.h"
#include "app/util/clipboard.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"

#include <cstring>
#include <limits>
#include <memory>

namespace app {

ScriptInputChain::~ScriptInputChain()
{
}

void ScriptInputChain::onNewInputPriority(InputChainElement* element, const ui::Message* msg)
{
}

bool ScriptInputChain::onCanCut(Context* ctx)
{
  return ctx->activeDocument() && ctx->activeDocument()->isMaskVisible();
}

bool ScriptInputChain::onCanCopy(Context* ctx)
{
  return onCanCut(ctx);
}

bool ScriptInputChain::onCanPaste(Context* ctx)
{
  const Clipboard* clipboard(ctx->clipboard());
  if (!clipboard)
    return false;
  return clipboard->format() == ClipboardFormat::Image && ctx->activeSite().layer() &&
         ctx->activeSite().layer()->type() == ObjectType::LayerImage;
}

bool ScriptInputChain::onCanClear(Context* ctx)
{
  return onCanCut(ctx);
}

bool ScriptInputChain::onCut(Context* ctx)
{
  ContextWriter writer(ctx);
  Clipboard* clipboard = ctx->clipboard();
  if (!clipboard)
    return false;
  if (writer.document()) {
    clipboard->cut(writer);
    return true;
  }
  return false;
}

bool ScriptInputChain::onCopy(Context* ctx)
{
  ContextReader reader(ctx);
  Clipboard* clipboard = ctx->clipboard();
  if (!clipboard)
    return false;
  if (reader.document()) {
    clipboard->copy(reader);
    return true;
  }
  return false;
}

bool ScriptInputChain::onPaste(Context* ctx, const gfx::Point* position)
{
  Clipboard* clipboard = ctx->clipboard();
  if (!clipboard)
    return false;
  if (clipboard->format() == ClipboardFormat::Image) {
    clipboard->paste(ctx, false, position);
    return true;
  }
  return false;
}

bool ScriptInputChain::onClear(Context* ctx)
{
  // TODO This code is similar to DocView::onClear() and Clipboard::cut()
  ContextWriter writer(ctx);
  Doc* document = ctx->activeDocument();
  if (writer.document()) {
    ctx->clipboard()->clearContent();
    CelList cels;
    const Site site = ctx->activeSite();
    cels.push_back(site.cel());
    if (cels.empty()) // No cels to modify
      return false;
    Tx tx(writer, "Clear");
    ctx->clipboard()->clearMaskFromCels(tx, document, site, cels, true);
    tx.commit();
    return true;
  }
  return false;
}

void ScriptInputChain::onCancel(Context* ctx)
{
  // Deselect mask
  if (ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasVisibleMask)) {
    Command* deselectMask = Commands::instance()->byId(CommandId::DeselectMask());
    ctx->executeCommand(deselectMask);
    ctx->activeDocument()->setMaskVisible(false);
  }
}

} // namespace app
