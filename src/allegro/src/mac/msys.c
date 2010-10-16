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
 *      Mac system, timer and mouse drivers.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "macalleg.h"
#include "allegro/platform/aintmac.h"

#define TRACE_MAC_MOUSE 0
#define TRACE_MAC_SYSTEM 0


/*Our TimerManager task entry queue*/
static TMTask   tm_entry;

/*Our TM task is running*/
short tm_running = 0;
long  tm_delay = 0;
short mac_timer_installed = FALSE;
short mac_mouse_installed = FALSE;
static int mouse_delay = 0;
static int key_delay = 0;
extern volatile short _mouse2nd;
short _interrupt_time = FALSE;

extern volatile KeyMap KeyNow;
extern volatile KeyMap KeyOld;
extern short _mac_keyboard_installed;
extern void _key_mac_interrupt(void);

/*Our window title ???*/
char macwindowtitle[256];
/*char buffer for Trace*/
char macsecuremsg[256];

static pascal void tm_interrupt(TMTaskPtr tmTaskPtr);
static int mac_timer_init(void);
static void mac_timer_exit(void);
static int mouse_mac_init(void);
static void mouse_mac_exit(void);
static int mac_init(void);
static void mac_exit(void);
static void mac_get_executable_name(char *output, int size);
static void mac_set_window_title(const char *name);
static void mac_message(const char *msg);
static void mac_assert(const char *msg);
static BITMAP * mac_create_bitmap(int color_depth, int width, int height);
static int mac_desktop_color_depth(void);
static void mac_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode);
static void mac_yield_timeslice(void);
static int mac_trace_handler(const char *msg);



SYSTEM_DRIVER system_macos ={
   SYSTEM_MACOS, /* id */
   empty_string, /* name */
   empty_string, /* desc */
   "MacOs",      /* ascii_name */
   mac_init,
   mac_exit,
   mac_get_executable_name,
   NULL, /* find_resource */
   mac_set_window_title,
   NULL, /* set_close_button_callback */
   mac_message,
   mac_assert,
   NULL, /* save_console_state */
   NULL, /* restore_console_state */
   NULL, /* mac_create_bitmap, */
   NULL, /* created_bitmap */
   NULL, /* create_sub_bitmap */
   NULL, /* created_sub_bitmap */
   NULL, /* destroy_bitmap */
   NULL, /* read_hardware_palette */
   NULL, /* set_palette_range */
   NULL, /* get_vtable */
   NULL, /* set_display_switch_mode */
   NULL, /* display_switch_lock */
   mac_desktop_color_depth,
   NULL, /* get_desktop_resolution */
   mac_get_gfx_safe_mode,
   mac_yield_timeslice,
   NULL, /* create_mutex */
   NULL, /* destroy_mutex */
   NULL, /* lock_mutex */
   NULL, /* unlock_mutex */
   NULL, /* gfx_drivers */
   NULL, /* digi_drivers */
   NULL, /* midi_drivers */
   NULL, /* keyboard_drivers */
   NULL, /* mouse_drivers */
   NULL, /* joystick_drivers */
   NULL  /* timer_drivers */
};



MOUSE_DRIVER mouse_macos ={
   MOUSE_MACOS,
   empty_string,
   empty_string,
   "MacOs Mouse",
   mouse_mac_init,
   mouse_mac_exit,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};



TIMER_DRIVER timer_macos ={
   TIMER_MACOS,
   empty_string,
   empty_string,
   "MacOs Timer",
   mac_timer_init,
   mac_timer_exit,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
};



/*
 *
 */
static int mac_init()
{
   os_type = OSTYPE_MACOS;
   os_multitasking = TRUE;
   if (!_al_trace_handler)
      register_trace_handler(mac_trace_handler);
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_init()::OK\n");
   fflush(stdout);
#endif
   _rgb_r_shift_15 = 10;         /*Ours truecolor pixel format */
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   return 0;
}



/*
 *
 */
static void mac_exit()
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_exit()\n");
   fflush(stdout);
