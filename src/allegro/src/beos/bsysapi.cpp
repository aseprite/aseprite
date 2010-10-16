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
 *      BeOS system driver routines.
 *
 *      By Jason Wilkins.
 *
 *      desktop_color_depth() and yield_timeslice() support added by
 *      Angelo Mottola.
 *
 *      Synchronization functions added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"
#ifndef SCAN_DEPEND
#include <sys/utsname.h>
#endif

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif              

#define SYS_THREAD_PRIORITY B_NORMAL_PRIORITY
#define SYS_THREAD_NAME     "system driver"

#define EXE_NAME_UNKNOWN "./UNKNOWN"

#define PREFIX_I                "al-bsys INFO: "
#define PREFIX_W                "al-bsys WARNING: "
#define PREFIX_E                "al-bsys ERROR: "



status_t ignore_result = 0;

volatile int _be_focus_count = 0;

BeAllegroApp *_be_allegro_app = NULL;

static thread_id system_thread_id = -1;
static thread_id main_thread_id = -1;
static sem_id system_started = -1;

static char app_path[MAXPATHLEN] = ".";
char wnd_title[WND_TITLE_SIZE];


static bool      using_custom_allegro_app;


/* BeAllegroApp::BeAllegroApp:
 */
BeAllegroApp::BeAllegroApp(const char *signature)
   : BApplication(signature)
{
}
                 


/* BeAllegroApp::QuitRequested:
 */
bool BeAllegroApp::QuitRequested(void)
{
   return false;
}



/* BeAllegroApp::ReadyToRun:
 */
void BeAllegroApp::ReadyToRun()
{
   release_sem(system_started);
}    



/* _be_sys_get_executable_name:
 */
extern "C" void _be_sys_get_executable_name(char *output, int size)
{
   image_info info;
   int32 cookie;

   cookie = 0;

   if (get_next_image_info(0, &cookie, &info) == B_NO_ERROR) {
      _al_sane_strncpy(output, info.name, size);
   }
   else {
      _al_sane_strncpy(output, EXE_NAME_UNKNOWN, size);
   }

   output[size-1] = '\0';
}



/* system_thread:
 */
static int32 system_thread(void *data)
{
   (void)data;

   if (_be_allegro_app == NULL) {
      char sig[MAXPATHLEN] = "application/x-vnd.Allegro-";
      char exe[MAXPATHLEN];
      char *term, *p;

      _be_sys_get_executable_name(exe, sizeof(exe));

      strncat(sig, get_filename(exe), sizeof(sig)-1);
      sig[sizeof(sig)-1] = '\0';

      _be_allegro_app = new BeAllegroApp(sig);

      using_custom_allegro_app = false;

      term = getenv("TERM");
      if (!strcmp(term, "dumb")) {
         /* The TERM environmental variable is set to "dumb" if the app was
          * not started from a terminal.
          */
         p = &exe[strlen(exe) - 1];
         while (*p != '/') p--;
         *(p + 1) = '\0';
         _al_sane_strncpy(app_path, exe, MAXPATHLEN);
      }
   }
   else {
      using_custom_allegro_app = true;
   }

   _be_allegro_app->Run();

   /* XXX commented out due to conflicting TRACE in Haiku
   TRACE(PREFIX_I "system thread exited\n");
   */

   return 0;
}



/* be_sys_init:
 */
extern "C" int be_sys_init(void)
{
   int32       cookie;
   thread_info info;
   struct utsname os_name;
   char path[MAXPATHLEN];

   cookie = 0;

   if (get_next_thread_info(0, &cookie, &info) == B_OK) {
      main_thread_id = info.thread;
   }
   else {
      goto cleanup;
   }

   _be_mouse_view_attached = create_sem(0, "waiting for mouse view attach...");

   if (_be_mouse_view_attached < 0) {
      goto cleanup;
   }
   
   _be_sound_stream_lock = create_sem(1, "audiostream lock");
   
   if (_be_sound_stream_lock < 0) {
      goto cleanup;
   }
   
   system_started = create_sem(0, "starting system driver...");

   if(system_started < 0) {
      goto cleanup;
   }

   system_thread_id = spawn_thread(system_thread, SYS_THREAD_NAME,
                         SYS_THREAD_PRIORITY, NULL);

   if (system_thread_id < 0) {
      goto cleanup;
   }

   resume_thread(system_thread_id);
   acquire_sem(system_started);
   delete_sem(system_started);

   uname(&os_name);
   os_type = OSTYPE_BEOS;
   os_multitasking = TRUE;

   chdir(app_path);

   _be_sys_get_executable_name(path, sizeof(path));
   path[sizeof(path)-1] = '\0';
   do_uconvert(get_filename(path), U_CURRENT, wnd_title, U_UTF8, WND_TITLE_SIZE);

   return 0;

   cleanup: {
      be_sys_exit();
      return 1;
   }
}



