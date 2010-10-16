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
 *      DrawSprocket graphics drivers.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */
#include "allegro.h"
#include "macalleg.h"
#include "allegro/platform/aintmac.h"
#include <string.h>

#define TRACE_MAC_GFX 0
/*Our main display device the display which contains the menubar on macs with more one display*/
GDHandle MainGDevice;
/*Our main Color Table for indexed devices*/
CTabHandle MainCTable = NULL;
/*Our current deph*/
short dspr_depth;
/*Vsync has ocurred*/
volatile short _sync = 0;
/*Vsync handler installed ?*/
short dspr_sync_installed = 0;
/*the control state of dspr*/
short dspr_state = 0;
/*Our dspr context*/
DSpContextReference   dspr_context;
/*??? Used for DrawSprocket e Vsync callback*/
const char refconst[16];

const RGBColor ForeDef={0,0,0};
const RGBColor BackDef={0xFFFF,0xFFFF,0xFFFF};

static char dspr_desc[256]=EMPTY_STRING;

static BITMAP *dspr_init(int w, int h, int v_w, int v_h, int color_depth);
static void dspr_exit(struct BITMAP *b);
static void dspr_vsync(void);
static void dspr_set_palette(const struct RGB *p, int from, int to, int retracesync);
static short dspr_active();
static short dspr_pause();
static short dspr_inactive();
static CGrafPtr dspr_get_back();
static CGrafPtr dspr_get_front();
static void dspr_swap();
static Boolean  dspr_vsync_interrupt(DSpContextReference inContext, void *inRefCon);

#pragma mark GFX_DRIVER
GFX_DRIVER gfx_drawsprocket ={
   GFX_DRAWSPROCKET,
   empty_string,
   empty_string,
   "DrawSprocket",
   dspr_init,
   dspr_exit,
   NULL,
   dspr_vsync,
   dspr_set_palette,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _mac_create_system_bitmap,
   _mac_destroy_system_bitmap,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,    // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,
   640, 480,
   TRUE,
   0,
   0,
   0,
   0,
};



/*
 * init an gfx mode return an pointer to BITMAP on sucess, NULL fails
 */
#pragma mark gfx driver routines
static BITMAP *dspr_init(int w, int h, int v_w, int v_h, int color_depth)
{
   OSStatus e;
   CGrafPtr cg;
   BITMAP* b;
   DSpContextAttributes Attr;
   Fixed myfreq;
   int done;

#if(TRACE_MAC_GFX)
   fprintf(stdout,"dspr_init(%d, %d, %d, %d, %d)\n", w, h, v_w, v_h, color_depth);
   fflush(stdout);
#endif

   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) return (NULL);

   
   Attr.frequency = Long2Fix(_refresh_rate_request);
   Attr.reserved1 = 0;
   Attr.reserved2 = 0;
   Attr.colorNeeds = kDSpColorNeeds_Require;
   Attr.colorTable = MainCTable;
   Attr.contextOptions = 0;
   Attr.gameMustConfirmSwitch = false;
   Attr.reserved3[0] = 0;
   Attr.reserved3[1] = 0;
   Attr.reserved3[2] = 0;
   Attr.reserved3[3] = 0;
   Attr.pageCount = 1;
   Attr.displayWidth = w;
   Attr.displayHeight = h;
   Attr.displayBestDepth = color_depth;

   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   _rgb_r_shift_16 = 10;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   _rgb_r_shift_32 = 16;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 0;

   switch(color_depth){
      case 8:
         dspr_depth = 8;
         Attr.displayDepthMask = kDSpDepthMask_8;
         break;
      case 15:
         dspr_depth = 15;
         Attr.displayDepthMask = kDSpDepthMask_16;
         break;
      case 24:
         dspr_depth = 24;
         Attr.displayDepthMask = kDSpDepthMask_32;
         break;
      default:
         goto Error;
   }
   Attr.backBufferBestDepth = color_depth;
   Attr.backBufferDepthMask = Attr.displayDepthMask;
   
   e = DSpFindBestContext(&Attr, &dspr_context);
   if(e != noErr){
      Attr.frequency = 0;
      e = DSpFindBestContext(&Attr, &dspr_context);
   }
   if(e != noErr) goto Error;/* I HATE "GOTO" */

   Attr.displayWidth = w;
   Attr.displayHeight = h;
   Attr.contextOptions = 0;

   e = DSpContext_Reserve(dspr_context, &Attr);
   if(e != noErr) goto Error;/* I HATE "GOTO" */
   dspr_state |= kRDDReserved;

   dspr_active();

   e = DSpContext_SetVBLProc (dspr_context, dspr_vsync_interrupt,(void *)refconst);
   if(e == noErr){dspr_sync_installed = 1;}
   else{dspr_sync_installed = 0;}

   cg = dspr_get_front();

   b =_CGrafPtr_to_system_bitmap(cg);
   if(b){
      DSpContext_GetMonitorFrequency  (dspr_context,&myfreq);
      _set_current_refresh_rate(Fix2Long(myfreq));

      gfx_drawsprocket.w = w;
      gfx_drawsprocket.h = h;

      uszprintf(dspr_desc, sizeof(dspr_desc), get_config_text("DrawSprocket %d x %d, %dbpp, %dhz"), w, h, dspr_depth, _current_refresh_rate);
      gfx_drawsprocket.desc = dspr_desc;
      return b;
   }
