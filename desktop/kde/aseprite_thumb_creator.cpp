// Aseprite Desktop Integration Module
// Copyright (C) 2016  Gabriel Rauter
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "aseprite_thumb_creator.h"

#include <QImage>
#include <QProcess>
#include <QStringList>
#include <QFile>
#include <QCryptographicHash>
#include <cstdio>

extern "C"
{
  Q_DECL_EXPORT ThumbCreator *new_creator()
  {
    return new AsepriteThumbCreator();
  }
};

bool AsepriteThumbCreator::create(const QString& path, int width, int height, QImage& img ) {
  QProcess process;
  QStringList list;
  QString tmpFile = QString(QCryptographicHash::hash(path.toLocal8Bit(),QCryptographicHash::Md5).toHex());
  list << path << tmpFile;
  process.start(QString("aseprite-thumbnailer"), list);
  if (!process.waitForFinished()) return false;
  img.load(tmpFile);
  QFile::remove(tmpFile);
  return true;
}

AsepriteThumbCreator::Flags AsepriteThumbCreator::flags() const
{
  return DrawFrame;
}
