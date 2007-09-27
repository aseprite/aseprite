/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include "modules/editors.h"

#endif

void command_execute_close_editor(const char *argument)
{
  close_editor(current_editor);
}

void command_execute_make_unique_editor(const char *argument)
{
  make_unique_editor(current_editor);
}

void command_execute_split_editor_horizontally(const char *argument)
{
  split_editor(current_editor, JI_HORIZONTAL);
}

void command_execute_split_editor_vertically(const char *argument)
{
  split_editor(current_editor, JI_VERTICAL);
}
