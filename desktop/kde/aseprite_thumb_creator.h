// Aseprite Desktop Integration Module
// Copyright (C) 2016  Gabriel Rauter
//
// Dual licensed under the MIT and GPL 2 licenses.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation or the MIT License
// (https://opensource.org/licenses/MIT).

#ifndef _ASEPRITE_THUMBCREATOR_H_
#define _ASEPRITE_THUMBCREATOR_H_
#pragma once

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(LOG_ASEPRITE_THUMBCREATOR)

#include <kio/thumbcreator.h>

class AsepriteThumbCreator : public ThumbCreator {
  public:
    bool create(const QString& path, int width, int height, QImage& img) override;
    Flags flags() const override;
};

#endif
