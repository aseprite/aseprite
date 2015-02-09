/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_CMD_SET_CEL_DATA_H_INCLUDED
#define APP_CMD_SET_CEL_DATA_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "doc/cel_data.h"

#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class SetCelData : public Cmd
                   , public WithCel {
  public:
    SetCelData(Cel* cel, const CelDataRef& newData);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (m_dataCopy ? m_dataCopy->getMemSize(): 0);
    }

  private:
    void createCopy();

    ObjectId m_oldDataId;
    ObjectId m_oldImageId;
    ObjectId m_newDataId;
    CelDataRef m_dataCopy;

    // Reference used only to keep the copy of the new CelData from
    // the SetCelData() ctor until the SetCelData::onExecute() call.
    // Then the reference is not used anymore.
    CelDataRef m_newData;
  };

} // namespace cmd
} // namespace app

#endif