Error:
#if(TRACE_MAC_GFX)
   fprintf(stdout,"dspr_init()failed\n");
   fflush(stdout);
#endif
   dspr_exit(b);
   return NULL;
}



/*
 * reservated
 */
static void dspr_exit(struct BITMAP *b)
{
#pragma unused b
   OSStatus e;
#if(TRACE_MAC_GFX)
   fprintf(stdout,"dspr_exit()\n");
   fflush(stdout);
#endif
   if((dspr_state & kRDDReserved) != 0){
      e = DSpContext_SetState(dspr_context, kDSpContextState_Inactive);
      e = DSpContext_Release(dspr_context);
   }
   dspr_state = 0;
   gfx_drawsprocket.w = 0;
   gfx_drawsprocket.h = 0;
   dspr_depth = 0;
}



/*
 * reservated
 */
static void dspr_vsync(void)
{
   if(dspr_sync_installed){
      _sync = 0;
      while(!_sync){}
   }
}



/*
 * reservated
 */
static void dspr_set_palette(const struct RGB *p, int from, int to, int retracesync)
{
   int i;OSErr e;
#if(TRACE_MAC_GFX)
   fprintf(stdout,"set_palette");
   fflush(stdout);
#endif
   if(MainCTable == NULL){
      MainCTable = GetCTable(8);
      DetachResource((Handle) MainCTable);
   }
   for(i = from;i<= to;i ++){
      (**MainCTable).ctTable[i].rgb.red = p[i].r*1040;
      (**MainCTable).ctTable[i].rgb.green = p[i].g*1040;
      (**MainCTable).ctTable[i].rgb.blue = p[i].b*1040;
   }
   if(retracesync)dspr_vsync();
   if(dspr_depth == 8){
      e = DSpContext_SetCLUTEntries(dspr_context, (**MainCTable).ctTable, from, to - from);
   }
}



/*
 * reservated
 */
static short dspr_active()
{
   if(! (dspr_state & kRDDActive)){
      if(! (dspr_state & kRDDPaused))
	      if(DSpContext_SetState(dspr_context , kDSpContextState_Active) != noErr)
    	     return 1;
      dspr_state &= (~kRDDPaused);
      dspr_state |= kRDDActive;
   }
   return 0;
}



/*
 * reservated
 */
static short dspr_pause()
{
   if(! (dspr_state & kRDDPaused)){
      if(DSpContext_SetState(dspr_context, kDSpContextState_Paused) != noErr)return 1;
      dspr_state &= (~kRDDActive);
      dspr_state |= kRDDPaused;
      DrawMenuBar();
   }
   return 0;
}



/*
 * reservated
 */
static short dspr_inactive()
{
   if(! (dspr_state & (kRDDPaused | kRDDActive))){
      if(DSpContext_SetState(dspr_context, kDSpContextState_Inactive) != noErr)return 1;
      dspr_state &= (~kRDDPaused);
      dspr_state &= (~kRDDActive);
      DrawMenuBar();
   }
   return 0;
}



/*
 * reservated
 */
static CGrafPtr dspr_get_back()
{
   CGrafPtr theBuffer;
   DSpContext_GetBackBuffer(dspr_context, kDSpBufferKind_Normal, &theBuffer);
   return theBuffer;
}



/*
 * reservated
 */
static CGrafPtr dspr_get_front()
{
   CGrafPtr theBuffer;
   DSpContext_GetFrontBuffer(dspr_context, &theBuffer);
   return theBuffer;
}




/*
 * reservated
 */
static void dspr_swap()
{
   DSpContext_SwapBuffers(dspr_context, nil, nil);
}



/*
 * our vsync interrupt handle
 */
static Boolean dspr_vsync_interrupt (DSpContextReference inContext, void *inRefCon)
{
#pragma unused inContext, inRefCon
   _sync = 1;
   return false;
}



/*
 * drawsprocket initialization code should be called only one time
 */
int _dspr_sys_init()
{
   OSErr e;
   MainGDevice = GetMainDevice();
   if (MainGDevice == 0L)
      return -1;
   MainCTable = GetCTable(8);
   DetachResource((Handle) MainCTable);
   if ((Ptr) DSpStartup == (Ptr) kUnresolvedCFragSymbolAddress)
      return -2;
   e = DSpStartup();
   if(e != noErr)
      return -3;
   dspr_state = 0;
   HideCursor();
   return 0;
}




/*
 * drawsprocket exit code to should called only one time
 */
void _dspr_sys_exit()
{
   DSpShutdown();
   ShowCursor();
}




