/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_UI_EDITOR_HANDLE_TYPE_H_INCLUDED
#define APP_UI_EDITOR_HANDLE_TYPE_H_INCLUDED
#pragma once

namespace app {

  // Handles available to transform a region of pixels in the editor.
  enum HandleType {
    // No handle selected
    NoHandle,
    // This is the handle to move the pixels region, generally, the
    // whole region activates this handle.
    MoveHandle,
    // One of the region's corders to scale.
    ScaleNWHandle, ScaleNHandle, ScaleNEHandle,
    ScaleWHandle,                ScaleEHandle,
    ScaleSWHandle, ScaleSHandle, ScaleSEHandle,
    // One of the region's corders to rotate.
    RotateNWHandle, RotateNHandle, RotateNEHandle,
    RotateWHandle,                 RotateEHandle,
    RotateSWHandle, RotateSHandle, RotateSEHandle,
    // Handle used to move the pivot
    PivotHandle,
  };

} // namespace app

#endif
