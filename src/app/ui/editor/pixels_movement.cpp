// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
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
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/snap_to_grid.h"
#include "app/ui/editor/pivot_helpers.h"
#include "app/ui/editor/vec2.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/cel_ops.h"
#include "app/util/expand_cel_canvas.h"
#include "app/util/new_image_from_mask.h"
#include "app/util/range_utils.h"
#include "base/pi.h"
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
  double cornerThick = (m_site.tilemapMode() == TilemapMode::Tiles) ?
                          CORNER_THICK_FOR_TILEMAP_MODE :
                          CORNER_THICK_FOR_PIXELS_MODE;
  Transformation transform(mask->bounds(), cornerThick);
  set_pivot_from_preferences(transform);

  m_initialData = transform;
  m_currentData = transform;

  m_initialMask.reset(new Mask(*mask));
  m_initialMask0.reset(new Mask(*mask));
  m_currentMask.reset(new Mask(*mask));

  m_pivotVisConn =
    Preferences::instance().selection.pivotVisibility.AfterChange.connect(
      [this]{ onPivotChange(); });
  m_pivotPosConn =
    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      [this]{ onPivotChange(); });
  m_rotAlgoConn =
    Preferences::instance().selection.rotationAlgorithm.AfterChange.connect(
      [this]{ onRotationAlgorithmChange(); });

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

