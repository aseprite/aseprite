// Aseprite
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

#include <sstream>

namespace doc {
class Cel;
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
  size_t onMemSize() const override { return sizeof(*this) + m_size; }

private:
  void addCel(Layer* layer, Cel* cel);
  void removeCel(Layer* layer, Cel* cel);

  size_t m_size;
  std::stringstream m_stream;
};

}} // namespace app::cmd

#endif
