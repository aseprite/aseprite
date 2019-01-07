// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_slice_key.h"
#include "app/cmd/set_slice_name.h"
#include "app/cmd/set_user_data.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/slice_window.h"
#include "base/convert_to.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app {

using namespace ui;

class SlicePropertiesCommand : public Command {
public:
  SlicePropertiesCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_sliceName;
  ObjectId m_sliceId;
};

SlicePropertiesCommand::SlicePropertiesCommand()
  : Command(CommandId::SliceProperties(), CmdUIOnlyFlag)
  , m_sliceId(NullId)
{
}

void SlicePropertiesCommand::onLoadParams(const Params& params)
{
  m_sliceName = params.get("name");

  std::string id = params.get("id");
  if (!id.empty())
    m_sliceId = ObjectId(base::convert_to<doc::ObjectId>(id));
  else
    m_sliceId = NullId;
}

bool SlicePropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void SlicePropertiesCommand::onExecute(Context* context)
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

  const doc::SliceKey* key = foundSlice->getByFrame(frame);
  if (!key)
    return;

  SliceWindow window(sprite, foundSlice, frame);
  if (!window.show())
    return;

  {
    ContextWriter writer(reader, 500);
    Tx tx(writer.context(), "Slice Properties");
    Slice* slice = const_cast<Slice*>(foundSlice);

    std::string name = window.nameValue();

    if (slice->name() != name)
      tx(new cmd::SetSliceName(slice, name));

    if (slice->userData() != window.userDataValue())
      tx(new cmd::SetUserData(slice, window.userDataValue()));

    if (key->bounds() != window.boundsValue() ||
        key->center() != window.centerValue() ||
        key->pivot() != window.pivotValue()) {
      SliceKey newKey = *key;
      newKey.setBounds(window.boundsValue());
      newKey.setCenter(window.centerValue());
      newKey.setPivot(window.pivotValue());
      tx(new cmd::SetSliceKey(slice, frame, newKey));
    }

    tx.commit();
    writer.document()->notifyGeneralUpdate();
  }
}

Command* CommandFactory::createSlicePropertiesCommand()
{
  return new SlicePropertiesCommand;
}

} // namespace app
