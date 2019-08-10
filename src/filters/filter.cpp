// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/filter.h"

#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"

namespace filters {

FilterWithPalette::FilterWithPalette()
  : m_usePaletteOnRGB(false)
{
}

void FilterWithPalette::applyToPalette(FilterManager* filterMgr)
{
  FilterIndexedData* fid = filterMgr->getIndexedData();
  doc::PalettePicks picks = fid->getPalettePicks();

  switch (filterMgr->pixelFormat()) {

    case doc::IMAGE_RGB:
      m_usePaletteOnRGB = (picks.picks() > 0);
      if (!m_usePaletteOnRGB)
        return;
      break;

    case doc::IMAGE_INDEXED:
      // If there is a selection, we don't apply the filter to color
      // palette, instead we apply the filter to the pixels as an RGB
      // image (using closest colors from the palette)
      if (filterMgr->isMaskActive())
        return;

      // If there are no picks, we apply the filter to the whole palette.
      if (picks.picks() == 0) {
        picks.resize(fid->getPalette()->size());
        picks.all();
      }
      break;

    default:
      // We cannot change the palette of a grayscale image
      return;
  }

  onApplyToPalette(filterMgr, picks);
}

} // namespace filters