void PixelsMovement::setDelegate(PixelsMovementDelegate* delegate)
{
  m_delegate = delegate;
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

void PixelsMovement::setTransformation(const Transformation& t)
{
  m_initialData = m_currentData;

  setTransformationBase(t);

  redrawCurrentMask();
  updateDocumentMask();

  update_screen_for_document(m_document);
}

void PixelsMovement::setTransformationBase(const Transformation& t)
{
  // Get old transformed corners, update transformation, and get new
  // transformed corners. These corners will be used to know what to
  // update in the editor's canvas.
  auto oldCorners = m_currentData.transformedCorners();
  m_currentData = t;
  auto newCorners = m_currentData.transformedCorners();

  redrawExtraImage();

  m_document->setTransformation(m_currentData);

  // Create a union of all corners, and that will be the bounds to
  // redraw of the sprite.
  gfx::Rect fullBounds;
  for (int i=0; i<Transformation::Corners::NUM_OF_CORNERS; ++i) {
    fullBounds |= gfx::Rect((int)oldCorners[i].x, (int)oldCorners[i].y, 1, 1);
    fullBounds |= gfx::Rect((int)newCorners[i].x, (int)newCorners[i].y, 1, 1);
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
      clear_mask_from_cel(m_tx,
                          writer.cel(),
                          m_site.tilemapMode(),
                          m_site.tilesetMode());

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

void PixelsMovement::catchImage(const gfx::PointF& pos, HandleType handle)
{
  ASSERT(handle != NoHandle);

  m_catchPos = pos;
  m_isDragging = true;
  m_handle = handle;
}

void PixelsMovement::catchImageAgain(const gfx::PointF& pos, HandleType handle)
{
  // Create a new Transaction to move the pixels to other position
  m_initialData = m_currentData;
  m_isDragging = true;
  m_catchPos = pos;
  m_handle = handle;

  hideDocumentMask();
}

void PixelsMovement::moveImage(const gfx::PointF& pos, MoveModifier moveModifier)
{
  ContextWriter writer(m_reader, 1000);
  gfx::RectF bounds = m_initialData.bounds();
  gfx::PointF abs_initial_pivot = m_initialData.pivot();
  gfx::PointF abs_pivot = m_currentData.pivot();

  auto newTransformation = m_currentData;

  switch (m_handle) {

    case MovePixelsHandle: {
      double dx = (pos.x - m_catchPos.x);
      double dy = (pos.y - m_catchPos.y);
      if ((moveModifier & FineControl) == 0) {
        if (dx >= 0.0) { dx = std::floor(dx); } else { dx = std::ceil(dx); }
        if (dy >= 0.0) { dy = std::floor(dy); } else { dy = std::ceil(dy); }
      }

      if ((moveModifier & LockAxisMovement) == LockAxisMovement) {
        if (std::abs(dx) < std::abs(dy))
          dx = 0.0;
        else
          dy = 0.0;
      }

      bounds.offset(dx, dy);

      if ((m_site.tilemapMode() == TilemapMode::Tiles) ||
          (moveModifier & SnapToGridMovement) == SnapToGridMovement) {
        // Snap the x1,y1 point to the grid.
        gfx::Rect gridBounds = m_site.gridBounds();
        gfx::PointF gridOffset(
          snap_to_grid(
            gridBounds,
            gfx::Point(bounds.origin()),
            PreferSnapTo::ClosestGridVertex));

        // Now we calculate the difference from x1,y1 point and we can
        // use it to adjust all coordinates (x1, y1, x2, y2).
        bounds.setOrigin(gridOffset);
      }

      newTransformation.bounds(bounds);
      newTransformation.pivot(abs_initial_pivot +
                              bounds.origin() -
                              m_initialData.bounds().origin());
      break;
    }

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
        pivot = m_currentData.pivot();
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
        vec2 u = to_vec2(m_catchPos - pivot);
        vec2 v = to_vec2(pos - pivot);
        vec2 w = v.projectOn(u);
        double scale = u.magnitude();
        if (scale != 0.0) {
          scale = (std::fabs(w.angle()-u.angle()) < PI/2.0 ? 1.0: -1.0) * w.magnitude() / scale;
        }
        else
          scale = 1.0;

        a.x = ((a.x-pivot.x)*scale + pivot.x);
        a.y = ((a.y-pivot.y)*scale + pivot.y);
        b.x = ((b.x-pivot.x)*scale + pivot.x);
        b.y = ((b.y-pivot.y)*scale + pivot.y);
      }
      else {
        handle.x = bounds.x + bounds.w*handle.x;
        handle.y = bounds.y + bounds.h*handle.y;

        double z = m_currentData.angle();
        double w = (handle.x-pivot.x);
        double h = (handle.y-pivot.y);
        double dx = ((pos.x - m_catchPos.x) *  std::cos(z) +
                     (pos.y - m_catchPos.y) * -std::sin(z));
        double dy = ((pos.x - m_catchPos.x) *  std::sin(z) +
                     (pos.y - m_catchPos.y) *  std::cos(z));
        if ((moveModifier & FineControl) == 0) {
          if (dx >= 0.0) { dx = std::floor(dx); } else { dx = std::ceil(dx); }
          if (dy >= 0.0) { dy = std::floor(dy); } else { dy = std::ceil(dy); }
        }

        if (m_handle == ScaleNHandle || m_handle == ScaleSHandle) {
          dx = 0.0;
          w = 1.0;              // Any value != 0.0 to avoid div by zero
        }
        else if (m_handle == ScaleWHandle || m_handle == ScaleEHandle) {
          dy = 0.0;
          h = 1.0;
        }

        a.x = ((a.x-pivot.x)*(1.0+dx/w) + pivot.x);
        a.y = ((a.y-pivot.y)*(1.0+dy/h) + pivot.y);
        b.x = ((b.x-pivot.x)*(1.0+dx/w) + pivot.x);
        b.y = ((b.y-pivot.y)*(1.0+dy/h) + pivot.y);
      }

      // Snap to grid when resizing tilemaps
      if (m_site.tilemapMode() == TilemapMode::Tiles) {
        gfx::Rect gridBounds = m_site.gridBounds();
        a = gfx::PointF(snap_to_grid(gridBounds, gfx::Point(a), PreferSnapTo::BoxOrigin));
        b = gfx::PointF(snap_to_grid(gridBounds, gfx::Point(b), PreferSnapTo::BoxOrigin));
      }

      // Do not use "gfx::Rect(a, b)" here because if a > b we want to
      // keep a rectangle with negative width or height (to know that
      // it was flipped).
      bounds.x = a.x;
      bounds.y = a.y;
      bounds.w = b.x - a.x;
      bounds.h = b.y - a.y;

      newTransformation.bounds(bounds);
      m_adjustPivot = true;
      break;
    }

    case RotateNWHandle:
    case RotateNEHandle:
    case RotateSWHandle:
    case RotateSEHandle: {
      // Cannot rotate tiles
      // TODO add support to rotate tiles in straight angles (changing tile flags)
      if (m_site.tilemapMode() == TilemapMode::Tiles)
        break;

      double da = (std::atan2((double)(-pos.y + abs_pivot.y),
                              (double)(+pos.x - abs_pivot.x)) -
                   std::atan2((double)(-m_catchPos.y + abs_initial_pivot.y),
                              (double)(+m_catchPos.x - abs_initial_pivot.x)));
      double newAngle = m_initialData.angle() + da;
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

      newTransformation.angle(newAngle);
      break;
    }

    case SkewNHandle:
    case SkewSHandle:
    case SkewWHandle:
    case SkewEHandle: {
      // Cannot skew tiles
      // TODO could we support to skew tiles if we have the set of tiles (e.g. diagonals)?
      //      maybe too complex to implement in UI terms
      if (m_site.tilemapMode() == TilemapMode::Tiles)
        break;

      //    u
      // ------>
      //
      // A --- B   |
      // |     |   | v
      // |     |   |
      // C --- D   v
      auto corners = m_initialData.transformedCorners();
      auto A = corners[Transformation::Corners::LEFT_TOP];
      auto B = corners[Transformation::Corners::RIGHT_TOP];
      auto C = corners[Transformation::Corners::LEFT_BOTTOM];
      auto D = corners[Transformation::Corners::RIGHT_BOTTOM];

      // Pivot in pixels
      gfx::PointF pivotPoint = m_currentData.pivot();

      // Pivot in [0.0, 1.0] range
      gfx::PointF pivot((pivotPoint.x - bounds.x) / ABS(bounds.w),
                        (pivotPoint.y - bounds.y) / ABS(bounds.h));

      // Vector from AB (or CD), and AC (or BD)
      vec2 u = to_vec2(B - A);
      vec2 v = to_vec2(C - A);

      // Move sides depending of a delta value (the mouse pos - catch
      // pos) projected on u or v vectors. North and south cases are
      // simple because only AB or CD sides can be modified (and then
      // skew angle is calculated from the pivot position), but with
      // east and west handles we modify all points to recalculate all
      // the transformation parameters from scratch.
      vec2 delta = to_vec2(pos - m_catchPos);
      switch (m_handle) {
        case SkewNHandle:
          delta = delta.projectOn(u);
          A.x += delta.x;
          A.y += delta.y;
          B.x += delta.x;
          B.y += delta.y;
          break;
        case SkewSHandle:
          delta = delta.projectOn(u);
          C.x += delta.x;
          C.y += delta.y;
          D.x += delta.x;
          D.y += delta.y;
          break;
        case SkewWHandle: {
          delta = delta.projectOn(v);
          A.x += delta.x;
          A.y += delta.y;
          C.x += delta.x;
          C.y += delta.y;

          vec2 toPivot = to_vec2(pivotPoint - (A*(1.0-pivot.y) + C*pivot.y));
          vec2 toOtherSide = toPivot / (std::fabs(pivot.x) > 0.00001 ? pivot.x: 1.0);
          B = A + to_point(toOtherSide);
          D = C + to_point(toOtherSide);
          break;
        }
        case SkewEHandle: {
          delta = delta.projectOn(v);
          B.x += delta.x;
          B.y += delta.y;
          D.x += delta.x;
          D.y += delta.y;

          vec2 toPivot = to_vec2(pivotPoint - (B*(1.0-pivot.y) + D*pivot.y));
          vec2 toOtherSide = toPivot / (std::fabs(1.0-pivot.x) > 0.00001 ? (1.0-pivot.x): 1.0);
          A = B + to_point(toOtherSide);
          C = D + to_point(toOtherSide);
          break;
        }
      }

      // t0 will be a transformation without skew, so we can compare
      // the angle between vector PR with skew and without skew.
      auto t0 = m_initialData;
      t0.skew(0.0);
      auto corners0 = t0.transformedCorners();
      auto A0 = corners0[Transformation::Corners::LEFT_TOP];
      auto C0 = corners0[Transformation::Corners::LEFT_BOTTOM];

      //      A0 ------- B
      //     /|         /
      //    / ACp      /   <- pivot position
      //   /  |       /
      //  C -C0----- D
      vec2 AC0 = to_vec2(C0 - A0);
      auto ACp = A0*(1.0-pivot.y) + C0*pivot.y;
      vec2 AC;
      switch (m_handle) {
        case SkewNHandle: AC = to_vec2(ACp - A); break;
        case SkewSHandle: AC = to_vec2(C - ACp); break;
        case SkewWHandle:
        case SkewEHandle: {
          vec2 AB = to_vec2(B - A);
          bounds.w = AB.magnitude();
          bounds.x = pivotPoint.x - bounds.w*pivot.x;

          // New rotation angle is the angle between AB points
          newTransformation.angle(-AB.angle());

          // New skew angle is the angle between AC0 (vector from A to
          // B rotated 45 degrees, like an AC vector without skew) and
          // the current to AC vector.
          //
          //          B
          //        / |
          //      /   |
          //    /     |
          //  A       |
          //  | \     D
          //  |  \  /
          //  |   / <- AC0=AB rotated 45 degrees, if pivot is here
          //  | /
          //  C
          auto ABp = A*(1.0-pivot.x) + B*pivot.x;
          AC0 = vec2(ABp.y - B.y, B.x - ABp.x);
          AC = to_vec2(C - A);

          bounds.h = AC.projectOn(AC0).magnitude();
          bounds.y = pivotPoint.y - bounds.h*pivot.y;
          newTransformation.bounds(bounds);
          break;
        }
      }

      // Calculate angle between AC and AC0
      double newSkew = std::atan2(AC.x*AC0.y - AC.y*AC0.x, AC * AC0);
      newSkew = std::clamp(newSkew, -PI*85.0/180.0, PI*85.0/180.0);
      newTransformation.skew(newSkew);
      break;
    }

    case PivotHandle: {
      // Calculate the new position of the pivot
      gfx::PointF newPivot = m_initialData.pivot() + gfx::Point(pos) - m_catchPos;
      newTransformation = m_initialData;
      newTransformation.displacePivotTo(newPivot);
      break;
    }
  }

  setTransformationBase(newTransformation);
}

void PixelsMovement::getDraggedImageCopy(std::unique_ptr<Image>& outputImage,
                                         std::unique_ptr<Mask>& outputMask)
{
  gfx::Rect bounds = m_currentData.transformedBounds();
  if (bounds.isEmpty())
    return;

  doc::PixelFormat pixelFormat;
  gfx::Size imgSize;
  if (m_site.tilemapMode() == TilemapMode::Tiles) {
    imgSize = m_site.grid().canvasToTile(bounds).size();
    pixelFormat = IMAGE_TILEMAP;
  }
  else {
    imgSize = bounds.size();
    pixelFormat = m_site.sprite()->pixelFormat();
  }

  std::unique_ptr<Image> image(
    Image::create(
      pixelFormat,
      imgSize.w,
      imgSize.h));

  drawImage(m_currentData, image.get(),
            gfx::PointF(bounds.origin()), false);

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

  if (currentCel && currentCel->layer() &&
      currentCel->layer()->isImage() &&
      !currentCel->layer()->isEditableHierarchy()) {
    Transformation initialCelPos(gfx::Rect(m_initialMask0->bounds()), m_currentData.cornerThick());
    redrawExtraImage(&initialCelPos);
    stampExtraCelImage();
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
  const Image* image = m_extraCel->image();
  if (!image)
    return;

  const Cel* cel = m_extraCel->cel();

  // Expand the canvas to paste the image in the fully visible
  // portion of sprite.
  ExpandCelCanvas expand(
    m_site, m_site.layer(),
    TiledMode::NONE, m_tx,
    ExpandCelCanvas::None);

  gfx::Point dstPt;
  gfx::Size canvasImageSize = image->size();
  if (m_site.tilemapMode() == TilemapMode::Tiles) {
    doc::Grid grid = m_site.grid();
    dstPt = grid.canvasToTile(cel->position());
    canvasImageSize = grid.tileToCanvas(gfx::Rect(dstPt, canvasImageSize)).size();
  }
  else {
    dstPt = cel->position() - expand.getCel()->position();
  }

  // We cannot use cel->bounds() because cel->image() is nullptr
  expand.validateDestCanvas(
    gfx::Region(gfx::Rect(cel->position(), canvasImageSize)));

  expand.getDestCanvas()->copy(image, gfx::Clip(dstPt, image->bounds()));

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
      adjustPivot();

      if (m_delegate)
        m_delegate->onPivotChange();
    }

    redrawCurrentMask();
    updateDocumentMask();

    update_screen_for_document(m_document);
  }
}

