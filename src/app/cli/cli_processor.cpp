// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/cli_processor.h"

#include "app/cli/app_options.h"
#include "app/cli/cli_delegate.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/doc_exporter.h"
#include "app/doc_undo.h"
#include "app/file/file.h"
#include "app/filename_formatter.h"
#include "app/restore_visible_layers.h"
#include "app/ui_context.h"
#include "base/clamp.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "doc/layer.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"
#include "doc/slice.h"
#include "doc/tag.h"
#include "doc/tags.h"
#include "render/dithering_algorithm.h"

#include <algorithm>
#include <queue>
#include <vector>

namespace app {

namespace {

std::string get_layer_path(const Layer* layer)
{
  std::string path;
  for (; layer != layer->sprite()->root(); layer=layer->parent()) {
    if (!path.empty())
      path.insert(0, "/");
    path.insert(0, layer->name());
  }
  return path;
}

bool match_path(const std::string& filter,
                const std::string& layer_path,
                const bool exclude)
{
  if (filter == layer_path)
    return true;

  std::vector<std::string> a, b;
  base::split_string(filter, a, "/");
  base::split_string(layer_path, b, "/");

  for (std::size_t i=0; i<a.size() && i<b.size(); ++i) {
    if (a[i] != b[i] && a[i] != "*")
      return false;
  }

  const bool wildcard = (!a.empty() && a[a.size()-1] == "*");

  // Exclude group itself when all children are excluded. This special
  // case is only for exclusion because if we leave the group
  // selected, the propagation of the selection will include all
  // visible children too (see SelectedLayers::propagateSelection()).
  if (exclude &&
      a.size() > 1 &&
      a.size() == b.size()+1 &&
      wildcard) {
    return true;
  }

  if (exclude || wildcard)
    return (a.size() <= b.size());
  else {
    // Include filters need exact match when there is no wildcard
    return (a.size() == b.size());
  }
}

bool filter_layer(const std::string& layer_path,
                  const std::vector<std::string>& filters,
                  const bool result)
{
  for (const auto& filter : filters) {
    if (match_path(filter, layer_path, !result))
      return result;
  }
  return !result;
}

// If there is one layer with the given name "filter", we can convert
// the filter to a full path to the layer (e.g. to match child layers
// of a group).
std::string convert_filter_to_layer_path_if_possible(
  const Sprite* sprite,
  const std::string& filter)
{
  std::string fullName;
  std::queue<Layer*> layers;
  layers.push(sprite->root());

  while (!layers.empty()) {
    const Layer* layer = layers.front();
    layers.pop();

    if (layer != sprite->root() &&
        layer->name() == filter) {
      if (fullName.empty()) {
        fullName = get_layer_path(layer);
      }
      else {
        // Two or more layers with the same name (use "filter" as a
        // general filter, not a specific layer name)
        return filter;
      }
    }
    if (layer->isGroup()) {
      for (auto child : static_cast<const LayerGroup*>(layer)->layers())
        layers.push(child);
    }
  }

  if (!fullName.empty())
    return fullName;
  else
    return filter;
}

} // anonymous namespace

// static
void CliProcessor::FilterLayers(const Sprite* sprite,
                                std::vector<std::string> includes,
                                std::vector<std::string> excludes,
                                SelectedLayers& filteredLayers)
{
  // Convert filters to full paths for the sprite layers if there are
  // just one layer with the given name.
  for (auto& include : includes)
    include = convert_filter_to_layer_path_if_possible(sprite, include);
  for (auto& exclude : excludes)
    exclude = convert_filter_to_layer_path_if_possible(sprite, exclude);

  for (Layer* layer : sprite->allLayers()) {
    auto layer_path = get_layer_path(layer);

    if ((includes.empty() && !layer->isVisibleHierarchy()) ||
        (!includes.empty() && !filter_layer(layer_path, includes, true)))
      continue;

    if (!excludes.empty() &&
        !filter_layer(layer_path, excludes, false))
      continue;

    filteredLayers.insert(layer);
  }
}

CliProcessor::CliProcessor(CliDelegate* delegate,
                           const AppOptions& options)
  : m_delegate(delegate)
  , m_options(options)
  , m_exporter(nullptr)
{
  if (options.hasExporterParams())
    m_exporter.reset(new DocExporter);
}

int CliProcessor::process(Context* ctx)
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
#ifdef ENABLE_SCRIPTING
    Params scriptParams;
#endif
    Console console;
    CliOpenFile cof;
    SpriteSheetType sheetType = SpriteSheetType::None;
    Doc* lastDoc = nullptr;
    render::DitheringAlgorithm ditheringAlgorithm = render::DitheringAlgorithm::None;
    std::string ditheringMatrix;

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
            SpriteSheetDataFormat format = SpriteSheetDataFormat::Default;

