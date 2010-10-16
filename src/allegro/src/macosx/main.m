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
 *      main() function replacement for MacOS X.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif

#include "allegro/internal/alconfig.h"
#undef main


/* For compatibility with the unix code */
extern int    __crt0_argc;
extern char **__crt0_argv;
extern void *_mangled_main_address;

static char *arg0, *arg1 = NULL;
static int refresh_rate = 70;

static volatile BOOL ready_to_terminate = NO;

static BOOL in_bundle(void)
{
   /* This comes from the ADC tips & tricks section: how to detect if the app
    * lives inside a bundle
    */
   FSRef processRef;
   ProcessSerialNumber psn = { 0, kCurrentProcess };
   FSCatalogInfo processInfo;
   GetProcessBundleLocation(&psn, &processRef);
   FSGetCatalogInfo(&processRef, kFSCatInfoNodeFlags, &processInfo, NULL, NULL, NULL);
   if (processInfo.nodeFlags & kFSNodeIsDirectoryMask) 
     return YES;
   else
     return NO;
}

@implementation AllegroAppDelegate

- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename
{
  arg1 = strdup([filename lossyCString]);
  return YES;
}



/* applicationDidFinishLaunching:
 *  Called when the app is ready to run. This runs the system events pump and
 *  updates the app window if it exists.
 */
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification
{
   NSAutoreleasePool *pool = NULL;
   CFDictionaryRef mode;
   NSString* exename, *resdir;
   NSFileManager* fm;
   BOOL isDir;

   /* create mutex */
   osx_event_mutex = _unix_create_mutex();
   osx_skip_events_processing_mutex = _unix_create_mutex();
   
   pool = [[NSAutoreleasePool alloc] init];
   if (in_bundle() == YES)   
   {
      /* In a bundle, so chdir to the containing directory,
       * or to the 'magic' resource directory if it exists.
       * (see the readme.osx file for more info)
       */
      osx_bundle = [NSBundle mainBundle];
      exename = [[osx_bundle executablePath] lastPathComponent];
      resdir = [[osx_bundle resourcePath] stringByAppendingPathComponent: exename];
      fm = [NSFileManager defaultManager];
      if ([fm fileExistsAtPath: resdir isDirectory: &isDir] && isDir) {
          /* Yes, it exists inside the bundle */
          [fm changeCurrentDirectoryPath: resdir];
      }
      else {
         /* No, change to the 'standard' OSX resource directory if it exists*/
         if ([fm fileExistsAtPath: [osx_bundle resourcePath] isDirectory: &isDir] && isDir)
         {
            [fm changeCurrentDirectoryPath: [osx_bundle resourcePath]];
         }
         /* It doesn't exist - this is unusual for a bundle. Don't chdir */
      }
      arg0 = strdup([[osx_bundle bundlePath] fileSystemRepresentation]);
      if (arg1) {
         static char *args[2];
	 args[0] = arg0;
	 args[1] = arg1;
	 __crt0_argv = args;
	 __crt0_argc = 2;
      }
      else {
         __crt0_argv = &arg0;
         __crt0_argc = 1;
      }
   }
   /* else: not in a bundle so don't chdir */
   
   mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &refresh_rate);
   if (refresh_rate <= 0)
      refresh_rate = 70;
   
   [NSThread detachNewThreadSelector: @selector(app_main:)
      toTarget: [AllegroAppDelegate class]
      withObject: nil];
   
   while (!ready_to_terminate) {
      if (osx_gfx_mode == OSX_GFX_WINDOW)
         osx_update_dirty_lines();
      _unix_lock_mutex(osx_event_mutex);
      if (osx_gfx_mode == OSX_GFX_FULL) {
         if ((osx_palette) && (osx_palette_dirty)) {
            CGDisplaySetPalette(kCGDirectMainDisplay, osx_palette);
	    osx_palette_dirty = FALSE;
	 }
      }
      osx_event_handler();
      _unix_unlock_mutex(osx_event_mutex);
      usleep(1000000 / refresh_rate);
   }
   
   [pool release];
   _unix_destroy_mutex(osx_event_mutex);

   [NSApp terminate:self];
}



/* applicationDidChangeScreenParameters:
 *  Invoked when the screen did change resolution/color depth.
 */
- (void)applicationDidChangeScreenParameters: (NSNotification *)aNotification
{
   CFDictionaryRef mode;
   int new_refresh_rate;
   
   if ((osx_window) && (osx_gfx_mode == OSX_GFX_WINDOW)) 
   {
      osx_setup_colorconv_blitter();
      [osx_window display];
   }
   mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &new_refresh_rate);
   if (new_refresh_rate <= 0)
      new_refresh_rate = 70;
   refresh_rate = new_refresh_rate;
}



