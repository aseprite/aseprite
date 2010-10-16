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
 *      Thread management.
 *
 *      By Stefan Schimanski.
 *
 *      Synchronization functions added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #include <objbase.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


/* COINIT_MULTITHREADED is not defined in objbase.h for MSVC */
#define _COINIT_MULTITHREADED 0

typedef HRESULT(CALLBACK * _CoInitializeEx_ptr) (LPVOID, DWORD);
static _CoInitializeEx_ptr _CoInitializeEx = NULL;
static int first_call = 1;



/* _win_thread_init:
 *  Initializes COM interface for the calling thread.
 *  Attempts to use Distributed COM if available (installed by default
 *  on every 32-bit Windows starting with Win98 and Win NT4).
 */
void _win_thread_init(void)
{
   HMODULE ole32 = NULL;
   HWND allegro_wnd = win_get_window();

   if (first_call) {
      first_call = 0;

      ole32 = GetModuleHandle("OLE32.DLL");
      if (ole32 != NULL) {
	 _CoInitializeEx = (_CoInitializeEx_ptr) GetProcAddress(
						ole32, "CoInitializeEx");
      }
      else {
	 MessageBox(allegro_wnd,
	 "OLE32.DLL can't be loaded.", "Warning", MB_ICONWARNING + MB_OK);
      }

      if (_CoInitializeEx == NULL) {
	 MessageBox(allegro_wnd,
		    "Microsoft Distributed COM is not installed on this system. If you have problems "
		    "with this application, please install the DCOM update. You can find it on the "
	 "Microsoft homepage.", "DCOM not found", MB_ICONWARNING + MB_OK);
      }
   }

   if (_CoInitializeEx != NULL)
      _CoInitializeEx(NULL, _COINIT_MULTITHREADED);
   else
      CoInitialize(NULL);
}



/* _win_thread_exit:
 *  Shuts down COM interface for the calling thread.
 */
void _win_thread_exit(void)
{
   CoUninitialize();
}



/* sys_directx_create_mutex:
 *  Creates a mutex and returns a pointer to it.
 *  Uses Critical Sections because Allegro mutexes are
 *  inner-process synchronization objects only.
 */
void *sys_directx_create_mutex(void)
{
   CRITICAL_SECTION *cs;

   cs = _AL_MALLOC(sizeof(CRITICAL_SECTION));
   if (!cs) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   InitializeCriticalSection(cs);

   return (void *)cs;
}



/* sys_directx_destroy_mutex:
 *  Destroys a mutex.
 */
void sys_directx_destroy_mutex(void *handle)
{
   CRITICAL_SECTION *cs = (CRITICAL_SECTION *)handle;

   DeleteCriticalSection(cs);

   _AL_FREE(cs);
}



/* sys_directx_lock_mutex:
 *  Locks a mutex.
 */
void sys_directx_lock_mutex(void *handle)
{
   CRITICAL_SECTION *cs = (CRITICAL_SECTION *)handle;

   EnterCriticalSection(cs);
}



/* sys_directx_unlock_mutex:
 *  Unlocks a mutex.
 */
void sys_directx_unlock_mutex(void *handle)
{
   CRITICAL_SECTION *cs = (CRITICAL_SECTION *)handle;

   LeaveCriticalSection(cs);
}