/* be_sys_exit:
 */
extern "C" void be_sys_exit(void)
{
   if (main_thread_id > 0) {
      main_thread_id = -1;
   }

   if (system_started > 0) {
      delete_sem(system_started);
      system_started = -1;
   }
   
   if (system_thread_id > 0) {
      ASSERT(_be_allegro_app != NULL);
      _be_allegro_app->Lock();
      _be_allegro_app->Quit();
      _be_allegro_app->Unlock();

      wait_for_thread(system_thread_id, &ignore_result);
      system_thread_id = -1;
   }

   if (_be_mouse_view_attached > 0) {
      delete_sem(_be_mouse_view_attached);
      _be_mouse_view_attached = -1;
   }
   
   if (_be_sound_stream_lock > 0) {
      delete_sem(_be_sound_stream_lock);
      _be_sound_stream_lock = -1;
   }

   if (!using_custom_allegro_app) {
      delete _be_allegro_app;
      _be_allegro_app = NULL;
   }
}



/* be_sys_get_executable_name:
 */
extern "C" void be_sys_get_executable_name(char *output, int size)
{
   char *buffer;

   buffer = (char *)malloc(size);

   if (buffer != NULL) {
      _be_sys_get_executable_name(buffer, size);
      do_uconvert(buffer, U_UTF8, output, U_CURRENT, size);
      free(buffer);
   }
   else {
      do_uconvert(EXE_NAME_UNKNOWN, U_ASCII, output, U_CURRENT, size);
   }
}



/* be_sys_find_resource:
 *  This is the same of the unix resource finder; it looks into the
 *  home directory and in /etc.
 */
int be_sys_find_resource(char *dest, AL_CONST char *resource, int size)
{
   char buf[256], tmp[256], *last;
   char *home = getenv("HOME");

   if (home) {
      /* look for ~/file */
      append_filename(buf, uconvert_ascii(home, tmp), resource, sizeof(buf));
      if (exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }

      /* if it is a .cfg, look for ~/.filerc */
      if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
	 ustrzcpy(buf, sizeof(buf) - ucwidth(OTHER_PATH_SEPARATOR), uconvert_ascii(home, tmp));
	 put_backslash(buf);
	 ustrzcat(buf, sizeof(buf), uconvert_ascii(".", tmp));
	 ustrzcpy(tmp, sizeof(tmp), resource);
	 ustrzcat(buf, sizeof(buf), ustrtok_r(tmp, ".", &last));
	 ustrzcat(buf, sizeof(buf), uconvert_ascii("rc", tmp));
	 if (file_exists(buf, FA_ARCH | FA_RDONLY | FA_HIDDEN, NULL)) {
	    ustrzcpy(dest, size, buf);
	    return 0;
	 }
      }
   }

   /* look for /etc/file */
   append_filename(buf, uconvert_ascii("/etc/", tmp), resource, sizeof(buf));
   if (exists(buf)) {
      ustrzcpy(dest, size, buf);
      return 0;
   }

   /* if it is a .cfg, look for /etc/filerc */
   if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
      ustrzcpy(buf, sizeof(buf), uconvert_ascii("/etc/", tmp));
      ustrzcpy(tmp, sizeof(tmp), resource);
      ustrzcat(buf, sizeof(buf), ustrtok_r(tmp, ".", &last));
      ustrzcat(buf, sizeof(buf), uconvert_ascii("rc", tmp));
      if (exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }
   }

   return -1;
}



extern "C" void be_sys_set_window_title(AL_CONST char *name)
{
   do_uconvert(name, U_CURRENT, wnd_title, U_UTF8, WND_TITLE_SIZE);
   if (_be_window)
      _be_window->SetTitle(wnd_title);
}



extern "C" int be_sys_set_close_button_callback(void (*proc)(void))
{
   if (!_be_window)
      return -1;

   _be_window_close_hook = proc;

   if (proc)
      _be_window->SetFlags(_be_window->Flags() & ~B_NOT_CLOSABLE);
   else
      _be_window->SetFlags(_be_window->Flags() | B_NOT_CLOSABLE);

   return 0;
}



