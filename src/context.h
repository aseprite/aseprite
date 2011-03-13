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

#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include <list>
#include "base/disable_copying.h"
#include "base/exception.h"
#include "settings/settings.h"

class Sprite;
class SpriteReader;
class Command;
class Params;

typedef std::list<Sprite*> Documents;

class CommandPreconditionException : public base::Exception
{
public:
  CommandPreconditionException() throw()
  : base::Exception("Cannot execute the command because its pre-conditions are false.") { }
};

class Context
{
public:
  virtual ~Context();

  virtual bool isUiAvailable() const     { return false; }
  virtual bool isRecordingMacro() const  { return false; }
  virtual bool isExecutingMacro() const  { return false; }
  virtual bool isExecutingScript() const { return false; }

  ISettings* getSettings() { return m_settings; }

  const Documents& getDocuments() const;
  Sprite* getFirstSprite() const;
  Sprite* getNextSprite(Sprite* sprite) const;

  // Appends the sprite to the context's sprites' list.
  void addSprite(Sprite* sprite);
  void removeSprite(Sprite* sprite);
  void sendSpriteToTop(Sprite* sprite);

  Sprite* getCurrentSprite() const;
  void setCurrentSprite(Sprite* sprite);

  virtual void executeCommand(Command* command, Params* params = NULL);

protected:

  // The "settings" are deleted automatically in the ~Context destructor
  Context(ISettings* settings);

  virtual void onAddSprite(Sprite* sprite);
  virtual void onRemoveSprite(Sprite* sprite);
  virtual void onSetCurrentSprite(Sprite* sprite);

private:

  // Without default constructor
  Context();

  // List of all sprites.
  Documents m_documents;

  // Current selected sprite to operate.
  Sprite* m_currentSprite;

  // Settings in this context.
  ISettings* m_settings;

  DISABLE_COPYING(Context);

};

#endif
