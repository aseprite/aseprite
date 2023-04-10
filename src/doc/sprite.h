// Aseprite Document Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITE_H_INCLUDED
#define DOC_SPRITE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/cel_data.h"
#include "doc/cel_list.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/image_ref.h"
#include "doc/image_spec.h"
#include "doc/layer_list.h"
#include "doc/object.h"
#include "doc/pixel_format.h"
#include "doc/pixel_ratio.h"
#include "doc/rgbmap_algorithm.h"
#include "doc/slices.h"
#include "doc/tags.h"
#include "doc/tile.h"
#include "doc/with_user_data.h"
#include "gfx/rect.h"

#include <memory>
#include <string>
#include <vector>

#define DOC_SPRITE_MAX_WIDTH  65535
#define DOC_SPRITE_MAX_HEIGHT 65535

namespace doc {
  class CelsRange;
  class Document;
  class Image;
  class Layer;
  class LayerGroup;
  class LayerImage;
  class Mask;
  class Palette;
  class Remap;
  class RenderPlan;
  class RgbMap;
  class RgbMapRGB5A3;
  class SelectedFrames;
  class Tileset;
  class Tilesets;

  typedef std::vector<Palette*> PalettesList;

  // The main structure used in the whole program to handle a sprite.
  class Sprite : public WithUserData {
  public:
    enum class RgbMapFor {
      OpaqueLayer,
      TransparentLayer
    };

    ////////////////////////////////////////
    // Constructors/Destructor

    Sprite(const ImageSpec& spec, int ncolors = 256);
    virtual ~Sprite();

    // Creates a new sprite with one transparent layer and one cel
    // with an image of the size of the sprite.
    static Sprite* MakeStdSprite(const ImageSpec& spec,
                                 const int ncolors = 256,
                                 const ImageBufferPtr& imageBuf = ImageBufferPtr());

    ////////////////////////////////////////
    // Main properties

    const ImageSpec& spec() const { return m_spec; }

    Document* document() const { return m_document; }
    void setDocument(Document* doc) { m_document = doc; }

    PixelFormat pixelFormat() const { return (PixelFormat)m_spec.colorMode(); }
    ColorMode colorMode() const { return m_spec.colorMode(); }
    const PixelRatio& pixelRatio() const { return m_pixelRatio; }
    gfx::Size size() const { return m_spec.size(); }
    gfx::Rect bounds() const { return m_spec.bounds(); }
    int width() const { return m_spec.width(); }
    int height() const { return m_spec.height(); }
    const gfx::ColorSpaceRef& colorSpace() const { return m_spec.colorSpace(); }

    void setPixelFormat(PixelFormat format);
    void setPixelRatio(const PixelRatio& pixelRatio);
    void setSize(int width, int height);
    void setColorSpace(const gfx::ColorSpaceRef& colorSpace);

    // This method is only required/used for the template functions app::script::UserData_set_text/color.
    const Sprite* sprite() const { return this; }

    // Returns true if the sprite has a background layer and it's visible
    bool isOpaque() const;

    // Returns true if the rendered images will contain alpha values less
    // than 255. Only RGBA and Grayscale images without background needs
    // alpha channel in the render.
    bool needAlpha() const;
    bool supportAlpha() const;

    color_t transparentColor() const { return m_spec.maskColor(); }
    void setTransparentColor(color_t color);

    // Defaults
    static gfx::Rect DefaultGridBounds();
    static void SetDefaultGridBounds(const gfx::Rect& defGridBounds);
    static RgbMapAlgorithm DefaultRgbMapAlgorithm();
    static void SetDefaultRgbMapAlgorithm(const RgbMapAlgorithm mapAlgo);

    const gfx::Rect& gridBounds() const { return m_gridBounds; }
    void setGridBounds(const gfx::Rect& rc) { m_gridBounds = rc; }

    virtual int getMemSize() const override;

    ////////////////////////////////////////
    // Layers

    LayerGroup* root() const { return m_root; }
    LayerImage* backgroundLayer() const;
    Layer* firstBrowsableLayer() const;
    Layer* firstLayer() const;
    layer_t allLayersCount() const;
    bool hasVisibleReferenceLayers() const;

    ////////////////////////////////////////
    // Palettes

    Palette* palette(frame_t frame) const;
    const PalettesList& getPalettes() const;

    void setPalette(const Palette* pal, bool truncate);

    // Removes all palettes from the sprites except the first one.
    void resetPalettes();

