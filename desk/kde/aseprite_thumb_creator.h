#ifndef _ASEPRITE_THUMBCREATOR_H_
#define _ASEPRITE_THUMBCREATOR_H_
#pragma once

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(LOG_ASEPRITE_THUMBCREATOR)

#include <kio/thumbcreator.h>

class AsepriteThumbCreator : public ThumbCreator {
  public:
    virtual bool create(const QString& path, int width, int height, QImage& img);
    virtual Flags flags() const;
};

#endif
