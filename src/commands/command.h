/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef COMMANDS_COMMAND_H_INCLUDED
#define COMMANDS_COMMAND_H_INCLUDED

enum CommandFlags {
  CmdUIOnlyFlag			 = 0x00000001,
  CmdRecordableFlag		 = 0x00000002,
};

class Context;
class Params;

//////////////////////////////////////////////////////////////////////
// Command

class Command
{
  const char* m_short_name;
  const char* m_friendly_name;
  CommandFlags m_flags;

public:

  Command(const char* short_name, const char* friendly_name, CommandFlags flags);
  virtual ~Command();

  virtual Command* clone() const { return new Command(*this); }

  const char* short_name() const { return m_short_name; }
  const char* friendly_name() const { return m_friendly_name; }

  void loadParams(Params* params);
  bool isEnabled(Context* context);
  bool isChecked(Context* context);
  void execute(Context* context);

protected:
  virtual void onLoadParams(Params* params);
  virtual bool onEnabled(Context* context);
  virtual bool onChecked(Context* context);
  virtual void onExecute(Context* context);
};

//////////////////////////////////////////////////////////////////////
// CommandFactory

class CommandFactory
{
public:
  #undef FOR_EACH_COMMAND
  #define FOR_EACH_COMMAND(name) \
    static Command* create_##name##_command();

  #include "commands/commands_list.h"
  #undef FOR_EACH_COMMAND
};

#endif
