// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_suspended.h"

#include "doc/cel.h"
#include "doc/cel_data.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "fmt/format.h"

namespace app { namespace cmd {

void encode_suspended_object(doc::Object* obj, std::ostream& os)
{
  doc::Cel* cel = (obj && obj->type() == doc::ObjectType::Cel ? static_cast<doc::Cel*>(obj) :
                                                                nullptr);
  uint8_t flags = (obj ? 1 : 0);
  if (cel) {
    if (cel->data()->refs() == 0) {
      flags |= 2;
      if (cel->image())
        flags |= 4;
    }
  }

  os.put(flags);
  if (obj) {
    if (cel) {
      if (flags & 4)
        doc::write_image(os, cel->image());
      if (flags & 2)
        doc::write_celdata(os, cel->data());
    }

    switch (obj->type()) {
      case doc::ObjectType::Cel: doc::write_cel(os, cel); break;
      default:                   throw CmdException(fmt::format("Cannot encode object type {}", (int)obj->type()));
    }
  }
}

doc::Object* decode_suspended_object(const doc::ObjectType type,
                                     std::istream& is,
                                     const doc::IdMapperIO& mapper,
                                     doc::SubObjectsIO* subObjects)
{
  doc::Object* obj = nullptr;
  auto flags = is.get();
  if (flags & 1) {
    if (flags & 4) {
      doc::ImageRef image(doc::read_image(is, mapper));
      if (image) {
        image->suspendObject();
        subObjects->addImageRef(image);
      }
    }
    if (flags & 2) {
      doc::CelDataRef celData(doc::read_celdata(is, mapper, subObjects));
      if (celData) {
        celData->suspendObject();
        subObjects->addCelDataRef(celData);
      }
    }

    switch (type) {
      case doc::ObjectType::Cel: obj = doc::read_cel(is, mapper, subObjects); break;
      default:                   throw CmdException(fmt::format("Cannot decode object type {}", (int)type));
    }

    if (obj)
      obj->suspendObject();
  }
  return obj;
}

}} // namespace app::cmd
