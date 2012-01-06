/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#ifndef FILTERS_TARGET_H_INCLUDED
#define FILTERS_TARGET_H_INCLUDED

#define TARGET_RED_CHANNEL              1
#define TARGET_GREEN_CHANNEL            2
#define TARGET_BLUE_CHANNEL             4
#define TARGET_ALPHA_CHANNEL            8
#define TARGET_GRAY_CHANNEL             16
#define TARGET_INDEX_CHANNEL            32
#define TARGET_ALL_FRAMES               64
#define TARGET_ALL_LAYERS               128

#define TARGET_ALL_CHANNELS             \
  (TARGET_RED_CHANNEL           |       \
   TARGET_GREEN_CHANNEL         |       \
   TARGET_BLUE_CHANNEL          |       \
   TARGET_ALPHA_CHANNEL         |       \
   TARGET_GRAY_CHANNEL          )

typedef int Target;

#endif
