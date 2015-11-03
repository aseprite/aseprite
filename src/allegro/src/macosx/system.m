/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      MacOS X system driver.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"
#include "qzmouse.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif

#ifndef SCAN_DEPEND
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_port.h>
#include <servers/bootstrap.h>
#endif


/* These are used to warn the dock about the application */
struct CPSProcessSerNum
{
   UInt32 lo;
   UInt32 hi;
};
extern OSErr CPSGetCurrentProcess(struct CPSProcessSerNum *psn);
extern OSErr CPSEnableForegroundOperation(struct CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr CPSSetFrontProcess(struct CPSProcessSerNum *psn);



static int osx_sys_init(void);
static void osx_sys_exit(void);
static void osx_sys_message(AL_CONST char *);
static void osx_sys_get_executable_name(char *, int);
static int osx_sys_find_resource(char *, AL_CONST char *, int);
static void osx_sys_set_window_title(AL_CONST char *);
static int osx_sys_set_close_button_callback(void (*proc)(void));
static int osx_sys_set_resize_callback(void (*proc)(RESIZE_DISPLAY_EVENT *ev));
static int osx_sys_set_display_switch_mode(int mode);
static void osx_sys_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);
static int osx_sys_desktop_color_depth(void);
static int osx_sys_get_desktop_resolution(int *width, int *height);


/* Global variables */
int    __crt0_argc;
char **__crt0_argv;
NSBundle *osx_bundle = NULL;
void* osx_event_mutex;
NSCursor *osx_cursor = NULL;
NSCursor *osx_blank_cursor = NULL;
AllegroWindow *osx_window = NULL;
char osx_window_title[ALLEGRO_MESSAGE_SIZE];
void (*osx_window_close_hook)(void) = NULL;
void (*osx_resize_callback)(RESIZE_DISPLAY_EVENT *ev) = NULL;
void (*osx_mouse_enter_callback)() = NULL;
void (*osx_mouse_leave_callback)() = NULL;
int osx_gfx_mode = OSX_GFX_NONE;
int osx_emulate_mouse_buttons = FALSE;
int osx_window_first_expose = FALSE;
int osx_skip_events_processing = FALSE;
void* osx_skip_events_processing_mutex;


static RETSIGTYPE (*old_sig_abrt)(int num);
static RETSIGTYPE (*old_sig_fpe)(int num);
static RETSIGTYPE (*old_sig_ill)(int num);
static RETSIGTYPE (*old_sig_segv)(int num);
static RETSIGTYPE (*old_sig_term)(int num);
static RETSIGTYPE (*old_sig_int)(int num);
static RETSIGTYPE (*old_sig_quit)(int num);

static unsigned char *cursor_data = NULL;
static NSBitmapImageRep *cursor_rep = NULL;
static NSImage *cursor_image = NULL;


SYSTEM_DRIVER system_macosx =
{
   SYSTEM_MACOSX,
   empty_string,
   empty_string,
   "MacOS X",
   osx_sys_init,
   osx_sys_exit,
   osx_sys_get_executable_name,
   osx_sys_find_resource,
   osx_sys_set_window_title,
   osx_sys_set_close_button_callback,
   osx_sys_set_resize_callback,
   osx_sys_message,
   NULL,  /* AL_METHOD(void, assert, (AL_CONST char *msg)); */
   NULL,  /* AL_METHOD(void, save_console_state, (void)); */
   NULL,  /* AL_METHOD(void, restore_console_state, (void)); */
   NULL,  /* AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height)); */
   NULL,  /* AL_METHOD(void, created_bitmap, (struct BITMAP *bmp)); */
   NULL,  /* AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height)); */
   NULL,  /* AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent)); */
   NULL,  /* AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap)); */
   NULL,  /* AL_METHOD(void, read_hardware_palette, (void)); */
   NULL,  /* AL_METHOD(void, set_palette_range, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,  /* AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth)); */
   osx_sys_set_display_switch_mode,
   NULL,  /* AL_METHOD(void, display_switch_lock, (int lock, int foreground)); */
   osx_sys_desktop_color_depth,
   osx_sys_get_desktop_resolution,
   osx_sys_get_gfx_safe_mode,
   _unix_yield_timeslice,
   _unix_create_mutex,
   _unix_destroy_mutex,
   _unix_lock_mutex,
   _unix_unlock_mutex,
   NULL,  /* AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void)); */
   NULL,  /* AL_METHOD(_DRIVER_INFO *, timer_drivers, (void)); */
};



