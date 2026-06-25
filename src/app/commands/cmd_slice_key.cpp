// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_slice.h"
#include "app/cmd/set_slice_key.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/context_flags.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "app/util/slice_utils.h"
#include "base/convert_to.h"
#include "doc/object_id.h"
#include "doc/slice.h"
#include "doc/sprite.h"

#include <string>
#include <vector>

namespace app {

static doc::Slice::iterator exact_key_iterator(Slice* slice, const frame_t frame)
{
  auto it = slice->getIteratorByFrame(frame);
  if (it != slice->end() && it->frame() == frame)
    return it;

  return slice->end();
}

static std::vector<Slice*> get_command_slices(const ContextReader& reader,
                                              const ObjectId sliceId,
                                              const std::string& sliceName)
{
  std::vector<Slice*> slices;
  if (!sliceName.empty()) {
    if (auto* slice = reader.sprite()->slices().getByName(sliceName))
      slices.push_back(slice);
  }
  else if (sliceId == NullId) {
    slices = get_selected_slices(reader.site());
  }
  else if (auto* slice = reader.sprite()->slices().getById(sliceId)) {
    slices.push_back(slice);
  }
  return slices;
}

class AddSliceKeyCommand : public Command {
public:
  AddSliceKeyCommand() : Command(CommandId::AddSliceKey()) {}

protected:
  void onLoadParams(const Params& params) override
  {
    m_sliceName = params.get("name");

    const std::string id = params.get("id");
    if (!id.empty())
      m_sliceId = ObjectId(base::convert_to<doc::ObjectId>(id));
    else
      m_sliceId = NullId;
  }

  bool onEnabled(Context* context) override
  {
    if (!Preferences::instance().slices.useKeys())
      return false;

    return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                               ContextFlags::HasActiveSprite | ContextFlags::HasActiveLayer);
  }

  void onExecute(Context* context) override
  {
    const ContextReader reader(context);
    const frame_t frame = reader.frame();
    if (frame == 0)
      return;

    auto slices = get_command_slices(reader, m_sliceId, m_sliceName);
    if (slices.empty())
      return;

    ContextWriter writer(reader);
    Tx tx(writer, "Add Slice Key");
    int added = 0;

    for (Slice* slice : slices) {
      if (!slice || exact_key_iterator(slice, frame) != slice->end())
        continue;

      if (const SliceKey* key = slice->getByFrame(frame)) {
        tx(new cmd::SetSliceKey(slice, frame, *key));
        ++added;
      }
    }

    if (added > 0) {
      tx.commit();
      writer.document()->notifyGeneralUpdate();
      if (auto* statusBar = StatusBar::instance())
        statusBar->showTip(1000, "Slice key added");
    }
  }

private:
  ObjectId m_sliceId = NullId;
  std::string m_sliceName;
};

class RemoveSliceKeyCommand : public Command {
public:
  RemoveSliceKeyCommand() : Command(CommandId::RemoveSliceKey()) {}

protected:
  void onLoadParams(const Params& params) override
  {
    m_sliceName = params.get("name");

    const std::string id = params.get("id");
    if (!id.empty())
      m_sliceId = ObjectId(base::convert_to<doc::ObjectId>(id));
    else
      m_sliceId = NullId;
  }

  bool onEnabled(Context* context) override
  {
    if (!Preferences::instance().slices.useKeys())
      return false;

    return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                               ContextFlags::HasActiveSprite | ContextFlags::HasActiveLayer);
  }

  void onExecute(Context* context) override
  {
    const ContextReader reader(context);
    const frame_t frame = reader.frame();
    auto slices = get_command_slices(reader, m_sliceId, m_sliceName);
    if (slices.empty())
      return;

    ContextWriter writer(reader);
    Sprite* sprite = writer.sprite();
    Tx tx(writer, "Remove Slice Key");
    int removed = 0;

    for (Slice* slice : slices) {
      if (!slice)
        continue;

      if (slice->size() <= 1) {
        tx(new cmd::RemoveSlice(sprite, slice));
        ++removed;
        continue;
      }

      const frame_t keyFrame = effective_slice_key_frame(slice, frame);
      if (keyFrame == 0) {
        auto first = slice->begin();
        auto second = first;
        ++second;
        if (second == slice->end()) {
          tx(new cmd::RemoveSlice(sprite, slice));
        }
        else {
          const frame_t promotedFrame = second->frame();
          const SliceKey promotedKey = *second->value();
          tx(new cmd::SetSliceKey(slice, 0, promotedKey));
          tx(new cmd::SetSliceKey(slice, promotedFrame, SliceKey()));
        }
      }
      else {
        tx(new cmd::SetSliceKey(slice, keyFrame, SliceKey()));
      }
      ++removed;
    }

    if (removed > 0) {
      tx.commit();
      writer.document()->notifyGeneralUpdate();
      if (auto* statusBar = StatusBar::instance())
        statusBar->showTip(1000, "Slice key removed");
    }
  }

private:
  ObjectId m_sliceId = NullId;
  std::string m_sliceName;
};

Command* CommandFactory::createAddSliceKeyCommand()
{
  return new AddSliceKeyCommand;
}

Command* CommandFactory::createRemoveSliceKeyCommand()
{
  return new RemoveSliceKeyCommand;
}

} // namespace app
