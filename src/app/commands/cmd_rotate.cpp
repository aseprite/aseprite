// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_cel_bounds.h"
#include "app/commands/cmd_rotate.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/job.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/tools/tool_box.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/toolbar.h"
#include "app/util/range_utils.h"
#include "base/convert_to.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

class RotateJob : public Job {
  ContextWriter m_writer;
  Document* m_document;
  Sprite* m_sprite;
  int m_angle;
  CelList m_cels;
  bool m_rotateSprite;

public:

  RotateJob(const ContextReader& reader, int angle, const CelList& cels, bool rotateSprite)
    : Job("Rotate Canvas")
    , m_writer(reader)
    , m_document(m_writer.document())
    , m_sprite(m_writer.sprite())
    , m_cels(cels)
    , m_rotateSprite(rotateSprite)
  {
    m_angle = angle;
  }

protected:

  template<typename T>
  void rotate_rect(gfx::RectT<T>& newBounds) {
    const gfx::RectT<T> bounds = newBounds;
    switch (m_angle) {
      case 180:
        newBounds.x = m_sprite->width() - bounds.x - bounds.w;
        newBounds.y = m_sprite->height() - bounds.y - bounds.h;
        break;
      case 90:
        newBounds.x = m_sprite->height() - bounds.y - bounds.h;
        newBounds.y = bounds.x;
        newBounds.w = bounds.h;
        newBounds.h = bounds.w;
        break;
      case -90:
        newBounds.x = bounds.y;
        newBounds.y = m_sprite->width() - bounds.x - bounds.w;
        newBounds.w = bounds.h;
        newBounds.h = bounds.w;
        break;
    }
  }

  // [working thread]
  virtual void onJob()
  {
    Transaction transaction(m_writer.context(), "Rotate Canvas");
    DocumentApi api = m_document->getApi(transaction);

    // 1) Rotate cel positions
    for (Cel* cel : m_cels) {
      Image* image = cel->image();
      if (!image)
        continue;

      if (cel->layer()->isReference()) {
        gfx::RectF bounds = cel->boundsF();
        rotate_rect(bounds);
        if (cel->boundsF() != bounds)
          transaction.execute(new cmd::SetCelBoundsF(cel, bounds));
      }
      else {
        gfx::Rect bounds = cel->bounds();
        rotate_rect(bounds);
        if (bounds.origin() != cel->bounds().origin())
          api.setCelPosition(m_sprite, cel, bounds.x, bounds.y);
      }
    }

    // 2) Rotate images
    int i = 0;
    for (Cel* cel : m_cels) {
      Image* image = cel->image();
      if (image) {
        ImageRef new_image(Image::create(image->pixelFormat(),
            m_angle == 180 ? image->width(): image->height(),
            m_angle == 180 ? image->height(): image->width()));
        new_image->setMaskColor(image->maskColor());

        doc::rotate_image(image, new_image.get(), m_angle);
        api.replaceImage(m_sprite, cel->imageRef(), new_image);
      }

      jobProgress((float)i / m_cels.size());
      ++i;

      // cancel all the operation?
      if (isCanceled())
        return;        // Transaction destructor will undo all operations
    }

    // rotate mask
    if (m_document->isMaskVisible()) {
      Mask* origMask = m_document->mask();
      base::UniquePtr<Mask> new_mask(new Mask());
      const gfx::Rect& origBounds = origMask->bounds();
      int x = 0, y = 0;

      switch (m_angle) {
        case 180:
          x = m_sprite->width() - origBounds.x - origBounds.w;
          y = m_sprite->height() - origBounds.y - origBounds.h;
          break;
        case 90:
          x = m_sprite->height() - origBounds.y - origBounds.h;
          y = origBounds.x;
          break;
        case -90:
          x = origBounds.y;
          y = m_sprite->width() - origBounds.x - origBounds.w;
          break;
      }

      // create the new rotated mask
      new_mask->replace(
        gfx::Rect(x, y,
          m_angle == 180 ? origBounds.w: origBounds.h,
          m_angle == 180 ? origBounds.h: origBounds.w));
      doc::rotate_image(origMask->bitmap(), new_mask->bitmap(), m_angle);

      // Copy new mask
      api.copyToCurrentMask(new_mask);

      // Regenerate mask
      m_document->resetTransformation();
      m_document->generateMaskBoundaries();
    }

    // change the sprite's size
    if (m_rotateSprite && m_angle != 180)
      api.setSpriteSize(m_sprite, m_sprite->height(), m_sprite->width());

    // commit changes
    transaction.commit();
  }

};

RotateCommand::RotateCommand()
  : Command("Rotate",
            "Rotate Canvas",
            CmdRecordableFlag)
{
  m_flipMask = false;
  m_angle = 0;
}

void RotateCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  m_flipMask = (target == "mask");

  if (params.has_param("angle")) {
    m_angle = strtol(params.get("angle").c_str(), NULL, 10);
  }
}

bool RotateCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void RotateCommand::onExecute(Context* context)
{
  {
    Site site = context->activeSite();
    CelList cels;
    bool rotateSprite = false;

    // Flip the mask or current cel
    if (m_flipMask) {
      auto range = App::instance()->timeline()->range();
      if (range.enabled())
        cels = get_unique_cels(site.sprite(), range);
      else if (site.cel()) {
        // If we want to rotate the visible mask for the current cel,
        // we can go to MovingPixelsState.
        if (static_cast<app::Document*>(site.document())->isMaskVisible()) {
          // Select marquee tool
          if (tools::Tool* tool = App::instance()->toolBox()
              ->getToolById(tools::WellKnownTools::RectangularMarquee)) {
            ToolBar::instance()->selectTool(tool);
            current_editor->startSelectionTransformation(gfx::Point(0, 0), m_angle);
            return;
          }
        }

        cels.push_back(site.cel());
      }
    }
    // Flip the whole sprite
    else if (site.sprite()) {
      for (Cel* cel : site.sprite()->uniqueCels())
        cels.push_back(cel);

      rotateSprite = true;
    }

    if (cels.empty())           // Nothing to do
      return;

    ContextReader reader(context);
    {
      RotateJob job(reader, m_angle, cels, rotateSprite);
      job.startJob();
      job.waitJob();
    }
    reader.document()->generateMaskBoundaries();
    update_screen_for_document(reader.document());
  }
}

std::string RotateCommand::onGetFriendlyName() const
{
  std::string text = "Rotate";

  if (m_flipMask)
    text += " Selection";
  else
    text += " Sprite";

  text += " " + base::convert_to<std::string>(m_angle) + "\xc2\xb0";

  return text;
}

Command* CommandFactory::createRotateCommand()
{
  return new RotateCommand;
}

} // namespace app
