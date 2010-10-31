This is a modified version of Allegro library (branch 4.4) specially
patched for ASE by David Capello.

Changes:
- Mouse driver for Windows was modified to use WM_MOUSEMOVE instead of
  DirectInput (like in Allegro 5).
- Keyboard driver for Windows was modified to use WM_KEYDOWN/UP messages
  instead of DirectInput (like in Allegro 5).
- Added resize support for Windows, X11, and Mac OS X ports.