/* osx_signal_handler:
 *  Used to trap various signals, to make sure things get shut down cleanly.
 */
static RETSIGTYPE osx_signal_handler(int num)
{
   _unix_unlock_mutex(osx_event_mutex);
   _unix_unlock_mutex(osx_window_mutex);

   allegro_exit();

   _unix_destroy_mutex(osx_event_mutex);
   _unix_destroy_mutex(osx_window_mutex);

   fprintf(stderr, "Shutting down Allegro due to signal #%d\n", num);
   raise(num);
}



static void handle_mouse_enter()
{
  if (_mouse_installed && !_mouse_on && osx_window) {
    _mouse_on = TRUE;
    osx_hide_native_mouse();
    if (osx_mouse_enter_callback)
      osx_mouse_enter_callback();
  }
}



static BOOL handle_mouse_leave()
{
  if (_mouse_installed) {
    if (_mouse_on) {
      _mouse_on = FALSE;
      osx_show_native_mouse();
      if (osx_mouse_leave_callback)
        osx_mouse_leave_callback();
    }
    return YES;
  }
  else
    return NO;
}



/* osx_event_handler:
 *  Event handling function; gets repeatedly called inside a dedicated thread.
 */
void osx_event_handler()
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   NSEvent *event;
   NSPoint point;
   NSView* view;
   int dx = 0, dy = 0, dz = 0;
   int mx = _mouse_x;
   int my = _mouse_y;
   static int buttons = 0;
   int event_type;
   BOOL gotmouseevent = NO;

   while (osx_window != nil)
   {
     event = [NSApp nextEventMatchingMask: NSAnyEventMask
                                untilDate: [NSDate distantPast]
                                   inMode: NSDefaultRunLoopMode
                                  dequeue: YES];
     if (!event)
       return;

     BOOL send_event = YES;

      _unix_lock_mutex(osx_skip_events_processing_mutex);
      int skip_events_processing = osx_skip_events_processing;
      _unix_unlock_mutex(osx_skip_events_processing_mutex);

      if ((skip_events_processing) || (osx_gfx_mode == OSX_GFX_NONE)) {
         [NSApp sendEvent: event];
         continue;
      }

      view = [osx_window contentView];
      point = [event locationInWindow];
      if (osx_window) {
         if ([event window] == nil)
            point = [osx_window convertScreenToBase:point];

         point = [view convertPoint:point fromView:nil];
      }

      event_type = [event type];
      switch (event_type) {

         case NSKeyDown:
            if (_keyboard_installed)
               osx_keyboard_handler(TRUE, event);

#if 0 // Avoid beeps TODO comment this when the OS X menus are ready
            if ([event modifierFlags] & NSCommandKeyMask)
               [NSApp sendEvent: event];
#endif
            send_event = NO;
            break;

         case NSKeyUp:
            if (_keyboard_installed)
               osx_keyboard_handler(FALSE, event);

#if 0 // Avoid beeps TODO uncomment this when the OS X menus are ready
            if ([event modifierFlags] & NSCommandKeyMask)
               [NSApp sendEvent: event];
#endif
            send_event = NO;
            break;

         case NSFlagsChanged:
            if (_keyboard_installed)
               osx_keyboard_modifiers([event modifierFlags]);
            break;

         case NSLeftMouseDown:
         case NSOtherMouseDown:
         case NSRightMouseDown:
            /* App is regaining focus */
            if (![NSApp isActive]) {
               if ([view mouse:point inRect:[view frame]])
                  handle_mouse_enter();

               if (osx_window)
                  [osx_window invalidateCursorRectsForView:view];

               if (_keyboard_installed)
                  osx_keyboard_focused(TRUE, 0);

               _switch_in();
            }

            if (_mouse_on) {
               buttons |= ((event_type == NSLeftMouseDown) ? 0x1 : 0);
               buttons |= ((event_type == NSRightMouseDown) ? 0x2 : 0);
               buttons |= ((event_type == NSOtherMouseDown) ? 0x4 : 0);
               mx = point.x;
               my = point.y;

               gotmouseevent = YES;
            }
            break;

         case NSLeftMouseUp:
         case NSOtherMouseUp:
         case NSRightMouseUp:
            if ([NSApp isActive] && [view mouse:point inRect:[view frame]])
               handle_mouse_enter();

            if (_mouse_on) {
               buttons &= ~((event_type == NSLeftMouseUp) ? 0x1 : 0);
               buttons &= ~((event_type == NSRightMouseUp) ? 0x2 : 0);
               buttons &= ~((event_type == NSOtherMouseUp) ? 0x4 : 0);

               mx = point.x;
               my = point.y;

               gotmouseevent = YES;
            }
            break;

         case NSLeftMouseDragged:
         case NSRightMouseDragged:
         case NSOtherMouseDragged:
         case NSMouseMoved:
            if ([NSApp isActive] && [view mouse:point inRect:[view frame]])
               handle_mouse_enter();

            if (_mouse_on) {
               dx += [event deltaX];
               dy += [event deltaY];
               mx = point.x;
               my = point.y;

               gotmouseevent = YES;
            }
            break;

         case NSScrollWheel:
            if (_mouse_on)
               dz += [event deltaY];

            mx = point.x;
            my = point.y;

            gotmouseevent = YES;
            break;

         case NSMouseEntered:
            if ([event window] == osx_window &&
                [view mouse:point inRect:[view frame]]) {
               handle_mouse_enter();

               if (_mouse_on) {
                  mx = point.x;
                  my = point.y;
                  gotmouseevent = YES;
               }
            }
            break;

         case NSMouseExited:
            if (handle_mouse_leave()) {
               mx = point.x;
               my = point.y;
               gotmouseevent = YES;
            }
            break;

         case NSAppKitDefined:
            switch ([event subtype]) {
               case NSApplicationActivatedEventType:
                  if (osx_window) {
                     [osx_window invalidateCursorRectsForView:view];
                     if (_keyboard_installed)
                        osx_keyboard_focused(TRUE, 0);

                     handle_mouse_enter();

                     if (_mouse_on) {
                        mx = point.x;
                        my = point.y;
                        gotmouseevent = YES;
                     }
                  }
                  _switch_in();
                  break;

               case NSApplicationDeactivatedEventType:
                  if (osx_window && _keyboard_installed)
                     osx_keyboard_focused(FALSE, 0);

                  handle_mouse_leave();
                  _switch_out();
                  break;

               case NSWindowMovedEventType:
                  /* This is needed to ensure the shadow gets drawn when the window is
                   * created. It's weird, but when the window is created on another
                   * thread, sometimes its shadow doesn't get drawn. The same applies
                   * to the cursor rectangle, which doesn't seem to be set at window
                   * creation (it works once you move the mouse though).
                   */
                  if ((osx_window) && (osx_window_first_expose)) {
                     osx_window_first_expose = FALSE;
                     [osx_window setHasShadow: NO];
                     [osx_window setHasShadow: YES];
                     [osx_window invalidateCursorRectsForView:view];
                  }
                  break;
            }
            break;
      }

      if (send_event == YES)
         [NSApp sendEvent: event];

      if (gotmouseevent == YES)
         osx_mouse_handler(mx, my, dx, dy, dz, buttons);
   }

   [pool release];
}