extern "C" void be_sys_message(AL_CONST char *msg)
{
   char  filename[MAXPATHLEN];
   char *title;
   char  tmp[ALLEGRO_MESSAGE_SIZE];
   char  tmp2[ALLEGRO_MESSAGE_SIZE];

   get_executable_name(filename, sizeof(filename));
   title = get_filename(filename);

   BAlert *alert = new BAlert(title,
      uconvert(msg, U_CURRENT, tmp, U_UTF8, ALLEGRO_MESSAGE_SIZE),
      uconvert(get_config_text("Ok"), U_CURRENT, tmp2, U_UTF8, ALLEGRO_MESSAGE_SIZE));
   alert->SetShortcut(0, B_ESCAPE);
   be_app->ShowCursor();
   alert->Go();
   be_app->HideCursor();
}



extern "C" int be_sys_desktop_color_depth(void)
{
   display_mode current_mode;
   
   BScreen().GetMode(&current_mode);
   switch(current_mode.space) {
      case B_CMAP8:  
         return 8;  
      case B_RGB15:  
      case B_RGBA15: 
        return 15; 
      case B_RGB16:  
        return 16; 
      case B_RGB32:  
      case B_RGBA32: 
        return 32; 
   }
   return -1;	
}



extern "C" int be_sys_get_desktop_resolution(int *width, int *height)
{
   BScreen screen(B_MAIN_SCREEN_ID);

   *width  = screen.Frame().IntegerWidth() + 1;
   *height = screen.Frame().IntegerHeight() + 1;

   return 0;
}



extern "C" void be_sys_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_BWINDOW;
   mode->width = 320;
   mode->height = 200;
   mode->bpp = 8;
}



extern "C" void be_sys_yield_timeslice(void)
{
   snooze(0);
}



/* custom mutex that supports nested locking */
struct my_mutex {
   int lock_count;                /* level of nested locking     */
   thread_id owner;               /* thread which owns the mutex */
   sem_id actual_mutex;           /* underlying mutex object     */
};



/* be_sys_create_mutex:
 *  Creates a mutex and returns a pointer to it.
 */
extern "C" void *be_sys_create_mutex(void)
{
   struct my_mutex *mx;

   mx = (struct my_mutex *)malloc(sizeof(struct my_mutex));
   if (!mx) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   mx->lock_count = 0;
   mx->owner = (thread_id) 0;

   mx->actual_mutex = create_sem(1, "internal mutex");
   if (mx->actual_mutex < 0) {
      free(mx);
      return NULL;
   }

   return (void *)mx;
}



/* be_sys_destroy_mutex:
 *  Destroys a mutex.
 */
extern "C" void be_sys_destroy_mutex(void *handle)
{
   struct my_mutex *mx = (struct my_mutex *)handle;

   delete_sem(mx->actual_mutex);

   free(mx);
}



/* be_sys_lock_mutex:
 *  Locks a mutex.
 */
extern "C" void be_sys_lock_mutex(void *handle)
{
   struct my_mutex *mx = (struct my_mutex *)handle;

   if (mx->owner != find_thread(NULL)) {
      acquire_sem(mx->actual_mutex);
      mx->owner = find_thread(NULL);      
   }

   mx->lock_count++;
}



/* be_sys_unlock_mutex:
 *  Unlocks a mutex.
 */
extern "C" void be_sys_unlock_mutex(void *handle)
{
   struct my_mutex *mx = (struct my_mutex *)handle;

   mx->lock_count--;

   if (mx->lock_count == 0) {
      mx->owner = (thread_id) 0;
      release_sem(mx->actual_mutex);
   }
}



extern "C" void be_sys_suspend(void)
{
   if (system_thread_id > 0) {
      suspend_thread(system_thread_id);
   }
}



extern "C" void be_sys_resume(void)
{
   if (system_thread_id > 0) {
      resume_thread(system_thread_id);
   }
}
                       

extern "C" void be_main_suspend(void)
{
   if (main_thread_id > 0) {
      suspend_thread(main_thread_id);
   }
}



extern "C" void be_main_resume(void)
{
   if (main_thread_id > 0) {
      resume_thread(main_thread_id);
   }
}
                              


int32 killer_thread(void *data)
{
   int32     cookie;
   team_info info;

   kill_thread(main_thread_id);
   allegro_exit();
   cookie = 0;
   get_next_team_info(&cookie, &info);
   kill_team(info.team);

   return 1;
}



void _be_terminate(thread_id caller, bool exit_caller)
{
   thread_id killer;

   killer = spawn_thread(killer_thread, "son of sam", 120, NULL);
   resume_thread(killer);

   if (exit_caller)
      exit_thread(1);
}
