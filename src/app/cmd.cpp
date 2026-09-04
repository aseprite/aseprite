// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd.h"

#include "app/context.h"
#include "base/debug.h"
#include "base/mem_utils.h"

#include <typeinfo>

#define CMD_TRACE(...)

namespace app {

Cmd::Cmd()
#if _DEBUG
  : m_state(State::NotExecuted)
#endif
{
}

Cmd::~Cmd()
{
}

std::string Cmd::typeStr() const
{
  std::string result(5, 0);
  cmdtype_t t = type();
  result[0] = ((t >> 24) & 0xff);
  result[1] = ((t >> 16) & 0xff);
  if (((t >> 8) & 0xff) != ' ') {
    result[2] = ((t >> 8) & 0xff);
    if ((t & 0xff) != ' ') {
      result[3] = (t & 0xff);
    }
  }
  return result;
}

void Cmd::execute(Context* ctx)
{
  CMD_TRACE("CMD: Executing cmd '%s'\n", typeid(*this).name());
  // ASSERT(m_state == State::NotExecuted);

  onExecute(ctx);
  onFireNotifications(ctx);

#if _DEBUG
  m_state = State::Executed;
#endif
}

void Cmd::undo(undo::UndoContext* ctx)
{
  CMD_TRACE("CMD: Undo cmd '%s'\n", typeid(*this).name());
  // ASSERT(m_state == State::Executed || m_state == State::Redone);

  onUndo(static_cast<Context*>(ctx));
  onFireNotifications(static_cast<Context*>(ctx));

#if _DEBUG
  m_state = State::Undone;
#endif
}

void Cmd::redo(undo::UndoContext* ctx)
{
  CMD_TRACE("CMD: Redo cmd '%s'\n", typeid(*this).name());
  // ASSERT(m_state == State::Undone);

  onRedo(static_cast<Context*>(ctx));
  onFireNotifications(static_cast<Context*>(ctx));

#if _DEBUG
  m_state = State::Redone;
#endif
}

void Cmd::dispose()
{
  CMD_TRACE("CMD: Deleting '%s' (%s)\n",
            typeid(*this).name(),
            base::get_pretty_memory_size(memSize()).c_str());

  delete this;
}

std::string Cmd::label() const
{
  return onLabel();
}

size_t Cmd::memSize() const
{
  return onMemSize();
}

bool Cmd::isSerializable() const
{
  return onIsSerializable();
}

void Cmd::serialize(CmdSerial& s)
{
  if (s.decoding())
    m_state = State::Executed;
  onSerialize(s);
}

void Cmd::onExecute(Context* ctx)
{
  // Do nothing
}

void Cmd::onUndo(Context* ctx)
{
  // Do nothing
}

void Cmd::onRedo(Context* ctx)
{
  // By default onRedo() uses onExecute() implementation
  onExecute(ctx);
}

void Cmd::onFireNotifications(Context* ctx)
{
  // Do nothing
}

std::string Cmd::onLabel() const
{
  return "";
}

size_t Cmd::onMemSize() const
{
  return sizeof(*this);
}

void Cmd::onSerialize(CmdSerial& s)
{
  auto t = type();
  s.cmdtype(t);
}

} // namespace app