#endif
}



/*
 *
 */
void _mac_get_executable_name(char *output, int size) {
   Str255 appName;
   OSErr e;
   ProcessInfoRec info;
   ProcessSerialNumber curPSN;

   e = GetCurrentProcess(&curPSN);

   info.processInfoLength = sizeof(ProcessInfoRec);
   info.processName = appName;
   info.processAppSpec = NULL;

   e = GetProcessInformation(&curPSN, &info);

   size = MIN(size, (unsigned char)appName[0]);
   _al_sane_strncpy(output, (char const *)appName+1, size);
   output[size] = 0;
}



/*
 *
 */
static void mac_get_executable_name(char *output, int size) {
   _mac_get_executable_name(output, size);
}



/*
 *
 */
static void mac_set_window_title(const char *name)
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_set_window_title(%s)\n", name);
   fflush(stdout);
#endif
   memcpy(macwindowtitle, name, 254);
   macwindowtitle[254] = 0;
}



/*
 *
 */
void _mac_message(const char *msg)
{
#if (TRACE_MAC_SYSTEM)
   fprintf(stdout, "mac_message(%s)\n", msg);
   fflush(stdout);
#endif
   memcpy(macsecuremsg, msg, 254);
   macsecuremsg[254] = 0;
   paramtext(macsecuremsg, "\0", "\0", "\0");
   if(keyboard_driver)
      keyboard_driver->exit();
   if(mouse_driver)
      mouse_driver->exit();
   ShowCursor();
   Alert(rmac_message, nil);
   HideCursor();
   if(keyboard_driver)
      keyboard_driver->init();
   if(mouse_driver)
      mouse_driver->init();
}



/*
 *
 */
static void mac_message(const char *msg)
{
   _mac_message(msg);
}



/*
 *
 */
static void mac_assert(const char *msg)
{
   debugstr(msg);
}



/* create_bitmap_ex
 *  Creates a new memory bitmap in the specified color_depth
 */
BITMAP *mac_create_bitmap(int color_depth, int width, int height)
{
   GFX_VTABLE *vtable;
   BITMAP *bitmap;
   int i,width_bytes;

   vtable = _get_vtable(color_depth);
   if (!vtable)
      return NULL;

   bitmap = _AL_MALLOC(sizeof(BITMAP) + (sizeof(char *) * height));
   if (!bitmap)
      return NULL;

   width_bytes=(width * BYTES_PER_PIXEL(color_depth) + (MEMORY_ALIGN - 1) ) & -MEMORY_ALIGN;
   
   bitmap->dat = _AL_MALLOC(width_bytes * height + MEMORY_ALIGN);
   if (!bitmap->dat) {
      _AL_FREE(bitmap);
      return NULL;
   }

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = vtable;
   bitmap->write_bank = bitmap->read_bank = _stub_bank_switch;
   bitmap->id = 0;
   bitmap->extra = NULL;
   bitmap->x_ofs = 0;
   bitmap->y_ofs = 0;
   bitmap->seg = _default_ds();

   bitmap->line[0] =( (long)bitmap->dat + (MEMORY_ALIGN - 1)) & -MEMORY_ALIGN;
   for (i=1; i<height; i++)
      bitmap->line[i] = bitmap->line[i-1] + width_bytes;

   if (system_driver->created_bitmap)
      system_driver->created_bitmap(bitmap);

   return bitmap;
}



/*
 *
 */
static int mac_desktop_color_depth(void)
{
   return 0;
}



/* mac_get_gfx_safe_mode:
 *  Defines the safe graphics mode for this system.
 */
static void mac_get_gfx_safe_mode(int *driver, struct GFX_MODE *mode)
{
   *driver = GFX_DRAWSPROCKET;
   mode->width = 320;
   mode->height = 200;
   mode->bpp = 8;
}



/*
 *
 */
static void mac_yield_timeslice(void)
{
/*   SystemTask();*/
}



/*
 *
 */
