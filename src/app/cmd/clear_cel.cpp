// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/clear_cel.h"

#include "app/cmd/clear_image.h"
#include "app/cmd/remove_cel.h"
#include "app/context.h"
#include "app/doc.h"
#include "doc/cel.h"
#include "doc/layer.h"

namespace app { namespace cmd {

using namespace doc;

ClearCel::ClearCel(Cel* cel) : WithCel(cel)
{
  Doc* doc = static_cast<Doc*>(cel->document());

  if (cel->layer()->isBackground()) {
    if (Image* image = cel->image())
      m_seq.add(new cmd::ClearImage(image, doc->bgColor(cel->layer())));
  }
  else {
    m_seq.add(new cmd::RemoveCel(cel));
  }
}

void ClearCel::onExecute(Context* ctx)
{
  m_seq.execute(ctx);
}

void ClearCel::onUndo(Context* ctx)
{
  m_seq.undo(ctx);
}

void ClearCel::onRedo(Context* ctx)
{
  m_seq.redo(ctx);
}

void ClearCel::onSerialize(CmdSerial& s)
{
  Cmd::onSerialize(s);
  serializeCelId(s);
  m_seq.serialize(s);
}

}} // namespace app::cmd
