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

/**
 * Looks the input arguments in the command line.
 */
class CheckArgs
{
public:
  /**
   * Option specified in the parameters.
   */
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

private:
  std::vector<Option*> m_options;
  String m_paletteFilename;
  String m_exeName;

public:
  typedef std::vector<Option*>::iterator iterator;
  
  CheckArgs();
  ~CheckArgs();

  void clear();

  iterator begin() { return m_options.begin(); }
  iterator end() { return m_options.end(); }

  String getPaletteFilename() const { return m_paletteFilename; }

private:
  void usage(bool showHelp);

};

#endif

