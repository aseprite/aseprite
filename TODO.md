# Very high priority (next release?)

* Warning icon when selecting RGB/HSB color in indexed image.
* Warning message when we open a file that is already opened file
  (show an option to create a second view).
* Paste in place doesn't work as expected sometimes (copy something
  from one frame, paste in other frame).
* After moving a frame, all thumbnails are shown incorrectly.
* New sprite with palette of the clipboard
* Ctrl+Shift selection should copy and snap axis
* "Duplicate sprite" should copy the "Background" layer as Background.
* Fix problem applyToTarget() can throw (it's called from other thread)
* After flatten show the background if it is hidden.
* Paste does not paste in the correct position if the cel is moved.
* Alpha when pasting.
* Shade drawing mode.
* Improve status bar indicators (show origin of selection in floating state).
* Export to sprite sheet: if the file we're using is .ase, select .png by default
  (and then the latest used extension).
* Fix problem with export sprite sheet when the cel has negative pos
* Add a warning when exporting RGB sprite to GIF, hinting at the quantize function.
* MovingPixelsState: Add undo information in each transformation step.
* Add IntEntry class in src/gui/ with spin-buttons.
* Add feedback to "Shift+S" shortcut to switch "snap to grid".
* Add color swatches bar.
* Sort palette entries.
* Add "Remap" button to palette editor after a palette entry is modified:
  This button should apply a color curve to the whole sprite to remap
  old indexes to the new positions.
* Move launcher.cpp to base/ lib adding an extension point for "gui" lib.
* Move src/dialogs/aniedit,filesel to src/widgets (remove dialogs/ directory).
* Merge everything related to configuration/settings in one class
  (allow configuration per document). Use cfg.cpp and settings/ dir.
* Refactor src/file/ in several layers.

# WIP

* Data recovery (src/app/data_recovery.cpp)
* Projects (src/app/project.cpp)
* Scriting support (src/scripting/engine.cpp)

# Refactoring

* Split UndoTransaction class into:
  * DocumentTransaction class, and
  * DocumentOperations namespace or something like that.
* Use streams instead of FILEs.
* Destroy modules/gui.h.
* Convert update_screen_for_document in an event from contexts or
  something similar.
* Use a class to wrap calls load_window_pos/save_window_pos.
* Command::loadParams() should be called one time when the command
  params is read from the XML file, not each time the command is
  executed. For this all commands must be got from the CommandsList,
  then cloned, and finally filled with params.
* About Signals/Slots: Add some field in slots to avoid disconnecting
  them from dead signals.
* Replace JRect & jrect with gfx::Rect.
* editors_ -> MultiEditors class widget
* convert all widgets to classes:
  * match UI library design with Vaca library.
  * move all functions (jwidget_*) to methods in Widget class.
  * the same for each widget (e.g. jbutton_* to Button widget)
  * AppHooks to Vaca::Signals
* all member functions should be named verbNoun instead of verb_noun or noun_verb.
* all functions to handle an object should be converted to member functions:
  * e.g. jwidget_set_text -> Widget::setText

# High priority work

* Ctrl+T should transform the current cel
* fix bilinear: when getpixel have alpha = 0 get a neighbor color.
* fix sliders in Tools Configuration, they are too big
  for small resolutions.
* rewrite palette-editor to edit multiple-palettes.
  * fix quantize (one palette for all frames, one palette per frame)
  * pal-operations (sort, quantize, gamma by color-curves, etc.);
  * complete palette operations, and palette editor (it needs a slider
    or something to move between palette changes)
  * drag & drop colors.
* if there is activated the Tools Configuration dialog box, the
  Shift+G and Shift+S should update it
* add two DrawClick2:
  * DrawClick2FreeHand
  * DrawClick2Shape
* see the new Allegro's load_font
* finish ICO files support.
* add "size" to GUI font (for TTF fonts);
* layer movement between sets in animation-editor;
  * add all the "set" stuff again;
* fix algo_ellipsefill;
* view_tiled() should support animation playback (partial support:
  with left and right keys).

# Wish-list

* dacap wish-list:
  * added starred file-items in the file-selector.
  * tweening of cels (translate, scale, rotate)
  * tweening of polygons
  * selection of frames (to modify several frames at the same time)
* manuq wish-list:
  * layer-with-constant-cel
* Mateusz Czaplinski ideas:
  * when move selections, will be good the possibility to see relative
    position from the starting point of movement;
  * make drawing the 'marching-ants-rectangle' a prioritaire thing to
    draw (when move it).

# Low priority stuff

* add unit-tests for "raster" and file formats.
* test routines: load/save_pic_file, load/save_msk_file,
  load/save_col_file.
* fix the fli reader (both Allegro and Gfli): when a frame hasn't
  chunks means that it's looks like the last frame;
* talk with Elias Pschernig, his "do_ellipse_diameter" (algo_ellipse)
  has a bug with ellipses of some dimensions (I made a test, and a
  various sizes have errors).
