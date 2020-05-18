// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
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
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "base/clamp.h"
#include "base/convert_to.h"
#include "doc/algorithm/modify_selection.h"
#include "doc/brush_type.h"
#include "doc/mask.h"
#include "filters/neighboring_pixels.h"
#include "fmt/format.h"

#include "modify_selection.xml.h"

#include <limits>

namespace app {

using namespace doc;
typedef doc::algorithm::SelectionModifier Modifier;

class ModifySelectionWindow : public app::gen::ModifySelection {
};

class ModifySelectionCommand : public Command {
public:
  ModifySelectionCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  std::string getActionName() const;

  Modifier m_modifier;
  int m_quantity;
  doc::BrushType m_brushType;
};

ModifySelectionCommand::ModifySelectionCommand()
  : Command(CommandId::ModifySelection(), CmdRecordableFlag)
  , m_modifier(Modifier::Expand)
  , m_quantity(0)
  , m_brushType(doc::kCircleBrushType)
{
}

void ModifySelectionCommand::onLoadParams(const Params& params)
{
  const std::string modifier = params.get("modifier");
  if (modifier == "border") m_modifier = Modifier::Border;
  else if (modifier == "expand") m_modifier = Modifier::Expand;
  else if (modifier == "contract") m_modifier = Modifier::Contract;

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
    if (m_modifier == Modifier::Border)
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
    quantity = base::clamp(quantity, 1, 100);

    brush = (window.circle()->isSelected() ? doc::kCircleBrushType:
                                             doc::kSquareBrushType);

    pref.selection.modifySelectionQuantity(quantity);
    pref.selection.modifySelectionBrush(
      (brush == doc::kCircleBrushType ? app::gen::BrushType::CIRCLE:
                                        app::gen::BrushType::SQUARE));
  }

  // Lock sprite
  ContextWriter writer(context);
  Doc* document(writer.document());
  Sprite* sprite(writer.sprite());

  std::unique_ptr<Mask> mask(new Mask);
  {
    mask->reserve(sprite->bounds());
    mask->freeze();
    doc::algorithm::modify_selection(
       m_modifier, document->mask(), mask.get(), quantity, brush);
    mask->unfreeze();
  }

  // Set the new mask
  Tx tx(writer.context(),
        friendlyName(),
        DoesntModifyDocument);
  tx(new cmd::SetMask(document, mask.get()));
  tx.commit();

  update_screen_for_document(document);
}

std::string ModifySelectionCommand::onGetFriendlyName() const
{
  std::string quantity;
  if (m_quantity > 0)
    quantity = fmt::format(Strings::commands_ModifySelection_Quantity(), m_quantity);

  return fmt::format(getBaseFriendlyName(),
                     getActionName(),
                     quantity);
}

std::string ModifySelectionCommand::getActionName() const
{
  switch (m_modifier) {
    case Modifier::Border: return Strings::commands_ModifySelection_Border();
    case Modifier::Expand: return Strings::commands_ModifySelection_Expand();
    case Modifier::Contract: return Strings::commands_ModifySelection_Contract();
    default: return Strings::commands_ModifySelection_Modify();
  }
}

Command* CommandFactory::createModifySelectionCommand()
{
  return new ModifySelectionCommand;
}

} // namespace app
