// Aseprite
// Copyright (C) 2015, 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "base/convert_to.h"
#include "doc/brush_type.h"
#include "doc/mask.h"
#include "filters/neighboring_pixels.h"

#include "modify_selection.xml.h"

#include <limits>

namespace app {

using namespace doc;

class ModifySelectionWindow : public app::gen::ModifySelection {
};

class ModifySelectionCommand : public Command {
public:
  enum Modifier { Border, Expand, Contract };

  ModifySelectionCommand();
  Command* clone() const override { return new ModifySelectionCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  std::string getActionName() const;
  void applyModifier(const Mask* srcMask, Mask* dstMask,
                     const int brushRadius,
                     const doc::BrushType brushType) const;

  Modifier m_modifier;
  int m_quantity;
  doc::BrushType m_brushType;
};

ModifySelectionCommand::ModifySelectionCommand()
  : Command("ModifySelection",
            "Modify Selection",
            CmdRecordableFlag)
  , m_modifier(Expand)
  , m_quantity(0)
  , m_brushType(doc::kCircleBrushType)
{
}

void ModifySelectionCommand::onLoadParams(const Params& params)
{
  const std::string modifier = params.get("modifier");
  if (modifier == "border") m_modifier = Border;
  else if (modifier == "expand") m_modifier = Expand;
  else if (modifier == "contract") m_modifier = Contract;

  const int quantity = params.get_as<int>("quantity");
  m_quantity = std::max<int>(0, quantity);

  const std::string brush = params.get("brush");
  if (brush == "circle") m_brushType = doc::kCircleBrushType;
  else if (brush == "square") m_brushType = doc::kSquareBrushType;
}

bool ModifySelectionCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasVisibleMask);
}

void ModifySelectionCommand::onExecute(Context* context)
{
  int quantity = m_quantity;
  doc::BrushType brush = m_brushType;

  if (quantity == 0) {
    Preferences& pref = Preferences::instance();
    ModifySelectionWindow window;

    window.setText(getActionName() + " Selection");
    if (m_modifier == Border)
      window.byLabel()->setText("Width:");
    else
      window.byLabel()->setText(getActionName() + " By:");

    window.quantity()->setTextf("%d", pref.selection.modifySelectionQuantity());

    brush = (pref.selection.modifySelectionBrush() == app::gen::BrushType::CIRCLE
             ? doc::kCircleBrushType:
               doc::kSquareBrushType);
    window.circle()->setSelected(brush == doc::kCircleBrushType);
    window.square()->setSelected(brush == doc::kSquareBrushType);

    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    quantity = window.quantity()->textInt();
    quantity = MID(1, quantity, 100);

    brush = (window.circle()->isSelected() ? doc::kCircleBrushType:
                                             doc::kSquareBrushType);

    pref.selection.modifySelectionQuantity(quantity);
    pref.selection.modifySelectionBrush(
      (brush == doc::kCircleBrushType ? app::gen::BrushType::CIRCLE:
                                        app::gen::BrushType::SQUARE));
  }

  // Lock sprite
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());

  base::UniquePtr<Mask> mask(new Mask());
  {
    mask->reserve(sprite->bounds());
    mask->freeze();
    applyModifier(document->mask(), mask, quantity, brush);
    mask->unfreeze();
  }

  // Set the new mask
  Transaction transaction(writer.context(),
                          getActionName() + " Selection",
                          DoesntModifyDocument);
  transaction.execute(new cmd::SetMask(document, mask));
  transaction.commit();

  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

std::string ModifySelectionCommand::onGetFriendlyName() const
{
  std::string text;

  text += getActionName();
  text += " Selection";

  if (m_quantity > 0) {
    text += " by ";
    text += base::convert_to<std::string>(m_quantity);
    text += " pixel";
    if (m_quantity > 1)
      text += "s";
  }

  return text;
}

std::string ModifySelectionCommand::getActionName() const
{
  switch (m_modifier) {
    case Border: return "Border";
    case Expand: return "Expand";
    case Contract: return "Contract";
    default: return "Modify";
  }
}

// TODO create morphological operators/functions in "doc" namespace
// TODO the impl is not optimal, but is good enough as a first version
void ModifySelectionCommand::applyModifier(const Mask* srcMask, Mask* dstMask,
                                           const int radius,
                                           const doc::BrushType brush) const
{
  const doc::Image* srcImage = srcMask->bitmap();
  doc::Image* dstImage = dstMask->bitmap();

  // Image bounds to clip get/put pixels
  const gfx::Rect srcBounds = srcImage->bounds();

  // Create a kernel
  const int size = 2*radius+1;
  base::UniquePtr<doc::Image> kernel(doc::Image::create(IMAGE_BITMAP, size, size));
  doc::clear_image(kernel, 0);
  if (brush == doc::kCircleBrushType)
    doc::fill_ellipse(kernel, 0, 0, size-1, size-1, 1);
  else
    doc::fill_rect(kernel, 0, 0, size-1, size-1, 1);
  doc::put_pixel(kernel, radius, radius, 0);

  int total = 0;                // Number of 1s in the kernel image
  for (int v=0; v<size; ++v)
    for (int u=0; u<size; ++u)
      total += kernel->getPixel(u, v);

  for (int y=-radius; y<srcBounds.h+radius; ++y) {
    for (int x=-radius; x<srcBounds.w+radius; ++x) {
      doc::color_t c;
      if (srcBounds.contains(x, y))
        c = srcImage->getPixel(x, y);
      else
        c = 0;

      int accum = 0;
      for (int v=0; v<size; ++v) {
        for (int u=0; u<size; ++u) {
          if (kernel->getPixel(u, v)) {
            if (srcBounds.contains(x+u-radius, y+v-radius))
              accum += srcImage->getPixel(x-radius+u, y-radius+v);
          }
        }
      }

      switch (m_modifier) {
        case Border: {
          c = (c && accum < total) ? 1: 0;
          break;
        }
        case Expand: {
          c = (c || accum > 0) ? 1: 0;
          break;
        }
        case Contract: {
          c = (c && accum == total) ? 1: 0;
          break;
        }
      }

      if (c)
        doc::put_pixel(dstImage,
                       srcMask->bounds().x+x,
                       srcMask->bounds().y+y, 1);
    }
  }
}

Command* CommandFactory::createModifySelectionCommand()
{
  return new ModifySelectionCommand;
}

} // namespace app
