// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/pixels_movement.h"

#include "app/app.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/set_mask.h"
#include "app/cmd/trim_cel.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/snap_to_grid.h"
#include "app/ui/editor/pivot_helpers.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "app/util/new_image_from_mask.h"
#include "app/util/range_utils.h"
#include "base/bind.h"
#include "base/pi.h"
#include "base/vector2d.h"
#include "doc/algorithm/flip_image.h"
#include "doc/algorithm/rotate.h"
#include "doc/algorithm/rotsprite.h"
#include "doc/algorithm/shift_image.h"
#include "doc/blend_internals.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "gfx/region.h"
#include "render/render.h"

#include <algorithm>

#if _DEBUG
#define DUMP_INNER_CMDS() dumpInnerCmds()
#else
#define DUMP_INNER_CMDS()
#endif

namespace app {

template<typename T>
static inline const base::Vector2d<double> point2Vector(const gfx::PointT<T>& pt) {
  return base::Vector2d<double>(pt.x, pt.y);
}

PixelsMovement::InnerCmd::InnerCmd(InnerCmd&& c)
  : type(None)
{
  std::swap(type, c.type);
  std::swap(data, c.data);
}

PixelsMovement::InnerCmd::~InnerCmd()
{
  if (type == InnerCmd::Stamp)
    delete data.stamp.transformation;
}

// static
PixelsMovement::InnerCmd
PixelsMovement::InnerCmd::MakeClear()
{
  InnerCmd c;
  c.type = InnerCmd::Clear;
  return c;
}

// static
PixelsMovement::InnerCmd
PixelsMovement::InnerCmd::MakeFlip(const doc::algorithm::FlipType flipType)
{
  InnerCmd c;
  c.type = InnerCmd::Flip;
  c.data.flip.type = flipType;
  return c;
}

// static
PixelsMovement::InnerCmd
PixelsMovement::InnerCmd::MakeShift(const int dx, const int dy, const double angle)
{
  InnerCmd c;
  c.type = InnerCmd::Shift;
  c.data.shift.dx = dx;
  c.data.shift.dy = dy;
  c.data.shift.angle = angle;
  return c;
}

// static
PixelsMovement::InnerCmd
PixelsMovement::InnerCmd::MakeStamp(const Transformation& t)
{
  InnerCmd c;
  c.type = InnerCmd::Stamp;
  c.data.stamp.transformation = new Transformation(t);
  return c;
}

PixelsMovement::PixelsMovement(
  Context* context,
  Site site,
  const Image* moveThis,
  const Mask* mask,
  const char* operationName)
  : m_reader(context)
  , m_site(site)
  , m_document(site.document())
  , m_tx(context, operationName)
  , m_isDragging(false)
  , m_adjustPivot(false)
  , m_handle(NoHandle)
  , m_originalImage(Image::createCopy(moveThis))
  , m_opaque(false)
  , m_maskColor(m_site.sprite()->transparentColor())
  , m_canHandleFrameChange(false)
  , m_fastMode(false)
  , m_needsRotSpriteRedraw(false)
{
  Transformation transform(mask->bounds());
  set_pivot_from_preferences(transform);

  m_initialData = transform;
  m_currentData = transform;

  m_initialMask.reset(new Mask(*mask));
  m_initialMask0.reset(new Mask(*mask));
  m_currentMask.reset(new Mask(*mask));

  m_pivotVisConn =
    Preferences::instance().selection.pivotVisibility.AfterChange.connect(
      base::Bind<void>(&PixelsMovement::onPivotChange, this));
  m_pivotPosConn =
    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      base::Bind<void>(&PixelsMovement::onPivotChange, this));
  m_rotAlgoConn =
    Preferences::instance().selection.rotationAlgorithm.AfterChange.connect(
      base::Bind<void>(&PixelsMovement::onRotationAlgorithmChange, this));

  // The extra cel must be null, because if it's not null, it means
  // that someone else is using it (e.g. the editor brush preview),
  // and its owner could destroy our new "extra cel".
  ASSERT(!m_document->extraCel());
  redrawExtraImage();
  redrawCurrentMask();

  // If the mask is different than the mask from the document
  // (e.g. it's from Paste command), we've to replace the document
  // mask and generate its boundaries.
  if (mask != m_document->mask()) {
    // Update document mask
    m_tx(new cmd::SetMask(m_document, m_currentMask.get()));
    m_document->generateMaskBoundaries(m_currentMask.get());
    update_screen_for_document(m_document);
  }
}