/* osx_tell_dock:
 *  Tell the dock about us; the origins of this hack are unknown, but it's
 *  currently the only way to make a Cocoa app to work when started from a
 *  console.
 *  For the future, (10.3 and above) investigate TranformProcessType in the
 *  HIServices framework.
 */
static void osx_tell_dock(void)
{
   struct CPSProcessSerNum psn;

   if ((!CPSGetCurrentProcess(&psn)) &&
       (!CPSEnableForegroundOperation(&psn, 0x03, 0x3C, 0x2C, 0x1103)) &&
       (!CPSSetFrontProcess(&psn)))
      [NSApplication sharedApplication];
}



/* osx_bootstrap_ok:
 *  Check if the current bootstrap context is privilege. If it's not, we can't
 *  use NSApplication, and instead have to go directly to main.
 *  Returns 1 if ok, 0 if not.
 */
int osx_bootstrap_ok(void)
{
   static int _ok = -1;
   mach_port_t bp;
   kern_return_t ret;
   CFMachPortRef cfport;

   /* If have tested once, just return that answer */
   if (_ok >= 0)
      return _ok;
   cfport = CFMachPortCreate(NULL, NULL, NULL, NULL);
   task_get_bootstrap_port(mach_task_self(), &bp);
   ret = bootstrap_register(bp, "bootstrap-ok-test", CFMachPortGetPort(cfport));
   CFRelease(cfport);
   _ok = (ret == KERN_SUCCESS) ? 1 : 0;
   return _ok;
}