static int mac_trace_handler(const char *msg)
{
   debugstr(msg);
   return 0;
}



/*
 *
 */
static int mac_timer_init(void)
{
   if (!tm_running)
      _tm_sys_init();
   mac_timer_installed = 1;
   return 0;
}



/*
 *
 */
static void mac_timer_exit(void)
{
   mac_timer_installed = 0;
}



/*
 *
 */
static int mouse_mac_init(void)
{
#if (TRACE_MAC_MOUSE)

   fprintf(stdout, "mouse_mac_init()\n");
   fflush(stdout);
#endif
   if (!tm_running)
      _tm_sys_init();
   mac_mouse_installed = 1;
   return 2;/*emulated two button command-click*/
}



/*
 *
 */
static void mouse_mac_exit(void)
{
#if (TRACE_MAC_MOUSE)
   fprintf(stdout, "mouse_mac_exit()\n");
   fflush(stdout);
#endif
   mac_mouse_installed = 0;
}



/*
 *
 */
static pascal void tm_interrupt(TMTaskPtr tmTaskPtr)
{
   int m_b, t;
   t = tm_delay;
   if (mac_timer_installed){
      t = _handle_timer_tick(t*TIMERS_PER_SECOND/1000)*1000/TIMERS_PER_SECOND;
      if (t < 5) t = 5;
   }
   if (mac_mouse_installed) {
      mouse_delay += t;
      if(mouse_delay > 50){
	 Point pt;
         mouse_delay=0;
         pt = (*((volatile Point *)0x082C));
         m_b = (*((volatile UInt8 *)0x0172)) & 0x80 ? 0: _mouse2nd ? 2 : 1;
	 if(_mouse_b != m_b || _mouse_x != pt.h || _mouse_y != pt.v ){
	    _mouse_b = m_b;
            _mouse_x = pt.h;
            _mouse_y = pt.v;
            _handle_mouse_input();
	 };
      };
   }
   if (_mac_keyboard_installed){
      key_delay += t;
      if(key_delay > 50){
         key_delay = 0;
         _key_mac_interrupt();
      }
   }
   PrimeTime((QElemPtr)tmTaskPtr, t);
   tm_delay = t;
}



/*
 *
 */
int _tm_sys_init()
{
   if (!tm_running) {
      LOCK_VARIABLE(tm_entry);
      LOCK_VARIABLE(tm_delay);
      LOCK_VARIABLE(mouse_delay);
      LOCK_VARIABLE(key_delay);
      tm_entry.tmAddr = NewTimerProc(tm_interrupt);
      tm_entry.tmWakeUp = 0;
      tm_entry.tmReserved = 0;
      InsXTime((QElemPtr)&tm_entry);
      tm_delay = 40;
      PrimeTime((QElemPtr)&tm_entry, tm_delay);
      tm_running = TRUE;
   }
   return 0;
}



/*
 *
 */
void _tm_sys_exit()
{
   
   if (tm_running){
      RmvTime((QElemPtr)&tm_entry);
      tm_running = FALSE;
   }
}
/*The our QuickDraw globals */
QDGlobals qd;



/*
 * an strdup function to mac
 */
char *strdup(const char *p){
   char *tmp=_AL_MALLOC(strlen(p)+1);
   if(tmp){
      _al_sane_strncpy(tmp,p,strlen(p)+1);
   }
   return tmp;
}



/*
 * convert pascal strings to c strings
 */
void ptoc(StringPtr pstr,char *cstr)
{
   BlockMove(pstr+1,cstr,pstr[0]);
   cstr[pstr[0]]='\0';
}



/*
 *  Simple function that returns TRUE if we're running on a Mac thatÉ
 *  is running Color Quickdraw.
 */
static Boolean DoWeHaveColor (void)
{
   SysEnvRec      thisWorld;

   SysEnvirons(2, &thisWorld);      // Call the old SysEnvirons() function.
   return (thisWorld.hasColorQD);   // And return whether it has Color QuickDraw.
}