    void deletePalette(frame_t frame);

    RgbMapFor rgbMapForSprite() const;
    RgbMap* rgbMap(const frame_t frame) const;
    RgbMap* rgbMap(const frame_t frame,
                   const RgbMapFor forLayer) const;
    RgbMap* rgbMap(const frame_t frame,
                   const RgbMapFor forLayer,
                   RgbMapAlgorithm mapAlgo) const;

    ////////////////////////////////////////
    // Frames

    frame_t totalFrames() const { return m_frames; }
    frame_t lastFrame() const { return m_frames-1; }

    void addFrame(frame_t newFrame);
    void removeFrame(frame_t frame);
    void setTotalFrames(frame_t frames);

    int frameDuration(frame_t frame) const;
    int totalAnimationDuration() const;
    void setFrameDuration(frame_t frame, int msecs);
    void setFrameRangeDuration(frame_t from, frame_t to, int msecs);
    void setDurationForAllFrames(int msecs);

    const Tags& tags() const { return m_tags; }
    Tags& tags() { return m_tags; }

    const Slices& slices() const { return m_slices; }
    Slices& slices() { return m_slices; }

    ////////////////////////////////////////
    // Shared Images and CelData (for linked Cels)

    ImageRef getImageRef(ObjectId imageId);
    CelDataRef getCelDataRef(ObjectId celDataId);

    ////////////////////////////////////////
    // Images

    void replaceImage(ObjectId curImageId, const ImageRef& newImage);
    void replaceTileset(tileset_index tsi, Tileset* newTileset);

    // Returns all sprite images (cel + tiles) that aren't tilemaps
    void getImages(std::vector<ImageRef>& images) const;

    // TODO replace this with a co-routine when we start using C++20 (std::generator<ImageRef>)
    void getTilemapsByTileset(const Tileset* tileset,
                              std::vector<ImageRef>& images) const;

    void remapImages(const Remap& remap);
    void remapTilemaps(const Tileset* tileset,
                       const Remap& remap);
    void pickCels(const gfx::PointF& pos,
                  const int opacityThreshold,
                  const RenderPlan& plan,
                  CelList& cels) const;

    ////////////////////////////////////////
    // Iterators

    LayerList allLayers() const;
    LayerList allVisibleLayers() const;
    LayerList allVisibleReferenceLayers() const;
    LayerList allBrowsableLayers() const;

    CelsRange cels() const;
    CelsRange cels(frame_t frame) const;
    CelsRange cels(const SelectedFrames& selFrames) const;
    CelsRange uniqueCels() const;
    CelsRange uniqueCels(const SelectedFrames& selFrames) const;

    ////////////////////////////////////////
    // Tilesets

    bool hasTilesets() const { return m_tilesets != nullptr; }
    Tilesets* tilesets() const;

    const std::string& tileManagementPlugin() const {
      return m_tileManagementPlugin;
    }

    bool hasTileManagementPlugin() const {
      return !m_tileManagementPlugin.empty();
    }

    void setTileManagementPlugin(const std::string& plugin) {
      m_tileManagementPlugin = plugin;
    }

  private:
    Document* m_document;
    ImageSpec m_spec;
    PixelRatio m_pixelRatio;
    frame_t m_frames;                      // how many frames has this sprite
    std::vector<int> m_frlens;             // duration per frame
    PalettesList m_palettes;               // list of palettes
    LayerGroup* m_root;                    // main group of layers
    gfx::Rect m_gridBounds;                // grid settings

    // Current rgb map
    mutable RgbMapAlgorithm m_rgbMapAlgorithm;
    mutable std::unique_ptr<RgbMap> m_rgbMap;

    Tags m_tags;
    Slices m_slices;

    // Tilesets
    mutable Tilesets* m_tilesets;

    // Custom tile management plugin. This can be an ID that specifies
    // a custom plugin that will be used to handle tilesets and
    // tilemaps for this specific sprite. This property is saved
    // inside .aseprite files (ASE_EXTERNAL_FILE_TILE_MANAGEMENT), and
    // it's used by the UI to disable the standard tileset/tilemap UX
    // (e.g. drag & drop tiles, or TilesetMode::Auto mode, etc.),
    // giving the possibility to handle tiles exclusively to a plugin.
    std::string m_tileManagementPlugin;

    // Disable default constructor and copying
    Sprite();
    DISABLE_COPYING(Sprite);
  };

} // namespace doc

#endif
