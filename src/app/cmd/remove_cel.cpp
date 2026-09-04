// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_cel.h"

#include "doc/cel.h"
#include "doc/layer.h"

namespace app { namespace cmd {

using namespace doc;

RemoveCel::RemoveCel(Cel* cel) : AddCel(cel->layer(), cel)
{
}

void RemoveCel::onExecute(Context* ctx)
{
  AddCel::onUndo(ctx);
}

void RemoveCel::onUndo(Context* ctx)
{
  AddCel::onRedo(ctx);
}

void RemoveCel::onRedo(Context* ctx)
{
  AddCel::onUndo(ctx);
}

}} // namespace app::cmd
