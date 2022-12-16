// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/replace_tileset.h"
#include "app/cmd/set_cel_bounds.h"
#include "app/cmd/set_slice_key.h"
#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/commands/params.h"
#include "app/doc_api.h"
#include "app/ini_file.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/sprite_job.h"
#include "app/util/resize_image.h"
#include "base/convert_to.h"
#include "doc/algorithm/resize_image.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "ui/ui.h"

#include "sprite_size.xml.h"

#include <algorithm>

#define PERC_FORMAT     "%.4g"

namespace app {

using namespace ui;
using doc::algorithm::ResizeMethod;

struct SpriteSizeParams : public NewParams {
  Param<bool> ui { this, true, { "ui", "use-ui" } };
  Param<int> width { this, 0, "width" };
  Param<int> height { this, 0, "height" };
  Param<bool> lockRatio { this, false, "lockRatio" };
  Param<double> scale { this, 1.0, "scale" };
  Param<double> scaleX { this, 1.0, "scaleX" };
  Param<double> scaleY { this, 1.0, "scaleY" };
  Param<ResizeMethod> method { this, ResizeMethod::RESIZE_METHOD_NEAREST_NEIGHBOR, { "method", "resize-method" } };
};

class SpriteSizeJob : public SpriteJob {
  int m_new_width;
  int m_new_height;
  ResizeMethod m_resize_method;

  template<typename T>
  T scale_x(T x) const { return x * T(m_new_width) / T(sprite()->width()); }

  template<typename T>
  T scale_y(T y) const { return y * T(m_new_height) / T(sprite()->height()); }

  template<typename T>
  gfx::RectT<T> scale_rect(const gfx::RectT<T>& rc) const {
    T x1 = scale_x(rc.x);
    T y1 = scale_y(rc.y);
    return gfx::RectT<T>(x1, y1,
                         scale_x(rc.x2()) - x1,
                         scale_y(rc.y2()) - y1);
  }

public:

  SpriteSizeJob(const ContextReader& reader, int new_width, int new_height, ResizeMethod resize_method)
    : SpriteJob(reader, Strings::sprite_size_title().c_str()) {
    m_new_width = new_width;
    m_new_height = new_height;
    m_resize_method = resize_method;
  }

protected:

