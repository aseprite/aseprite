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

#ifndef CHECK_ARGS_H_INCLUDED
#define CHECK_ARGS_H_INCLUDED

#include "Vaca/String.h"
#include <vector>

using Vaca::String;

// Parses the input arguments in the command line.
class CheckArgs
{
public:
  // Represents one specifial option specified in the parameters that
  // need some special operation by the application class.
  class Option
  {
    int m_type;
    String m_data;

  public:
    enum {
      OpenSprite,
    };

    Option(int type, const String& data) : m_type(type), m_data(data) { }

    int type() const { return m_type; }
    const String& data() const { return m_data; }
  };

  typedef std::vector<Option*> Options;
  typedef Options::iterator iterator;
  
  CheckArgs();
  ~CheckArgs();

  void clear();

  iterator begin() { return m_options.begin(); }
  iterator end() { return m_options.end(); }

  String getPaletteFilename() const { return m_paletteFilename; }

  bool isConsoleOnly() const { return m_consoleOnly; }
  bool isVerbose() const { return m_verbose; }

private:
  void usage(bool showHelp);

  Options m_options;
  String m_paletteFilename;
  String m_exeName;
  bool m_consoleOnly;
  bool m_verbose;
};

#endif

