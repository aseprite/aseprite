// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_cel_data.h"

#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_io.h"
#include "doc/image_ref.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "doc/subobjects_io.h"

namespace app { namespace cmd {

using namespace doc;

SetCelData::SetCelData(Cel* cel, const CelDataRef& newData)
  : WithCel(cel)
  , m_oldDataId(cel->data()->id())
  , m_oldImageId(cel->image()->id())
  , m_newDataId(newData->id())
  , m_newData(newData)
{
}

void SetCelData::onExecute()
{
  Cel* cel = this->cel();
  if (!cel->links())
    createCopy();

  cel->setDataRef(m_newData);
  cel->incrementVersion();
  m_newData.reset();
}

void SetCelData::onUndo()
{
  Cel* cel = this->cel();

  if (m_dataCopy) {
    ASSERT(!cel->sprite()->getCelDataRef(m_oldDataId));
    m_dataCopy->setId(m_oldDataId);
    m_dataCopy->image()->setId(m_oldImageId);

    cel->setDataRef(m_dataCopy);
    m_dataCopy.reset();
  }
  else {
    CelDataRef oldData = cel->sprite()->getCelDataRef(m_oldDataId);
    ASSERT(oldData);
    cel->setDataRef(oldData);
  }

  cel->incrementVersion();
}

void SetCelData::onRedo()
{
  Cel* cel = this->cel();
  if (!cel->links())
    createCopy();

  CelDataRef newData = cel->sprite()->getCelDataRef(m_newDataId);
  ASSERT(newData);
  cel->setDataRef(newData);
  cel->incrementVersion();
}

void SetCelData::createCopy()
{
  Cel* cel = this->cel();

  ASSERT(!m_dataCopy);
  m_dataCopy.reset(new CelData(*cel->data()));
  m_dataCopy->setImage(ImageRef(Image::createCopy(cel->image())), cel->layer());
}

}} // namespace app::cmd