bool PixelsMovement::editMultipleCels() const
{
  return
    (m_site.range().enabled() &&
     (Preferences::instance().selection.multicelWhenLayersOrFrames() ||
      m_site.range().type() == DocRange::kCels));
}

void PixelsMovement::setFastMode(const bool fastMode)
{
  bool redraw = (m_fastMode && !fastMode);
  m_fastMode = fastMode;
  if (m_needsRotSpriteRedraw && redraw) {
    redrawExtraImage();
    update_screen_for_document(m_document);
    m_needsRotSpriteRedraw = false;
  }
}

void PixelsMovement::flipImage(doc::algorithm::FlipType flipType)
{
  m_innerCmds.push_back(InnerCmd::MakeFlip(flipType));

  flipOriginalImage(flipType);

  {
    ContextWriter writer(m_reader, 1000);

    // Regenerate the transformed (rotated, scaled, etc.) image and
    // mask.
    redrawExtraImage();
    redrawCurrentMask();
    updateDocumentMask();

    update_screen_for_document(m_document);
  }
}

void PixelsMovement::rotate(double angle)
{
  ContextWriter writer(m_reader, 1000);
  m_currentData.angle(
    base::fmod_radians(
      m_currentData.angle() + PI * -angle / 180.0));

  m_document->setTransformation(m_currentData);

  redrawExtraImage();
  redrawCurrentMask();
  updateDocumentMask();

  update_screen_for_document(m_document);
}

void PixelsMovement::shift(int dx, int dy)
{
  const double angle = m_currentData.angle();
  m_innerCmds.push_back(InnerCmd::MakeShift(dx, dy, angle));
  shiftOriginalImage(dx, dy, angle);

  {
    ContextWriter writer(m_reader, 1000);

    redrawExtraImage();
    redrawCurrentMask();
    updateDocumentMask();

    update_screen_for_document(m_document);
  }
}

void PixelsMovement::trim()
{
  ContextWriter writer(m_reader, 1000);
  Cel* activeCel = m_site.cel();
  bool restoreMask = false;

  // TODO this is similar to clear_mask_from_cels()

  for (Cel* cel : getEditableCels()) {
    if (cel != activeCel) {
      if (!restoreMask) {
        m_document->setMask(m_initialMask0.get());
        restoreMask = true;
      }
      m_tx(new cmd::ClearMask(cel));
    }
    // Current cel (m_site.cel()) can be nullptr when we paste in an
    // empty cel (Ctrl+V) and cut (Ctrl+X) the floating pixels.
    if (cel &&
        cel->layer()->isTransparent()) {
      m_tx(new cmd::TrimCel(cel));
    }
  }

  if (restoreMask)
    updateDocumentMask();
}

void PixelsMovement::cutMask()
{
  m_innerCmds.push_back(InnerCmd::MakeClear());

  {
    ContextWriter writer(m_reader, 1000);
    if (writer.cel()) {
      m_tx(new cmd::ClearMask(writer.cel()));

      // Do not trim here so we don't lost the information about all
      // linked cels related to "writer.cel()"
    }
  }

  copyMask();
}

void PixelsMovement::copyMask()
{
  hideDocumentMask();
}

void PixelsMovement::catchImage(const gfx::Point& pos, HandleType handle)
{
  ASSERT(handle != NoHandle);

  m_catchPos = pos;
  m_isDragging = true;
  m_handle = handle;
}

void PixelsMovement::catchImageAgain(const gfx::Point& pos, HandleType handle)
{
  // Create a new Transaction to move the pixels to other position
  m_initialData = m_currentData;
  m_isDragging = true;
  m_catchPos = pos;
  m_handle = handle;

  hideDocumentMask();
}

