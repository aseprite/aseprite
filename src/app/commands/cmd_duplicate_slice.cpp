// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_slice.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/context_flags.h"
#include "app/i18n/strings.h"
#include "app/site.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "app/util/slice_utils.h"
#include "base/convert_to.h"
#include "doc/object_id.h"
#include "doc/slice.h"

namespace app {

// Moves the given slice by the dx and dy values
void offset(Slice* slice, int dx, int dy)
{
  for (auto it = slice->begin(); it != slice->end(); ++it) {
    auto* sk = (*it).value();
    gfx::Rect bounds = sk->bounds();
    bounds.offset(gfx::Point{ dx, dy });
    sk->setBounds(bounds);
  }
}

class DuplicateSliceCommand : public Command {
public:
  DuplicateSliceCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  ObjectId m_sliceId;
};

DuplicateSliceCommand::DuplicateSliceCommand() : Command(CommandId::DuplicateSlice())
{
}

void DuplicateSliceCommand::onLoadParams(const Params& params)
{
  std::string id = params.get("id");
  if (!id.empty())
    m_sliceId = ObjectId(base::convert_to<doc::ObjectId>(id));
  else
    m_sliceId = NullId;
}

bool DuplicateSliceCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite | ContextFlags::HasActiveLayer);
}

void DuplicateSliceCommand::onExecute(Context* context)
{
  std::vector<Slice*> selectedSlices;
  {
    const ContextReader reader(context);
    if (m_sliceId == NullId) {
      selectedSlices = get_selected_slices(reader.site());
      if (selectedSlices.empty())
        return;
    }
    else
      selectedSlices.push_back(reader.sprite()->slices().getById(m_sliceId));
  }

  ContextWriter writer(context);
  Sprite* sprite = writer.site().sprite();
  Doc* doc = static_cast<Doc*>(sprite->document());

  // Show the tooltip feedback only if we are not inside a transaction
  // (e.g. we can be already in a transaction if we are running in a
  // Lua script app.transaction()).
  const bool showTooltip = (doc->transaction() == nullptr);

  Tx tx(writer, Strings::commands_DuplicateSlice());
  doc->notifyBeforeSlicesDuplication();
  for (auto* s : selectedSlices) {
    Slice* slice = new Slice(*s);
    slice->setName(Strings::general_copy_of(slice->name()));
    // Offset a bit the duplicated slice to avoid overlapping
    offset(slice, 2, 2);

    tx(new cmd::AddSlice(sprite, slice));
    doc->notifySliceDuplicated(slice);
  }
  tx.commit();

  std::string sliceName;
  if (selectedSlices.size() == 1)
    sliceName = selectedSlices[0]->name();

  auto* statusBar = StatusBar::instance();
  if (showTooltip && statusBar && context->isUIAvailable()) {
    if (!sliceName.empty())
      statusBar->showTip(1000, Strings::duplicate_slice_x_duplicated(sliceName));
    else
      statusBar->showTip(1000, Strings::duplicate_slice_n_slices_duplicated(selectedSlices.size()));
  }
}

Command* CommandFactory::createDuplicateSliceCommand()
{
  return new DuplicateSliceCommand;
}

} // namespace app
