This is a modified version of Allegro library (branch 4.4)
specially patched for Aseprite by David Capello.

Changes:

* Mouse driver for Windows was modified to use WM_MOUSEMOVE instead of
  DirectInput (like in Allegro 5).
* Keyboard driver for Windows was modified to use WM_KEYDOWN/UP messages
  instead of DirectInput (like in Allegro 5).
* Added resize support for Windows, X11, and Mac OS X ports.
* Removed code and functions that are not used (Allegro GUI,
  audio, MIDI, joystick, etc.).
* The HWND class has CS_DBLCLKS enabled (so UI code can detect
  double-clicks from Windows messages).
* Fixed issues recreating DirectX surfaces
* Fixed OS X mouse issues
* Support for x64
* Several [other changes](https://github.com/aseprite/aseprite/commits/master/src/allegro)