void PixelsMovement::adjustPivot()
{
  // Get the a factor for the X/Y position of the initial pivot
  // position inside the initial non-rotated bounds.
  gfx::PointF pivotPosFactor(m_initialData.pivot() - m_initialData.bounds().origin());
  pivotPosFactor.x /= m_initialData.bounds().w;
  pivotPosFactor.y /= m_initialData.bounds().h;

  // Get the current transformed bounds.
  auto corners = m_currentData.transformedCorners();

  // The new pivot will be located from the rotated left-top
  // corner a distance equal to the transformed bounds's
  // width/height multiplied with the previously calculated X/Y
  // factor.
  vec2 newPivot(corners.leftTop().x,
                corners.leftTop().y);
  newPivot += pivotPosFactor.x * to_vec2(corners.rightTop() - corners.leftTop());
  newPivot += pivotPosFactor.y * to_vec2(corners.leftBottom() - corners.leftTop());

  m_currentData.displacePivotTo(gfx::PointF(newPivot.x, newPivot.y));
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

  if (!m_extraCel)
    m_extraCel.reset(new ExtraCel);

  gfx::Rect bounds = transformation->transformedBounds();

  if (!bounds.isEmpty()) {
    gfx::Size extraCelSize;
    if (m_site.tilemapMode() == TilemapMode::Tiles) {
      // Transforming tiles
      extraCelSize = m_site.grid().canvasToTile(bounds).size();
    }
    else {
      // Transforming pixels
      extraCelSize = bounds.size();
    }

    m_extraCel->create(
      m_site.tilemapMode(),
      m_document->sprite(),
      bounds,
      extraCelSize,
      m_site.frame(),
      opacity);
    m_extraCel->setType(render::ExtraType::PATCH);
    m_extraCel->setBlendMode(m_site.layer()->isImage() ?
                             static_cast<LayerImage*>(m_site.layer())->blendMode():
                             BlendMode::NORMAL);
  }
  else
    m_extraCel->reset();

  m_document->setExtraCel(m_extraCel);

  if (m_extraCel->image()) {
    // Draw the transformed pixels in the extra-cel which is the chunk
    // of pixels that the user is moving.
    drawImage(*transformation, m_extraCel->image(),
              gfx::PointF(bounds.origin()), true);
  }
}

