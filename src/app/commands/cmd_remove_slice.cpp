// Aseprite
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
#include "app/ui/status_bar.h"
#include "app/tx.h"
#include "base/convert_to.h"
#include "doc/slice.h"
#include "doc/sprite.h"
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
  const Slice* foundSlice = nullptr;

  if (!m_sliceName.empty())
    foundSlice = sprite->slices().getByName(m_sliceName);
  else if (m_sliceId != NullId)
    foundSlice = sprite->slices().getById(m_sliceId);

  if (!foundSlice)
    return;

  std::string sliceName = foundSlice->name();

  {
    ContextWriter writer(reader, 500);
    Doc* document(writer.document());
    Sprite* sprite(writer.sprite());
    Tx tx(writer.context(), "Remove Slice");
    Slice* slice = const_cast<Slice*>(foundSlice);

    if (slice->size() > 1) {
      tx(new cmd::SetSliceKey(slice, frame, SliceKey()));
    }
    else {
      tx(new cmd::RemoveSlice(sprite, slice));
    }
    tx.commit();
    document->notifyGeneralUpdate();
  }

  StatusBar::instance()->invalidate();
  if (!sliceName.empty())
    StatusBar::instance()->showTip(1000, "Slice '%s' removed", sliceName.c_str());
  else
    StatusBar::instance()->showTip(1000, "Slice removed");
}

Command* CommandFactory::createRemoveSliceCommand()
{
  return new RemoveSliceCommand;
}

} // namespace app
