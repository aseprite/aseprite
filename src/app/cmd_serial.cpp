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
#include "app/serial_span.h"
#include "base/base64.h"
#include "base/convert_to.h"
#include "base/exception.h"
#include "doc/object_io.h"
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
      doc::write_object<doc::ImageRef>(stream, image);
      operator()(stream);
    }
  }
  else if (valid) {
    std::stringstream stream;
    operator()(stream);

    image = doc::read_object<doc::ImageRef>(stream, this);
  }
}

void CmdSerial::addImageRef(const doc::ImageRef& image)
{
  PRINTARGS("TODO CmdSerial::addImageRef", image.get());
}

void CmdSerial::addCelDataRef(const doc::CelDataRef& celdata)
{
  PRINTARGS("TODO CmdSerial::addCelDataRef", celdata.get());
}

doc::ImageRef CmdSerial::getImageRef(doc::ObjectId imageId)
{
  auto it = m_idsMap.find(imageId);
  if (it == m_idsMap.end()) {
    PRINTARGS("Image ID %d", imageId, "not found");
  }
  return doc::ImageRef(doc::get<doc::Image>(it->second));
}

doc::CelDataRef CmdSerial::getCelDataRef(doc::ObjectId celdataId)
{
  auto it = m_idsMap.find(celdataId);
  if (it == m_idsMap.end()) {
    PRINTARGS("CelData ID", celdataId, "not found");
  }
  return doc::CelDataRef(doc::get<doc::CelData>(it->second));
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

void TextEncCmdSerial::curStateBegin()
{
  // m_f->write8('*');
}

void TextEncCmdSerial::curStateEnd()
{
}

void TextEncCmdSerial::serializeObjectId(doc::ObjectId& v)
{
  operator()(v);
}

void TextEncCmdSerial::operator()(uint8_t& v)
{
  if (m_sep)
    m_f->write8(' ');

  std::string res = fmt::format("{}", (int)v);
  m_f->writeBytes((const uint8_t*)res.c_str(), res.size());
  m_sep = true;
}

void TextEncCmdSerial::operator()(uint32_t& v)
{
  if (m_sep)
    m_f->write8(' ');

  std::string res = fmt::format("{}", v);
  m_f->writeBytes((const uint8_t*)res.c_str(), res.size());
  m_sep = true;
}

void TextEncCmdSerial::operator()(gfx::Point& pt)
{
  if (m_sep)
    m_f->write8(' ');

  std::string res = fmt::format("({} {})", pt.x, pt.y);
  m_f->writeBytes((const uint8_t*)res.c_str(), res.size());
  m_sep = true;
}

void TextEncCmdSerial::operator()(gfx::Rect& rc)
{
  if (m_sep)
    m_f->write8(' ');

  std::string res = fmt::format("({} {} {} {})", rc.x, rc.y, rc.w, rc.h);
  m_f->writeBytes((const uint8_t*)res.c_str(), res.size());
  m_sep = true;
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
  m_f->writeBytes((const uint8_t*)encoded.data(), encoded.size());
  PRINTARGS("save serial_span encoded=",
            encoded.size(),
            " (original",
            buf.size(),
            ")",
            "base64=",
            encoded);
  m_sep = true;
}

//////////////////////////////////////////////////////////////////////
// TextDecCmdSerial

void TextDecCmdSerial::cmdtype(cmdtype_t& t)
{
  auto tok = nextToken();
  t = make_cmdtype(tok.size() > 0 ? tok[0] : ' ',
                   tok.size() > 1 ? tok[1] : ' ',
                   tok.size() > 2 ? tok[2] : ' ',
                   tok.size() > 3 ? tok[3] : ' ');
  // PRINTARGS("cmdtype tok=", tok);
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

void TextDecCmdSerial::curStateBegin()
{
  // TODO
}

void TextDecCmdSerial::curStateEnd()
{
  // TODO
}

void TextDecCmdSerial::serializeObjectId(doc::ObjectId& v)
{
  auto id = parseInt();
  auto it = m_idsMap.find(id);
  if (it != m_idsMap.end()) {
    v = it->second;
    PRINTARGS("serializeObjectId read id=", id, "id in memory", v);
  }
  else {
    v = m_idsMap[id] = doc::new_id();
    PRINTARGS("new ID for future object id=", id, "id in memory", v);
  }
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
  // PRINTARGS("  operator()(Point)");
  expectToken("(", "missing '(' to start a point");
  pt.x = parseInt();
  pt.y = parseInt();
  expectToken(")", "missing ')' to end a point");
}

void TextDecCmdSerial::operator()(gfx::Rect& rc)
{
  // PRINTARGS("  operator()(Rect)");
  expectToken("(", "missing '(' to start a rectangle");
  rc.x = parseInt();
  rc.y = parseInt();
  rc.w = parseInt();
  rc.h = parseInt();
  expectToken(")", "missing ')' to end a rectangle");
}

void TextDecCmdSerial::operator()(gfx::Region& rg)
{
  // PRINTARGS("  operator()(Region)");

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

  PRINTARGS("load serial_span encoded=",
            encoded.size(),
            " (original",
            span.size(),
            ")",
            "base64=",
            std::string(encoded.begin(), encoded.end()));
}

int TextDecCmdSerial::nextChar()
{
  m_chr = m_f->read8();
  // PRINTARGS("nextChar", (char)m_chr);
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
    // PRINTARGS("    nextToken prev=", prev);
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
  // PRINTARGS("    nextToken ->", m_tok);
  return m_tok;
}

void TextDecCmdSerial::unusedToken()
{
  m_prevTok = m_tok;
}

void TextDecCmdSerial::expectToken(const char* expected, const char* error)
{
  auto tok = nextToken();
  // PRINTARGS("  expectToken ", expected, "<=>", tok);
  if (tok != expected)
    throw std::runtime_error(error);
}

int TextDecCmdSerial::parseInt()
{
  auto tok = nextToken();
  return base::convert_to<int>(tok);
}

} // namespace app