void PixelsMovement::redrawCurrentMask()
{
  drawMask(m_currentMask.get(), true);
}

void PixelsMovement::drawImage(
  const Transformation& transformation,
  doc::Image* dst, const gfx::PointF& pt,
  const bool renderOriginalLayer)
{
  ASSERT(dst);

  auto corners = transformation.transformedCorners();
  gfx::Rect bounds = corners.bounds(transformation.cornerThick());

  if (m_site.tilemapMode() == TilemapMode::Tiles) {
    dst->setMaskColor(doc::notile);
    dst->clear(dst->maskColor());

    if (renderOriginalLayer && m_site.cel()) {
      doc::Grid grid = m_site.grid();
      dst->copy(m_site.cel()->image(),
                gfx::Clip(0, 0, grid.canvasToTile(bounds)));
    }

    drawTransformedTilemap(
      transformation,
      dst, m_originalImage.get(),
      m_initialMask.get());
  }
  else {
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
}

void PixelsMovement::drawMask(doc::Mask* mask, bool shrink)
{
  auto corners = m_currentData.transformedCorners();
  gfx::Rect bounds = corners.bounds(m_currentData.cornerThick());

  if (bounds.isEmpty()) {
    mask->clear();
    return;
  }

  mask->replace(bounds);
  if (shrink)
    mask->freeze();
  clear_image(mask->bitmap(), 0);
  drawParallelogram(m_currentData,
                    mask->bitmap(),
                    m_initialMask->bitmap(),
                    nullptr,
                    corners,
                    gfx::PointF(bounds.origin()));
  if (shrink)
    mask->unfreeze();
}

void PixelsMovement::drawParallelogram(
  const Transformation& transformation,
  doc::Image* dst, const doc::Image* src, const doc::Mask* mask,
  const Transformation::Corners& corners,
  const gfx::PointF& leftTop)
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
        StatusBar::instance()->showTip(
          1000,
          Strings::statusbar_tips_not_enough_rotsprite_memory());

        rotAlgo = tools::RotationAlgorithm::FAST;
        goto retry;
      }
      break;

  }
}