/* Call the user main() */
static void call_user_main(void)
{
   int (*real_main)(int, char*[]) = (int (*)(int, char*[])) _mangled_main_address;
   real_main(__crt0_argc, __crt0_argv);
   allegro_exit();
   ready_to_terminate = YES;
}



/* app_main:
 *  Thread dedicated to the user program; real main() gets called here.
 */
+ (void)app_main: (id)arg
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   call_user_main();
   [pool release];
}



/* applicationShouldTerminate:
 *  Called upon Command-Q or "Quit" menu item selection.
 *  If the window close callback is set, calls it, otherwise does
 *  not exit.
 */
- (NSApplicationTerminateReply) applicationShouldTerminate: (id)sender
{
   if (ready_to_terminate) {
      return NSTerminateNow;
   }
   else {
      if (osx_window_close_hook)
         osx_window_close_hook();
      return NSTerminateCancel;
   }
}



/* quitAction:
 *  This is connected to the Quit menu.  The menu sends this message to the
 *  delegate, rather than sending terminate: directly to NSApp, so that we get a
 *  chance to validate the menu item first.
 */
- (void) quitAction: (id) sender {
   [NSApp terminate: self];
}



/* validateMenuItem:
 *  If the user has not set a window close hook, return NO from this function so
 *  that the Quit menu item is greyed out.
 */
- (BOOL) validateMenuItem: (NSMenuItem*) item {
   if ([item action] == @selector(quitAction:)) {
      return (osx_window_close_hook == NULL) ? NO : YES;
   }
   /* Default, all other items are valid (there are none at the moment */
   return YES;
}

/* end of AllegroAppDelegate implementation */
@end



/* This prevents warnings that 'NSApplication might not
 * respond to setAppleMenu' on OS X 10.4
 */
@interface NSApplication(AllegroOSX)
- (void)setAppleMenu:(NSMenu *)menu;
@end



/* main:
 *  Replacement for main function.
 */
int main(int argc, char *argv[])
{
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
   AllegroAppDelegate *app_delegate = [[AllegroAppDelegate alloc] init];
   NSMenu *menu;
   NSMenuItem *menu_item, *temp_item;

   /* I don't know what that NSAutoreleasePool line does but the variable was
    * otherwise unused --pw
    */
   (void)pool;

   __crt0_argc = argc;
   __crt0_argv = argv;
   
   if (!osx_bootstrap_ok()) /* not safe to use NSApplication */
      call_user_main();
      
   [NSApplication sharedApplication];

   /* Load the main menu nib if possible */
   if ((!in_bundle()) || ([NSBundle loadNibNamed: @"MainMenu"
                                    owner: NSApp] == NO))
   {
       /* Didn't load the nib; create a default menu programmatically */
       NSString* title = nil;
       NSDictionary* app_dictionary = [[NSBundle mainBundle] infoDictionary];
       if (app_dictionary) 
       {
          title = [app_dictionary objectForKey: @"CFBundleName"];
       }
       if (title == nil) 
       {
          title = [[NSProcessInfo processInfo] processName];
       }
       [NSApp setMainMenu: [[NSMenu allocWithZone: [NSMenu menuZone]] initWithTitle: @"temp"]];
       menu = [[NSMenu allocWithZone: [NSMenu menuZone]] initWithTitle: @"temp"];
       temp_item = [[NSMenuItem allocWithZone: [NSMenu menuZone]]
		     initWithTitle: @"temp"
		     action: NULL
		     keyEquivalent: @""];
       [[NSApp mainMenu] addItem: temp_item];
       [[NSApp mainMenu] setSubmenu: menu forItem: temp_item];
       [NSApp setAppleMenu: menu];
       NSString *quit = [@"Quit " stringByAppendingString: title];
       menu_item = [[NSMenuItem allocWithZone: [NSMenu menuZone]]
		     initWithTitle: quit
		     action: @selector(quitAction:)
		     keyEquivalent: @"q"];
       [menu_item setKeyEquivalentModifierMask: NSCommandKeyMask];
       [menu_item setTarget: app_delegate];
       [menu addItem: menu_item];
   }

   [NSApp setDelegate: app_delegate];
   
   [NSApp run];
   /* Can never get here */
   
   return 0;
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* c-file-style: "linux" */
/* End:                   */