void PixelsMovement::moveImage(const gfx::Point& pos, MoveModifier moveModifier)
{
  Transformation::Corners oldCorners;
  m_currentData.transformBox(oldCorners);

  ContextWriter writer(m_reader, 1000);
  gfx::RectF bounds = m_initialData.bounds();
  bool updateBounds = false;
  double dx, dy;

  dx = ((pos.x - m_catchPos.x) *  cos(m_currentData.angle()) +
        (pos.y - m_catchPos.y) * -sin(m_currentData.angle()));
  dy = ((pos.x - m_catchPos.x) *  sin(m_currentData.angle()) +
        (pos.y - m_catchPos.y) *  cos(m_currentData.angle()));

  switch (m_handle) {

    case MovePixelsHandle:
      if ((moveModifier & LockAxisMovement) == LockAxisMovement) {
        if (ABS(dx) < ABS(dy))
          dx = 0.0;
        else
          dy = 0.0;
      }

      bounds.offset(dx, dy);
      updateBounds = true;

      if ((moveModifier & SnapToGridMovement) == SnapToGridMovement) {
        // Snap the x1,y1 point to the grid.
        gfx::Rect gridBounds = m_document->sprite()->gridBounds();
        gfx::PointF gridOffset(
          snap_to_grid(
            gridBounds,
            gfx::Point(bounds.origin()),
            PreferSnapTo::ClosestGridVertex));

        // Now we calculate the difference from x1,y1 point and we can
        // use it to adjust all coordinates (x1, y1, x2, y2).
        gridOffset -= bounds.origin();
        bounds.offset(gridOffset);
      }
      break;

    case ScaleNWHandle:
    case ScaleNHandle:
    case ScaleNEHandle:
    case ScaleWHandle:
    case ScaleEHandle:
    case ScaleSWHandle:
    case ScaleSHandle:
    case ScaleSEHandle: {
      static double handles[][2] = {
        { 0.0, 0.0 }, { 0.5, 0.0 }, { 1.0, 0.0 },
        { 0.0, 0.5 },               { 1.0, 0.5 },
        { 0.0, 1.0 }, { 0.5, 1.0 }, { 1.0, 1.0 }
      };
      gfx::PointF pivot;
      gfx::PointF handle(
        handles[m_handle-ScaleNWHandle][0],
        handles[m_handle-ScaleNWHandle][1]);

      if ((moveModifier & ScaleFromPivot) == ScaleFromPivot) {
        pivot.x = m_currentData.pivot().x;
        pivot.y = m_currentData.pivot().y;
      }
      else {
        pivot.x = 1.0 - handle.x;
        pivot.y = 1.0 - handle.y;
        pivot.x = bounds.x + bounds.w*pivot.x;
        pivot.y = bounds.y + bounds.h*pivot.y;
      }

      gfx::PointF a = bounds.origin();
      gfx::PointF b = bounds.point2();

      if ((moveModifier & MaintainAspectRatioMovement) == MaintainAspectRatioMovement) {
        auto u = point2Vector(gfx::PointF(m_catchPos) - pivot);
        auto v = point2Vector(gfx::PointF(pos) - pivot);
        auto w = v.projectOn(u);
        double scale = u.magnitude();
        if (scale != 0.0)
          scale = (std::fabs(w.angle()-u.angle()) < PI/2.0 ? 1.0: -1.0) * w.magnitude() / scale;
        else
          scale = 1.0;

        a.x = int((a.x-pivot.x)*scale + pivot.x);
        a.y = int((a.y-pivot.y)*scale + pivot.y);
        b.x = int((b.x-pivot.x)*scale + pivot.x);
        b.y = int((b.y-pivot.y)*scale + pivot.y);
      }
      else {
        handle.x = bounds.x + bounds.w*handle.x;
        handle.y = bounds.y + bounds.h*handle.y;

        double w = (handle.x-pivot.x);
        double h = (handle.y-pivot.y);

        if (m_handle == ScaleNHandle || m_handle == ScaleSHandle) {
          dx = 0.0;
          w = 1.0;              // Any value != 0.0 to avoid div by zero
        }
        else if (m_handle == ScaleWHandle || m_handle == ScaleEHandle) {
          dy = 0.0;
          h = 1.0;
        }

        a.x = int((a.x-pivot.x)*(1.0+dx/w) + pivot.x);
        a.y = int((a.y-pivot.y)*(1.0+dy/h) + pivot.y);
        b.x = int((b.x-pivot.x)*(1.0+dx/w) + pivot.x);
        b.y = int((b.y-pivot.y)*(1.0+dy/h) + pivot.y);
      }

      // Do not use "gfx::Rect(a, b)" here because if a > b we want to
      // keep a rectangle with negative width or height (to know that
      // it was flipped).
      bounds.x = a.x;
      bounds.y = a.y;
      bounds.w = b.x - a.x;
      bounds.h = b.y - a.y;

      updateBounds = true;
      break;
    }

    case RotateNWHandle:
    case RotateNHandle:
    case RotateNEHandle:
    case RotateWHandle:
    case RotateEHandle:
    case RotateSWHandle:
    case RotateSHandle:
    case RotateSEHandle:
      {
        gfx::PointF abs_initial_pivot = m_initialData.pivot();
        gfx::PointF abs_pivot = m_currentData.pivot();

        double newAngle =
          m_initialData.angle()
          + atan2((double)(-pos.y + abs_pivot.y),
                  (double)(+pos.x - abs_pivot.x))
          - atan2((double)(-m_catchPos.y + abs_initial_pivot.y),
                  (double)(+m_catchPos.x - abs_initial_pivot.x));

        newAngle = base::fmod_radians(newAngle);

        // Is the "angle snap" is activated, we've to snap the angle
        // to common (pixel art) angles.
        if ((moveModifier & AngleSnapMovement) == AngleSnapMovement) {
          // TODO make this configurable
          static const double keyAngles[] = {
            0.0, 26.565, 45.0, 63.435, 90.0, 116.565, 135.0, 153.435, 180.0,
            180.0, -153.435, -135.0, -116, -90.0, -63.435, -45.0, -26.565
          };

          double newAngleDegrees = 180.0 * newAngle / PI;

          int closest = 0;
          int last = sizeof(keyAngles) / sizeof(keyAngles[0]) - 1;
          for (int i=0; i<=last; ++i) {
            if (std::fabs(newAngleDegrees-keyAngles[closest]) >
                std::fabs(newAngleDegrees-keyAngles[i]))
              closest = i;
          }

          newAngle = PI * keyAngles[closest] / 180.0;
        }

        m_currentData.angle(newAngle);
      }
      break;

    case PivotHandle:
      {
        // Calculate the new position of the pivot
        gfx::PointF newPivot(m_initialData.pivot().x + (pos.x - m_catchPos.x),
                             m_initialData.pivot().y + (pos.y - m_catchPos.y));

        m_currentData = m_initialData;
        m_currentData.displacePivotTo(newPivot);
      }
      break;
  }

  if (updateBounds) {
    m_currentData.bounds(bounds);
    m_adjustPivot = true;
  }

  redrawExtraImage();

  m_document->setTransformation(m_currentData);

  // Get the new transformed corners
  Transformation::Corners newCorners;
  m_currentData.transformBox(newCorners);

  // Create a union of all corners, and that will be the bounds to
  // redraw of the sprite.
  gfx::Rect fullBounds;
  for (int i=0; i<Transformation::Corners::NUM_OF_CORNERS; ++i) {
    fullBounds = fullBounds.createUnion(gfx::Rect((int)oldCorners[i].x, (int)oldCorners[i].y, 1, 1));
    fullBounds = fullBounds.createUnion(gfx::Rect((int)newCorners[i].x, (int)newCorners[i].y, 1, 1));
  }

  // If "fullBounds" is empty is because the cel was not moved
  if (!fullBounds.isEmpty()) {
    // Notify the modified region.
    m_document->notifySpritePixelsModified(
      m_site.sprite(),
      gfx::Region(fullBounds),
      m_site.frame());
  }
}

