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

#ifndef WIDGETS_FILEVIEW_H_INCLUDED
#define WIDGETS_FILEVIEW_H_INCLUDED

#include "jinete/jbase.h"
#include "jinete/jstring.h"

#include "core/file_system.h"

/* TODO use some JI_SIGNAL_USER */
#define SIGNAL_FILEVIEW_FILE_SELECTED		0x10006
#define SIGNAL_FILEVIEW_FILE_ACCEPT		0x10007
#define SIGNAL_FILEVIEW_CURRENT_FOLDER_CHANGED	0x10008

JWidget fileview_new(IFileItem* start_folder, const jstring& exts);
int fileview_type();

IFileItem* fileview_get_current_folder(JWidget fileview);
IFileItem* fileview_get_selected(JWidget fileview);
void fileview_set_current_folder(JWidget widget, IFileItem* folder);
const FileItemList& fileview_get_filelist(JWidget fileview);

void fileview_goup(JWidget fileview);

#endif
