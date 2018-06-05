// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/input_chain.h"

#include "app/ui/input_chain_element.h"

#include <algorithm>

namespace app {

void InputChain::prioritize(InputChainElement* element,
                            const ui::Message* msg)
{
  const bool alreadyInFront =
    (!m_elements.empty() && m_elements.front() == element);

  if (!alreadyInFront) {
    auto it = std::find(m_elements.begin(), m_elements.end(), element);
    if (it != m_elements.end())
      m_elements.erase(it);
  }

  for (auto e : m_elements)
    e->onNewInputPriority(element, msg);

  if (!alreadyInFront)
    m_elements.insert(m_elements.begin(), element);
}

bool InputChain::canCut(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanCut(ctx))
      return true;
  }
  return false;
}

bool InputChain::canCopy(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanCopy(ctx))
      return true;
  }
  return false;
}

bool InputChain::canPaste(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanPaste(ctx))
      return true;
  }
  return false;
}

bool InputChain::canClear(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanClear(ctx))
      return true;
  }
  return false;
}

void InputChain::cut(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanCut(ctx) && e->onCut(ctx))
      break;
  }
}

void InputChain::copy(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanCopy(ctx) && e->onCopy(ctx))
      break;
  }
}

void InputChain::paste(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanPaste(ctx) && e->onPaste(ctx))
      break;
  }
}

void InputChain::clear(Context* ctx)
{
  for (auto e : m_elements) {
    if (e->onCanClear(ctx) && e->onClear(ctx))
      break;
  }
}

void InputChain::cancel(Context* ctx)
{
  for (auto e : m_elements)
    e->onCancel(ctx);
}

} // namespace app
