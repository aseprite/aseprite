// Aseprite Desktop Integration Module
// Copyright (C) 2016  Gabriel Rauter
//
// Licensed under the the MIT License (https://opensource.org/licenses/MIT).

#include "aseprite_thumb_creator.h"

#include <QCryptographicHash>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QStringList>
#include <cstdio>

extern "C" {
Q_DECL_EXPORT ThumbCreator* new_creator()
{
  return new AsepriteThumbCreator();
}
};

bool AsepriteThumbCreator::create(const QString& path, int width, int height, QImage& img)
{
  QProcess process;
  QStringList list;
  QString tmpFile = QString(
    QCryptographicHash::hash(path.toLocal8Bit(), QCryptographicHash::Md5).toHex());
  list << path << tmpFile;
  process.start(QString("aseprite-thumbnailer"), list);
  if (!process.waitForFinished())
    return false;
  img.load(tmpFile);
  QFile::remove(tmpFile);
  return true;
}

AsepriteThumbCreator::Flags AsepriteThumbCreator::flags() const
{
  return DrawFrame;
}
