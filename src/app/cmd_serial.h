// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SERIAL_H_INCLUDED
#define APP_CMD_SERIAL_H_INCLUDED
#pragma once

#include "app/cmdtype.h"
#include "base/buffer.h"
#include "base/ints.h"
#include "dio/file_interface.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/object_id.h"
#include "doc/subobjects_io.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace app {

class serial_span;

class CmdSerial : public doc::SubObjectsIO {
public:
  enum Coding { Decoding = 0, Encoding = 1 };
  using IdsMap = std::map<doc::ObjectId, doc::ObjectId>;

  CmdSerial(doc::Sprite* s, dio::FileInterface* f, Coding coding)
    : m_sprite(s)
    , m_f(f)
    , m_coding(coding)
  {
  }
  virtual ~CmdSerial() {}

  void setIdsMap(const IdsMap& idsMap) { m_idsMap = idsMap; }

  bool decoding() const { return m_coding == Decoding; }
  bool encoding() const { return m_coding == Encoding; }

  virtual void cmdtype(cmdtype_t& t) = 0;
  virtual void unused() {}
  virtual void txBegin() = 0;
  virtual void txEnd() = 0;
  virtual void seqBegin() = 0;
  virtual void seqEnd() = 0;
  virtual void seqSeparator() = 0;
  virtual void curStateBegin() = 0;
  virtual void curStateEnd() = 0;
  virtual void serializeObjectId(doc::ObjectId& v) = 0;
  virtual void operator()(uint8_t& v) = 0;
  virtual void operator()(uint32_t& v) = 0;
  virtual void operator()(gfx::Point& pt) = 0;
  virtual void operator()(gfx::Rect& rc) = 0;
  virtual void operator()(gfx::Region& rg) = 0;
  virtual void operator()(serial_span& buf) = 0;

  void operator()(doc::frame_t& v) { operator()((uint32_t&)v); }
  void operator()(base::buffer& buf);
  void operator()(std::stringstream& ss);
  void operator()(doc::ImageRef& image);

  // SubObjectsIO impl
  doc::Sprite* sprite() const override { return m_sprite; }
  void addImageRef(const doc::ImageRef& image) override;
  void addCelDataRef(const doc::CelDataRef& celdata) override;
  doc::ImageRef getImageRef(doc::ObjectId imageId) override;
  doc::CelDataRef getCelDataRef(doc::ObjectId celdataId) override;

protected:
  doc::Sprite* m_sprite = nullptr;
  dio::FileInterface* m_f = nullptr;
  Coding m_coding = Coding::Decoding;
  IdsMap m_idsMap;
};

class TextEncCmdSerial : public CmdSerial {
public:
  TextEncCmdSerial(doc::Sprite* s, dio::FileInterface* f) : CmdSerial(s, f, Coding::Encoding) {}
  void cmdtype(cmdtype_t& t) override;
  void txBegin() override;
  void txEnd() override;
  void seqBegin() override;
  void seqEnd() override;
  void seqSeparator() override;
  void curStateBegin() override;
  void curStateEnd() override;
  void serializeObjectId(doc::ObjectId& v) override;
  void operator()(uint8_t& v) override;
  void operator()(uint32_t& v) override;
  void operator()(gfx::Point& pt) override;
  void operator()(gfx::Rect& rc) override;
  void operator()(gfx::Region& rg) override;
  void operator()(serial_span& buf) override;

private:
  bool m_sep = false;
};

class TextDecCmdSerial : public CmdSerial {
public:
  TextDecCmdSerial(doc::Sprite* s, dio::FileInterface* f) : CmdSerial(s, f, Coding::Decoding) {}
  void cmdtype(cmdtype_t& t) override;
  void unused() override;
  void txBegin() override;
  void txEnd() override;
  void seqBegin() override;
  void seqEnd() override;
  void seqSeparator() override;
  void curStateBegin() override;
  void curStateEnd() override;
  void serializeObjectId(doc::ObjectId& v) override;
  void operator()(uint8_t& v) override;
  void operator()(uint32_t& v) override;
  void operator()(gfx::Point& pt) override;
  void operator()(gfx::Rect& rc) override;
  void operator()(gfx::Region& rg) override;
  void operator()(serial_span& buf) override;

private:
  int curChar() { return m_chr; }
  int nextChar();
  void skipWhitespace();
  std::string nextToken();
  void unusedToken();
  void expectToken(const char* expected, const char* error);
  int parseInt();

  int m_chr = 0;
  std::string m_prevTok;
  std::string m_tok;
};

} // namespace app

#endif
