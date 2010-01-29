/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#ifndef CORE_CHECK_ARGS_H_INCLUDED
#define CORE_CHECK_ARGS_H_INCLUDED

#include <string>
#include <vector>

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
    std::string m_data;

  public:
    enum {
      OpenSprite,
    };

    Option(int type, const char* data) : m_type(type), m_data(data) { }

    int type() const { return m_type; }
    const char* data() const { return m_data.c_str(); }
  };

private:
  std::vector<Option*> m_options;
  std::string m_palette_filename;
  const char* m_exe_name;

public:
  typedef std::vector<Option*>::iterator iterator;
  
  CheckArgs(int argc, char *argv[]);
  ~CheckArgs();

  void clear();

  iterator begin() { return m_options.begin(); }
  iterator end() { return m_options.end(); }

  std::string get_palette_filename() { return m_palette_filename; }

private:
  void usage(bool show_help);

};

#endif