  // [working thread]
  void onJob() override {
    DocApi api = writer().document()->getApi(tx());
    Tilesets* tilesets = sprite()->tilesets();

    int img_count = 0;
    if (tilesets) {
      for (Tileset* tileset : *tilesets) {
        img_count += tileset->size();
      }
    }
    for (Cel* cel : sprite()->uniqueCels()) { // TODO add size() member function to CelsRange
      (void)cel;
      ++img_count;
    }

    int progress = 0;
    const gfx::SizeF scale(
      double(m_new_width) / double(sprite()->width()),
      double(m_new_height) / double(sprite()->height()));

    // Resize tilesets
    if (tilesets) {
      for (tileset_index tsi=0; tsi<tilesets->size(); ++tsi) {
        Tileset* tileset = tilesets->get(tsi);
        ASSERT(tileset);

        gfx::Size newGridSize = tileset->grid().tileSize();
        newGridSize.w *= scale.w;
        newGridSize.h *= scale.h;
        if (newGridSize.w < 1) newGridSize.w = 1;
        if (newGridSize.h < 1) newGridSize.h = 1;
        doc::Grid newGrid(newGridSize);

        auto newTileset = new doc::Tileset(sprite(), newGrid, tileset->size());
        doc::tile_index idx = 0;
        newTileset->setName(tileset->name());
        newTileset->setUserData(tileset->userData());
        for (doc::ImageRef tileImg : *tileset) {
          if (idx != 0) {
            doc::ImageRef newTileImg(
              resize_image(
                tileImg.get(),
                scale,
                m_resize_method,
                sprite()->palette(0),
                sprite()->rgbMap(0)));   // TODO first frame?

            newTileset->set(idx, newTileImg);
            newTileset->setTileData(idx, tileset->getTileData(idx));
          }

          jobProgress((float)progress / img_count);
          ++progress;
          ++idx;
        }
        tx()(new cmd::ReplaceTileset(sprite(), tsi, newTileset));

        // Cancel all the operation?
        if (isCanceled())
          return;        // Tx destructor will undo all operations
      }
    }

    // For each cel...
    for (Cel* cel : sprite()->uniqueCels()) {
      // We need to adjust only the origin/position of tilemap cels
      // (because tiles are resized automatically when we resize the
      // tileset).
      if (cel->layer()->isTilemap()) {
        Tileset* tileset = static_cast<LayerTilemap*>(cel->layer())->tileset();
        gfx::Size canvasSize =
          tileset->grid().tilemapSizeToCanvas(
            gfx::Size(cel->image()->width(),
                      cel->image()->height()));
        gfx::Rect newBounds(cel->x()*scale.w,
                            cel->y()*scale.h,
                            canvasSize.w,
                            canvasSize.h);
        tx()(new cmd::SetCelBoundsF(cel, newBounds));
      }
      else {
        resize_cel_image(
          tx(), cel, scale,
          m_resize_method,
          cel->layer()->isReference() ?
          -cel->boundsF().origin():
          gfx::PointF(-cel->bounds().origin()));
      }

      jobProgress((float)progress / img_count);
      ++progress;

      // Cancel all the operation?
      if (isCanceled())
        return;        // Tx destructor will undo all operations
    }

    // Resize mask
    if (document()->isMaskVisible()) {
      ImageRef old_bitmap
        (crop_image(document()->mask()->bitmap(), -1, -1,
                    document()->mask()->bitmap()->width()+2,
                    document()->mask()->bitmap()->height()+2, 0));

      int w = scale_x(old_bitmap->width());
      int h = scale_y(old_bitmap->height());
      std::unique_ptr<Mask> new_mask(new Mask);
      new_mask->replace(
        gfx::Rect(
          scale_x(document()->mask()->bounds().x-1),
          scale_y(document()->mask()->bounds().y-1),
          std::max(1, w),
          std::max(1, h)));

      // Always use the nearest-neighbor method to resize the bitmap
      // mask.
      algorithm::resize_image(
        old_bitmap.get(), new_mask->bitmap(),
        doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR,
        sprite()->palette(0), // Ignored
        sprite()->rgbMap(0),  // Ignored
        -1);                  // Ignored

      // Reshrink
      new_mask->intersect(new_mask->bounds());

      // Copy new mask
      api.copyToCurrentMask(new_mask.get());
    }

    // Resize slices
    for (auto& slice : sprite()->slices()) {
      for (auto& k : *slice) {
        const SliceKey& key = *k.value();
        if (key.isEmpty())
          continue;

        SliceKey newKey = key;
        newKey.setBounds(scale_rect(newKey.bounds()));

        if (newKey.hasCenter())
          newKey.setCenter(scale_rect(newKey.center()));

        if (newKey.hasPivot())
          newKey.setPivot(gfx::Point(scale_x(newKey.pivot().x),
                                     scale_y(newKey.pivot().y)));

        tx()(new cmd::SetSliceKey(slice, k.frame(), newKey));
      }
    }

    // Resize Sprite
    api.setSpriteSize(sprite(), m_new_width, m_new_height);
  }

};

#ifdef ENABLE_UI

class SpriteSizeWindow : public app::gen::SpriteSize {
public:
  SpriteSizeWindow(Context* ctx, const SpriteSizeParams& params) : m_ctx(ctx) {
    lockRatio()->Click.connect([this]{ onLockRatioClick(); });
    widthPx()->Change.connect([this]{ onWidthPxChange(); });
    heightPx()->Change.connect([this]{ onHeightPxChange(); });
    widthPerc()->Change.connect([this]{ onWidthPercChange(); });
    heightPerc()->Change.connect([this]{ onHeightPercChange(); });

    widthPx()->setTextf("%d", params.width());
    heightPx()->setTextf("%d", params.height());
    widthPerc()->setTextf(PERC_FORMAT, params.scaleX() * 100.0);
    heightPerc()->setTextf(PERC_FORMAT, params.scaleY() * 100.0);

    static_assert(doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR == 0 &&
                  doc::algorithm::RESIZE_METHOD_BILINEAR == 1 &&
                  doc::algorithm::RESIZE_METHOD_ROTSPRITE == 2,
                  "ResizeMethod enum has changed");
    method()->addItem(Strings::sprite_size_method_nearest_neighbor());
    method()->addItem(Strings::sprite_size_method_bilinear());
    method()->addItem(Strings::sprite_size_method_rotsprite());
    int resize_method;
    if (params.method.isSet())
      resize_method = (int)params.method();
    else
      resize_method = get_config_int("SpriteSize", "Method",
                                     doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR);
    method()->setSelectedItemIndex(resize_method);
    const bool lock = (params.lockRatio.isSet())? params.lockRatio() : get_config_bool("SpriteSize", "LockRatio", false);
    lockRatio()->setSelected(lock);
  }

private:

  void onLockRatioClick() {
    const ContextReader reader(m_ctx);
    onWidthPxChange();
  }

  void onWidthPxChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    int width = widthPx()->textInt();
    double perc = 100.0 * width / sprite->width();

    widthPerc()->setTextf(PERC_FORMAT, perc);

    if (lockRatio()->isSelected()) {
      heightPerc()->setTextf(PERC_FORMAT, perc);
      heightPx()->setTextf("%d", sprite->height() * width / sprite->width());
    }
  }

