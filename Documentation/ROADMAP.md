# Custom Aseprite - Feature Roadmap

## Overview

This document outlines planned tools and features for the Custom Aseprite build, focusing on improving pixel art creation workflow and automation.

---

## Completed Features

### Auto-Shade Tool
- [x] Click-to-shade with automatic region detection
- [x] 16 shading styles (ClassicCartoon, SoftCartoon, OilSoft, etc.)
- [x] Shape types (Sphere, Adaptive, Cylinder, Flat)
- [x] Draggable highlight point
- [x] Light angle and elevation controls
- [x] Reflected light and selective outlining

### Palette Generator
- [x] Generate shading ramps from base color
- [x] Material types (Matte, Glossy, Metallic, Skin)
- [x] Style presets (Natural, Vibrant, Muted, Dramatic)
- [x] Color harmonies (Monochromatic, Analogous, Complementary, Triadic, etc.)
- [x] Temperature shifts (Warm/Cool highlights)
- [x] Add to palette / Replace palette / Set FG/BG
- [x] Unit tests for color calculations

---

## Phase 1: Essential Tools (High Priority)

### 1. Advanced Dithering Tool
**Purpose:** Create smooth gradients with limited palettes

**Features:**
- [ ] Dithering patterns library
  - Checkerboard (2x2)
  - Bayer matrix (2x2, 4x4, 8x8)
  - Custom pattern editor
- [ ] Dithering algorithms
  - Ordered dithering
  - Floyd-Steinberg error diffusion
  - Atkinson dithering
- [ ] Gradient fill with dithering
  - Linear and radial gradients
  - Palette-aware color stops
  - Preview before applying
- [ ] Dither brush mode
  - Paint with dithering patterns
  - Configurable density

**Technical Notes:**
- Integrate with existing gradient tool
- Use current palette colors only
- Support undo/redo