void PixelsMovement::getDraggedImageCopy(std::unique_ptr<Image>& outputImage,
                                         std::unique_ptr<Mask>& outputMask)
{
  gfx::Rect bounds = m_currentData.transformedBounds();
  std::unique_ptr<Image> image(
    Image::create(
      m_site.sprite()->pixelFormat(), bounds.w, bounds.h));

  drawImage(m_currentData, image.get(), bounds.origin(), false);

  // Draw mask without shrinking it, so the mask size is equal to the
  // "image" render.
  std::unique_ptr<Mask> mask(new Mask);
  drawMask(mask.get(), false);

  // Now we can shrink and crop the image.
  gfx::Rect oldMaskBounds = mask->bounds();
  mask->shrink();
  gfx::Rect newMaskBounds = mask->bounds();
  if (newMaskBounds != oldMaskBounds) {
    newMaskBounds.x -= oldMaskBounds.x;
    newMaskBounds.y -= oldMaskBounds.y;
    image.reset(crop_image(image.get(),
                           newMaskBounds.x,
                           newMaskBounds.y,
                           newMaskBounds.w,
                           newMaskBounds.h, 0));
  }

  outputImage.reset(image.release());
  outputMask.reset(mask.release());
}

void PixelsMovement::stampImage()
{
  stampImage(false);
  m_innerCmds.push_back(InnerCmd::MakeStamp(m_currentData));
}