/*  Another simple "environment" function.Returns TRUE if the Mac we're runningÉ
 *  on has System 6.0.5 or more recent.(6.0.5 introduced the ability to switchÉ
 *  color depths.)
 */
static Boolean DoWeHaveSystem605 (void)
{
   SysEnvRec      thisWorld;
   Boolean         haveIt;

   SysEnvirons(2, &thisWorld);      // Call the old SysEnvirons() function.
   if (thisWorld.systemVersion >= 0x0605)
      haveIt = TRUE;            // Check the System version for 6.0.5É
   else                     // or more recent
      haveIt = FALSE;
   return (haveIt);
}



/*
 *
 */
Boolean RTrapAvailable(short tNumber,TrapType tType)
{
   return NGetTrapAddress( tNumber, tType ) != NGetTrapAddress( _Unimplemented, ToolTrap);
}



/*
 * Executs an cleanup before return to system called from atexit()
 */
static void mac_cleanup(){
   _tm_sys_exit();
   _dspr_sys_exit();
   FlushEvents(nullEvent|mouseDown|mouseUp|keyDown|keyUp|autoKey|updateEvt,0);
}



/*
 *
 *The entry point for MacAllegro programs setup mac toolbox application memory
 *This routine should be called before main() this is do by -main MacEntry option in final Link
 *if not called then the program can crash!!!
 */
void MacEntry(){
   char argbuf[512];
   char *argv[64];
   int argc;
   int i, q;
/*   ControlHandle ch;*/
   SysEnvRec envRec;long   heapSize;
   OSErr e;

   InitGraf((Ptr) &qd.thePort);      /*init the normal mac toolbox*/
   InitFonts();
   InitWindows();
   InitMenus();
   TEInit();
   InitDialogs(nil);
   InitCursor();
   InitPalettes();

/*memory setup*/
   SysEnvirons( curSysEnvVers, &envRec );
   if (envRec.machineType < 0 )
      ExitToShell();
   if (kStackNeeded > StackSpace()){
      SetApplLimit((Ptr) ((long) GetApplLimit() - kStackNeeded + StackSpace()));
   }
   heapSize = (long) GetApplLimit() - (long) ApplicationZone();
   if ( heapSize < kHeapNeeded){
      _mac_message("no enough memory");
      ExitToShell();
   }
   MaxApplZone();
   if(!(RTrapAvailable(_WaitNextEvent, ToolTrap)))ExitToShell();

   GetDateTime((unsigned long*) &qd.randSeed);

/*   memcpy(macsecuremsg, "teste", 254);*/
/**/
/*   paramtext(macsecuremsg, "\0", "\0", "\0");*/
/**/
/*   Alert(rmac_command, nil);*/
/*   */
/*   e=GetDialogItemAsControl(ch);*/
/*   */
/*   printf("%d",e);*/

   if(_dspr_sys_init()){
      _mac_message("no could start drawsprocket");
	  return;
   }

   _mac_file_sys_init();

   _mac_init_system_bitmap();           /*init our system bitmap vtable*/

   atexit(&mac_cleanup);

   _mac_get_executable_name(argbuf,511);/*get command line*/

   argc = 0;
   i = 0;

   /* parse commandline into argc/argv format */
   while (argbuf[i]) {
      while ((argbuf[i]) && (uisspace(argbuf[i])))i++;
      if (argbuf[i]) {
         if ((argbuf[i] == '\'') || (argbuf[i] == '"')){
            q = argbuf[i++];
            if (!argbuf[i])break;
         }
         else q = 0;
         argv[argc++] = &argbuf[i];
         while ((argbuf[i]) && ((q) ? (argbuf[i] != q) : (!uisspace(argbuf[i]))))i++;
         if (argbuf[i])
		    {
            argbuf[i] = 0;
            i++;
         }
      }
   }

#if (TRACE_MAC_SYSTEM)
   fprintf(stdout,"MacEntry Done\n");
   fflush(stdout);
#endif
   main(argc, argv);
   FlushEvents(everyEvent,0);
}