            if (value.value() == "json-hash")
              format = SpriteSheetDataFormat::JsonHash;
            else if (value.value() == "json-array")
              format = SpriteSheetDataFormat::JsonArray;

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
            m_exporter->setTextureWidth(strtol(value.value().c_str(), nullptr, 0));
        }
        // --sheet-height <height>
        else if (opt == &m_options.sheetHeight()) {
          if (m_exporter)
            m_exporter->setTextureHeight(strtol(value.value().c_str(), nullptr, 0));
        }
        // --sheet-columns <columns>
        else if (opt == &m_options.sheetColumns()) {
          if (m_exporter)
            m_exporter->setTextureColumns(strtol(value.value().c_str(), nullptr, 0));
        }
        // --sheet-rows <rows>
        else if (opt == &m_options.sheetRows()) {
          if (m_exporter)
            m_exporter->setTextureRows(strtol(value.value().c_str(), nullptr, 0));
        }
        // --sheet-type <sheet-type>
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
          if (m_exporter)
            m_exporter->setSplitLayers(true);
        }
        // --split-tags
        else if (opt == &m_options.splitTags()) {
          cof.splitTags = true;
          if (m_exporter)
            m_exporter->setSplitTags(true);
        }
        // --split-slice
        else if (opt == &m_options.splitSlices()) {
          cof.splitSlices = true;
        }
        // --layer <layer-name>
        else if (opt == &m_options.layer()) {
          cof.includeLayers.push_back(value.value());
        }
        // --ignore-layer <layer-name>
        else if (opt == &m_options.ignoreLayer()) {
          cof.excludeLayers.push_back(value.value());
        }
        // --all-layers
        else if (opt == &m_options.allLayers()) {
          cof.allLayers = true;
        }
        // --tag <tag-name>
        else if (opt == &m_options.tag()) {
          cof.tag = value.value();
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
        // --merge-duplicates
        else if (opt == &m_options.mergeDuplicates()) {
          if (m_exporter)
            m_exporter->setMergeDuplicates(true);
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
        // --trim-sprite
        else if (opt == &m_options.trimSprite()) {
          cof.trim = true;
          if (m_exporter)
            m_exporter->setTrimSprite(true);
        }
        // --trim-by-grid
        else if (opt == &m_options.trimByGrid()) {
          cof.trim = cof.trimByGrid = true;
          if (m_exporter) {
            m_exporter->setTrimCels(true);
            m_exporter->setTrimByGrid(true);
          }
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
        // --slice <slice>
        else if (opt == &m_options.slice()) {
          cof.slice = value.value();
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

            // Automatic --split-layer, --split-tags, --split-slices
            // in case the output filename already contains {layer},
            // {tag}, or {slice} template elements.
            bool hasLayerTemplate = (is_layer_in_filename_format(fn) ||
                                     is_group_in_filename_format(fn));
            bool hasTagTemplate = is_tag_in_filename_format(fn);
            bool hasSliceTemplate = is_slice_in_filename_format(fn);

            if (hasLayerTemplate || hasTagTemplate || hasSliceTemplate) {
              cof.splitLayers = (cof.splitLayers || hasLayerTemplate);
              cof.splitTags = (cof.splitTags || hasTagTemplate);
              cof.splitSlices = (cof.splitSlices || hasSliceTemplate);
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
            saveFile(ctx, cof);
          }
          else
            console.printf("A document is needed before --save-as argument\n");
        }
        // --palette <filename>
        else if (opt == &m_options.palette()) {
          if (lastDoc) {
            ASSERT(cof.document == lastDoc);

            std::string filename = value.value();
            m_delegate->loadPalette(ctx, cof, filename);
          }
          else {
            console.printf("You need to load a document to change its palette with --palette\n");
          }
        }
        // --scale <factor>
        else if (opt == &m_options.scale()) {
          Params params;
          params.set("scale", value.value().c_str());

          // Scale all sprites
          for (auto doc : ctx->documents()) {
            ctx->setActiveDocument(doc);
            ctx->executeCommand(Commands::instance()->byId(CommandId::SpriteSize()),
                                params);
          }
        }
        // --dithering-algorithm <algorithm>
        else if (opt == &m_options.ditheringAlgorithm()) {
          if (value.value() == "none")
            ditheringAlgorithm = render::DitheringAlgorithm::None;
          else if (value.value() == "ordered")
            ditheringAlgorithm = render::DitheringAlgorithm::Ordered;
          else if (value.value() == "old")
            ditheringAlgorithm = render::DitheringAlgorithm::Old;
          else if (value.value() == "error-diffusion")
            ditheringAlgorithm = render::DitheringAlgorithm::ErrorDiffusion;
          else
            throw std::runtime_error("--dithering-algorithm needs a valid algorithm name\n"
                                     "Usage: --dithering-algorithm <algorithm>\n"
                                     "Where <algorithm> can be none, ordered, old, or error-diffusion");
        }
        // --dithering-matrix <id>
        else if (opt == &m_options.ditheringMatrix()) {
          ditheringMatrix = value.value();
        }
        // --color-mode <mode>
        else if (opt == &m_options.colorMode()) {
          Command* command = Commands::instance()->byId(CommandId::ChangePixelFormat());
          Params params;
          if (value.value() == "rgb") {
            params.set("format", "rgb");
          }
          else if (value.value() == "grayscale") {
            params.set("format", "grayscale");
          }
          else if (value.value() == "indexed") {
            params.set("format", "indexed");
            switch (ditheringAlgorithm) {
              case render::DitheringAlgorithm::None:
                params.set("dithering", "none");
                break;
              case render::DitheringAlgorithm::Ordered:
                params.set("dithering", "ordered");
                break;
              case render::DitheringAlgorithm::Old:
                params.set("dithering", "old");
                break;
              case render::DitheringAlgorithm::ErrorDiffusion:
                params.set("dithering", "error-diffusion");
                break;
            }

            if (ditheringAlgorithm != render::DitheringAlgorithm::None &&
                !ditheringMatrix.empty()) {
              params.set("dithering-matrix", ditheringMatrix.c_str());
            }
          }
          else {
            throw std::runtime_error("--color-mode needs a valid color mode for conversion\n"
                                     "Usage: --color-mode <mode>\n"
                                     "Where <mode> can be rgb, grayscale, or indexed");
          }

          for (auto doc : ctx->documents()) {
            ctx->setActiveDocument(doc);
            ctx->executeCommand(command, params);
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
            ctx->setActiveDocument(doc);
            scaleWidth = (doc->width() > maxWidth ? maxWidth / doc->width() : 1.0);
            scaleHeight = (doc->height() > maxHeight ? maxHeight / doc->height() : 1.0);
            if (scaleWidth < 1.0 || scaleHeight < 1.0) {
              scale = std::min(scaleWidth, scaleHeight);
              Params params;
              params.set("scale", base::convert_to<std::string>(scale).c_str());
              ctx->executeCommand(Commands::instance()->byId(CommandId::SpriteSize()),
                                  params);
            }
          }
        }
#ifdef ENABLE_SCRIPTING
        // --script <filename>
        else if (opt == &m_options.script()) {
          std::string filename = value.value();
          int code = m_delegate->execScript(filename, scriptParams);
          if (code != 0)
            return code;
        }
        // --script-param <name=value>
        else if (opt == &m_options.scriptParam()) {
          const std::string& v = value.value();
          auto i = v.find('=');
          if (i != std::string::npos)
            scriptParams.set(v.substr(0, i).c_str(),
                             v.substr(i+1).c_str());
          else
            scriptParams.set(v.c_str(), "1");
        }
#endif
        // --list-layers
        else if (opt == &m_options.listLayers()) {
          if (m_exporter)
            m_exporter->setListLayers(true);
          else
            cof.listLayers = true;
        }
        // --list-tags
        else if (opt == &m_options.listTags()) {
          if (m_exporter)
            m_exporter->setListTags(true);
          else
            cof.listTags = true;
        }
        // --list-slices
        else if (opt == &m_options.listSlices()) {
          if (m_exporter)
            m_exporter->setListSlices(true);
          else
            cof.listSlices = true;
        }
        // --oneframe
        else if (opt == &m_options.oneFrame()) {
          cof.oneFrame = true;
        }
      }
      // File names aren't associated to any option
      else {
        cof.document = nullptr;
        cof.filename = base::normalize_path(value.value());
        if (openFile(ctx, cof))
          lastDoc = cof.document;
      }
    }

    if (m_exporter) {
      // Rows sprite sheet as the default type
      if (sheetType == SpriteSheetType::None)
        sheetType = SpriteSheetType::Rows;
      m_exporter->setSpriteSheetType(sheetType);

      m_delegate->exportFiles(ctx, *m_exporter.get());
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
  return 0;
}

bool CliProcessor::openFile(Context* ctx, CliOpenFile& cof)
{
  m_delegate->beforeOpenFile(cof);

  Doc* oldDoc = ctx->activeDocument();
  Command* openCommand = Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("filename", cof.filename.c_str());
  if (cof.oneFrame)
    params.set("oneframe", "true");
  ctx->executeCommand(openCommand, params);

  Doc* doc = ctx->activeDocument();
  // If the active document is equal to the previous one, it
  // means that we couldn't open this specific document.
  if (doc == oldDoc)
    doc = nullptr;

  cof.document = doc;

  if (doc) {
    // Show all layers
    if (cof.allLayers) {
      for (doc::Layer* layer : doc->sprite()->allLayers())
        layer->setVisible(true);
    }

    // Add document to exporter
    if (m_exporter) {
      Tag* tag = nullptr;
      SelectedFrames selFrames;

      if (cof.hasTag()) {
        tag = doc->sprite()->tags().getByName(cof.tag);
      }
      if (cof.hasFrameRange()) {
        // --frame-range with --frame-tag
        if (tag) {
          selFrames.insert(
            tag->fromFrame()+base::clamp(cof.fromFrame, 0, tag->frames()-1),
            tag->fromFrame()+base::clamp(cof.toFrame, 0, tag->frames()-1));
        }
        // --frame-range without --frame-tag
        else {
          selFrames.insert(cof.fromFrame, cof.toFrame);
        }
      }

      SelectedLayers filteredLayers;
      if (cof.hasLayersFilter())
        filterLayers(doc->sprite(), cof, filteredLayers);

      m_exporter->addDocumentSamples(
        doc, tag,
        cof.splitLayers,
        cof.splitTags,
        (cof.hasLayersFilter() ? &filteredLayers: nullptr),
        (!selFrames.empty() ? &selFrames: nullptr));
    }
  }

  m_delegate->afterOpenFile(cof);

  return (doc ? true: false);
}

void CliProcessor::saveFile(Context* ctx, const CliOpenFile& cof)
{
  ctx->setActiveDocument(cof.document);

  Command* trimCommand = Commands::instance()->byId(CommandId::AutocropSprite());
  Command* undoCommand = Commands::instance()->byId(CommandId::Undo());
  Doc* doc = cof.document;
  bool clearUndo = false;

  if (!cof.crop.isEmpty()) {
    Params cropParams;
    cropParams.set("x", base::convert_to<std::string>(cof.crop.x).c_str());
    cropParams.set("y", base::convert_to<std::string>(cof.crop.y).c_str());
    cropParams.set("width", base::convert_to<std::string>(cof.crop.w).c_str());
    cropParams.set("height", base::convert_to<std::string>(cof.crop.h).c_str());
    ctx->executeCommand(
      Commands::instance()->byId(CommandId::CropSprite()),
      cropParams);
  }

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

  SelectedLayers filteredLayers;
  LayerList layers;
  // --save-as with --split-layers or --split-tags
  if (cof.splitLayers) {
    for (doc::Layer* layer : doc->sprite()->allVisibleLayers())
      layers.push_back(layer);
  }
  else {
    // Filter layers
    if (cof.hasLayersFilter())
      filterLayers(doc->sprite(), cof, filteredLayers);

    // All visible layers
    layers.push_back(nullptr);
  }

  std::vector<doc::Tag*> tags;
  if (cof.hasTag()) {
    tags.push_back(
      doc->sprite()->tags().getByName(cof.tag));
  }
  else {
    doc::Tags& origTags = cof.document->sprite()->tags();
    if (cof.splitTags && !origTags.empty()) {
      for (doc::Tag* tag : origTags) {
        // In case the tag is outside the given --frame-range
        if (cof.hasFrameRange()) {
          if (tag->toFrame() < cof.fromFrame ||
              tag->fromFrame() > cof.toFrame)
            continue;
        }
        tags.push_back(tag);
      }
    }
    else
      tags.push_back(nullptr);
  }

  std::vector<doc::Slice*> slices;
  if (cof.hasSlice()) {
    slices.push_back(
      doc->sprite()->slices().getByName(cof.slice));
  }
  else {
    doc::Slices& origSlices = cof.document->sprite()->slices();
    if (cof.splitSlices && !origSlices.empty()) {
      for (doc::Slice* slice : origSlices)
        slices.push_back(slice);
    }
    else
      slices.push_back(nullptr);
  }

  bool layerInFormat = is_layer_in_filename_format(fn);
  bool groupInFormat = is_group_in_filename_format(fn);

  for (doc::Slice* slice : slices) {
    for (doc::Tag* tag : tags) {
      // For each layer, hide other ones and save the sprite.
      for (doc::Layer* layer : layers) {
        RestoreVisibleLayers layersVisibility;

        if (cof.splitLayers) {
          ASSERT(layer);

          // If the user doesn't want all layers and this one is hidden.
          if (!layer->isVisible())
            continue;     // Just ignore this layer.

          // Make this layer ("show") the only one visible.
          layersVisibility.showLayer(layer);
        }
        else if (!filteredLayers.empty())
          layersVisibility.showSelectedLayers(doc->sprite(), filteredLayers);

        if (layer) {
          if ((layerInFormat && layer->isGroup()) ||
              (!layerInFormat && groupInFormat && !layer->isGroup())) {
            continue;
          }
        }

        // TODO --trim --save-as --split-layers doesn't make too much
        // sense as we lost the trim rectangle information (e.g. we
        // don't have sheet .json) Also, we should trim each frame
        // individually (a process that can be done only in
        // FileOp::operate()).
        if (cof.trim) {
          Params params;
          if (cof.trimByGrid) {
            params.set("byGrid", "true");
          }
          ctx->executeCommand(trimCommand, params);
        }

        CliOpenFile itemCof = cof;
        FilenameInfo fnInfo;
        fnInfo.filename(fn);
        if (layer) {
          fnInfo.layerName(layer->name());

          if (layer->isGroup())
            fnInfo.groupName(layer->name());
          else if (layer->parent() != layer->sprite()->root())
            fnInfo.groupName(layer->parent()->name());

          itemCof.includeLayers.push_back(layer->name());
        }
        if (tag) {
          fnInfo
            .innerTagName(tag->name())
            .outerTagName(tag->name());
          itemCof.tag = tag->name();
        }
        if (slice) {
          fnInfo.sliceName(slice->name());
          itemCof.slice = slice->name();
        }
        itemCof.filename = filename_formatter(filenameFormat, fnInfo);
        itemCof.filenameFormat = filename_formatter(filenameFormat, fnInfo, false);

        // Call delegate
        m_delegate->saveFile(ctx, itemCof);

        if (cof.trim) {
          ctx->executeCommand(undoCommand);
          clearUndo = true;
        }
      }
    }
  }

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