// finalStamp: true if we have to stamp the current transformation
// (m_currentData) in all cels of the active range, or false if we
// have to stamp the image only in the current cel.
void PixelsMovement::stampImage(bool finalStamp)
{
  ContextWriter writer(m_reader, 1000);
  Cel* currentCel = m_site.cel();

  CelList cels;
  if (finalStamp) {
    cels = getEditableCels();
  }
  // Current cel (m_site.cel()) can be nullptr when we paste in an
  // empty cel (Ctrl+V) and cut (Ctrl+X) the floating pixels.
  else {
    cels.push_back(currentCel);
  }

  for (Cel* target : cels) {
    // We'll re-create the transformation for the other cels
    if (target != currentCel) {
      ASSERT(target);
      m_site.layer(target->layer());
      m_site.frame(target->frame());
      ASSERT(m_site.cel() == target);

      reproduceAllTransformationsWithInnerCmds();
    }

    redrawExtraImage();
    stampExtraCelImage();
  }

  currentCel = m_site.cel();
  if (currentCel &&
      (m_site.layer() != currentCel->layer() ||
       m_site.frame() != currentCel->frame())) {
    m_site.layer(currentCel->layer());
    m_site.frame(currentCel->frame());
    redrawExtraImage();
  }
}

void PixelsMovement::stampExtraCelImage()
{
  const Cel* cel = m_extraCel->cel();
  const Image* image = m_extraCel->image();

  // Expand the canvas to paste the image in the fully visible
  // portion of sprite.
  ExpandCelCanvas expand(
    m_site, m_site.layer(),
    TiledMode::NONE, m_tx,
    ExpandCelCanvas::None);

  // We cannot use cel->bounds() because cel->image() is nullptr
  gfx::Rect modifiedRect(
    cel->x(),
    cel->y(),
    image->width(),
    image->height());

  gfx::Region modifiedRegion(modifiedRect);
  expand.validateDestCanvas(modifiedRegion);

  expand.getDestCanvas()->copy(
    image, gfx::Clip(
      cel->x()-expand.getCel()->x(),
      cel->y()-expand.getCel()->y(),
      image->bounds()));

  expand.commit();
}

void PixelsMovement::dropImageTemporarily()
{
  m_isDragging = false;

  {
    ContextWriter writer(m_reader, 1000);

    // TODO Add undo information so the user can undo each transformation step.

    // Displace the pivot to the new site:
    if (m_adjustPivot) {
      m_adjustPivot = false;

      // Get the a factor for the X/Y position of the initial pivot
      // position inside the initial non-rotated bounds.
      gfx::PointF pivotPosFactor(m_initialData.pivot() - m_initialData.bounds().origin());
      pivotPosFactor.x /= m_initialData.bounds().w;
      pivotPosFactor.y /= m_initialData.bounds().h;

      // Get the current transformed bounds.
      Transformation::Corners corners;
      m_currentData.transformBox(corners);

      // The new pivot will be located from the rotated left-top
      // corner a distance equal to the transformed bounds's
      // width/height multiplied with the previously calculated X/Y
      // factor.
      base::Vector2d<double> newPivot(corners.leftTop().x,
                                      corners.leftTop().y);
      newPivot += pivotPosFactor.x * point2Vector(corners.rightTop() - corners.leftTop());
      newPivot += pivotPosFactor.y * point2Vector(corners.leftBottom() - corners.leftTop());

      m_currentData.displacePivotTo(gfx::PointF(newPivot.x, newPivot.y));
    }

    redrawCurrentMask();
    updateDocumentMask();

    update_screen_for_document(m_document);
  }
}

void PixelsMovement::dropImage()
{
  m_isDragging = false;

  // Stamp the image in the current layer.
  stampImage(true);

  // Put the new mask
  m_document->setMask(m_initialMask0.get());
  m_tx(new cmd::SetMask(m_document, m_currentMask.get()));

  // This is the end of the whole undo transaction.
  m_tx.commit();

  // Destroy the extra cel (this cel will be used by the drawing
  // cursor surely).
  ContextWriter writer(m_reader, 1000);
  m_document->setExtraCel(ExtraCelRef(nullptr));
}

void PixelsMovement::discardImage(const CommitChangesOption commit,
                                  const KeepMaskOption keepMask)
{
  m_isDragging = false;

  // Deselect the mask (here we don't stamp the image)
  m_document->setMask(m_initialMask0.get());
  if (keepMask == DontKeepMask) {
    m_tx(new cmd::DeselectMask(m_document));
  }
  else {
    m_tx(new cmd::SetMask(m_document, m_currentMask.get()));
  }

  if (commit == CommitChanges)
    m_tx.commit();

  // Destroy the extra cel and regenerate the mask boundaries (we've
  // just deselect the mask).
  ContextWriter writer(m_reader, 1000);
  m_document->setExtraCel(ExtraCelRef(nullptr));
  m_document->generateMaskBoundaries();
}

