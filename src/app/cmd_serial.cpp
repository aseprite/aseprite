// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd_serial.h"

#include "app/cmd/transaction.h"
#include "app/cmd_exception.h"
#include "app/serial_span.h"
#include "base/base64.h"
#include "base/convert_to.h"
#include "base/exception.h"
#include "doc/image_io.h"
#include "fmt/format.h"

namespace app {

//////////////////////////////////////////////////////////////////////
// CmdSerial

void CmdSerial::operator()(base::buffer& buf)
{
  serial_span span;
  span.init_with_buffer_view(buf);
  operator()(span);
}

void CmdSerial::operator()(std::stringstream& ss)
{
  serial_span span;
  if (encoding()) {
    span.init_copying_stringstream(ss);
    operator()(span);
  }
  else {
    span.init_with_new_buffer();
    operator()(span);
    ss.write((const char*)span.data(), span.size());
  }
}

void CmdSerial::operator()(doc::ImageRef& image)
{
  uint8_t valid = (image ? 1 : 0);
  operator()(valid);

  if (encoding()) {
    if (image) {
      std::stringstream stream;
      doc::write_image(stream, image.get());
      operator()(stream);
    }
  }
  else if (valid) {
    std::stringstream stream;
    operator()(stream);

    image.reset(doc::read_image(stream, doc::IdFromStreamMapperIO()));
  }
}

doc::ObjectId CmdSerial::mapId(const doc::ObjectId id, const doc::ObjectType type) const
{
  doc::ObjectId result;
  auto it = m_idsMap.find(id);
  if (it != m_idsMap.end()) {
    result = it->second;
  }
  else {
    result = m_idsMap[id] = doc::new_id();
  }
  return result;
}

void CmdSerial::addImageRef(const doc::ImageRef& image)
{
  ASSERT(image->isSuspended());
  m_images[image->id()] = image;
}

void CmdSerial::addCelDataRef(const doc::CelDataRef& celdata)
{
  ASSERT(celdata->isSuspended());
  m_celdatas[celdata->id()] = celdata;
}

doc::ImageRef CmdSerial::getImageRef(doc::ObjectId imageId)
{
  auto it2 = m_idsMap.find(imageId);
  if (it2 == m_idsMap.end())
    throw CmdException(fmt::format("Non-mapped Image reference {}", imageId));

  imageId = it2->second;
  auto it = m_images.find(imageId);
  if (it != m_images.end())
    return it->second;

  doc::ImageRef image(doc::get<doc::Image>(imageId));
  if (!image)
    throw CmdException(fmt::format("Non-existent Image reference {}", imageId));
  return image;
}

doc::CelDataRef CmdSerial::getCelDataRef(doc::ObjectId celdataId)
{
  auto it2 = m_idsMap.find(celdataId);
  if (it2 == m_idsMap.end())
    throw CmdException("Non-mapped CelData reference");

  celdataId = it2->second;
  auto it = m_celdatas.find(celdataId);
  if (it != m_celdatas.end())
    return it->second;

  doc::CelDataRef celData(doc::get<doc::CelData>(celdataId));
  if (!celData)
    throw CmdException("Non-existent CelData reference");
  return celData;
}

//////////////////////////////////////////////////////////////////////
// TextEncCmdSerial

void TextEncCmdSerial::cmdtype(cmdtype_t& t)
{
  m_f->write8((t >> 24) & 0xff);
  m_f->write8((t >> 16) & 0xff);
  if (((t >> 8) & 0xff) != ' ') {
    m_f->write8((t >> 8) & 0xff);
    if ((t & 0xff) != ' ') {
      m_f->write8(t & 0xff);
    }
  }
  m_sep = true;
}

void TextEncCmdSerial::txBegin()
{
  if (m_writeCurState) {
    m_writeCurState = false;
    m_f->write8('*');
  }

  cmdtype_t t = cmd::CmdTransaction::kType;
  cmdtype(t);
  m_sep = true;
}

void TextEncCmdSerial::txEnd()
{
  m_f->write8('\n');
}

void TextEncCmdSerial::seqBegin()
{
  if (m_sep)
    m_f->write8(' ');

  m_f->write8('[');
  m_sep = false;
}

void TextEncCmdSerial::seqEnd()
{
  m_f->write8(']');
  m_sep = false;
}

void TextEncCmdSerial::seqSeparator()
{
  m_f->write8(',');
  m_sep = false;
}

void TextEncCmdSerial::curStateMark()
{
  m_writeCurState = true;
  m_beforeCurState = false;
}

void TextEncCmdSerial::serializeObjectId(doc::ObjectId& v)
{
  operator()(v);
}

void TextEncCmdSerial::operator()(bool& v)
{
  writeString(fmt::format("{}", (int)v));
}

void TextEncCmdSerial::operator()(int& v)
{
  writeString(fmt::format("{}", v));
}

void TextEncCmdSerial::operator()(uint8_t& v)
{
  writeString(fmt::format("{}", (int)v));
}

void TextEncCmdSerial::operator()(uint32_t& v)
{
  writeString(fmt::format("{}", v));
}

void TextEncCmdSerial::operator()(gfx::Point& pt)
{
  writeString(fmt::format("({} {})", pt.x, pt.y));
}

void TextEncCmdSerial::operator()(gfx::Rect& rc)
{
  writeString(fmt::format("({} {} {} {})", rc.x, rc.y, rc.w, rc.h));
}

void TextEncCmdSerial::operator()(gfx::Region& rg)
{
  if (m_sep)
    m_f->write8(' ');

  m_f->write8('(');
  m_sep = false;
  for (auto& rc : rg)
    operator()(rc);
  m_f->write8(')');
  m_sep = true;
}

void TextEncCmdSerial::operator()(serial_span& buf)
{
  if (m_sep)
    m_f->write8(' ');

  std::string encoded;
  base::encode_base64((const char*)buf.data(), buf.size(), encoded);
  ASSERT(std::string_view((const char*)buf.data(), buf.size()) == base::decode_base64s(encoded));

  m_f->writeBytes((const uint8_t*)encoded.data(), encoded.size());
  m_sep = true;
}

void TextEncCmdSerial::writeString(const std::string& v)
{
  if (m_sep)
    m_f->write8(' ');

  m_f->writeBytes((const uint8_t*)v.c_str(), v.size());
  m_sep = true;
}

//////////////////////////////////////////////////////////////////////
// TextDecCmdSerial

void TextDecCmdSerial::cmdtype(cmdtype_t& t)
{
  auto tok = nextToken();

  // Current state marker
  if (tok == "*") {
    m_beforeCurState = false;
    tok = nextToken();
  }

  t = make_cmdtype(tok.size() > 0 ? tok[0] : ' ',
                   tok.size() > 1 ? tok[1] : ' ',
                   tok.size() > 2 ? tok[2] : ' ',
                   tok.size() > 3 ? tok[3] : ' ');
}

void TextDecCmdSerial::unused()
{
  unusedToken();
}

void TextDecCmdSerial::txBegin()
{
  cmdtype_t t = 0;
  cmdtype(t);
  if (t != cmd::CmdTransaction::kType)
    throw std::runtime_error("missing Tx to start a transaction");
}

void TextDecCmdSerial::txEnd()
{
  // Do nothing
}

void TextDecCmdSerial::seqBegin()
{
  expectToken("[", "missing '[' char to start a sequence");
}

void TextDecCmdSerial::seqEnd()
{
  expectToken("]", "missing ']' char to end a sequence");
}

void TextDecCmdSerial::seqSeparator()
{
  auto tok = nextToken();
  if (tok != ",")
    unusedToken();
}

void TextDecCmdSerial::curStateMark()
{
  // Do nothing
}

void TextDecCmdSerial::serializeObjectId(doc::ObjectId& v)
{
  auto id = parseInt();
  auto it = m_idsMap.find(id);
  if (it != m_idsMap.end()) {
    v = it->second;
  }
  else {
    v = m_idsMap[id] = doc::new_id();
  }
}

void TextDecCmdSerial::operator()(bool& v)
{
  v = parseInt();
}

void TextDecCmdSerial::operator()(int& v)
{
  v = parseInt();
}

void TextDecCmdSerial::operator()(uint8_t& v)
{
  v = parseInt();
}

void TextDecCmdSerial::operator()(uint32_t& v)
{
  v = parseInt();
}

void TextDecCmdSerial::operator()(gfx::Point& pt)
{
  expectToken("(", "missing '(' to start a point");
  pt.x = parseInt();
  pt.y = parseInt();
  expectToken(")", "missing ')' to end a point");
}

void TextDecCmdSerial::operator()(gfx::Rect& rc)
{
  expectToken("(", "missing '(' to start a rectangle");
  rc.x = parseInt();
  rc.y = parseInt();
  rc.w = parseInt();
  rc.h = parseInt();
  expectToken(")", "missing ')' to end a rectangle");
}

void TextDecCmdSerial::operator()(gfx::Region& rg)
{
  expectToken("(", "missing '(' to start a region");
  while (m_f->ok()) {
    auto tok = nextToken();
    if (tok == ")")
      break;

    unusedToken();

    gfx::Rect rc;
    operator()(rc);
    rg |= gfx::Region(rc);
  }
}

void TextDecCmdSerial::operator()(serial_span& span)
{
  base::buffer encoded;
  skipWhitespace();
  while (m_f->ok()) {
    int chr = curChar();
    if (std::isalnum(chr) || chr == '+' || chr == '/' || chr == '=')
      encoded.push_back(chr);
    else
      break;
    nextChar();
  }
  span.copy_to_buffer(base::decode_base64(encoded));
}

int TextDecCmdSerial::nextChar()
{
  m_chr = m_f->read8();
  return m_chr;
}

void TextDecCmdSerial::skipWhitespace()
{
  if (!curChar())
    nextChar();
  while (std::isspace(curChar()) && m_f->ok())
    nextChar();
}

std::string TextDecCmdSerial::nextToken()
{
  if (!m_prevTok.empty()) {
    auto prev = m_prevTok;
    m_prevTok.clear();
    return prev;
  }

  if (!m_f->ok())
    throw std::runtime_error("token expected but EOF reached");

  m_tok.clear();
  skipWhitespace();
  if (curChar() != '-' && std::ispunct(curChar())) {
    m_tok.push_back(curChar());
    nextChar();
  }
  else if (curChar() == '-' || std::isalnum(curChar())) {
    if (curChar() == '-') {
      m_tok.push_back(curChar());
      nextChar();
    }
    while (m_f->ok()) {
      if (std::isalnum(curChar()))
        m_tok.push_back(curChar());
      else
        break;
      nextChar();
    }
  }
  return m_tok;
}

void TextDecCmdSerial::unusedToken()
{
  m_prevTok = m_tok;
}

void TextDecCmdSerial::expectToken(const char* expected, const char* error)
{
  auto tok = nextToken();
  if (tok != expected)
    throw std::runtime_error(error);
}

int TextDecCmdSerial::parseInt()
{
  auto tok = nextToken();
  return base::convert_to<int>(tok);
}

} // namespace app
