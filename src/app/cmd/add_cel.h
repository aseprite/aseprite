// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_CEL_H_INCLUDED
#define APP_CMD_ADD_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "app/cmd/with_layer.h"
#include "app/cmd/with_suspended.h"
#include "doc/cel.h"

namespace doc {
class Layer;
} // namespace doc

namespace app { namespace cmd {
using namespace doc;

class AddCel : public Cmd,
               public WithLayer,
               public WithCel {
public:
  AddCel(Layer* layer, Cel* cel);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_suspendedCel.size(); }

private:
  void addCel(Layer* layer, Cel* cel);
  void removeCel(Layer* layer, Cel* cel);

  WithSuspended<doc::Cel*> m_suspendedCel;
};

}} // namespace app::cmd

#endif