bool PixelsMovement::isDragging() const
{
  return m_isDragging;
}

gfx::Rect PixelsMovement::getImageBounds()
{
  const Cel* cel = m_extraCel->cel();
  const Image* image = m_extraCel->image();

  ASSERT(cel != NULL);
  ASSERT(image != NULL);

  return gfx::Rect(cel->x(), cel->y(), image->width(), image->height());
}

gfx::Size PixelsMovement::getInitialImageSize() const
{
  return gfx::Size(m_originalImage->width(),
                   m_originalImage->height());
}

void PixelsMovement::setMaskColor(bool opaque, color_t mask_color)
{
  ContextWriter writer(m_reader, 1000);
  m_opaque = opaque;
  m_maskColor = mask_color;
  redrawExtraImage();

  update_screen_for_document(m_document);
}

void PixelsMovement::redrawExtraImage(Transformation* transformation)
{
  if (!transformation)
    transformation = &m_currentData;

  int t, opacity = (m_site.layer()->isImage() ?
                    static_cast<LayerImage*>(m_site.layer())->opacity(): 255);
  Cel* cel = m_site.cel();
  if (cel) opacity = MUL_UN8(opacity, cel->opacity(), t);

  gfx::Rect bounds = transformation->transformedBounds();

  if (!m_extraCel)
    m_extraCel.reset(new ExtraCel);

  m_extraCel->create(m_document->sprite(), bounds, m_site.frame(), opacity);
  m_extraCel->setType(render::ExtraType::PATCH);
  m_extraCel->setBlendMode(m_site.layer()->isImage() ?
                           static_cast<LayerImage*>(m_site.layer())->blendMode():
                           BlendMode::NORMAL);
  m_document->setExtraCel(m_extraCel);

  // Draw the transformed pixels in the extra-cel which is the chunk
  // of pixels that the user is moving.
  drawImage(*transformation, m_extraCel->image(), bounds.origin(), true);
}

void PixelsMovement::redrawCurrentMask()
{
  drawMask(m_currentMask.get(), true);
}

void PixelsMovement::drawImage(
  const Transformation& transformation,
  doc::Image* dst, const gfx::Point& pt,
  const bool renderOriginalLayer)
{
  ASSERT(dst);

  Transformation::Corners corners;
  transformation.transformBox(corners);
  gfx::Rect bounds = corners.bounds();

  dst->setMaskColor(m_site.sprite()->transparentColor());
  dst->clear(dst->maskColor());

  if (renderOriginalLayer) {
    render::Render render;
    render.renderLayer(
      dst, m_site.layer(), m_site.frame(),
      gfx::Clip(bounds.x-pt.x, bounds.y-pt.y, bounds),
      BlendMode::SRC);
  }

  color_t maskColor = m_maskColor;

  // In case that Opaque option is enabled, or if we are drawing the
  // image for the clipboard (renderOriginalLayer is false), we use a
  // dummy mask color to call drawParallelogram(). In this way all
  // pixels will be opaqued (all colors are copied)
  if (m_opaque ||
      !renderOriginalLayer) {
    if (m_originalImage->pixelFormat() == IMAGE_INDEXED)
      maskColor = -1;
    else
      maskColor = 0;
  }
  m_originalImage->setMaskColor(maskColor);

  drawParallelogram(
    transformation,
    dst, m_originalImage.get(),
    m_initialMask.get(), corners, pt);
}

void PixelsMovement::drawMask(doc::Mask* mask, bool shrink)
{
  Transformation::Corners corners;
  m_currentData.transformBox(corners);
  gfx::Rect bounds = corners.bounds();

  mask->replace(bounds);
  if (shrink)
    mask->freeze();
  clear_image(mask->bitmap(), 0);
  drawParallelogram(m_currentData,
                    mask->bitmap(),
                    m_initialMask->bitmap(),
                    nullptr,
                    corners, bounds.origin());
  if (shrink)
    mask->unfreeze();
}