static void merge_tilemaps(Image* dst, const Image* src, gfx::Clip area)
{
  if (!area.clip(dst->width(), dst->height(), src->width(), src->height()))
    return;

  ImageConstIterator<TilemapTraits> src_it(src, area.srcBounds(), area.src.x, area.src.y);
  ImageIterator<TilemapTraits> dst_it(dst, area.dstBounds(), area.dst.x, area.dst.y);

  for (int y=0; y<area.size.h; ++y) {
    for (int x=0; x<area.size.w; ++x) {
      if (*src_it != doc::notile)
        *dst_it = *src_it;
      ++src_it;
      ++dst_it;
    }
  }
}

void PixelsMovement::drawTransformedTilemap(
  const Transformation& transformation,
  doc::Image* dst, const doc::Image* src, const doc::Mask* mask)
{
  ASSERT(dst->pixelFormat() == IMAGE_TILEMAP);
  ASSERT(src->pixelFormat() == IMAGE_TILEMAP);

  const int boxw = std::max(1, src->width()-2);
  const int boxh = std::max(1, src->height()-2);

  // Function to copy a whole row of tiles (h=number of tiles in Y axis)
  auto draw_row =
    [dst, src, boxw](int y, int v, int h) {
      merge_tilemaps(dst, src, gfx::Clip(0, y, 0, v, 1, h));
      if (boxw) {
        const int u = std::min(1, src->width()-1);
        for (int x=1; x<dst->width()-1; x+=boxw)
          merge_tilemaps(dst, src, gfx::Clip(x, y, u, v, boxw, h));
      }
      merge_tilemaps(dst, src, gfx::Clip(dst->width()-1, y, src->width()-1, v, 1, h));
    };

  draw_row(0, 0, 1);
  if (boxh) {
    const int v = std::min(1, src->height()-1);
    for (int y=1; y<dst->height()-1; y+=boxh)
      draw_row(y, v, boxh);
  }
  draw_row(dst->height()-1, src->height()-1, 1);
}

