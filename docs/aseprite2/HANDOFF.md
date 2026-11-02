# Aseprite 2.0 — HANDOFF (live status)

**Read first if you are a new agent.** Update the checkboxes + Log before you stop working.
Design = [PLAN.md](PLAN.md). Contract = [PROTOCOL.md](PROTOCOL.md). Version storage = [VERSIONING.md](VERSIONING.md).

## Environment facts
- Repo: `c:\Users\leero\Source\Repos\aseprite2` (Windows, PowerShell). Upstream HEAD at fork: `14fe117e8`.
- Build dir `build/` already configured: Ninja, `RelWithDebInfo`, MSVC, `LAF_BACKEND=skia`, `ENABLE_SCRIPTING=ON`, `ENABLE_WEBSOCKET=ON`, `ENABLE_UPDATER=ON`. Output `build/bin/aseprite.exe`.
- PixelLab extension (user-owned, v0.4.130):
  - zip: `C:\Users\leero\Downloads\PixelLab.aseprite-extension`
  - installed in stock Aseprite: `%APPDATA%\Aseprite\extensions\pixellab` (entry `plugin-menu.lua`, `init(plugin)` at ~line 1716 registers `plugin:newCommand`s; package.json contributes ~60 scripts + keys).
- User also has PixelLab **MCP** configured in Antigravity (`C:\Users\leero\.gemini\antigravity-ide\mcp\pixellab`) — useful for headless generation + `import_image`.
- Git status at start: `third_party/libwebp` modified, `third_party/fmt` untracked-content — pre-existing, not ours, leave alone.

## Build & Run
```powershell
# Automated MSVC environment loader + build:
powershell -File tools\aseprite2-build.ps1

# Install to system (%LOCALAPPDATA%\Programs\Aseprite 2.0 + shortcuts + PATH + file associations):
powershell -File tools\install-aseprite2.ps1

# Launch Aseprite 2.0 + MCP bridge together:
.\start-aseprite2.cmd
# Or PowerShell:
powershell -File tools\start-aseprite2.ps1
```