void PixelsMovement::drawParallelogram(
  const Transformation& transformation,
  doc::Image* dst, const doc::Image* src, const doc::Mask* mask,
  const Transformation::Corners& corners,
  const gfx::Point& leftTop)
{
  tools::RotationAlgorithm rotAlgo = Preferences::instance().selection.rotationAlgorithm();

  // When the scale isn't modified and we have no rotation or a
  // right/straight-angle, we should use the fast rotation algorithm,
  // as it's pixel-perfect match with the original selection when just
  // a translation is applied.
  double angle = 180.0*transformation.angle()/PI;
  if (!Preferences::instance().selection.forceRotsprite() &&
      (std::fabs(std::fmod(std::fabs(angle), 90.0)) < 0.01 ||
       std::fabs(std::fmod(std::fabs(angle), 90.0)-90.0) < 0.01)) {
    rotAlgo = tools::RotationAlgorithm::FAST;
  }

  // Don't use RotSprite if we are in "fast mode"
  if (rotAlgo == tools::RotationAlgorithm::ROTSPRITE && m_fastMode) {
    m_needsRotSpriteRedraw = true;
    rotAlgo = tools::RotationAlgorithm::FAST;
  }

retry:;      // In case that we don't have enough memory for RotSprite
             // we can try with the fast algorithm anyway.

  switch (rotAlgo) {

    case tools::RotationAlgorithm::FAST:
      doc::algorithm::parallelogram(
        dst, src, (mask ? mask->bitmap(): nullptr),
        int(corners.leftTop().x-leftTop.x),
        int(corners.leftTop().y-leftTop.y),
        int(corners.rightTop().x-leftTop.x),
        int(corners.rightTop().y-leftTop.y),
        int(corners.rightBottom().x-leftTop.x),
        int(corners.rightBottom().y-leftTop.y),
        int(corners.leftBottom().x-leftTop.x),
        int(corners.leftBottom().y-leftTop.y));
      break;

    case tools::RotationAlgorithm::ROTSPRITE:
      try {
        doc::algorithm::rotsprite_image(
          dst, src, (mask ? mask->bitmap(): nullptr),
          int(corners.leftTop().x-leftTop.x),
          int(corners.leftTop().y-leftTop.y),
          int(corners.rightTop().x-leftTop.x),
          int(corners.rightTop().y-leftTop.y),
          int(corners.rightBottom().x-leftTop.x),
          int(corners.rightBottom().y-leftTop.y),
          int(corners.leftBottom().x-leftTop.x),
          int(corners.leftBottom().y-leftTop.y));
      }
      catch (const std::bad_alloc&) {
        StatusBar::instance()->showTip(1000,
          "Not enough memory for RotSprite");

        rotAlgo = tools::RotationAlgorithm::FAST;
        goto retry;
      }
      break;

  }
}

void PixelsMovement::onPivotChange()
{
  set_pivot_from_preferences(m_currentData);
  onRotationAlgorithmChange();
}

