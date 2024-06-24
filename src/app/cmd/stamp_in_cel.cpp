// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/stamp_in_cel.h"

#include "app/cmd/copy_region.h"
#include "app/cmd/crop_cel.h"
#include "app/cmd/trim_cel.h"
#include "app/cmd/with_image.h"
#include "app/util/buffer_region.h"
#include "base/debug.h"
#include "doc/algorithm/rotate.h"
#include "doc/cel.h"
#include "doc/layer_tilemap.h"
#include "doc/mask.h"
#include "gfx/point.h"
#include "gfx/region.h"


namespace app {
namespace cmd {

using namespace doc;

class StampImage : public Cmd
                 , public WithImage {

public:
  StampImage(doc::Image* dst,
             const doc::ImageRef& src,
             const doc::MaskRef& mask,
             const gfx::Rect& stampBounds) : WithImage(dst)
                                           , m_src(src)
                                           , m_mask(mask)
                                           , m_stampBounds(stampBounds) {
    ASSERT(!stampBounds.isEmpty());
    ASSERT(src);
    ASSERT(mask);


    gfx::Rect rc = stampBounds;
    gfx::Clip clip(
      rc.x, rc.y,
      0, 0, rc.w, rc.h);
    if (clip.clip(
          dst->width(), dst->height(),
          rc.w, rc.h)) {
      // Create region to save/swap later
      m_region = gfx::Region(stampBounds);
      m_region &= gfx::Region(clip.dstBounds());
    }

    save_image_region_in_buffer(m_region, dst, gfx::Point(0,0), m_buffer);
  }

protected:
  void onExecute() override {
    ASSERT(image());

    gfx::Rect rc = m_stampBounds;
    doc::algorithm::parallelogram(
      image(), m_src.get(), m_mask->bitmap(),
      rc.x         , rc.y,
      rc.x+rc.w    , rc.y,
      rc.x+rc.w    , rc.y+rc.h,
      rc.x         , rc.y+rc.h
    );

    image()->incrementVersion();
  }

  void onUndo() override {
    swap();
  }

  void onRedo() override {
    swap();
  }

  void swap()
  {
    Image* image = this->image();
    ASSERT(image);

    swap_image_region_with_buffer(m_region, image, m_buffer);
    image->incrementVersion();

    //rehash();
  }

  size_t onMemSize() const override {
    return sizeof(*this) + m_buffer.size();
  }

private:
    const doc::ImageRef& m_src = nullptr;
    const doc::MaskRef& m_mask = nullptr;
    const gfx::Rect m_stampBounds;
    gfx::Region m_region;
    base::buffer m_buffer;
};


StampInCel::StampInCel(doc::Cel* dstCel,
                       const doc::ImageRef& image,
                       const doc::MaskRef& mask,
                       const gfx::Rect& stampBounds)
  : WithCel(dstCel)
  , m_image(image)
  , m_mask(mask)
  , m_stampBounds(stampBounds)
{
  ASSERT(image.get());
  ASSERT(mask.get());
  ASSERT(!stampBounds.isEmpty());
}

void StampInCel::onExecute()
{
  Cel* cel = this->cel();

  gfx::Rect newBounds;
  gfx::Region regionInTiles;
  doc::Grid grid;
  if (cel->image()->pixelFormat() == IMAGE_TILEMAP) {
    //newBounds = cel->bounds() | m_region.bounds();
    //auto tileset = static_cast<LayerTilemap*>(cel->layer())->tileset();
    //grid = tileset->grid();
    //grid.origin(m_pos);
    //regionInTiles = grid.canvasToTile(m_region);
  }
  else {
    newBounds = cel->bounds() | m_stampBounds;
  }

  if (cel->bounds() != newBounds)
    executeAndAdd(new CropCel(cel, newBounds));

  if (cel->image()->pixelFormat() == IMAGE_TILEMAP) {
/*
    executeAndAdd(
      new CopyRegion(cel->image(),
                     m_patch,
                     regionInTiles,
                     -grid.canvasToTile(cel->position())));
*/
  }
  else {
    executeAndAdd(
      new StampImage(cel->image(),
                     m_image,
                     m_mask,
                     m_stampBounds.offset(-cel->position())));

  }

  executeAndAdd(new TrimCel(cel));
}

} // namespace cmd
} // namespace app
