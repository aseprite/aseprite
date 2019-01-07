// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_bounds.h"
#include "app/cmd/set_slice_key.h"
#include "app/commands/cmd_sprite_size.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/doc_api.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/sprite_job.h"
#include "base/bind.h"
#include "doc/algorithm/resize_image.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "sprite_size.xml.h"

#define PERC_FORMAT     "%.4g"

namespace app {

using namespace ui;
using doc::algorithm::ResizeMethod;

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
    : SpriteJob(reader, "Sprite Size") {
    m_new_width = new_width;
    m_new_height = new_height;
    m_resize_method = resize_method;
  }

protected:

  // [working thread]
  void onJob() override {
    DocApi api = writer().document()->getApi(tx());

    int cels_count = 0;
    for (Cel* cel : sprite()->uniqueCels()) { // TODO add size() member function to CelsRange
      (void)cel;
      ++cels_count;
    }

    // For each cel...
    int progress = 0;
    for (Cel* cel : sprite()->uniqueCels()) {
      // Get cel's image
      Image* image = cel->image();
      if (image && !cel->link()) {
        // Resize the cel bounds only if it's from a reference layer
        if (cel->layer()->isReference()) {
          gfx::RectF newBounds = scale_rect<double>(cel->boundsF());
          tx()(new cmd::SetCelBoundsF(cel, newBounds));
        }
        else {
          // Change its location
          api.setCelPosition(sprite(), cel, scale_x(cel->x()), scale_y(cel->y()));

          // Resize the image
          int w = scale_x(image->width());
          int h = scale_y(image->height());
          ImageRef new_image(Image::create(image->pixelFormat(), MAX(1, w), MAX(1, h)));
          new_image->setMaskColor(image->maskColor());

          doc::algorithm::fixup_image_transparent_colors(image);
          doc::algorithm::resize_image(
            image, new_image.get(),
            m_resize_method,
            sprite()->palette(cel->frame()),
            sprite()->rgbMap(cel->frame()),
            (cel->layer()->isBackground() ? -1: sprite()->transparentColor()));

          api.replaceImage(sprite(), cel->imageRef(), new_image);
        }
      }

      jobProgress((float)progress / cels_count);
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
          scale_y(document()->mask()->bounds().y-1), MAX(1, w), MAX(1, h)));
      algorithm::resize_image(
        old_bitmap.get(), new_mask->bitmap(),
        m_resize_method,
        sprite()->palette(0), // Ignored
        sprite()->rgbMap(0),  // Ignored
        -1);                  // Ignored

      // Reshrink
      new_mask->intersect(new_mask->bounds());

      // Copy new mask
      api.copyToCurrentMask(new_mask.get());

      // Regenerate mask
      document()->resetTransformation();
      document()->generateMaskBoundaries();
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

class SpriteSizeWindow : public app::gen::SpriteSize {
public:
  SpriteSizeWindow(Context* ctx, int new_width, int new_height) : m_ctx(ctx) {
    lockRatio()->Click.connect(base::Bind<void>(&SpriteSizeWindow::onLockRatioClick, this));
    widthPx()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onWidthPxChange, this));
    heightPx()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onHeightPxChange, this));
    widthPerc()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onWidthPercChange, this));
    heightPerc()->Change.connect(base::Bind<void>(&SpriteSizeWindow::onHeightPercChange, this));

    widthPx()->setTextf("%d", new_width);
    heightPx()->setTextf("%d", new_height);

    static_assert(doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR == 0 &&
                  doc::algorithm::RESIZE_METHOD_BILINEAR == 1 &&
                  doc::algorithm::RESIZE_METHOD_ROTSPRITE == 2,
                  "ResizeMethod enum has changed");
    method()->addItem("Nearest-neighbor");
    method()->addItem("Bilinear");
    method()->addItem("RotSprite");
    method()->setSelectedItemIndex(
      get_config_int("SpriteSize", "Method",
                     doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR));
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

SpriteSizeCommand::SpriteSizeCommand()
  : Command(CommandId::SpriteSize(), CmdRecordableFlag)
{
  m_useUI = true;
  m_width = 0;
  m_height = 0;
  m_scaleX = 1.0;
  m_scaleY = 1.0;
  m_resizeMethod = doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
}

void SpriteSizeCommand::onLoadParams(const Params& params)
{
  std::string useUI = params.get("use-ui");
  m_useUI = (useUI.empty() || (useUI == "true"));

  std::string width = params.get("width");
  if (!width.empty()) {
    m_width = std::strtol(width.c_str(), NULL, 10);
  }
  else
    m_width = 0;

  std::string height = params.get("height");
  if (!height.empty()) {
    m_height = std::strtol(height.c_str(), NULL, 10);
  }
  else
    m_height = 0;

  std::string resize_method = params.get("resize-method");
  if (!resize_method.empty()) {
    if (resize_method == "bilinear")
      m_resizeMethod = doc::algorithm::RESIZE_METHOD_BILINEAR;
    else if (resize_method == "rotsprite")
      m_resizeMethod = doc::algorithm::RESIZE_METHOD_ROTSPRITE;
    else
      m_resizeMethod = doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
  }
  else
    m_resizeMethod = doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR;
}

bool SpriteSizeCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void SpriteSizeCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  int new_width = (m_width ? m_width: int(sprite->width()*m_scaleX));
  int new_height = (m_height ? m_height: int(sprite->height()*m_scaleY));
  ResizeMethod resize_method = m_resizeMethod;

#ifdef ENABLE_UI
  if (m_useUI && context->isUIAvailable()) {
    SpriteSizeWindow window(context, new_width, new_height);
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
  }
#endif // ENABLE_UI

  new_width = MID(1, new_width, DOC_SPRITE_MAX_WIDTH);
  new_height = MID(1, new_height, DOC_SPRITE_MAX_HEIGHT);

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
