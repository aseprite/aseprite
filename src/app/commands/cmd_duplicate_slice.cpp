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
#include "app/site.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/util/slice_utils.h"
#include "base/convert_to.h"
#include "doc/object_id.h"
#include "doc/slice.h"

namespace app {

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

DuplicateSliceCommand::DuplicateSliceCommand()
  : Command(CommandId::DuplicateSlice(), CmdRecordableFlag)
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

  Editor* editor = Editor::activeEditor();
  ContextWriter writer(context);
  Tx tx(writer, "Duplicate Slice");
  Sprite* sprite = writer.site().sprite();
  editor->clearSlicesSelection();
  for (auto* s : selectedSlices) {
    Slice* slice = new Slice(*s);
    slice->setName(get_unique_slice_name(sprite, s->name()));
    tx(new cmd::AddSlice(sprite, slice));
    editor->selectSlice(slice);
  }
  tx.commit();
}

Command* CommandFactory::createDuplicateSliceCommand()
{
  return new DuplicateSliceCommand;
}

} // namespace app