void PixelsMovement::onPivotChange()
{
  set_pivot_from_preferences(m_currentData);
  onRotationAlgorithmChange();

  if (m_delegate)
    m_delegate->onPivotChange();
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
    cels = get_unique_cels_to_edit_pixels(m_site.sprite(),
                                          m_site.range());
  }
  else {
    // TODO This case is used in paste too, where the cel() can be
    //      nullptr (e.g. we paste the clipboard image into an empty
    //      cel).
    if (m_site.layer() &&
        m_site.layer()->canEditPixels()) {
      cels.push_back(m_site.cel());
    }
    return cels;
  }

  // Current cel (m_site.cel()) can be nullptr when we paste in an
  // empty cel (Ctrl+V) and cut (Ctrl+X) the floating pixels.
  if (m_site.cel() &&
      m_site.cel()->layer()->canEditPixels()) {
    CelList::iterator it;

    // If we are in a linked cel, remove the cel that matches the
    // linked cel. In this way we avoid having two Cel in cels
    // pointing to the same CelData.
    if (Cel* link = m_site.cel()->link()) {
      it = std::find_if(cels.begin(), cels.end(),
                        [link](const Cel* cel){
                          return (cel == link ||
                                  cel->link() == link);
                        });
    }
    else {
      it = std::find(cels.begin(), cels.end(), m_site.cel());
    }
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
        clear_mask_from_cel(m_tx,
                            m_site.cel(),
                            m_site.tilemapMode(),
                            m_site.tilesetMode());
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
