/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "console.h"
#include "context.h"
#include "commands/command.h"
#include "raster/sprite.h"

#include <algorithm>

Context::Context(ISettings* settings)
{
  m_currentSprite = NULL;
  m_settings = settings;
}

Context::~Context()
{
  delete m_settings;
}

const Documents& Context::getDocuments() const
{
  return m_documents;
}

Sprite* Context::getFirstSprite() const
{
  if (!m_documents.empty())
    return m_documents.getByIndex(0);
  else
    return NULL;
}

Sprite* Context::getNextSprite(Sprite* sprite) const
{
  ASSERT(sprite != NULL);

  Documents::const_iterator it = std::find(m_documents.begin(), m_documents.end(), sprite);

  if (it != m_documents.end()) {
    ++it;
    if (it != m_documents.end())
      return *it;
  }
  return NULL;
}

void Context::addSprite(Sprite* sprite)
{
  ASSERT(sprite != NULL);

  m_documents.addDocument(sprite);

  // Generate onAddSprite event
  onAddSprite(sprite);
}

void Context::removeSprite(Sprite* sprite)
{
  ASSERT(sprite != NULL);

  // Remove the item from the sprites list.
  m_documents.removeDocument(sprite);

  // generate on_remove_sprite event
  onRemoveSprite(sprite);

  // the current sprite cannot be the removed sprite anymore
  if (m_currentSprite == sprite)
    setCurrentSprite(NULL);
}

void Context::sendSpriteToTop(Sprite* sprite)
{
  ASSERT(sprite != NULL);

  m_documents.moveDocument(sprite, 0);
}

Sprite* Context::getCurrentSprite() const
{
  return m_currentSprite;
}

void Context::setCurrentSprite(Sprite* sprite)
{
  m_currentSprite = sprite;

  onSetCurrentSprite(sprite);
}

void Context::executeCommand(Command* command, Params* params)
{
  Console console;

  ASSERT(command != NULL);

  PRINTF("Executing '%s' command.\n", command->short_name());

  try {
    if (params)
      command->loadParams(params);

    if (command->isEnabled(this))
      command->execute(this);
  }
  catch (base::Exception& e) {
    PRINTF("Exception caught executing '%s' command\n%s\n",
	   command->short_name(), e.what());

    Console::showException(e);
  }
  catch (std::exception& e) {
    PRINTF("std::exception caught executing '%s' command\n%s\n",
	   command->short_name(), e.what());

    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
#ifndef DEBUGMODE
  catch (...) {
    PRINTF("unknown exception executing '%s' command\n",
	   command->short_name());

    console.printf("An unknown error ocurred executing the command.\n"
  		   "Please save your work, close the program, try it\n"
		   "again, and report this bug.\n\n"
  		   "Details: Unknown exception caught. This can be bad\n"
		   "memory access, divison by zero, etc.");
  }
#endif
}

void Context::onAddSprite(Sprite* sprite)
{
  // do nothing
}

void Context::onRemoveSprite(Sprite* sprite)
{
  // do nothing
}

void Context::onSetCurrentSprite(Sprite* sprite)
{
  // do nothing
}
