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
 *      Some definitions for internal use by the Unix library code.
 *
 *      By Shawn Hargreaves.
 * 
 *      See readme.txt for copyright information.
 */

#ifndef AINTUNIX_H
#define AINTUNIX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ALLEGRO_HAVE_LIBPTHREAD
   /* Synchronization routines using POSIX threads */
   AL_FUNC(void *, _unix_create_mutex, (void));
   AL_FUNC(void, _unix_destroy_mutex, (void *handle));
   AL_FUNC(void, _unix_lock_mutex, (void *handle));
   AL_FUNC(void, _unix_unlock_mutex, (void *handle));
#else
   /* Asynchronous event processing with SIGALRM */
   AL_FUNC(void, _sigalrm_request_abort, (void));
   AL_FUNCPTR(void, _sigalrm_timer_interrupt_handler, (unsigned long interval));
#endif

   /* Macros to enable and disable interrupts */
   #define DISABLE() _unix_bg_man->disable_interrupts()
   #define ENABLE()  _unix_bg_man->enable_interrupts()


   /* Helper for locating config files */
   AL_FUNC(int, _unix_find_resource, (char *dest, AL_CONST char *resource, int size));


   /* Generic system driver entry for finding the executable */
   AL_FUNC(void, _unix_get_executable_name, (char *output, int size));


   /* Helper for setting os_type */
   AL_FUNC(void, _unix_read_os_type, (void));


   /* Helper for yield CPU */
   AL_FUNC(void, _unix_yield_timeslice, (void));


   /* Unix rest function */
   AL_FUNC(void, _unix_rest, (unsigned int, AL_METHOD(void, callback, (void))));


   /* Module support */
   AL_FUNC(void, _unix_load_modules, (int system_driver_id));
   AL_FUNC(void, _unix_unload_modules, (void));


   /* Dynamic driver lists, for modules */
   AL_VAR(_DRIVER_INFO *, _unix_gfx_driver_list);
   AL_VAR(_DRIVER_INFO *, _unix_digi_driver_list);
   AL_VAR(_DRIVER_INFO *, _unix_midi_driver_list);
   AL_FUNC(void, _unix_driver_lists_init, (void));
   AL_FUNC(void, _unix_driver_lists_shutdown, (void));
   AL_FUNC(void, _unix_register_gfx_driver, (int id, GFX_DRIVER *driver, int autodetect, int priority));
   AL_FUNC(void, _unix_register_digi_driver, (int id, DIGI_DRIVER *driver, int autodetect, int priority));
   AL_FUNC(void, _unix_register_midi_driver, (int id, MIDI_DRIVER *driver, int autodetect, int priority));

   /* Get size of a memory page in bytes */
   AL_FUNC(size_t, _unix_get_page_size, (void));


#ifdef ALLEGRO_WITH_XWINDOWS
   AL_FUNCPTR(void, _xwin_keyboard_interrupt, (int pressed, int code));
   AL_FUNCPTR(void, _xwin_keyboard_focused, (int focused, int state));
   AL_FUNCPTR(void, _xwin_mouse_interrupt, (int x, int y, int z, int w, int buttons));
   AL_FUNCPTR(void, _xwin_timer_interrupt, (unsigned long interval));

   AL_ARRAY(_DRIVER_INFO, _xwin_gfx_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_keyboard_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_mouse_driver_list);
   AL_ARRAY(_DRIVER_INFO, _xwin_timer_driver_list);

   AL_FUNC(void, _xwin_handle_input, (void));
   AL_FUNC(void, _xwin_private_handle_input, (void));

#ifndef ALLEGRO_MULTITHREADED

   AL_VAR(int, _xwin_missed_input);

   #define XLOCK()                              \
      do {                                      \
         _xwin.lock_count++;                    \
      } while (0)

   #define XUNLOCK()                            \
      do {                                      \
         if (_xwin.lock_count == 1) {           \
            while(_xwin_missed_input) {         \
               if (_xwin_input_handler)         \
                  _xwin_input_handler();        \
               else                             \
                  _xwin_private_handle_input(); \
               --_xwin_missed_input;            \
            }                                   \
         }                                      \
         _xwin.lock_count--;                    \
      } while (0)

#else

   #define XLOCK()                              \
      do {                                      \
         if (_xwin.mutex)                       \
            _unix_lock_mutex (_xwin.mutex);     \
         _xwin.lock_count++;                    \
      } while (0)

   #define XUNLOCK()                            \
      do {                                      \
         if (_xwin.mutex)                       \
            _unix_unlock_mutex (_xwin.mutex);   \
         _xwin.lock_count--;                    \
      } while (0)

#endif

#endif


#ifdef ALLEGRO_WITH_OSSDIGI
   /* So the setup program can read what we detected */
   AL_VAR(int, _oss_fragsize);
   AL_VAR(int, _oss_numfrags);
#endif


#ifdef __cplusplus
}
#endif


#ifdef ALLEGRO_LINUX
   #include "aintlnx.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Typedef for background functions, called frequently in the background.
 * `threaded' is nonzero if the function is being called from a thread.
 */
typedef void (*bg_func) (int threaded);

/* Background function manager -- responsible for calling background 
 * functions.  `int' methods return -1 on failure, 0 on success. */
struct bg_manager
{
   int multi_threaded;
   int (*init) (void);
   void (*exit) (void);
   int (*register_func) (bg_func f);
   int (*unregister_func) (bg_func f);
   void (*enable_interrupts) (void);
   void (*disable_interrupts) (void);
   int (*interrupts_disabled) (void);
};	

extern struct bg_manager _bg_man_pthreads;
extern struct bg_manager _bg_man_sigalrm;

extern struct bg_manager *_unix_bg_man;

extern _DRIVER_INFO _linux_joystick_driver_list[]; /* also in aintlnx.h */


#ifdef __cplusplus
}
#endif

#endif /* ifndef AINTUNIX_H */
