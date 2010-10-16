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
 *      Get version of installed DirectX.
 *
 *      By Stefan Schimanski.
 *
 *      Modified get_dx_ver() from DirectX 7 SDK.
 *
 *      See readme.txt for copyright information.
 */


/* Set DIRECTX_SDK_VERSION according to the installed SDK version.
 * Note that this file requires at least the SDK version 5 to compile,
 * so that DIRECTX_SDK_VERSION must be >= 0x500.
 */
#define DIRECTX_SDK_VERSION 0x500


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

/* needed by the SDK version 8 with MSVC 5 (SP3) */
#define DIRECTINPUT_VERSION 0x300

#ifndef SCAN_DEPEND
   #include <ddraw.h>
   #include <dinput.h>
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


typedef HRESULT(WINAPI *DIRECTDRAWCREATE) (GUID *, LPDIRECTDRAW *, IUnknown *);
typedef HRESULT(WINAPI *DIRECTINPUTCREATE) (HINSTANCE, DWORD, LPDIRECTINPUT *, IUnknown *);
typedef HRESULT(WINAPI *DSETUPCREATE)(DWORD*,DWORD*);



/* get_dx_ver:
 *  returns the DirectX dx_version number:
 *
 *          0       no DirectX installed
 *          0x100   DirectX 1 installed
 *          0x200   DirectX 2 installed
 *          0x300   DirectX 3 installed
 *          0x500   at least DirectX 5 installed
 *          0x600   at least DirectX 6 installed
 *          0x700   at least DirectX 7 installed
 */
