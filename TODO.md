# Hard to reproduce bugs

* add PRINTFs to observers, there is something wrong with save as
* sometimes, when the background layer is in a strange state:
  * two layers, hide background, the extra cel/brush preview isn't visible in the mini-editor
  * timeline: can move background
* does lock alpha work correctly?
* does onscrollchange notification calls onscrollchange notification?
* random clicks on toolbar crashes the program
* click Desktop item in file selector crashes the program

# Tasks

* onion-skin-bug.ase: See frame 2 with onion skin, there is a bug
* update copyright year of source files (automate this in with a script)
* Remove Image::getPixelAddress(), it shouldn't be public and almost
  everywhere we should use iterators for images. Also get/put_pixel_fast
  and ImageImpl::address() are dangerous.
* Warning icon when selecting RGB/HSB color in indexed image.
* Warning message when we open a file that is already opened
  (show an option to create a second view, or maybe this should
  be forbidden).
* New sprite with palette of the clipboard
* Ctrl+Shift selection should copy and snap axis
* "Duplicate sprite" should copy the "Background" layer as Background.
* Fix problem applyToTarget() can throw (it's called from other thread)
* After flatten show the background if it is hidden.
* Alpha when pasting.
* Shade drawing mode.
* Improve status bar indicators (show origin of selection in floating state).
* Export to sprite sheet: if the file we're using is .ase, select .png by default
  (and then the latest used extension).
* Fix problem with export sprite sheet when the cel has negative pos
* Add a warning when exporting RGB sprite to GIF, hinting at the quantize function.
* MovingPixelsState: Add undo information in each transformation step.
* Add IntEntry class in src/ui/ with spin-buttons.
* Add feedback to "Shift+S" shortcut to switch "snap to grid".
* Add color swatches bar.
* Sort palette entries.
* Add "Remap" button to palette editor after a palette entry is modified:
  This button should apply a color curve to the whole sprite to remap
  old indexes to the new positions.

# Refactoring

* Merge everything related to configuration/settings in one class
  (allow configuration per document). Use cfg.cpp and settings/ dir.
* Refactor src/file/ in several layers.
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
* editors_ -> MultiEditors class widget
* all member functions should be named verbNoun instead of verb_noun or noun_verb.

# Old issues

* fix bilinear: when getpixel have alpha = 0 get a neighbor color.
* fix sliders in Tools Configuration, they are too big
  for small resolutions.
* rewrite palette-editor to edit multiple-palettes.
  * fix quantize (one palette for all frames, one palette per frame)
  * pal-operations (sort, quantize, gamma by color-curves, etc.);
  * complete palette operations, and palette editor (it needs a slider
    or something to move between palette changes)
  * drag & drop colors.
* if the Tools Configuration dialog box is activated, Shift+G and
  Shift+S keys should update it.
* add two DrawClick2:
  * DrawClick2FreeHand
  * DrawClick2Shape
* see the new Allegro's load_font
* review ICO files support.
* add "size" to GUI font (for TTF fonts);
* layer movement between sets in animation-editor;
  * add all the "set" stuff again;
* fix algo_ellipsefill;
* view_tiled() should support animation playback (partial support:
  with left and right keys).
* fix the fli reader (both Allegro and Gfli): when a frame hasn't
  chunks means that it's looks like the last frame;
* talk with Elias Pschernig, his "do_ellipse_diameter" (algo_ellipse)
  has a bug with ellipses of some dimensions (I made a test, and a
  various sizes have errors).