**References:**
- [Drububu Dithering Tutorial](https://www.drububu.com/tutorial/pixel-art-and-dithering.html)
- [Pixel Parmesan Dithering Guide](https://pixelparmesan.com/blog/dithering-for-pixel-artists)

---

### 2. Selective Outline Generator
**Purpose:** Auto-generate shaded outlines based on lighting

**Features:**
- [ ] Outline detection
  - Edge detection on selection/layer
  - Inner and outer outline modes
- [ ] Light-aware coloring
  - Lighter outlines where light hits
  - Darker outlines in shadow areas
  - Uses existing shading as reference
- [ ] Outline styles
  - Solid color
  - Gradient (light to dark)
  - Per-edge coloring
- [ ] Configuration
  - Outline thickness (1-3 pixels)
  - Color offset from adjacent pixels
  - Separate light/shadow outline colors

**Technical Notes:**
- Reuse light angle from Auto-Shade tool
- Support both selection-based and layer-based operation
- Non-destructive preview mode

**References:**
- [Lospec Selective Outlining](https://lospec.com/pixel-art-tutorials/tags/selectiveoutlining)
- [Lospec Outlines Tutorial](https://lospec.com/articles/pixel-art-outlines-part-2-using-color)

---

### 3. Normal Map Generator
**Purpose:** Generate normal maps for 2D sprites for use in game engines

**Features:**
- [ ] Generation methods
  - Height map conversion
  - Sobel operator from sprite
  - Edge-based estimation
- [ ] Real-time preview
  - Interactive light source
  - Preview lighting on sprite
- [ ] Export options
  - Standard normal map (RGB)
  - Separate channels
  - Multiple resolutions
- [ ] Manual editing
  - Paint normals directly
  - Angle brush for direction
  - Smooth/sharpen tools

**Technical Notes:**
- Output as separate layer or file
- Support common game engine formats (Unity, Godot, GameMaker)
- Consider height map intermediate step

**References:**
- [Laigter](https://azagaya.itch.io/laigter)
- [Aseprite Normal Toolkit](https://mooosik.itch.io/normal-toolkit)
- [SpriteIlluminator](https://www.codeandweb.com/spriteilluminator)

---

## Phase 2: Grid & Perspective Tools (Medium Priority)

### 4. Isometric Grid Helper
**Purpose:** Assist with isometric pixel art creation

**Features:**
- [ ] Grid overlays
  - 2:1 line pattern (standard isometric)
  - Common tile sizes: 32x16, 64x32, 128x64
  - Hexagonal grids
- [ ] Snap-to-grid
  - Snap cursor to isometric grid
  - Constrain lines to isometric angles
- [ ] Isometric primitives
  - Cube generator
  - Cylinder generator
  - Floor/wall tiles
- [ ] Grid visibility
  - Toggle overlay
  - Adjustable opacity
  - Custom grid colors

**Technical Notes:**
- Non-destructive overlay layer
- Grid settings saved per document
- Export without grid

**References:**
- [SLYNYRD Isometric Tutorial](https://www.slynyrd.com/blog/2022/11/28/pixelblog-41-isometric-pixel-art)
- [Pixel Parmesan Isometric Fundamentals](https://pixelparmesan.com/blog/fundamentals-of-isometric-pixel-art)

---

### 5. Perspective Grid Tool
**Purpose:** Create perspective guides for architectural/environment art

**Features:**
- [ ] Perspective types
  - 1-point perspective
  - 2-point perspective
  - 3-point perspective (optional)
- [ ] Interactive vanishing points
  - Draggable VP markers
  - Grid line density control
- [ ] Guide properties
  - Non-pixelated anti-aliased lines
  - Adjustable opacity
  - Color customization
- [ ] Animation support
  - Animate vanishing point position
  - Per-frame perspective

**Technical Notes:**
- Render guides in separate overlay
- Consider integration with existing grid system

**References:**
- [1-Point Perspective Helper for Aseprite](https://gunturtle.itch.io/perspective-helper)

---

## Phase 3: Animation & Advanced Tools (Lower Priority)

### 6. Sub-Pixel Animation Helper
**Purpose:** Create smooth animations at very low resolutions

**Features:**
- [ ] Color blending for micro-movements
- [ ] Preview at multiple scales
- [ ] Onion skin with sub-pixel visualization
- [ ] Template animations (walk cycles, idle)

---

### 7. Cluster Analysis Tool
**Purpose:** Optimize pixel placement and detect issues

**Features:**
- [ ] Visualize pixel clusters by size
- [ ] Detect "jaggies" (uneven diagonal lines)
- [ ] Suggest smoothing corrections
- [ ] Highlight orphan pixels

---

### 8. Batch Export Automation
**Purpose:** Streamline export workflow for game development

**Features:**
- [ ] Export presets (1x, 2x, 4x scales)
- [ ] Sprite sheet generation with metadata
- [ ] Command-line export support
- [ ] Watch folder for auto-export

---

## Phase 4: Workflow Improvements

### 9. Quick Inking Panel
**Purpose:** Fast access to common inking operations

**Features:**
- [ ] Modifier + right-click popup
- [ ] Recent colors
- [ ] Brush presets
- [ ] Quick layer operations

---

### 10. HSL Color Bars
**Purpose:** Alternative color selection method

**Features:**
- [ ] Hue/Saturation/Lightness sliders
- [ ] Display below color wheel
- [ ] Numeric input support
- [ ] Color history integration

---

## Implementation Priority Matrix

| Tool | Complexity | User Value | Priority |
|------|------------|------------|----------|
| Advanced Dithering | Medium | High | 1 |
| Selective Outline | Medium | High | 2 |
| Normal Map Generator | High | Medium | 3 |
| Isometric Grid | Medium | Medium | 4 |
| Perspective Grid | Medium | Medium | 5 |
| Sub-Pixel Animation | High | Low | 6 |
| Cluster Analysis | Medium | Low | 7 |
| Batch Export | Low | Medium | 8 |
| Quick Inking Panel | Low | Medium | 9 |
| HSL Color Bars | Low | Low | 10 |

---

## Technical Considerations

### Code Organization
- New tools should follow existing patterns in `src/app/tools/`
- UI popups go in `src/app/ui/`
- Commands registered in `src/app/commands/`
- Shared utilities in `src/app/tools/auto_shade/` or new dedicated folders

### Testing Strategy
- Unit tests for algorithmic components
- Test files in `tests/` subdirectories
- Standalone test executables for quick verification

### UI Guidelines
- Match existing Aseprite UI style
- Use existing widgets (ColorButton, Slider, ComboBox)
- Support keyboard shortcuts
- Provide real-time preview where possible

---

## Contributing

When implementing a new feature:
1. Create feature branch from `feature/auto-shade-improvements`
2. Implement core algorithm with tests
3. Add UI integration
4. Update this roadmap document
5. Create commit with descriptive message

---

*Last updated: January 2026*
