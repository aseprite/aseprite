/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/unicode.h>

#include "jinete/base.h"

#include "modules/tools.h"

#endif

/* ======================== */
/* brush_tool               */
/* ======================== */

bool command_checked_brush_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_brush;
}

void command_execute_brush_tool(const char *argument)
{
  select_tool(&ase_tool_brush);
}

/* ======================== */
/* dots_tool                */
/* ======================== */

bool command_checked_dots_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_dots;
}

void command_execute_dots_tool(const char *argument)
{
  select_tool(&ase_tool_dots);
}

/* ======================== */
/* ellipse_tool             */
/* ======================== */

bool command_checked_ellipse_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_ellipse;
}

void command_execute_ellipse_tool(const char *argument)
{
  select_tool(&ase_tool_ellipse);
}

/* ======================== */
/* floodfill_tool           */
/* ======================== */

bool command_checked_floodfill_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_floodfill;
}

void command_execute_floodfill_tool(const char *argument)
{
  select_tool(&ase_tool_floodfill);
}

/* ======================== */
/* line_tool                */
/* ======================== */

bool command_checked_line_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_line;
}

void command_execute_line_tool(const char *argument)
{
  select_tool(&ase_tool_line);
}

/* ======================== */
/* marker_tool              */
/* ======================== */

bool command_checked_marker_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_marker;
}

void command_execute_marker_tool(const char *argument)
{
  select_tool(&ase_tool_marker);
}

/* ======================== */
/* pencil_tool              */
/* ======================== */

bool command_checked_pencil_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_pencil;
}

void command_execute_pencil_tool(const char *argument)
{
  select_tool(&ase_tool_pencil);
}

/* ======================== */
/* rectangle_tool           */
/* ======================== */

bool command_checked_rectangle_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_rectangle;
}

void command_execute_rectangle_tool(const char *argument)
{
  select_tool(&ase_tool_rectangle);
}

/* ======================== */
/* spray_tool               */
/* ======================== */

bool command_checked_spray_tool(const char *argument)
{
  return current_tool && current_tool == &ase_tool_spray;
}

void command_execute_spray_tool(const char *argument)
{
  select_tool(&ase_tool_spray);
}
