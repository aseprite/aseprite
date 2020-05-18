// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/remove_slice.h"
#include "app/cmd/set_slice_key.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "base/convert_to.h"
#include "doc/selected_objects.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/alert.h"
#include "ui/widget.h"

namespace app {

class RemoveSliceCommand : public Command {
public:
  RemoveSliceCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_sliceName;
  ObjectId m_sliceId;
};

RemoveSliceCommand::RemoveSliceCommand()
  : Command(CommandId::RemoveSlice(), CmdRecordableFlag)
{
}

void RemoveSliceCommand::onLoadParams(const Params& params)
{
  m_sliceName = params.get("name");

  std::string id = params.get("id");
  if (!id.empty())
    m_sliceId = ObjectId(base::convert_to<doc::ObjectId>(id));
  else
    m_sliceId = NullId;
}

bool RemoveSliceCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer);
}

void RemoveSliceCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  frame_t frame = reader.frame();
  SelectedObjects slicesToDelete;

  std::string sliceName;
  {
    Slice* slice = nullptr;
    if (!m_sliceName.empty())
      slice = sprite->slices().getByName(m_sliceName);
    else if (m_sliceId != NullId)
      slice = sprite->slices().getById(m_sliceId);

    if (slice)
      slicesToDelete.insert(slice->id());
    else
      slicesToDelete = reader.site()->selectedSlices();
  }

  // Nothing to delete
  if (slicesToDelete.empty())
    return;

  if (slicesToDelete.size() == 1) {
    Slice* slice = slicesToDelete.frontAs<Slice>();
    ASSERT(slice);
    if (slice)
    sliceName = slice->name();
  }

  {
    ContextWriter writer(reader);
    Doc* document(writer.document());
    Sprite* sprite(writer.sprite());
    Tx tx(writer.context(), "Remove Slice");

    for (auto slice : slicesToDelete.iterateAs<Slice>()) {
      ASSERT(slice);
      if (!slice)
        continue;

      if (slice->size() > 1) {
        tx(new cmd::SetSliceKey(slice, frame, SliceKey()));
      }
      else {
        tx(new cmd::RemoveSlice(sprite, slice));
      }
    }

    tx.commit();
    document->notifyGeneralUpdate();
  }

  StatusBar::instance()->invalidate();
  if (!sliceName.empty())
    StatusBar::instance()->showTip(
      1000, fmt::format("Slice '{}' removed", sliceName));
  else
    StatusBar::instance()->showTip(
      1000, fmt::format("{} slice(s) removed", slicesToDelete.size()));
}

Command* CommandFactory::createRemoveSliceCommand()
{
  return new RemoveSliceCommand;
}

} // namespace app
