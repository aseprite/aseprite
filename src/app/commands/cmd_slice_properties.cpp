// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
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
  SelectedObjects slices;

  {
    Slice* slice = nullptr;
    if (!m_sliceName.empty())
      slice = sprite->slices().getByName(m_sliceName);
    else if (m_sliceId != NullId)
      slice = sprite->slices().getById(m_sliceId);

    if (slice)
      slices.insert(slice->id());
    else
      slices = reader.site()->selectedSlices();
  }

  // Nothing to delete
  if (slices.empty())
    return;

  SliceWindow window(sprite, slices, frame);
  if (!window.show())
    return;

  {
    const SliceWindow::Mods mods = window.modifiedFields();
    ContextWriter writer(reader);
    Tx tx(writer.context(), "Slice Properties");

    for (Slice* slice : slices.iterateAs<Slice>()) {
      // Change name
      if (mods & SliceWindow::kName) {
        std::string name = window.nameValue();
        if (slice->name() != name)
          tx(new cmd::SetSliceName(slice, name));
      }

      // Change user data
      if ((mods & SliceWindow::kUserData) &&
          slice->userData() != window.userDataValue()) {
        tx(new cmd::SetUserData(slice, window.userDataValue()));
      }

      // Change slice properties
      const doc::SliceKey* key = slice->getByFrame(frame);
      if (!key)
        continue;

      SliceKey newKey = *key;
      gfx::Rect newBounds = newKey.bounds();
      gfx::Rect newCenter = newKey.center();
      gfx::Point newPivot = newKey.pivot();
      if (mods & SliceWindow::kBoundsX) newBounds.x = window.boundsValue().x;
      if (mods & SliceWindow::kBoundsY) newBounds.y = window.boundsValue().y;
      if (mods & SliceWindow::kBoundsW) newBounds.w = window.boundsValue().w;
      if (mods & SliceWindow::kBoundsH) newBounds.h = window.boundsValue().h;
      if (mods & SliceWindow::kCenterX) newCenter.x = window.centerValue().x;
      if (mods & SliceWindow::kCenterY) newCenter.y = window.centerValue().y;
      if (mods & SliceWindow::kCenterW) newCenter.w = window.centerValue().w;
      if (mods & SliceWindow::kCenterH) newCenter.h = window.centerValue().h;
      if (mods & SliceWindow::kPivotX) newPivot.x = window.pivotValue().x;
      if (mods & SliceWindow::kPivotY) newPivot.y = window.pivotValue().y;
      newKey.setBounds(newBounds);
      newKey.setCenter(newCenter);
      newKey.setPivot(newPivot);

      if (key->bounds() != newKey.bounds() ||
          key->center() != newKey.center() ||
          key->pivot() != newKey.pivot()) {
        tx(new cmd::SetSliceKey(slice, frame, newKey));
      }
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