/* osx_sys_init:
 *  Initalizes the MacOS X system driver.
 */
static int osx_sys_init(void)
{
   long result;

#if 0
   /* If we're in the 'dead bootstrap' environment, the Mac driver won't work. */
   if (!osx_bootstrap_ok()) {
      return -1;
   }
#endif

   /* Install emergency-exit signal handlers */
   old_sig_abrt = signal(SIGABRT, osx_signal_handler);
   old_sig_fpe  = signal(SIGFPE,  osx_signal_handler);
   old_sig_ill  = signal(SIGILL,  osx_signal_handler);
   old_sig_segv = signal(SIGSEGV, osx_signal_handler);
   old_sig_term = signal(SIGTERM, osx_signal_handler);
   old_sig_int  = signal(SIGINT,  osx_signal_handler);
   old_sig_quit = signal(SIGQUIT, osx_signal_handler);



   if (osx_bundle == NULL) {
       /* If in a bundle, the dock will recognise us automatically */
       osx_tell_dock();
   }

   /* Setup OS type & version */
   os_type = OSTYPE_MACOSX;
   Gestalt(gestaltSystemVersion, &result);
   os_version = (((result >> 12) & 0xf) * 10) + ((result >> 8) & 0xf);
   os_revision = (result >> 4) & 0xf;
   os_multitasking = TRUE;

   /* Setup a blank cursor */
   cursor_data = calloc(1, 16 * 16 * 4);
   cursor_rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: &cursor_data
      pixelsWide: 16
      pixelsHigh: 16
      bitsPerSample: 8
      samplesPerPixel: 4
      hasAlpha: YES
      isPlanar: NO
      colorSpaceName: NSDeviceRGBColorSpace
      bytesPerRow: 64
      bitsPerPixel: 32];
   cursor_image = [[NSImage alloc] initWithSize: NSMakeSize(16, 16)];
   [cursor_image addRepresentation: cursor_rep];
   osx_blank_cursor = [[NSCursor alloc] initWithImage: cursor_image
      hotSpot: NSMakePoint(0, 0)];
   osx_cursor = osx_blank_cursor;

   osx_gfx_mode = OSX_GFX_NONE;

   set_display_switch_mode(SWITCH_BACKGROUND);
   set_window_title([[[NSProcessInfo processInfo] processName] cString]);

   return 0;
}



/* osx_sys_exit:
 *  Shuts down the system driver.
 */
static void osx_sys_exit(void)
{
   signal(SIGABRT, old_sig_abrt);
   signal(SIGFPE,  old_sig_fpe);
   signal(SIGILL,  old_sig_ill);
   signal(SIGSEGV, old_sig_segv);
   signal(SIGTERM, old_sig_term);
   signal(SIGINT,  old_sig_int);
   signal(SIGQUIT, old_sig_quit);

   if (osx_blank_cursor)
      [osx_blank_cursor release];
   if (cursor_image)
      [cursor_image release];
   if (cursor_rep)
      [cursor_rep release];
   if (cursor_data)
      free(cursor_data);
   osx_cursor = NULL;
   cursor_image = NULL;
   cursor_rep = NULL;
   cursor_data = NULL;
}