int get_dx_ver(void)
{
   HRESULT hr;
   HINSTANCE ddraw_hinst = NULL;
   HINSTANCE dinput_hinst = NULL;
   HINSTANCE dsetup_hinst = NULL;
   LPDIRECTDRAW directdraw = NULL;
   LPDIRECTDRAW2 directdraw2 = NULL;
   DIRECTDRAWCREATE DirectDrawCreate = NULL;
   DIRECTINPUTCREATE DirectInputCreate = NULL;
   DSETUPCREATE DSetupCreate = NULL;
   OSVERSIONINFO os_version;
   LPDIRECTDRAWSURFACE ddraw_surf = NULL;
   LPDIRECTDRAWSURFACE3 ddraw_surf3 = NULL;
   DWORD dsetup_revision;
   DWORD dsetup_version;
   INT dsetup_result;

#if DIRECTX_SDK_VERSION >= 0x600
   LPDIRECTDRAWSURFACE4 ddraw_surf4 = NULL;

#if DIRECTX_SDK_VERSION >= 0x700
   LPDIRECTDRAWSURFACE7 ddraw_surf7 = NULL;
#endif

#endif

   DDSURFACEDESC ddraw_surf_desc;
   LPVOID temp;
   int dx_version = 0;

   /* first get the Windows platform */
   os_version.dwOSVersionInfoSize = sizeof(os_version);
   if (!GetVersionEx(&os_version))
      return dx_version;

   if (os_version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
      /* NT is easy... NT 4.0 is DX2, 4.0 SP3 is DX3, 5.0 is DX5 at least
       * and no DX on earlier versions
       */
      if (os_version.dwMajorVersion < 4) {
         /* No DX on NT 3.51 or earlier */
         return dx_version;
      }

      /* First check for DX 8 and 9 */
      dsetup_hinst = LoadLibrary( "DSETUP.DLL" );
      if ( dsetup_hinst ) {
         DSetupCreate = (DSETUPCREATE)GetProcAddress(dsetup_hinst, "DirectXSetupGetVersion");
         if ( DSetupCreate ) {
            dsetup_result = DSetupCreate( &dsetup_version, &dsetup_revision );	// returns 0 on failure
            if ( dsetup_result ) {
               switch (dsetup_version) {
                  case 0x00040005:
                     dx_version = 0x500;
                     break;
                  case 0x00040006:
                     dx_version = 0x600;
                     break;
                  case 0x00040007:
                     dx_version = 0x700;
                     break;
                  case 0x00040008:              /* v8.x */
                     dx_version = 0x800;
                     switch (dsetup_revision) {
                        case 0x0001032A:
                        case 0x00010371:
                           dx_version = 0x810; /* 8.1 */
                           dx_version = 0x810; /* 8.1 */
                           break;
                        case 0x00010385:
                           dx_version = 0x81a; /* 8.1a or b (stupid MS...) */
                           break;
                        case 0x00020386:
                           dx_version = 0x820; /* 8.2 */
                           break;
                        default:
                           dx_version = 0x800; /* 8.0 */
                     } /* switch (dsetup_revision) */
                  case 0x00040009:
                     switch (dsetup_revision) {
                        case 0x00000384:
                           dx_version = 0x900; /* 9.0 */
                           break;
                        case 0x00000385:
                           dx_version = 0x90a; /* 9.0a */
                           break;
                        case 0x00000386:
                           dx_version = 0x90b; /* 9.0b */
                           break;
                        case 0x00000387:
                           dx_version = 0x90b; /* 9.0(b or c) */
                           break;
                        case 0x00000388:
                           dx_version = 0x90c; /* 9.0c */
                           break;
                        default:
                           dx_version = 0x900;
                     } /* switch (dsetup_revision) */
               } /* switch (dsetup_version) */
            }
         }
         FreeLibrary( dsetup_hinst );
         if ( dx_version )
            return dx_version;
      }

      if (os_version.dwMajorVersion == 4) {
         /* NT4 up to SP2 is DX2, and SP3 onwards is DX3, so we are at least DX2 */
         dx_version = 0x200;

         /* we are not supposed to be able to tell which SP we are on, so check for DInput */
         dinput_hinst = LoadLibrary("DINPUT.DLL");
         if (!dinput_hinst) {
            /* no DInput... must be DX2 on NT 4 pre-SP3 */
            OutputDebugString("Couldn't LoadLibrary DInput\r\n");
            return dx_version;
         }

         DirectInputCreate = (DIRECTINPUTCREATE) GetProcAddress(dinput_hinst, "DirectInputCreateA");
         FreeLibrary(dinput_hinst);

         if (!DirectInputCreate) {
            /* no DInput... must DX2 on NT 4 pre-SP3 */
            return dx_version;
         }

         /* DX3 on NT4 SP3 or higher */
         dx_version = 0x300;

         return dx_version;
      }

      /* it's at least NT 5 and it's DX5a or higher:
       *  drop through to Win9x tests for a test of DDraw
       */
   }

   /* now we know we are in Windows 9x (or maybe 3.1), so anything's possible;
    * first see if DDRAW.DLL even exists.
    */
   ddraw_hinst = LoadLibrary("DDRAW.DLL");
   if (!ddraw_hinst) {
      dx_version = 0;
      goto End;
   }

   /* see if we can create the DirectDraw object */
   DirectDrawCreate = (DIRECTDRAWCREATE) GetProcAddress(ddraw_hinst, "DirectDrawCreate");
   if (!DirectDrawCreate) {
      dx_version = 0;
      goto End;
   }

   hr = DirectDrawCreate(NULL, &directdraw, NULL);
   if (FAILED(hr)) {
      dx_version = 0;
      goto End;
   }

   /* so DirectDraw exists, we are at least DX1 */
   dx_version = 0x100;

   /* let's see if IDirectDraw2 exists */
   hr = IDirectDraw_QueryInterface(directdraw, &IID_IDirectDraw2, &temp);
   if (FAILED(hr)) {
      /* no IDirectDraw2 exists... must be DX1 */
      goto End;
   }

   directdraw2 = temp;

   /* IDirectDraw2 exists... must be at least DX2 */
   IDirectDraw2_Release(directdraw2);
   dx_version = 0x200;

   /* see if we can create the DirectInput object */
   dinput_hinst = LoadLibrary("DINPUT.DLL");
   if (!dinput_hinst) {
      /* no DInput... must be DX2 */
      goto End;
   }

   DirectInputCreate = (DIRECTINPUTCREATE) GetProcAddress(dinput_hinst, "DirectInputCreateA");
   FreeLibrary(dinput_hinst);

   if (!DirectInputCreate) {
      /* no DInput... must be DX2 */
      goto End;
   }

   /* DirectInputCreate exists; that's enough to tell us that we are at least DX3 */
   dx_version = 0x300;

   /* we can tell if DX5 is present by checking for the existence of IDirectDrawSurface3;
    * first we need a surface to QI off of
    */
   memset(&ddraw_surf_desc, 0, sizeof(ddraw_surf_desc));
   ddraw_surf_desc.dwSize = sizeof(ddraw_surf_desc);
   ddraw_surf_desc.dwFlags = DDSD_CAPS;
   ddraw_surf_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

   hr = IDirectDraw_SetCooperativeLevel(directdraw, NULL, DDSCL_NORMAL);
   if (FAILED(hr)) {
      /* failure: this means DDraw isn't properly installed */
      dx_version = 0;
      goto End;
   }

   hr = IDirectDraw_CreateSurface(directdraw, &ddraw_surf_desc, &ddraw_surf, NULL);
   if (FAILED(hr)) {
      /* failure: this means DDraw isn't properly installed */
      dx_version = 0;
      goto End;
   }

   /* try for the IDirectDrawSurface3 interface; if it works, we're on DX5 at least */
   hr = IDirectDrawSurface_QueryInterface(ddraw_surf, &IID_IDirectDrawSurface3, &temp);
   if (FAILED(hr))
      goto End;

   ddraw_surf3 = temp;

   /* QI for IDirectDrawSurface3 succeeded; we must be at least DX5 */
   dx_version = 0x500;

#if DIRECTX_SDK_VERSION >= 0x600
   /* try for the IDirectDrawSurface4 interface; if it works, we're on DX6 at least */
   hr = IDirectDrawSurface_QueryInterface(ddraw_surf, &IID_IDirectDrawSurface4, (LPVOID *) &ddraw_surf4);
   if (FAILED(hr))
      goto End;

   /* QI for IDirectDrawSurface4 succeeded; we must be at least DX6 */
   dx_version = 0x600;

#if DIRECTX_SDK_VERSION >= 0x700
   /* try for the IDirectDrawSurface7 interface; if it works, we're on DX7 at least */
   hr = IDirectDrawSurface_QueryInterface(ddraw_surf, &IID_IDirectDrawSurface7, (LPVOID *) &ddraw_surf7);
   if (FAILED(hr))
      goto End;

   /* QI for IDirectDrawSurface7 succeeded; we must be at least DX7 */
   dx_version = 0x700;
#endif

#endif

 End:
   if (directdraw)
      IDirectDraw_Release(directdraw);

   if (ddraw_hinst)
      FreeLibrary(ddraw_hinst);

   return dx_version;
}