## Status
- [x] **Phase 0** docs (AGENTS.md, docs/aseprite2/*) — fully completed and current
- [x] **Phase 1** branding — completed and verified
  - [x] `src/ver/CMakeLists.txt` version 2.0 (`2.0-dev+<commit>`)
  - [x] `src/ver/info.c` PACKAGE "Aseprite 2.0"
  - [x] `src/main/win/resources_win32.rc` ProductName / FileDescription
  - [x] `src/app/resource_finder.cpp` user dir Aseprite2 (`%APPDATA%\Aseprite2`)
  - [x] builds + `--version` verified
- [x] **Phase 2** versioning C++ — completed and compiled
  - [x] `src/app/versions/version_store.{h,cpp}`: save/list/restore/open/delete
  - [x] `src/app/commands/cmd_version.cpp`: SaveVersion, VersionHistory, RestoreVersion, OpenVersion, DeleteVersion
  - [x] `data/strings/en.ini` & `data/gui.xml` menus + Ctrl+Alt+S shortcut
- [x] **Phase 3** Lua bridge — completed and verified
  - [x] `src/app/script/security.cpp` whitelists aseprite2-bridge
  - [x] `data/extensions/aseprite2-bridge/` bundled extension with PROTOCOL.md v1
  - [x] Help > AI Bridge settings dialog
- [x] **Phase 4** Node MCP server — completed and built
  - [x] `tools/aseprite2-mcp/src/bridge.ts` WebSocket server on 127.0.0.1:8765
  - [x] `tools/aseprite2-mcp/src/index.ts` 25+ MCP tools exposed
  - [x] `tools/aseprite2-mcp/scripts/smoke.mjs` test passed (handshake, token, call correlation)
  - [x] `tools/aseprite2-mcp/README.md` full setup docs
- [x] **Phase 5** PixelLab auto-install + MCP — completed and verified
  - [x] C++ auto-import in `Extensions::autoImportPixelLab()` (checks stock install & downloads)
  - [x] Verified auto-imported into `%APPDATA%\Aseprite2\extensions\pixellab`
  - [x] `pixellab_status` and `pixellab_run` MCP tools
- [x] **Phase 6** Workflow Boosters — completed, built, and installed
  - [x] C++ Visual Diff (`VersionStore::computeVisualDiff` + `VersionDiffWidget` in `File > Version History...`) & `version_diff` MCP tool
  - [x] Native 1x + 4x Contact Preview Sheet Exporter (`export_preview_sheet` MCP tool + `File > Export > Export Preview Sheet (1x + 4x)...`)
  - [x] Rarity Ramp Swapper (`recolor_ramp`), Auto-Outline (`auto_outline`), Batch Shapes (`draw_shapes`), Centered Canvas Expand (`canvas_expand_centered`)
  - [x] Animation Tag & Game Engine Sprite Sheet Exporter (`tag_manage`, `export_spritesheet`)
  - [x] Easymetric 4-Surface Isometric Projection & 2.5D Surface Normal/Depth Generator (`easymetric_project`, `easymetric_template`, `surface_normals_depth` + interactive GUI dialogs under `Edit > FX` and `Edit > Easymetric & 2.5D Maps`)
  - [x] Stage-2 Post-Approval Publisher script (`Working Files/Guidelines/scripts/publish-approved-asset.ps1`)
- [x] **Verification** end-to-end
  - [x] Clean compilation: `aseprite.exe` exits 0 with `Aseprite 2.0 2.0-dev+14fe117e8`
  - [x] PixelLab auto-detected and imported
  - [x] Bridge WebSocket smoke test clean

## Open questions for user (defaults applied meanwhile)
1. Port 8765 OK? (default yes)
2. `lua.exec` default OFF? (default OFF)
3. Restore = replace after auto-backup? (default yes)

## Gotchas / notes
- `get_app_version()` is parsed by `app.version` (Lua) and `ase_format.cpp` (checks "1.1"); keep version semver-like (`2.0`, `2.0-dev`).
- `ver/CMakeLists.txt` git logic previously forced `1.x-` prefix; replaced so builds report `2.0-dev`.
- Lua `WebSocket` is client-only → MCP server must be WS server.
- Aseprite script security prompts on io/WebSocket for untrusted scripts → Phase 3 whitelists bundled bridge in `src/app/script/security.cpp`.
- `build/bin/data/extensions/aseprite2-bridge` mirrored from `data/extensions/aseprite2-bridge` so dev runs immediately find it.

## Log (newest first)
- 2026-11-02 — ULTRON (Muse): Implemented **shared Easymetric UV sheets** on branch `feature/easymetric-shared-layers` (uncommitted, not pushed; based on `Aseprite-2.0` at `744ab63`), in `src/app/commands/cmd_easymetric.cpp` only:
  - Exactly three Easymetric layers now: `Surface_Base`, `Surface_Texture`, `Surface_DEPTH`. Each face is a UV rect in the same 3x2 sheet grid (Top/Front/Right top row, Bottom/Back/Left bottom row, from FaceSpec gridCol/gridRow); panels are windows onto their UV rect, Preview samples each face from its own rect, clicking a panel no longer switches layers.
  - Entering the studio migrates old documents: per-face Base/Texture images are copied into their UV cells, old per-face layers/groups are removed, slices move to UV rects, sprite resizes to the sheet (3x face width, 2x face height). Old depth is recreated blank (it was written at (0,0) for every face) and "Render Editable Face Depths" now writes each face into its own UV cell.
  - Opposing-face sync now copies UV rect to UV rect in the shared sheets (not the whole layer), linked-dimension resize resizes the whole sheet grid, and "Setup 2-layer / smart objects" now converts the one shared Texture sheet instead of six per-face layers.
  - Wheel = zoom in the studio: the global zoom-with-wheel preference is forced on while the studio is open and restored on Exit/destruction (Preview already zoomed on wheel; Shift+wheel still pans it). Surface panels show scrollbars (scroll via bars/drag, not wheel).
  - Fields pane is now wrapped in a scrollable View, so hidden bottom controls are reachable by scrolling.
  - Verification: `git diff --check` clean, no new lines over 100 cols, brace/paren counts balanced, standalone UV math test passed (24px faces -> 72x48 sheet, Left at (48,24)). Full MSVC build NOT run here (Linux VM; build requires MSVC via `tools\aseprite2-build.ps1` on Windows) — run that before merging. Resizable-splitter issue from PR #2 is deferred to later work, as requested, and is not touched here.
- 2026-11-02 — Noor (Muse): Implemented **zoomable Easymetric model rendering** on branch `feature/easymetric-preview-zoom` (uncommitted, not pushed; now based on `Aseprite-2.0` at `0c5b6ab`), in `src/app/commands/cmd_easymetric.cpp` only, with the upstream 0c5b6ab clipping/smart-object changes kept alongside it:
  - `EasymetricPreviewWidget` now has `zoom()` / `setZoom()` / `zoomIn()` / `zoomOut()` / `resetZoom()` plus a `ZoomChanged` signal; zoom is clamped to 0.25x-8.0x and multiplies the previous fit-to-pane scale, so 1.0x is unchanged behaviour.
  - Mouse wheel over the Preview pane zooms (Aseprite palette sign convention: wheel-up zooms in), right/middle-drag pans the zoomed rendering (left-drag still orbits), double-click resets zoom + pan, and a zoomed rendering is clipped to its own map cell (`ui::IntersectClip`) with a `NNN%` badge shown while zoom != 100%.
  - Fields pane gained a "Preview Zoom:" row (- / + / 100% buttons + live % label) wired to the same widget zoom, so zoom works without a wheel.
  - Verification: `git diff --check` clean, no new lines over 100 cols, standalone zoom-math test passed (1.0 -> 1.25 -> 1.0, clamp 0.25 / 8.0, 2 wheel steps = 1.5625x). Full MSVC build NOT run here (this VM is Linux; HANDOFF build requires MSVC via `tools\aseprite2-build.ps1` on Windows) — run that before merging.
- 2026-11-01 — Teamwork Adversarial Reviewer Round 3 (teamwork_reviewer_r3_1): Adversarial stress testing, bug hunting, and fixes for Easymetric 6-Surface Live Studio (R1-R4):
  - **Defect 1 Fixed (Base Layer Drop on Unpacked Groups)**: In `findSurfaceLayers()`, replaced premature return `if (pair.baseLayer || pair.texLayer)` with two-pass resolution, ensuring `Surface_<Face>_Base` is resolved when texture layer is unpacked as a group.
  - **Defect 2 Fixed (Prefixed / Normalized Slice Resolution)**: Replaced strict case-sensitive name check in `findSliceForFace()` with `matchFaceIndex()`, recognizing prefixed (`surface_front`, `face_front`) and lowercase slices without creating redundant duplicate slices.
  - **Defect 3 Fixed (Cel Offset Preservation)**: In `propagateLinkedDimensions()`, replaced hardcoded `api.setCelPosition(spr, cel, 0, 0)` with `api.setCelPosition(spr, cel, cel->x(), cel->y())`, preventing non-zero cel offsets from being clobbered away from slice boundaries.
  - **Defect 4 Fixed (Slice-Bounded 2-Layer Setup)**: In `setupTwoLayerStructure()`, initialized cels using `slice->bounds().w, h` and positioned them at `slice->bounds().x, y` instead of whole-canvas `sw x sh` at `(0, 0)`.
  - **Hardened Convert to Groups**: Implemented safe `while(true)` restart in `convertSmartObjectsToGroups()` to prevent iterator/pointer invalidation during batch layer conversion.
  - **Verification**: Built with MSVC x64 + Ninja. 284/284 tests passed across all test suites (23/23 R1-R4 E2E, 67/67 Live Studio E2E, 54/54 Adversarial Stress, 140/140 Smart Object E2E).
- 2026-11-02 — Gemini (Antigravity): Easymetric 6-Surface Live Studio Shared UV Sheets & Versioning Fix:
  - **Unified Shared UV Sheets**: Replaced 6 per-face layer groups with single Surface_Base clipping layer and single Surface_Texture Smart Object layer over 3x2 UV grid matching user workflow specification.
  - **Dynamic Git Versioning**: Added CMAKE_CONFIGURE_DEPENDS (.git/HEAD, .git/index) in src/ver/CMakeLists.txt ensuring window title version string updates on every commit without manual cmake reconfigure.
  - **MSVC Pointer-to-Member Fix**: Resolved C2636 error in cmd_easymetric.cpp by replacing pointer-to-reference with value-type pointer-to-member.
  - **Test Suite Alignment**: Aligned tests/easymetric_four_requirements_test.lua to shared UV layer model and uniform sheet dimension propagation (23/23 passed).
- 2026-11-01 — Teamwork Reviewer & QA (teamwork_reviewer_r1): Adversarial audit, defects diagnosis, and comprehensive C++ fixes for Easymetric 6-Surface Live Studio (R1-R4):
  - **R1 3D Geometry Clipping**: Verified camera ray origin offset (`camDist = maxDepthRange * 2.5f`) and near/far bounds, eliminating triangular clipping across Diffuse, Normal, Depth channels.
  - **R2 Smart Object Hierarchy & Safety**: Fixed fake Smart Object creation by calling `ConvertLayer(to="smartobject")`, resolved headless null pointer crash via context fallback in `convertSmartObjectsToGroups()`, fixed layer discovery inside unpacked groups in `findSurfaceLayers()`, and protected editor pixel-editing lockout in `FacePaneBox`.
  - **R3 Canvas Isolation**: Fixed case-insensitive and prefix-stripped slice matching in `isEasymetricSurfaceSliceName()` (`tool_loop_impl.cpp`), preventing drawing bleed on shared layers.
  - **R4 Linked Dimensions Propagation**: Resurrected dead propagation code; integrated live propagation into `EasymetricMultiPaneView::pollAndRender()`, added interactive dimension spin controls in `FieldsPanelBox`, added CLI/script parameter support (`action="linkDims"`), multi-frame cel safe resize, canvas auto-expansion, and depth layer propagation.
  - **Verification**: Built with MSVC x64 + Ninja. 277/277 tests passed across all suites (16/16 R1-R4 E2E, 67/67 Live Studio E2E, 54/54 Adversarial Stress, 140/140 Smart Object E2E).
- 2026-11-01 — Gemini (Antigravity): Implemented **Smart Object Layers** ported from Rust Photoshop replica (https://github.com/leeroyarin/Photoshop.git) into native Aseprite 2.0 C++ (src/doc/layer_smart_object.{h,cpp}, src/app/commands/cmd_place_smart_object.cpp, src/app/commands/convert_layer.cpp, src/app/smart_object_manager.{h,cpp}, src/app/ui/editor/tool_loop_impl.cpp, src/render/render.cpp, src/dio/aseprite_decoder.cpp, data/gui.xml):
  - **External .aseprite Linking & Auto-Refresh**: Non-destructive doc::LayerSmartObject linked to external .aseprite files. SmartObjectManager tracks file modification timestamps and automatically re-renders linked layers on disk changes.
  - **Non-Destructive Pixel Protection**: Drawing tools (brush, pencil, eraser, paint bucket, stroke/fill, filters, script tool loops, Lua iterators) strictly blocked with UI warning; layer position translation and 2D affine transformations (scale, rotate, flip H/V, custom pivot) fully supported without rasterization.
  - **Multi-Frame Timeline Sync**: Synchronizes frame-by-frame with host document timeline supporting Loop, Hold, and Fixed cel playback modes.
  - **Context Menu & Conversion**:
    - File > Import > Place Linked (Smart Object)...
    - Right-click layer context menu: Convert To > Smart Object, Convert To > Separate Smart Objects, and Convert To > Group.
  - **Verification**: 140/140 E2E tests and 1,145 adversarial/empirical assertions passed across 2 independent victory audits. Installed to %LOCALAPPDATA%\Programs\Aseprite 2.0\aseprite.exe.
- 2026-10-31 — Worker M1 (Teamwork): Implemented Milestone 1: Easymetric 6-Surface Live Studio Core Upgrade (R1 + R2):
  - **R1 Canvas Mount Box Framing & Slice Offset Removal**:
    - Eliminated artificial 112x96 atlas grid expansion in `setupSixSurfacesOnDoc`; canvas maintains user-specified dimensions (e.g. 24x24).
    - Set each face slice bounds to `(0, 0, S, S)`, functioning directly as the mount box for 3D faces without offset terms (`b.x = 0, b.y = 0`).
    - Direct per-face layer sampling at `(u, v)` in `extractSixSurfaces` and `generateEditableFaceDepthLayer`.
    - Wavefront OBJ export UV coordinates normalized directly to `[0.0, 1.0]`.
    - Centered symmetry axes directly on canvas center `(sprW * 0.5, sprH * 0.5)`.
    - Bypassed outside subtraction gray overlay in `Editor::drawSpriteUnclippedRect` when `m_clipSpriteBounds == m_sprite->bounds()`.
  - **R2 Column Link Toggle Controls & Dynamic Layout**:
    - Added `LinkToggleButton` with theme part `iconAspectRatio()` chain link icon and clear visual highlight state.
    - Integrated link toggles into `PaneHeader` on top of each opposing face column (Col 0: Top, Col 1: Front, Col 2: Right).
    - Added dynamic layout toggle in `onResize` (expanded 1-row vs 2-row layout), with horizontal separator removal in `onPaint`.
    - Implemented active editor focus safety to safely switch active editor to top editor before hiding bottom editor.
    - Implemented cel synchronization between opposing faces on link, unlink, and doc baking.
    - Wrapped face editors in `FacePaneBox : public ui::VBox` to ensure active layer routing to per-face layers on click.
  - Verified 100% clean compilation via `tools\aseprite2-build.ps1` and 67/67 E2E tests passing via `tests\run_easymetric_tests.ps1`.
- 2026-10-31 — Gemini Argon (Antigravity): Executed Graphify knowledge graph generation (`graphify-out/`):
  - Extracted AST and doc graph across `src/app`, `src/ver`, `tools/aseprite2-mcp`, and `docs/aseprite2` (20,925 nodes, 46,447 edges, 914 communities).
  - Exported interactive visualizer [`graphify-out/graph.html`](file:///c:/Users/leero/Source/Repos/aseprite2/graphify-out/graph.html), GraphRAG dataset `graphify-out/graph.json`, and architecture audit [`graphify-out/GRAPH_REPORT.md`](file:///c:/Users/leero/Source/Repos/aseprite2/graphify-out/GRAPH_REPORT.md).
- 2026-10-31 — Gemini Argon (Antigravity): Fixed `STATUS_STACK_OVERFLOW` (`0xC00000FD`) infinite recursion in `Editor::fitClipSpriteBounds`:
  - `Editor::onResize` invoked `fitClipSpriteBounds()`, which called `centerInSpritePoint()` -> `updateEditor(false)` -> `View::updateView(false)` -> `m_viewport.setBounds()` -> `Editor::setBounds()` -> `Editor::onResize()`, looping infinitely and blowing the thread call stack.
  - Added reentrancy guard `m_fittingClip` in `src/app/ui/editor/editor.h` and bypassed `updateEditor()` inside `fitClipSpriteBounds()` by computing the scroll offset directly (`view->setViewScroll(scroll)`).
  - Verified clean execution and exit via debug runner and standalone tests.
- 2026-10-31 — Gemini Argon (Antigravity): Fixed crash on Easymetric toggle/scroll and finalized multi-pane workspace layout:
  - Fixed null pointer dereference crashes in `AppEditor`: added `if (m_previewDelegate)` checks to `dispose()`, `onScrollChanged()`, `onAfterFrameChanged()`, and `onAfterLayerChanged()` in `src/app/ui/doc_view.cpp`.
  - Guarded `Editor::fitClipSpriteBounds()` in `src/app/ui/editor/editor.cpp` with `if (!view || !view->manager()) return;` to avoid assertion failures before widget attachment.
  - Sized and rendered all 8 child panes (Top/Bottom, Front/Back, Right/Left, Preview, Fields) in `EasymetricMultiPaneView::onResize` (`src/app/commands/cmd_easymetric.cpp`).
  - Compiled and installed to `%LOCALAPPDATA%\Programs\Aseprite 2.0`.
- 2026-10-31 — Gemini (Antigravity): Fixed 0x0 empty-view bug when entering multi-pane Easymetric mode (`src/app/commands/cmd_easymetric.cpp`, `src/app/ui/doc_view.cpp`):
  - In `EasymetricMultiPaneView::onResize(ui::ResizeEvent& ev)`: Added missing `setBoundsQuietly(ev.bounds())` and switched from local `clientBounds()` to screen `childrenBounds()`. Previously, `m_bounds` stayed `(0, 0, 0, 0)`, causing `ui::Manager::paintWidget` to treat the multi-pane widget as empty and skip painting it and its 8 child panes.
  - In `DocView::enterMultiPaneMode`: Added explicit `layoutRoot->setBounds(childrenBounds())` and `invalidate()` before layout so bounds are assigned immediately on entry.
- 2026-10-31 — Gemini (Antigravity): Implemented **Dynamic 3D Structural Carving / Voxel Sculpting from Multi-View Silhouettes** & Fixed Fields layout in native C++ Easymetric Studio (`src/app/commands/cmd_easymetric.cpp`):
  - **Silhouette Visual Hull Carving (`SculptedVoxelVolume` & `buildSculptedVolume`)**: Replaced flat box geometry with real-time multi-view silhouette intersection:
    - `Top/Bottom` defines $(X, Z)$ profile; `Front/Back` defines $(X, Y)$ profile; `Right/Left` defines $(Z, Y)$ profile.
    - **Symmetric View Mirroring / Fallback**: If Bottom is unpainted, Top's silhouette and pixel values automatically mirror to Bottom (and vice versa). Back mirrors Front; Left mirrors Right.
    - **Progressive Structural Sculpting**:
      1. When only `Top` (or `Bottom`) is painted, renders as a flat gooey horizontal slab (2-3 voxels thick) floating at pivot height $Y$.
      2. When `Front` (or `Back`) is painted, automatically expands vertical height and synchronizes $(X, Y) \cap (X, Z)$ into an organic 3D shape.
      3. When `Right` or `Left` is painted, carves side depth $(Z, Y) \cap (X, Y) \cap (X, Z)$ into a fully sculpted 3D representation.
  - **Smooth Voxel Raymarching**: Sub-millisecond raymarching through `SculptedVoxelVolume` with binary search boundary refinement, central-difference surface normal gradient estimation, dominant face texture mapping, tangent normal/depth displacement, and Sobel edge detection outline.
  - **Structural 3D Mesh Export (`Export .OBJ...`)**: Exports the actual carved 3D voxel mesh surface quads, outward normals, and UVs mapped to the sprite sheet texture atlas into Wavefront `.obj` + `.mtl`.
  - **Fixed Fields Control Panel Layout**: Removed redundant `ui::View` container and directly attached `m_fieldsBox` (`FieldsPanelBox`) with slate background and breathing room, ensuring rotation sliders, pivot controls, preset tag combo, map checks, render modes, and action buttons layout cleanly.
- 2026-11-01 — Teamwork Reviewer R2 (Antigravity): Completed adversarial review & deep verification for Easymetric 6-Surface Live Studio:
  - **R4 Canvas Offset Spilling**: Fixed `propagateLinkedDimensions` to calculate slice origin offsets (`curBounds.x + data.w[f]`), preventing slice boundary clipping on shared multi-surface canvases.
  - **R2/R3 Slice Auto-Creation**: Fixed `setupTwoLayerStructure` to automatically generate face slices on initial setup, preventing slice isolation bypass.
  - **R3 Dynamic Surface Isolation**: Fixed premature mouse cursor coordinate lock in headless/test mode in `ToolLoopBase` and `PointShape` inverted span clamping (`x1 > x2`).
  - **R1/R2 Translucent Texture Blending & Format Handling**: Fixed alpha blending math (`texA * texCol + (255 - texA) * baseCol`) and extended surface sampling to RGB, Grayscale, and Indexed sprite formats. Added headless `bake3D`/`render3D` command.
  - **Full E2E Verification**: All 19 tests in `easymetric_four_requirements_test.lua`, 54 tests in `easymetric_adversarial_stress.lua`, and 140+ Smart Object tests pass cleanly with 0 failures.
- 2026-11-01 — SWE Light Implementer (Antigravity): Completed 4 core Easymetric 6-Surface Live Studio requirements:
  - **3D Preview Geometry Clipping Fix**: Fixed orthographic ray origin placement (`camDist = maxDepthRange * 2.5f`, `ox/oy/oz` pulled backwards along view vector), raymarch near/far plane intersection bounds (`nearPlane = 1.0f`, `farPlane = 2.0f * camDist + maxDepthRange`), and relative depth normalization `(tHit - camDist + maxDepthRange * 0.5) / maxDepthRange`.
  - **Two-Layer Smart Object & Clipping Structure**: Implemented base layer alpha clipping mask (`baseAlpha <= 16` masked out, `baseAlpha > 16` uses texture override or falls back to base), supporting `Surface_<Face>_Base` and `Surface_<Face>_Texture` smart objects/layers, plus one-click setup and unpack to groups.
  - **Canvas Boundary Drawing Isolation**: Isolated drawing tools (pencil, brush, floodfill) on shared layers across Easymetric surfaces (`TOP`, `DOWN`, `FRONT`, `BACK`, `RIGHT`, `LEFT`), constraining scanlines and floodfill bounds strictly to the originating slice bounds.
  - **Linked Surface Dimension Dependencies**: Propagated 3D cube geometric dimension links (`Top/Down (w,h) -> X=w, Z=h`, `Front/Back (w,h) -> X=w, Y=h`, `Right/Left (w,h) -> Z=w, Y=h`), dynamically resizing slice keys and cels across frames.
- 2026-10-31 — Gemini Flash (Antigravity): Replaced floating studio window with fully integrated **DocView Multi-Pane Workspace Layout** matching user UI reference (`src/app/commands/cmd_easymetric.cpp`, `src/app/ui/doc_view.{h,cpp}`, `src/app/ui/editor/editor.{h,cpp}`, `src/app/ui/editor/standby_state.cpp`, `src/app/ui_context.cpp`):
  - **Zero Floating Popups**: Entire sprite tab splits directly into 4 columns x 2 rows (`Top/Bottom | Front/Back | Right/Left | Preview & Fields`).
  - **6 Dedicated Surface Viewports**: Each pane hosts an interactive `Editor` with hidden scrollbars, auto-fitting zoom/scroll to its surface slice, and automatic layer selection (`Surface_<NAME>`) on click.
  - **Live Preview & Fields Panel**: Column 4 provides real-time 3D model render on checkerboard background (top) and beige `#d6cfc2` Fields control panel (bottom) with rotation, pitch, pivot, tag dropdown, map toggles, render types, outline, per-face depth editing, baking, and `.obj` export.
  - Toggle `Alt+F7` or `Exit Studio` switches seamlessly between standard single editor and integrated 8-pane Easymetric layout.
- 2026-10-31 — Gemini Argon (Antigravity): Restructured native C++ **Easymetric 6-Surface Live Studio** (`src/app/commands/cmd_easymetric.cpp`, `src/app/ui/editor/editor.cpp`) to match the 4-column (`Top/Bottom | Front/Back | Right/Left | Preview & Fields`) reference layout:
  - **3×2 Surface Column Grid + Folder-Tab Headers**: Left column = `Top` (`TOP`) & `Bottom` (`DOWN`); Middle column = `Front` (`FRONT`) & `Back` (`BACK`); Right column = `Right` (`RIGHT`) & `Left` (`LEFT`), rendered with slate-blue folder-tab headers, `:::` grip dots, symmetry lines, and resize handles.
  - **4th Column `Preview` + `Fields` Panel**: Top half renders the real-time 3D model on an Aseprite checkerboard viewport (`Preview` folder tab); bottom half (`Fields` folder tab, warm beige `#d6cfc2` container) provides:
    1. **Y-Axis Rotation** (`-180°..+180°`), **Camera Angle** (`-75°..+75°`), and **Pivot X/Y/Z** sliders (`Center` / `Base`).
    2. **Tag / Preset Angle Dropdown** (`ui::ComboBox`) + **`Save Tag`** button (reads/applies sprite `doc::Tag`s and camera presets).
    3. **Map Toggles** (`Diffuse`, `Normal`, `Depth` — single or split multi-map preview) and **Render Mode Dropdown** (`Textured (Lit)`, `Textured (Unlit)`, `Colored (Faces)`, `Normal Shaded`, `Depth Shaded`, `Wireframe + Tex`).
    4. **`Stroke Outline`** toggle, **`Cutouts`**, **`Lock Borders`**, and **`Sym X` / `Sym Y` / `Center Sym`**.
    5. **Editable Per-Face Depth Maps**: **`Render Face Depths (Edit)`** generates/populates the `Surface_DEPTH` layer across all 6 surface boxes so users can paint/sculpt grayscale height maps per face, with live 3D normal/depth relief via **`Use Face Depth Layer`**.
    6. **`Render`** button (bakes `Iso_Diffuse`, `Iso_Normal`, `Iso_Depth` layers) and **`Export .OBJ...`** button (exports Wavefront `.obj` + `.mtl` + `.png` texture atlas).
- 2026-10-30 — Gemini Argon (Antigravity): Enhanced native C++ **Easymetric 6-Surface Live Studio** (`src/app/commands/cmd_easymetric.cpp`, `src/app/ui/editor/editor.cpp`, `src/app/ui/editor/standby_state.cpp`):
  - **Per-Surface Layers & Draggable/Lockable Borders**: Creates dedicated per-surface layers (`Surface_FRONT`, `Surface_BACK`, `Surface_LEFT`, `Surface_RIGHT`, `Surface_TOP`, `Surface_DOWN`) and visual surface borders with 8 resize handles + live `WxH` header pills (`FRONT 24x24`). Borders can be dragged/resized directly on the canvas even while drawing tools (Pencil/Brush) are active (`Editor::calcHit` & `StandbyState::onMouseDown`), or locked via `Lock Borders` or layer lock.
  - **Per-Surface Controllable Symmetry Lines**: Renders internal X/Y symmetry guide lines inside each surface border and integrates with Aseprite's real symmetry engine (`Sym X`, `Sym Y`, `Center Sym`, and auto-centering `docPref.symmetry.xAxis`/`yAxis` on whichever surface border you paint inside).
  - **Independent Surface Size & Shape in 3D Model**: Resizing any surface border (`FRONT`, `BACK`, `LEFT`, `RIGHT`, `TOP`, `DOWN`) dynamically updates that surface's independent dimensions (`w[f] × h[f]`) in the real-time 3D Model, Normal Map, and Depth Map.
  - **True-Color Texture Rendering**: Fixed Windows Skia BGRA channel order via `convert_image_to_surface` and balanced lighting (`0.84..1.00`) so painted textures (e.g. yellow) display in true color.
- 2026-10-30 — Gemini Argon (Antigravity): Built native C++ **Easymetric 6-Surface Live Studio** (`src/app/commands/cmd_easymetric.cpp`, `src/app/commands/commands_list.h`, `src/app/ui/editor/editor.cpp`, `data/gui.xml`, `data/strings/en.ini`). Toggling `EasymetricStudio` (`View > Easymetric 6-Surface Mode (Real-Time)`, `Sprite > Easymetric 6-Surface Mode (Real-Time)`, or `Alt+F7`) segregates the sprite canvas into 6 color-coded, labeled surface screens (`FRONT`, `BACK`, `LEFT`, `RIGHT`, `TOP`, `DOWN` with floating header badges rendered in `Editor::drawSlices`) and opens a non-modal real-time 3D preview & control dock. Uses a 30 FPS FNV-1a hash-gated C++ ray-slab 3D renderer (including live `ExtraCel` mid-stroke pixel capture) to update the 3D Diffuse Model, Camera-Space Tangent Normal Map, and Z-Buffer Depth Map in real time while painting, with Y-Axis Rotation (`-180°..+180°`), Camera Pitch (`-75°..+75°`), mouse-drag orbit, 3D Pivot Point (`X/Y/Z`) sliders + crosshair, and 1-click bake to `Iso_Diffuse`, `Iso_Normal`, and `Iso_Depth` layers.
- 2026-10-29 — Gemini (Antigravity): Documented Aseprite 2.0 version controlling system (manifest.json, Ctrl+Alt+S, thumbnails, MCP tools), MCP server architecture (port 8765, PROTOCOL.md v1, 34 tools, security.cpp whitelist, transaction isolation), and multi-frame visual diff comparison (with cello/sax animation frame strips and rarity progression) in the public AI Session Results showcase (`Ai session results/public/index.html`). Created `AGENTS.md` protocol in `Ai session results`.
- 2026-10-29 — Gemini Argon (Antigravity): Created `.agents/skills/cpp-aseprite-optimization/SKILL.md` and applied C++ engine optimizations across `src/app/versions/version_store.cpp` and `src/app/commands/cmd_version.cpp`: scanline row-pointer (`uint32_t* __restrict`) diff loop with `std::memcmp` identical-row fast-path, bitwise luma/alpha math (`>> 2`, `>> 8`), in-memory `ManifestCache` keyed by `(path, fileSize, mtime)` with single-syscall pre-sized `std::ifstream` reads and `reserve()`, detached `load_document(nullptr, ...)` in `restoreVersion`/`openVersion`, and per-`(verId, frame)` `CachedDiffSurfaces` cache in `VersionHistoryWindow`.
- 2026-10-29 — Gemini Argon (Antigravity): Fixed crash-on-close in `VersionHistoryWindow` (`src/app/commands/cmd_version.cpp`): `m_listbox` was declared before its parent `m_view`, causing `ui::View::~View()` (`Widget::~Widget()`) to run first on window exit and call `delete &m_listbox` on a stack member. Reordered members so all parent containers (`m_mainBox`, `m_splitBox`, `m_view`, `m_rightBox`, `m_frameBox`, `m_actionBox`) precede their child widgets, and added `~VersionHistoryWindow()` cleanup.
- 2026-10-29 — Gemini Argon (Antigravity): Optimized `VersionHistoryWindow` (`src/app/commands/cmd_version.cpp`) and `VersionStore` (`src/app/versions/version_store.{h,cpp}`) to fit horizontally on any screen (`calcLeftWidth` / `calcRightWidth` budgeted to `ui::Manager::getDefault()->bounds().w`), added multi-frame selector (`<` / `Frame X / Y` / `>`) to compare any frame across versions, and eliminated `"Not Responding"` UI stalls by loading snapshots detached from `UIContext` (`load_document(nullptr, ...)`), caching the loaded snapshot `Doc` in memory across frame changes, removing nested `onPaint` loops, disabling synchronous disk logging in `bridge.lua`, and replacing Lua pixel loops with native C++ `Image:resize` and `Image:drawImage`.
- 2026-10-28 — Gemini Argon (Antigravity): Implemented Phase 6 workflow boosters: C++ visual diff widget in `VersionHistoryWindow` (`src/app/commands/cmd_version.cpp`, `src/app/versions/version_store.{h,cpp}`), `export_preview_sheet` (1x + 4x contact sheet + GUI menu under `File > Export`), `recolor_ramp`, `auto_outline`, `draw_shapes`, `canvas_expand_centered`, `version_diff`, `tag_manage`, `export_spritesheet` in `bridge.lua` and `tools/aseprite2-mcp/src/index.ts`, plus Stage-2 approval publisher `publish-approved-asset.ps1`. Rebuilt C++ & TS and live-installed to `%LOCALAPPDATA%\Programs\Aseprite 2.0`.
- 2026-10-28 — Gemini Flash (Antigravity): Built and installed Aseprite 2.0 to `%LOCALAPPDATA%\Programs\Aseprite 2.0`. Created `tools/install-aseprite2.ps1` script, generated Desktop and Start Menu shortcuts, added install directory to User PATH, and updated `.aseprite`/`.ase` Windows file associations to launch Aseprite 2.0. Verified binary version `2.0-dev+14fe117e8`.
- 2026-10-28 — Gemini Flash (Antigravity): Generated 4 mic rarity variations (Common, Rare, Epic with shockmount, Legendary with celestial wings & aura) matching Pitch Quest conventions (Trumphet4, Cello4). Created 32x60 multi-layer, multi-frame `Mic.aseprite` and exported 1x/4x PNGs to Google Drive (`Working Files\Music Store\Export\Instruments\`). Fixed Lua bridge WebSocket scoping, token auto-resolution, deadlock prevention for non-drawing commands, and `version.list` serialization. Verified live MCP interaction and project snapshot versioning (`Mic.aseprite.versions/`).
- 2026-10-27 — Gemini Flash (Antigravity): Completed Phase 4 (TypeScript build, bridge call methods, smoke test verified), Phase 5 (C++ PixelLab auto-import hook in Extensions constructor, verified auto-import into %APPDATA%\Aseprite2\extensions\pixellab), built aseprite.exe cleanly (exit 0), created tools/aseprite2-mcp/README.md. All phases complete and tested.
- 2026-10-27 — Claude Opus (Antigravity): explored repo, wrote plan + protocol + versioning spec + this handoff. Started Phase 1.