void PixelsMovement::onRotationAlgorithmChange()
{
  try {
    redrawExtraImage();
    redrawCurrentMask();
    updateDocumentMask();

    update_screen_for_document(m_document);
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
}

void PixelsMovement::updateDocumentMask()
{
  m_document->setMask(m_currentMask.get());
  m_document->generateMaskBoundaries(m_currentMask.get());
}

void PixelsMovement::hideDocumentMask()
{
  m_document->destroyMaskBoundaries();
  update_screen_for_document(m_document);
}

void PixelsMovement::flipOriginalImage(const doc::algorithm::FlipType flipType)
{
  // Flip the image.
  doc::algorithm::flip_image(
    m_originalImage.get(),
    gfx::Rect(gfx::Point(0, 0),
              gfx::Size(m_originalImage->width(),
                        m_originalImage->height())),
    flipType);

  // Flip the mask.
  doc::algorithm::flip_image(
    m_initialMask->bitmap(),
    gfx::Rect(gfx::Point(0, 0), m_initialMask->bounds().size()),
    flipType);
}

void PixelsMovement::shiftOriginalImage(const int dx, const int dy,
                                        const double angle)
{
  doc::algorithm::shift_image(
    m_originalImage.get(), dx, dy, angle);
}

// Returns the list of cels that will be transformed (the first item
// in the list must be the current cel that was transformed if the cel
// wasn't nullptr).
CelList PixelsMovement::getEditableCels()
{
  CelList cels;

  if (editMultipleCels()) {
    cels = get_unlocked_unique_cels(
      m_site.sprite(), m_site.range());
  }
  else {
    // TODO This case is used in paste too, where the cel() can be
    //      nullptr (e.g. we paste the clipboard image into an empty
    //      cel).
    cels.push_back(m_site.cel());
    return cels;
  }

  // Current cel (m_site.cel()) can be nullptr when we paste in an
  // empty cel (Ctrl+V) and cut (Ctrl+X) the floating pixels.
  if (m_site.cel() &&
      m_site.cel()->layer()->isEditable()) {
    auto it = std::find(cels.begin(), cels.end(), m_site.cel());
    if (it != cels.end())
      cels.erase(it);
    cels.insert(cels.begin(), m_site.cel());
  }

  return cels;
}

bool PixelsMovement::gotoFrame(const doc::frame_t deltaFrame)
{
  if (editMultipleCels()) {
    Layer* layer = m_site.layer();
    ASSERT(layer);

    const doc::SelectedFrames frames = m_site.range().selectedFrames();
    doc::frame_t initialFrame = m_site.frame();
    doc::frame_t frame = initialFrame + deltaFrame;

    if (frames.size() >= 2) {
      for (; !frames.contains(frame) &&
             !layer->cel(frame); frame+=deltaFrame) {
        if (deltaFrame > 0 && frame > frames.lastFrame()) {
          frame = frames.firstFrame();
          break;
        }
        else if (deltaFrame < 0 && frame < frames.firstFrame()) {
          frame = frames.lastFrame();
          break;
        }
      }

      if (frame == initialFrame ||
          !frames.contains(frame) ||
          // TODO At the moment we don't support going to an empty cel,
          //      so we don't handle these cases
          !layer->cel(frame)) {
        return false;
      }

      // Rollback all the actions, go to the next/previous frame and
      // reproduce all transformation again so the new frame is the
      // preview for the transformation.
      m_tx.rollbackAndStartAgain();

      {
        m_canHandleFrameChange = true;
        {
          ContextWriter writer(m_reader, 1000);
          writer.context()->setActiveFrame(frame);
          m_site.frame(frame);
        }
        m_canHandleFrameChange = false;
      }

      reproduceAllTransformationsWithInnerCmds();
      return true;
    }
  }
  return false;
}

// Reproduces all the inner commands in the active m_site
void PixelsMovement::reproduceAllTransformationsWithInnerCmds()
{
  TRACEARGS("MOVPIXS: reproduceAllTransformationsWithInnerCmds",
            "layer", m_site.layer()->name(),
            "frame", m_site.frame());
  DUMP_INNER_CMDS();

  m_document->setMask(m_initialMask0.get());
  m_initialMask->copyFrom(m_initialMask0.get());
  m_originalImage.reset(
    new_image_from_mask(
      m_site, m_initialMask.get(),
      Preferences::instance().experimental.newBlend()));

  for (const InnerCmd& c : m_innerCmds) {
    switch (c.type) {
      case InnerCmd::Clear:
        m_tx(new cmd::ClearMask(m_site.cel()));
        break;
      case InnerCmd::Flip:
        flipOriginalImage(c.data.flip.type);
        break;
      case InnerCmd::Shift:
        shiftOriginalImage(c.data.shift.dx,
                           c.data.shift.dy,
                           c.data.shift.angle);
        break;
      case InnerCmd::Stamp:
        redrawExtraImage(c.data.stamp.transformation);
        stampExtraCelImage();
        break;
    }
  }

  redrawExtraImage();
  redrawCurrentMask();
  updateDocumentMask();
}

#if _DEBUG
void PixelsMovement::dumpInnerCmds()
{
  TRACEARGS("MOVPIXS: InnerCmds size=", m_innerCmds.size());
  for (auto& c : m_innerCmds) {
    switch (c.type) {
      case InnerCmd::None:
        TRACEARGS("MOVPIXS: - None");
        break;
      case InnerCmd::Clear:
        TRACEARGS("MOVPIXS: - Clear");
        break;
      case InnerCmd::Flip:
        TRACEARGS("MOVPIXS: - Flip",
                  (c.data.flip.type == doc::algorithm::FlipHorizontal ? "Horizontal":
                                                                        "Vertical"));
        break;
      case InnerCmd::Shift:
        TRACEARGS("MOVPIXS: - Shift",
                  "dx=", c.data.shift.dx,
                  "dy=", c.data.shift.dy,
                  "angle=", c.data.shift.angle);
        break;
      case InnerCmd::Stamp:
        TRACEARGS("MOVPIXS: - Stamp",
                  "angle=", c.data.stamp.transformation->angle(),
                  "pivot=", c.data.stamp.transformation->pivot().x,
                  c.data.stamp.transformation->pivot().y,
                  "bounds=", c.data.stamp.transformation->bounds().x,
                  c.data.stamp.transformation->bounds().y,
                  c.data.stamp.transformation->bounds().w,
                  c.data.stamp.transformation->bounds().h);
        break;
    }
  }
}
#endif // _DEBUG

} // namespace app