/* osx_sys_get_executable_name:
 *  Returns the full path to the application executable name. Note that if the
 *  exe is inside a bundle, this returns the full path of the bundle.
 */
static void osx_sys_get_executable_name(char *output, int size)
{
   if (osx_bundle)
      do_uconvert([[osx_bundle bundlePath] lossyCString], U_ASCII, output, U_CURRENT, size);
   else
      do_uconvert(__crt0_argv[0], U_ASCII, output, U_CURRENT, size);
}



/* osx_sys_find_resource:
 *  Searches the resource in the bundle resource path if the app is in a
 *  bundle, otherwise calls the unix resource finder routine.
 */
static int osx_sys_find_resource(char *dest, AL_CONST char *resource, int size)
{
   const char *path;
   char buf[256], tmp[256];

   if (osx_bundle) {
      path = [[osx_bundle resourcePath] cString];
      append_filename(buf, uconvert_ascii(path, tmp), resource, sizeof(buf));
      if (exists(buf)) {
         ustrzcpy(dest, size, buf);
         return 0;
      }
   }
   return _unix_find_resource(dest, resource, size);
}



/* osx_sys_message:
 *  Displays a message using an alert panel.
 */
static void osx_sys_message(AL_CONST char *msg)
{
   char tmp[ALLEGRO_MESSAGE_SIZE];
   NSString *ns_title, *ns_msg;

   fputs(uconvert_toascii(msg, tmp), stderr);

   do_uconvert(msg, U_CURRENT, tmp, U_UTF8, ALLEGRO_MESSAGE_SIZE);
   ns_title = [NSString stringWithUTF8String: osx_window_title];
   ns_msg = [NSString stringWithUTF8String: tmp];

   NSRunAlertPanel(ns_title, ns_msg, nil, nil, nil);
}



/* osx_sys_set_window_title:
 *  Sets the title for both the application menu and the window if present.
 */
static void osx_sys_set_window_title(AL_CONST char *title)
{
   char tmp[ALLEGRO_MESSAGE_SIZE];

   if (osx_window_title != title)
      _al_sane_strncpy(osx_window_title, title, ALLEGRO_MESSAGE_SIZE);
   do_uconvert(title, U_CURRENT, tmp, U_UTF8, ALLEGRO_MESSAGE_SIZE);

   NSString *ns_title = [NSString stringWithUTF8String: tmp];

   if (osx_window) {
      [osx_window performSelectorOnMainThread:@selector(setTitle:) withObject:ns_title waitUntilDone: NO];
   }
}



/* osx_sys_set_close_button_callback:
 *  Sets the window close callback. Also used when user hits Command-Q or
 *  selects "Quit" from the application menu.
 */
static int osx_sys_set_close_button_callback(void (*proc)(void))
{
   osx_window_close_hook = proc;
   return 0;
}



static int osx_sys_set_resize_callback(void (*proc)(RESIZE_DISPLAY_EVENT *ev))
{
  osx_resize_callback = proc;
  return 0;
}



/* osx_sys_set_display_switch_mode:
 *  Sets the current display switch mode.
 */
static int osx_sys_set_display_switch_mode(int mode)
{
   if (mode != SWITCH_BACKGROUND)
      return -1;
   return 0;
}



/* osx_sys_get_gfx_safe_mode:
 *  Defines the safe graphics mode for this system.
 */
static void osx_sys_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_QUARTZ_WINDOW;
   mode->width = 320;
   mode->height = 200;
   mode->bpp = 8;
}



/* osx_sys_desktop_color_depth:
 *  Queries the desktop color depth.
 */
static int osx_sys_desktop_color_depth(void)
{
   CFDictionaryRef mode = NULL;
   int color_depth;

   mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   if (!mode)
      return -1;
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayBitsPerPixel), kCFNumberSInt32Type, &color_depth);

   return color_depth == 16 ? 15 : color_depth;
}


/* osx_sys_get_desktop_resolution:
 *  Queries the desktop resolution.
 */
static int osx_sys_get_desktop_resolution(int *width, int *height)
{
   CFDictionaryRef mode = NULL;

   mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   if (!mode)
      return -1;
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, width);
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, height);

   return 0;
}