  void onHeightPxChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    int height = heightPx()->textInt();
    double perc = 100.0 * height / sprite->height();

    heightPerc()->setTextf(PERC_FORMAT, perc);

    if (lockRatio()->isSelected()) {
      widthPerc()->setTextf(PERC_FORMAT, perc);
      widthPx()->setTextf("%d", sprite->width() * height / sprite->height());
    }
  }

  void onWidthPercChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    double width = widthPerc()->textDouble();

    widthPx()->setTextf("%d", (int)(sprite->width() * width / 100));

    if (lockRatio()->isSelected()) {
      heightPx()->setTextf("%d", (int)(sprite->height() * width / 100));
      heightPerc()->setText(widthPerc()->text());
    }
  }

  void onHeightPercChange() {
    const ContextReader reader(m_ctx);
    const Sprite* sprite(reader.sprite());
    double height = heightPerc()->textDouble();

    heightPx()->setTextf("%d", (int)(sprite->height() * height / 100));

    if (lockRatio()->isSelected()) {
      widthPx()->setTextf("%d", (int)(sprite->width() * height / 100));
      widthPerc()->setText(heightPerc()->text());
    }
  }

  Context* m_ctx;
};
#endif // ENABLE_UI

class SpriteSizeCommand : public CommandWithNewParams<SpriteSizeParams> {
public:
  SpriteSizeCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SpriteSizeCommand::SpriteSizeCommand()
  : CommandWithNewParams<SpriteSizeParams>(CommandId::SpriteSize(), CmdRecordableFlag)
{
}

bool SpriteSizeCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void SpriteSizeCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif
  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  auto& params = this->params();

  double ratio = sprite->width() / double(sprite->height());
  if (params.scale.isSet()) {
    params.width(int(sprite->width() * params.scale()));
    params.height(int(sprite->height() * params.scale()));
    params.scaleX(params.scale());
    params.scaleY(params.scale());
  }
  else if (params.lockRatio()) {
    if (params.width.isSet()) {
      params.height(int(params.width() / ratio));
      params.scaleX(params.width() / double(sprite->width()));
      params.scaleY(params.scaleX());
    }
    else if (params.height.isSet()) {
      params.width(int(params.height() * ratio));
      params.scaleY(params.height() / double(sprite->height()));
      params.scaleX(params.scaleY());
    }
    else if (params.scaleX.isSet()) {
      params.width(int(params.scaleX() * sprite->width()));
      params.height(int(params.scaleX() * sprite->height()));
      params.scaleY(params.scaleX());
    }
    else if (params.scaleY.isSet()) {
      params.width(int(params.scaleY() * sprite->width()));
      params.height(int(params.scaleY() * sprite->height()));
      params.scaleX(params.scaleY());
    }
    else {
      params.width(sprite->width());
      params.height(sprite->height());
    }
  }
  else {
    if (params.width.isSet()) {
      params.scaleX(params.width() / double(sprite->width()));
    }
    else if (params.scaleX.isSet()) {
      params.width(int(params.scaleX() * sprite->width()));
    }
    else {
      params.width(sprite->width());
    }
    if (params.height.isSet()) {
      params.scaleY(params.height() / double(sprite->height()));
    }
    else if (params.scaleY.isSet()) {
      params.height(int(params.scaleY() * sprite->height()));
    }
    else {
      params.height(sprite->height());
    }
  }
  int new_width = params.width();
  int new_height = params.height();
  ResizeMethod resize_method = params.method();

#ifdef ENABLE_UI
  if (ui) {
    SpriteSizeWindow window(context, params);
    window.remapWindow();
    window.centerWindow();

    load_window_pos(&window, "SpriteSize");
    window.setVisible(true);
    window.openWindowInForeground();
    save_window_pos(&window, "SpriteSize");

    if (window.closer() != window.ok())
      return;

    new_width = window.widthPx()->textInt();
    new_height = window.heightPx()->textInt();
    resize_method = (ResizeMethod)window.method()->getSelectedItemIndex();

    set_config_int("SpriteSize", "Method", resize_method);
    set_config_bool("SpriteSize", "LockRatio", window.lockRatio()->isSelected());
  }
#endif // ENABLE_UI

  new_width = std::clamp(new_width, 1, DOC_SPRITE_MAX_WIDTH);
  new_height = std::clamp(new_height, 1, DOC_SPRITE_MAX_HEIGHT);

  {
    SpriteSizeJob job(reader, new_width, new_height, resize_method);
    job.startJob();
    job.waitJob();
  }

#ifdef ENABLE_UI
  update_screen_for_document(reader.document());
#endif
}

Command* CommandFactory::createSpriteSizeCommand()
{
  return new SpriteSizeCommand;
}

} // namespace app
