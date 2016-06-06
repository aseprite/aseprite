// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/cli_processor.h"

#include "app/cli/app_options.h"
#include "app/cli/cli_delegate.h"
#include "app/commands/cmd_sprite_size.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/document.h"
#include "app/document_exporter.h"
#include "app/document_undo.h"
#include "app/file/file.h"
#include "app/filename_formatter.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "base/path.h"
#include "base/split_string.h"
#include "doc/frame_tag.h"
#include "doc/frame_tags.h"
#include "doc/layer.h"
#include "doc/layers_range.h"

namespace app {

CliProcessor::CliProcessor(CliDelegate* delegate,
                             const AppOptions& options)
  : m_delegate(delegate)
  , m_options(options)
  , m_exporter(nullptr)
{
  if (options.hasExporterParams())
    m_exporter.reset(new DocumentExporter);
}

void CliProcessor::process()
{
  // --help
  if (m_options.showHelp()) {
    m_delegate->showHelp(m_options);
  }
  // --version
  else if (m_options.showVersion()) {
    m_delegate->showVersion();
  }
  // Process other options and file names
  else if (!m_options.values().empty()) {
    Console console;
    UIContext* ctx = UIContext::instance();
    CliOpenFile cof;
    SpriteSheetType sheetType = SpriteSheetType::None;
    app::Document* lastDoc = nullptr;

    for (const auto& value : m_options.values()) {
      const AppOptions::Option* opt = value.option();

      // Special options/commands
      if (opt) {
        // --data <file.json>
        if (opt == &m_options.data()) {
          if (m_exporter)
            m_exporter->setDataFilename(value.value());
        }
        // --format <format>
        else if (opt == &m_options.format()) {
          if (m_exporter) {
            DocumentExporter::DataFormat format = DocumentExporter::DefaultDataFormat;

            if (value.value() == "json-hash")
              format = DocumentExporter::JsonHashDataFormat;
            else if (value.value() == "json-array")
              format = DocumentExporter::JsonArrayDataFormat;

            m_exporter->setDataFormat(format);
          }
        }
        // --sheet <file.png>
        else if (opt == &m_options.sheet()) {
          if (m_exporter)
            m_exporter->setTextureFilename(value.value());
        }
        // --sheet-width <width>
        else if (opt == &m_options.sheetWidth()) {
          if (m_exporter)
            m_exporter->setTextureWidth(strtol(value.value().c_str(), NULL, 0));
        }
        // --sheet-height <height>
        else if (opt == &m_options.sheetHeight()) {
          if (m_exporter)
            m_exporter->setTextureHeight(strtol(value.value().c_str(), NULL, 0));
        }
        // --sheet-pack
        else if (opt == &m_options.sheetType()) {
          if (value.value() == "horizontal")
            sheetType = SpriteSheetType::Horizontal;
          else if (value.value() == "vertical")
            sheetType = SpriteSheetType::Vertical;
          else if (value.value() == "rows")
            sheetType = SpriteSheetType::Rows;
          else if (value.value() == "columns")
            sheetType = SpriteSheetType::Columns;
          else if (value.value() == "packed")
            sheetType = SpriteSheetType::Packed;
        }
        // --sheet-pack
        else if (opt == &m_options.sheetPack()) {
          sheetType = SpriteSheetType::Packed;
        }
        // --split-layers
        else if (opt == &m_options.splitLayers()) {
          cof.splitLayers = true;
        }
        // --split-tags
        else if (opt == &m_options.splitTags()) {
          cof.splitTags = true;
        }
        // --layer <layer-name>
        else if (opt == &m_options.layer()) {
          cof.importLayer = value.value();
        }
        // --all-layers
        else if (opt == &m_options.allLayers()) {
          cof.allLayers = true;
        }
        // --frame-tag <tag-name>
        else if (opt == &m_options.frameTag()) {
          cof.frameTag = value.value();
        }
        // --frame-range from,to
        else if (opt == &m_options.frameRange()) {
          std::vector<std::string> splitRange;
          base::split_string(value.value(), splitRange, ",");
          if (splitRange.size() < 2)
            throw std::runtime_error("--frame-range needs two parameters separated by comma (,)\n"
                                     "Usage: --frame-range from,to\n"
                                     "E.g. --frame-range 0,99");

          cof.fromFrame = base::convert_to<frame_t>(splitRange[0]);
          cof.toFrame   = base::convert_to<frame_t>(splitRange[1]);
        }
        // --ignore-empty
        else if (opt == &m_options.ignoreEmpty()) {
          cof.ignoreEmpty = true;
          if (m_exporter)
            m_exporter->setIgnoreEmptyCels(true);
        }
        // --border-padding
        else if (opt == &m_options.borderPadding()) {
          if (m_exporter)
            m_exporter->setBorderPadding(strtol(value.value().c_str(), NULL, 0));
        }
        // --shape-padding
        else if (opt == &m_options.shapePadding()) {
          if (m_exporter)
            m_exporter->setShapePadding(strtol(value.value().c_str(), NULL, 0));
        }
        // --inner-padding
        else if (opt == &m_options.innerPadding()) {
          if (m_exporter)
            m_exporter->setInnerPadding(strtol(value.value().c_str(), NULL, 0));
        }
        // --trim
        else if (opt == &m_options.trim()) {
          cof.trim = true;
          if (m_exporter)
            m_exporter->setTrimCels(true);
        }
        // --crop x,y,width,height
        else if (opt == &m_options.crop()) {
          std::vector<std::string> parts;
          base::split_string(value.value(), parts, ",");
          if (parts.size() < 4)
            throw std::runtime_error("--crop needs four parameters separated by comma (,)\n"
                                     "Usage: --crop x,y,width,height\n"
                                     "E.g. --crop 0,0,32,32");

          cof.crop.x = base::convert_to<int>(parts[0]);
          cof.crop.y = base::convert_to<int>(parts[1]);
          cof.crop.w = base::convert_to<int>(parts[2]);
          cof.crop.h = base::convert_to<int>(parts[3]);
        }
        // --filename-format
        else if (opt == &m_options.filenameFormat()) {
          cof.filenameFormat = value.value();
          if (m_exporter)
            m_exporter->setFilenameFormat(cof.filenameFormat);
        }
        // --save-as <filename>
        else if (opt == &m_options.saveAs()) {
          if (lastDoc) {
            std::string fn = value.value();

            // Automatic --split-layer or --split-tags in case the
            // output filename already contains {layer} or {tag}
            // template elements.
            bool hasLayerTemplate = (fn.find("{layer}") != std::string::npos);
            bool hasTagTemplate = (fn.find("{tag}") != std::string::npos);
            if (hasLayerTemplate || hasTagTemplate) {
              cof.splitLayers = (cof.splitLayers || hasLayerTemplate);
              cof.splitTags = (cof.splitTags || hasTagTemplate);
              cof.filenameFormat =
                get_default_filename_format(
                  fn,
                  true,                                   // With path
                  (lastDoc->sprite()->totalFrames() > 1), // Has frames
                  false,                                  // Has layer
                  false);                                 // Has frame tag
            }

            cof.document = lastDoc;
            cof.filename = fn;
            saveFile(cof);
          }
          else
            console.printf("A document is needed before --save-as argument\n");
        }
        // --scale <factor>
        else if (opt == &m_options.scale()) {
          Command* command = CommandsModule::instance()->getCommandByName(CommandId::SpriteSize);
          double scale = strtod(value.value().c_str(), NULL);
          static_cast<SpriteSizeCommand*>(command)->setScale(scale, scale);

          // Scale all sprites
          for (auto doc : ctx->documents()) {
            ctx->setActiveDocument(static_cast<app::Document*>(doc));
            ctx->executeCommand(command);
          }
        }
        // --shrink-to <width,height>
        else if (opt == &m_options.shrinkTo()) {
          std::vector<std::string> dimensions;
          base::split_string(value.value(), dimensions, ",");
          if (dimensions.size() < 2)
            throw std::runtime_error("--shrink-to needs two parameters separated by comma (,)\n"
                                     "Usage: --shrink-to width,height\n"
                                     "E.g. --shrink-to 128,64");

          double maxWidth = base::convert_to<double>(dimensions[0]);
          double maxHeight = base::convert_to<double>(dimensions[1]);
          double scaleWidth, scaleHeight, scale;

          // Shrink all sprites if needed
          for (auto doc : ctx->documents()) {
            ctx->setActiveDocument(static_cast<app::Document*>(doc));
            scaleWidth = (doc->width() > maxWidth ? maxWidth / doc->width() : 1.0);
            scaleHeight = (doc->height() > maxHeight ? maxHeight / doc->height() : 1.0);
            if (scaleWidth < 1.0 || scaleHeight < 1.0) {
              scale = MIN(scaleWidth, scaleHeight);
              Command* command = CommandsModule::instance()->getCommandByName(CommandId::SpriteSize);
              static_cast<SpriteSizeCommand*>(command)->setScale(scale, scale);
              ctx->executeCommand(command);
            }
          }
        }
        // --script <filename>
        else if (opt == &m_options.script()) {
          std::string filename = value.value();
          m_delegate->execScript(filename);
        }
        // --list-layers
        else if (opt == &m_options.listLayers()) {
          cof.listLayers = true;
          if (m_exporter)
            m_exporter->setListLayers(true);
        }
        // --list-tags
        else if (opt == &m_options.listTags()) {
          cof.listTags = true;
          if (m_exporter)
            m_exporter->setListFrameTags(true);
        }
      }
      // File names aren't associated to any option
      else {
        cof.document = nullptr;
        cof.filename = base::normalize_path(value.value());
        if (openFile(cof))
          lastDoc = cof.document;
      }
    }

    if (m_exporter) {
      if (sheetType != SpriteSheetType::None)
        m_exporter->setSpriteSheetType(sheetType);

      m_delegate->exportFiles(*m_exporter.get());
      m_exporter.reset(nullptr);
    }
  }

  // Running mode
  if (m_options.startUI()) {
    m_delegate->uiMode();
  }
  else if (m_options.startShell()) {
    m_delegate->shellMode();
  }
  else {
    m_delegate->batchMode();
  }
}

bool CliProcessor::openFile(CliOpenFile& cof)
{
  m_delegate->beforeOpenFile(cof);

  Context* ctx = UIContext::instance();
  app::Document* oldDoc = ctx->activeDocument();
  Command* openCommand = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  Params params;
  params.set("filename", cof.filename.c_str());
  ctx->executeCommand(openCommand, params);

  app::Document* doc = ctx->activeDocument();
  // If the active document is equal to the previous one, it
  // means that we couldn't open this specific document.
  if (doc == oldDoc)
    doc = nullptr;

  cof.document = doc;

  if (doc) {
    // Show all layers
    if (cof.allLayers) {
      for (doc::Layer* layer : doc->sprite()->layers())
        layer->setVisible(true);
    }

    // Add document to exporter
    if (m_exporter) {
      FrameTag* frameTag = nullptr;
      bool isTemporalTag = false;

      if (cof.hasFrameTag()) {
        frameTag = doc->sprite()->frameTags().getByName(cof.frameTag);
      }
      if (cof.hasFrameRange()) {
        // --frame-range with --frame-tag
        if (frameTag) {
          frameTag = new FrameTag(
            frameTag->fromFrame()+MID(0, cof.fromFrame, frameTag->frames()-1),
            frameTag->fromFrame()+MID(0, cof.toFrame, frameTag->frames()-1));
        }
        // --frame-range without --frame-tag
        else {
          frameTag = new FrameTag(cof.fromFrame, cof.toFrame);
        }
        isTemporalTag = true;
      }

      if (!cof.importLayer.empty()) {
        Layer* foundLayer = nullptr;
        for (Layer* layer : doc->sprite()->layers()) {
          if (layer->name() == cof.importLayer) {
            foundLayer = layer;
            break;
          }
        }
        if (foundLayer)
          m_exporter->addDocument(doc, foundLayer, frameTag, isTemporalTag);
      }
      else if (cof.splitLayers) {
        for (auto layer : doc->sprite()->layers()) {
          if (layer->isVisible())
            m_exporter->addDocument(doc, layer, frameTag, isTemporalTag);
        }
      }
      else {
        m_exporter->addDocument(doc, nullptr, frameTag, isTemporalTag);
      }
    }
  }

  m_delegate->afterOpenFile(cof);

  return (doc ? true: false);
}

void CliProcessor::saveFile(const CliOpenFile& cof)
{
  UIContext* ctx = UIContext::instance();
  ctx->setActiveDocument(cof.document);

  Command* trimCommand = CommandsModule::instance()->getCommandByName(CommandId::AutocropSprite);
  Command* cropCommand = CommandsModule::instance()->getCommandByName(CommandId::CropSprite);
  Command* undoCommand = CommandsModule::instance()->getCommandByName(CommandId::Undo);
  app::Document* doc = cof.document;
  bool clearUndo = false;

  if (!cof.crop.isEmpty()) {
    Params cropParams;
    cropParams.set("x", base::convert_to<std::string>(cof.crop.x).c_str());
    cropParams.set("y", base::convert_to<std::string>(cof.crop.y).c_str());
    cropParams.set("width", base::convert_to<std::string>(cof.crop.w).c_str());
    cropParams.set("height", base::convert_to<std::string>(cof.crop.h).c_str());
    ctx->executeCommand(cropCommand, cropParams);
  }

  // Store in "visibility" the original "visible" state of every layer.
  std::vector<bool> visibility(doc->sprite()->countLayers());
  int i = 0;
  for (doc::Layer* layer : doc->sprite()->layers())
    visibility[i++] = layer->isVisible();

  std::string fn = cof.filename;
  std::string filenameFormat = cof.filenameFormat;
  if (filenameFormat.empty()) { // Default format
    bool hasFrames = (cof.roi().frames() > 1);
    filenameFormat = get_default_filename_format(
      fn,
      true,                     // With path
      hasFrames,                // Has frames
      cof.splitLayers,          // Has layer
      cof.splitTags);           // Has frame tag
  }

  std::vector<doc::Layer*> layers;
  // --save-as with --split-layers or --split-tags
  if (cof.splitLayers) {
    for (doc::Layer* layer : doc->sprite()->layers())
      layers.push_back(layer);
  }
  else {
    // Show only one layer
    if (!cof.importLayer.empty()) {
      for (Layer* layer : doc->sprite()->layers()) {
        if (layer->name() == cof.importLayer) {
          layer->setVisible(true);
          layers.push_back(layer);
        }
        else
          layer->setVisible(false);
      }
    }
    // All visible layers
    else
      layers.push_back(nullptr);
  }

  std::vector<doc::FrameTag*> frameTags;
  if (cof.hasFrameTag()) {
    frameTags.push_back(
      doc->sprite()->frameTags().getByName(cof.frameTag));
  }
  else {
    doc::FrameTags& origFrameTags = cof.document->sprite()->frameTags();
    if (cof.splitTags && !origFrameTags.empty()) {
      for (doc::FrameTag* frameTag : origFrameTags) {
        // In case the tag is outside the given --frame-range
        if (cof.hasFrameRange()) {
          if (frameTag->toFrame() < cof.fromFrame ||
              frameTag->fromFrame() > cof.toFrame)
            continue;
        }
        frameTags.push_back(frameTag);
      }
    }
    else
      frameTags.push_back(nullptr);
  }

  for (doc::FrameTag* frameTag : frameTags) {
    // For each layer, hide other ones and save the sprite.
    i = 0;
    for (doc::Layer* layer : layers) {
      if (cof.splitLayers) {
        ASSERT(layer);

        // If the user doesn't want all layers and this one is hidden.
        if (!visibility[i++])
          continue;     // Just ignore this layer.

        // Make this layer ("show") the only one visible.
        for (doc::Layer* hide : doc->sprite()->layers())
          hide->setVisible(hide == layer);
      }

      // TODO --trim --save-as --split-layers doesn't make too much
      // sense as we lost the trim rectangle information (e.g. we
      // don't have sheet .json) Also, we should trim each frame
      // individually (a process that can be done only in
      // FileOp::operate()).
      if (cof.trim)
        ctx->executeCommand(trimCommand);

      CliOpenFile itemCof = cof;
      FilenameInfo fnInfo;
      fnInfo.filename(fn);
      if (layer) {
        fnInfo.layerName(layer->name());
        itemCof.importLayer = layer->name();
      }
      if (frameTag) {
        fnInfo
          .innerTagName(frameTag->name())
          .outerTagName(frameTag->name());
        itemCof.frameTag = frameTag->name();
      }
      itemCof.filename = filename_formatter(filenameFormat, fnInfo);
      itemCof.filenameFormat = filename_formatter(filenameFormat, fnInfo, false);

      // Call delegate
      m_delegate->saveFile(itemCof);

      if (cof.trim) {
        ctx->executeCommand(undoCommand);
        clearUndo = true;
      }
    }
  }

  // Restore layer visibility
  i = 0;
  for (Layer* layer : doc->sprite()->layers())
    layer->setVisible(visibility[i++]);

  // Undo crop
  if (!cof.crop.isEmpty()) {
    ctx->executeCommand(undoCommand);
    clearUndo = true;
  }

  if (clearUndo) {
    // Just in case allow non-linear history is enabled
    // we clear redo information
    doc->undoHistory()->clearRedo();
  }
}

} // namespace app
