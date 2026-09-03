// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/file/undo/undo_history.h"

#include "app/cmd.h"
#include "app/cmd/transaction.h"
#include "app/cmd_serial.h"
#include "app/doc_undo.h"
#include "app/drm.h"
#include "app/file/file.h"
#include "app/file/format_options.h"
#include "base/convert_to.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "dio/file_interface.h"
#include "doc/layer.h"
#include "undo/undo_state.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace app {

void load_undo_history(FileOp* fop)
{
  PRINTARGS("load_undo_history for=",
            fop->filename(),
            "from=",
            base::replace_extension(fop->filename(), "aseprite-undo"));

  if (!fop->document())
    return;

  doc::Sprite* sprite = fop->document()->sprite();
  DocUndo* undo = fop->document()->undoHistory();
  if (!sprite || !undo)
    return;

  base::FileHandle handle(
    base::open_file(base::replace_extension(fop->filename(), "aseprite-undo"), "rt"));
  if (!handle)
    return;

  dio::StdioFileInterface fi(handle.get());

  // Read IDs map
  std::map<doc::ObjectId, doc::ObjectId> idMap;
  std::string tok;
  doc::Object* obj;
  doc::Layer* lastLayer = nullptr;
  doc::Cel* lastCel = nullptr;
  doc::LayerList allLayers = sprite->allLayers();
  int layIdx = 0;
  int celIdx = 0;
  while (fi.ok()) {
    char chr = fi.read8();
    if (chr == '\n')
      break;
    else if (!std::isalpha(chr))
      continue;

    // Read "[ObjectType] ObjectID"
    tok.clear();
    do {
      tok.push_back(chr);
      chr = fi.read8();
    } while (fi.ok() && std::isalpha(chr));

    // Jump whitespace between "ObjectType[ ]ObjectID"
    do {
      chr = fi.read8();
      if (chr == '\n')
        break;
    } while (fi.ok() && !std::isalnum(chr));
    if (chr == '\n')
      break;

    obj = nullptr;
    if (tok == "Sp") {
      obj = sprite;
    }
    else if (tok == "Ly") {
      if (layIdx >= allLayers.size())
        throw std::runtime_error("undo history: more layer ids than existing layers in sprite");

      obj = lastLayer = allLayers[layIdx];
      ++layIdx;
      celIdx = 0;
    }
    else if (tok == "Cl") {
      if (!lastLayer)
        throw std::runtime_error("undo history: unexpected cel before layer");

      if (celIdx >= lastLayer->cels().size())
        throw std::runtime_error("undo history: more cel ids than existing cels in layer");

      obj = lastCel = lastLayer->cels()[celIdx];
      ++celIdx;
    }
    else if (tok == "Im") {
      if (!lastCel)
        throw std::runtime_error("undo history: unexpected image before cel");

      obj = lastCel->image();
    }

    if (!obj)
      throw std::runtime_error("undo history: invalid object type");

    // Read "ObjectType [ObjectID]"
    tok.clear();
    do {
      tok.push_back(chr);
      chr = fi.read8();
    } while (fi.ok() && std::isdigit(chr));

    auto idInFile = base::convert_to<int>(tok);
    idMap[idInFile] = obj->id();

    PRINTARGS(" map", idInFile, "->", idMap[idInFile]);

    if (chr == '\n')
      break;
  }

  // States
  TextDecCmdSerial serial(sprite, &fi);
  serial.setIdsMap(idMap);

  // std::string line;
  while (fi.ok()) {
    auto* cmd = Cmd::Decode(serial);
    if (!cmd)
      break;

    if (auto* cmdTx = dynamic_cast<cmd::CmdTransaction*>(cmd))
      undo->add(cmdTx);
    else
      throw std::runtime_error("undo history: invalid non-transactional cmd at root level");
  }
}

#ifdef ENABLE_SAVE

// Experimental undo history information (in text format)
void save_undo_history(FileOp* fop)
{
  base::FileHandle handle(
    base::open_file(base::replace_extension(fop->filename(), "aseprite-undo"), "wt"));
  if (!handle)
    return;

  Sprite* sprite = fop->document()->sprite();
  DocUndo* undo = fop->document()->undoHistory();

  // Write IDs map
  {
    std::vector<Object*> objects;
    fprintf(handle.get(), "Sp %d", sprite->id());

    std::function<void(const Layer*)> writeLayerIds = [&](const Layer* layer) {
      fprintf(handle.get(), " Ly %d", layer->id());
      for (const Cel* cel : layer->cels()) {
        fprintf(handle.get(), " Cl %d", cel->id());
        if (cel->image())
          fprintf(handle.get(), " Im %d", cel->image()->id());
      }
      for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers())
        writeLayerIds(child);
    };

    for (const Layer* child : sprite->root()->layers())
      writeLayerIds(child);

    fprintf(handle.get(), "\n");
  }

  // States
  dio::StdioFileInterface fi(handle.get());
  TextEncCmdSerial serial(sprite, &fi);
  for (const auto* state = undo->firstState(); state; state = state->next()) {
    auto* cmd = static_cast<Cmd*>(state->cmd());
    if (state == undo->currentState())
      serial.curStateBegin();
    cmd->serialize(serial);
    if (state == undo->currentState())
      serial.curStateEnd();
  }
}

#endif // ENABLE_SAVE

} // namespace app
