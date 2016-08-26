// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd.h"

#include <typeinfo>

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

void Cmd::execute(Context* ctx)
{
  TRACE("Cmd: Executing cmd '%s'\n", typeid(*this).name());
  ASSERT(m_state == State::NotExecuted);

  m_ctx = ctx;

  onExecute();
  onFireNotifications();

#if _DEBUG
  m_state = State::Executed;
#endif
}

void Cmd::undo()
{
  TRACE("Cmd: Undo cmd '%s'\n", typeid(*this).name());
  ASSERT(m_state == State::Executed || m_state == State::Redone);

  onUndo();
  onFireNotifications();

#if _DEBUG
  m_state = State::Undone;
#endif
}

void Cmd::redo()
{
  TRACE("Cmd: Redo cmd '%s'\n", typeid(*this).name());
  ASSERT(m_state == State::Undone);

  onRedo();
  onFireNotifications();

#if _DEBUG
  m_state = State::Redone;
#endif
}

void Cmd::dispose()
{
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

void Cmd::onExecute()
{
  // Do nothing
}

void Cmd::onUndo()
{
  // Do nothing
}

void Cmd::onRedo()
{
  // By default onRedo() uses onExecute() implementation
  onExecute();
}

void Cmd::onFireNotifications()
{
  // Do nothing
}

std::string Cmd::onLabel() const
{
  return "";
}

size_t Cmd::onMemSize() const {
  return sizeof(*this);
}

} // namespace app
