/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

extern FileType filetype_ase;
extern FileType filetype_bmp;
extern FileType filetype_fli;
extern FileType filetype_jpeg;
extern FileType filetype_pcx;
extern FileType filetype_tga;
extern FileType filetype_gif;
extern FileType filetype_ico;
extern FileType filetype_png;

static FileType *filetypes[] =
{
  &filetype_ase,
  &filetype_bmp,
  &filetype_fli,
  &filetype_jpeg,
  &filetype_pcx,
  &filetype_tga,
  &filetype_gif,
  &filetype_ico,
  &filetype_png,
  NULL
};
