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
 *      Video driver for VESA compatible graphics cards. Supports VESA 2.0 
 *      linear framebuffers and protected mode bank switching interface.
 *
 *      By Shawn Hargreaves.
 *
 *      Refresh rate control added by S. Sakamaki.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



static BITMAP *vesa_1_init(int w, int h, int v_w, int v_h, int color_depth);
static BITMAP *vesa_2b_init(int w, int h, int v_w, int v_h, int color_depth);
static BITMAP *vesa_2l_init(int w, int h, int v_w, int v_h, int color_depth);
static BITMAP *vesa_3_init(int w, int h, int v_w, int v_h, int color_depth);
static void vesa_exit(BITMAP *b);
static int vesa_scroll(int x, int y);
static void vesa_vsync(void);
static void vesa_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync);
static int vesa_request_scroll(int x, int y);
static int vesa_poll_scroll(void);
static GFX_MODE_LIST *vesa_fetch_mode_list(void);

static char vesa_desc[256] = EMPTY_STRING;



GFX_DRIVER gfx_vesa_1 = 
{
   GFX_VESA1,
   empty_string,
   empty_string,
   "VESA 1.x",
   vesa_1_init,
   vesa_exit,
   vesa_scroll,
   _vga_vsync,
   _vga_set_palette_range,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   NULL, NULL,                   /* no state saving */
   NULL,    /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   vesa_fetch_mode_list,         /* aye! */
   0, 0, FALSE, 0, 0, 0, 0, FALSE
};



GFX_DRIVER gfx_vesa_2b = 
{
   GFX_VESA2B,
   empty_string,
   empty_string,
   "VESA 2.0 (banked)",
   vesa_2b_init,
   vesa_exit,
   vesa_scroll,
   vesa_vsync,
   vesa_set_palette_range,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   NULL, NULL,                   /* no state saving */
   NULL,
   vesa_fetch_mode_list,         /* aye! */
   0, 0, FALSE, 0, 0, 0, 0, FALSE
};



GFX_DRIVER gfx_vesa_2l = 
{
   GFX_VESA2L,
   empty_string,
   empty_string,
   "VESA 2.0 (linear)",
   vesa_2l_init,
   vesa_exit,
   vesa_scroll,
   vesa_vsync,
   vesa_set_palette_range,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   NULL, NULL,                   /* no state saving */
   NULL,
   vesa_fetch_mode_list,         /* aye! */
   0, 0, FALSE, 0, 0, 0, 0, FALSE
};



GFX_DRIVER gfx_vesa_3 = 
{
   GFX_VESA3,
   empty_string,
   empty_string,
   "VESA 3.0",
   vesa_3_init,
   vesa_exit,
   vesa_scroll,
   vesa_vsync,
   vesa_set_palette_range,
   vesa_request_scroll,
   vesa_poll_scroll,
   NULL,
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   NULL, NULL,                   /* no state saving */
   NULL,
   vesa_fetch_mode_list,         /* aye! */
   0, 0, FALSE, 0, 0, 0, 0, FALSE
};



#define MASK_LINEAR(addr)     ((addr) & 0x000FFFFF)
#define RM_TO_LINEAR(addr)    ((((addr) & 0xFFFF0000) >> 12) + ((addr) & 0xFFFF))
#define RM_OFFSET(addr)       ((addr) & 0xF)
#define RM_SEGMENT(addr)      (((addr) >> 4) & 0xFFFF)



#ifdef ALLEGRO_GCC
   #define __PACKED__   __attribute__ ((packed))
#else
   #define __PACKED__
#endif

#ifdef ALLEGRO_WATCOM
   #pragma pack (1)
#endif



typedef struct VESA_INFO         /* VESA information block structure */
{ 
   char           VESASignature[4];
   unsigned short VESAVersion          __PACKED__;
   unsigned long  OEMStringPtr         __PACKED__;
   unsigned char  Capabilities[4];
   unsigned long  VideoModePtr         __PACKED__; 
   unsigned short TotalMemory          __PACKED__; 
   unsigned short OemSoftwareRev       __PACKED__; 
   unsigned long  OemVendorNamePtr     __PACKED__; 
   unsigned long  OemProductNamePtr    __PACKED__; 
   unsigned long  OemProductRevPtr     __PACKED__; 
   unsigned char  Reserved[222]; 
   unsigned char  OemData[256]; 
} VESA_INFO;



typedef struct MODE_INFO         /* VESA information for a specific mode */
{
   unsigned short ModeAttributes       __PACKED__; 
   unsigned char  WinAAttributes; 
   unsigned char  WinBAttributes; 
   unsigned short WinGranularity       __PACKED__; 
   unsigned short WinSize              __PACKED__; 
   unsigned short WinASegment          __PACKED__; 
   unsigned short WinBSegment          __PACKED__; 
   unsigned long  WinFuncPtr           __PACKED__; 
   unsigned short BytesPerScanLine     __PACKED__; 
   unsigned short XResolution          __PACKED__; 
   unsigned short YResolution          __PACKED__; 
   unsigned char  XCharSize; 
   unsigned char  YCharSize; 
   unsigned char  NumberOfPlanes; 
   unsigned char  BitsPerPixel; 
   unsigned char  NumberOfBanks; 
   unsigned char  MemoryModel; 
   unsigned char  BankSize; 
   unsigned char  NumberOfImagePages;
   unsigned char  Reserved_page; 
   unsigned char  RedMaskSize; 
   unsigned char  RedMaskPos; 
   unsigned char  GreenMaskSize; 
   unsigned char  GreenMaskPos;
   unsigned char  BlueMaskSize; 
   unsigned char  BlueMaskPos; 
   unsigned char  ReservedMaskSize; 
   unsigned char  ReservedMaskPos; 
   unsigned char  DirectColorModeInfo;

   /* VBE 2.0 extensions */
   unsigned long  PhysBasePtr          __PACKED__; 
   unsigned long  OffScreenMemOffset   __PACKED__; 
   unsigned short OffScreenMemSize     __PACKED__; 

   /* VBE 3.0 extensions */
   unsigned short LinBytesPerScanLine  __PACKED__;
   unsigned char  BnkNumberOfPages;
   unsigned char  LinNumberOfPages;
   unsigned char  LinRedMaskSize;
   unsigned char  LinRedFieldPos;
   unsigned char  LinGreenMaskSize;
   unsigned char  LinGreenFieldPos;
   unsigned char  LinBlueMaskSize;
   unsigned char  LinBlueFieldPos;
   unsigned char  LinRsvdMaskSize;
   unsigned char  LinRsvdFieldPos;
   unsigned long  MaxPixelClock        __PACKED__;

   unsigned char  Reserved[190]; 
} MODE_INFO;



typedef struct PM_INFO           /* VESA 2.0 protected mode interface */
{
   unsigned short setWindow            __PACKED__; 
   unsigned short setDisplayStart      __PACKED__; 
   unsigned short setPalette           __PACKED__; 
   unsigned short IOPrivInfo           __PACKED__; 
} PM_INFO;



typedef struct CRTCInfoBlock     /* VESA 3.0 CRTC timings structure */
{
    unsigned short HorizontalTotal     __PACKED__;
    unsigned short HorizontalSyncStart __PACKED__;
    unsigned short HorizontalSyncEnd   __PACKED__;
    unsigned short VerticalTotal       __PACKED__;
    unsigned short VerticalSyncStart   __PACKED__;
    unsigned short VerticalSyncEnd     __PACKED__;
    unsigned char  Flags;
    unsigned long  PixelClock          __PACKED__;    /* units of Hz */
    unsigned short RefreshRate         __PACKED__;    /* units of 0.01 Hz */
    unsigned char  reserved[40];
} CRTCInfoBlock;


#define HPOS         0
#define HNEG         (1 << 2)
#define VPOS         0
#define VNEG         (1 << 3)
#define INTERLACED   (1 << 1)
#define DOUBLESCAN   (1 << 0)



static VESA_INFO vesa_info;               /* SVGA info block */
static MODE_INFO mode_info;               /* info for this video mode */
static char oem_string[256];              /* vendor name */

static unsigned long lb_linear = 0;       /* linear address of framebuffer */
static int lb_segment = 0;                /* descriptor for the buffer */

static PM_INFO *pm_info = NULL;           /* VESA 2.0 pmode interface */

static unsigned long mmio_linear = 0;     /* linear addr for mem mapped IO */

static int vesa_xscroll = 0;              /* current display start address */
static int vesa_yscroll = 0;

__dpmi_regs _dpmi_reg;                    /* for the bank switch routines */

int _window_2_offset = 0;                 /* window state information */
int _mmio_segment = 0;

void (*_pm_vesa_switcher)(void) = NULL;   /* VBE 2.0 pmode interface */
void (*_pm_vesa_scroller)(void) = NULL;
void (*_pm_vesa_palette)(void) = NULL;

static int evilness_flag = 0;             /* set if we are doing dodgy things
					     with VGA registers because the
					     VESA implementation is no good */



/* get_vesa_info:
 *  Retrieves a VESA info block structure, returning 0 for success.
 */
static int get_vesa_info(void)
{
   unsigned long addr;
   int c;

   _farsetsel(_dos_ds);

   for (c=0; c<(int)sizeof(VESA_INFO); c++)
      _farnspokeb(MASK_LINEAR(__tb)+c, 0);

   dosmemput("VBE2", 4, MASK_LINEAR(__tb));

   _dpmi_reg.x.ax = 0x4F00;
   _dpmi_reg.x.di = RM_OFFSET(__tb);
   _dpmi_reg.x.es = RM_SEGMENT(__tb);
   __dpmi_int(0x10, &_dpmi_reg);
   if (_dpmi_reg.h.ah)
      return -1;

   dosmemget(MASK_LINEAR(__tb), sizeof(VESA_INFO), &vesa_info);

   if (strncmp(vesa_info.VESASignature, "VESA", 4) != 0)
      return -1;

   addr = RM_TO_LINEAR(vesa_info.OEMStringPtr);
   _farsetsel(_dos_ds);
   c = 0;

   do {
      oem_string[c] = _farnspeekb(addr++);
   } while (oem_string[c++]);

   return 0;
}



/* get_mode_info:
 *  Retrieves a mode info block structure, for a specific graphics mode.
 *  Returns 0 for success.
 */
static int get_mode_info(int mode)
{
   int c;

   _farsetsel(_dos_ds);

   for (c=0; c<(int)sizeof(MODE_INFO); c++)
      _farnspokeb(MASK_LINEAR(__tb)+c, 0);

   _dpmi_reg.x.ax = 0x4F01;
   _dpmi_reg.x.di = RM_OFFSET(__tb);
   _dpmi_reg.x.es = RM_SEGMENT(__tb);
   _dpmi_reg.x.cx = mode;
   __dpmi_int(0x10, &_dpmi_reg);
   if (_dpmi_reg.h.ah)
      return -1;

   dosmemget(MASK_LINEAR(__tb), sizeof(MODE_INFO), &mode_info);
   return 0;
}



/* _vesa_vidmem_check:
 *  Trying to autodetect the available video memory in my hardware level 
 *  card drivers is a hopeless task. Fortunately that seems to be one of 
 *  the few things that VESA usually gets right, so if VESA is available 
 *  the non-VESA drivers can use it to confirm how much vidmem is present.
 */
long _vesa_vidmem_check(long mem)
{
   if (get_vesa_info() != 0)
      return mem;

   if (vesa_info.TotalMemory <= 0)
      return mem;

   return MIN(mem, (vesa_info.TotalMemory << 16));
}



/* find_vesa_mode:
 *  Tries to find a VESA mode number for the specified screen size.
 *  Searches the mode list from the VESA info block, and if that doesn't
 *  work, uses the standard VESA mode numbers.
 */
static int find_vesa_mode(int w, int h, int color_depth, int vbe_version)
{
   #define MAX_VESA_MODES 1024

   unsigned short mode[MAX_VESA_MODES];
   int memorymodel, bitsperpixel;
   int redmasksize, greenmasksize, bluemasksize;
   int greenmaskpos;
   int reservedmasksize, reservedmaskpos;
   int rs, gs, bs, gp, rss, rsp;
   int c, modes;
   long mode_ptr;

   if (get_vesa_info() != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VESA not available"));
      return 0;
   }

   if (vesa_info.VESAVersion < (vbe_version<<8)) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VBE %d.0 not available"), vbe_version);
      return 0;
   }

   mode_ptr = RM_TO_LINEAR(vesa_info.VideoModePtr);
   modes = 0;

   _farsetsel(_dos_ds);

   while ((mode[modes] = _farnspeekw(mode_ptr)) != 0xFFFF) {
      modes++;
      mode_ptr += 2;
   }

   switch (color_depth) {

      #ifdef ALLEGRO_COLOR8

	 case 8:
	    memorymodel = 4;
	    bitsperpixel = 8;
	    redmasksize = greenmasksize = bluemasksize = 0;
	    greenmaskpos = 0;
	    reservedmasksize = 0;
	    reservedmaskpos = 0;
	    break;

      #endif

      #ifdef ALLEGRO_COLOR16

	 case 15:
	    memorymodel = 6;
	    bitsperpixel = 15;
	    redmasksize = greenmasksize = bluemasksize = 5;
	    greenmaskpos = 5;
	    reservedmasksize = 1;
	    reservedmaskpos = 15;
	    break;

	 case 16:
	    memorymodel = 6;
	    bitsperpixel = 16;
	    redmasksize = bluemasksize = 5;
	    greenmasksize = 6;
	    greenmaskpos = 5;
	    reservedmasksize = 0;
	    reservedmaskpos = 0;
	    break;

      #endif

      #ifdef ALLEGRO_COLOR24

	 case 24:
	    memorymodel = 6;
	    bitsperpixel = 24;
	    redmasksize = bluemasksize = greenmasksize = 8;
	    greenmaskpos = 8;
	    reservedmasksize = 0;
	    reservedmaskpos = 0;
	    break;

      #endif

      #ifdef ALLEGRO_COLOR32

	 case 32:
	    memorymodel = 6;
	    bitsperpixel = 32;
	    redmasksize = greenmasksize = bluemasksize = 8;
	    greenmaskpos = 8;
	    reservedmasksize = 8;
	    reservedmaskpos = 24;
	    break;

      #endif

      default:
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
	 return 0;
   }

   #define MEM_MATCH(mem, wanted_mem) \
      ((mem == wanted_mem) || ((mem == 4) && (wanted_mem == 6)))

   #define BPP_MATCH(bpp, wanted_bpp) \
      ((bpp == wanted_bpp) || ((bpp == 16) && (wanted_bpp == 15)))

   #define RES_SIZE_MATCH(size, wanted_size, bpp) \
      ((size == wanted_size) || ((size == 0) && ((bpp == 15) || (bpp == 32))))

   #define RES_POS_MATCH(pos, wanted_pos, bpp) \
      ((pos == wanted_pos) || ((pos == 0) && ((bpp == 15) || (bpp == 32))))

   /* search the list of modes */
   for (c=0; c<modes; c++) {
      if (get_mode_info(mode[c]) == 0) {
	 if ((vbe_version >= 3) && (mode_info.ModeAttributes & 0x80)) {
	    rs = mode_info.LinRedMaskSize;
	    gs = mode_info.LinGreenMaskSize;
	    bs = mode_info.LinBlueMaskSize;
	    gp = mode_info.LinGreenFieldPos;
	    rss = mode_info.LinRsvdMaskSize;
	    rsp = mode_info.LinRsvdFieldPos;
	 }
	 else {
	    rs = mode_info.RedMaskSize;
	    gs = mode_info.GreenMaskSize;
	    bs = mode_info.BlueMaskSize;
	    gp = mode_info.GreenMaskPos;
	    rss = mode_info.ReservedMaskSize;
	    rsp = mode_info.ReservedMaskPos;
	 }

	 if (((mode_info.ModeAttributes & 25) == 25) &&
	     (mode_info.XResolution == w) && 
	     (mode_info.YResolution == h) && 
	     (mode_info.NumberOfPlanes == 1) && 
	     MEM_MATCH(mode_info.MemoryModel, memorymodel) &&
	     BPP_MATCH(mode_info.BitsPerPixel, bitsperpixel) &&
	     (rs == redmasksize) && (gs == greenmasksize) && 
	     (bs == bluemasksize) && (gp == greenmaskpos) &&
	     RES_SIZE_MATCH(rss, reservedmasksize, mode_info.BitsPerPixel) &&
	     RES_POS_MATCH(rsp, reservedmaskpos, mode_info.BitsPerPixel))

	    /* looks like this will do... */
	    return mode[c];
      } 
   }

   /* try the standard mode numbers */
   if ((w == 640) && (h == 400) && (color_depth == 8))
      c = 0x100;
   else if ((w == 640) && (h == 480) && (color_depth == 8))
      c = 0x101;
   else if ((w == 800) && (h == 600) && (color_depth == 8)) 
      c = 0x103;
   else if ((w == 1024) && (h == 768) && (color_depth == 8))
      c = 0x105;
   else if ((w == 1280) && (h == 1024) && (color_depth == 8))
      c = 0x107;
   else {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return 0; 
   }

   if (get_mode_info(c) == 0)
      return c;

   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
   return 0;
}



/* setup_vesa_desc:
 *  Sets up the VESA driver description string.
 */
static void setup_vesa_desc(GFX_DRIVER *driver, int vbe_version, int linear)
{
   char tmp1[64], tmp2[1600];

   /* VESA version number */
   uszprintf(vesa_desc, sizeof(vesa_desc), uconvert_ascii("%4.4s %d.%d (", tmp1), 
		                           uconvert_ascii(vesa_info.VESASignature, tmp2),
		                           vesa_info.VESAVersion>>8, 
		                           vesa_info.VESAVersion&0xFF);

   /* VESA description string */
   ustrzcat(vesa_desc, sizeof(vesa_desc), uconvert_ascii(oem_string, tmp2));
   ustrzcat(vesa_desc, sizeof(vesa_desc), uconvert_ascii(")", tmp1));

   /* warn about dodgy hardware */
   if (evilness_flag & 1)
      ustrzcat(vesa_desc, sizeof(vesa_desc), uconvert_ascii(", Trio64 bodge", tmp1));

   if (evilness_flag & 2)
      ustrzcat(vesa_desc, sizeof(vesa_desc), uconvert_ascii(", 0x4F06 N.A.", tmp1));

   /* banked/linear status for VBE 3.0 */
   if (vbe_version >= 3) {
      if (linear)
	 ustrzcat(vesa_desc, sizeof(vesa_desc), uconvert_ascii(", linear", tmp1));
      else
	 ustrzcat(vesa_desc, sizeof(vesa_desc), uconvert_ascii(", banked", tmp1));
   }

   driver->desc = vesa_desc;
}



/* vesa_fetch_mode_list:
 *  Generates a list of valid video modes for the VESA drivers.
 *  Returns the mode list on success or NULL on failure.
 */
static GFX_MODE_LIST *vesa_fetch_mode_list(void)
{
   GFX_MODE_LIST  *mode_list;
   unsigned long  mode_ptr;
   unsigned short *vesa_mode;
   int            mode, pos, vesa_list_length;

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list) return NULL;

   /* fetch list of VESA modes */
   if (get_vesa_info()) return NULL;

   _farsetsel(_dos_ds);

   /* count number of VESA modes */
   mode_ptr = RM_TO_LINEAR(vesa_info.VideoModePtr);
   for (mode_list->num_modes = 0; _farnspeekw(mode_ptr) != 0xFFFF; mode_list->num_modes++)
      mode_ptr += sizeof(unsigned short);

   /* allocate and fill in temporary vesa mode list */
   vesa_mode = _AL_MALLOC(sizeof(unsigned short) * mode_list->num_modes);
   if (!vesa_mode) return NULL;

   mode_ptr = RM_TO_LINEAR(vesa_info.VideoModePtr);
   for (mode = 0; _farnspeekw(mode_ptr) != 0xFFFF; mode++) {
      vesa_mode[mode] = _farnspeekw(mode_ptr);
      mode_ptr += sizeof(unsigned short);
   }

   vesa_list_length = mode_list->num_modes;

   /* zero out text and <8 bpp modes for later exclusion */
   for (mode = 0; mode < mode_list->num_modes; mode++) {
      if (get_mode_info(vesa_mode[mode])) return NULL;
      if ((mode_info.MemoryModel == 0) || (mode_info.BitsPerPixel < 8)) {
         vesa_mode[mode] = 0;
         mode_list->num_modes--;
      }
   }

   /* allocate mode list */
   mode_list->mode = _AL_MALLOC(sizeof(GFX_MODE) * (mode_list->num_modes + 1));
   if (!mode_list->mode) return NULL;

   /* fill in width, height and color depth for each VESA mode */
   mode = 0;
   for (pos = 0; pos < vesa_list_length; pos++) {
      if (get_mode_info(vesa_mode[pos])) return NULL;
      if (!vesa_mode[pos]) continue;
      mode_list->mode[mode].width  = mode_info.XResolution;
      mode_list->mode[mode].height = mode_info.YResolution;
      mode_list->mode[mode].bpp    = mode_info.BitsPerPixel;
      mode++;
   }

   /* terminate list */
   mode_list->mode[mode_list->num_modes].width  = 0;
   mode_list->mode[mode_list->num_modes].height = 0;
   mode_list->mode[mode_list->num_modes].bpp    = 0;

   /* free up temporary vesa mode list */
   _AL_FREE(vesa_mode);

   return mode_list;
}



/* get_pmode_functions:
 *  Attempts to use VBE 2.0 functions to get access to protected mode bank
 *  switching, set display start, and palette routines, storing the bank
 *  switchers in the w* parameters.
 */
static int get_pmode_functions(void (**w1)(void), void (**w2)(void))
{
   unsigned short *p;

   if (vesa_info.VESAVersion < 0x200)        /* have we got VESA 2.0? */
      return -1;

   _dpmi_reg.x.ax = 0x4F0A;                  /* retrieve pmode interface */
   _dpmi_reg.x.bx = 0;
   __dpmi_int(0x10, &_dpmi_reg);
   if (_dpmi_reg.h.ah)
      return -1;

   if (pm_info)
      _AL_FREE(pm_info);

   pm_info = _AL_MALLOC(_dpmi_reg.x.cx);         /* copy into our address space */
   LOCK_DATA(pm_info, _dpmi_reg.x.cx);
   dosmemget((_dpmi_reg.x.es*16)+_dpmi_reg.x.di, _dpmi_reg.x.cx, pm_info);

   _mmio_segment = 0;

   if (pm_info->IOPrivInfo) {                /* need memory mapped IO? */
      p = (unsigned short *)((char *)pm_info + pm_info->IOPrivInfo);
      while (*p != 0xFFFF)                   /* skip the port table */
	 p++;
      p++;
      if (*p != 0xFFFF) {                    /* get descriptor */
	 if (_create_physical_mapping(&mmio_linear, &_mmio_segment, *((unsigned long *)p), *(p+2)) != 0)
	    return -1;
      }
   }

   _pm_vesa_switcher = (void *)((char *)pm_info + pm_info->setWindow);
   _pm_vesa_scroller = (void *)((char *)pm_info + pm_info->setDisplayStart);
   _pm_vesa_palette  = (void *)((char *)pm_info + pm_info->setPalette);

   if (_mmio_segment) {
      *w1 = _vesa_pm_es_window_1;
      *w2 = _vesa_pm_es_window_2;
   }
   else {
      *w1 = _vesa_pm_window_1;
      *w2 = _vesa_pm_window_2;
   }

   return 0;
}



/* sort_out_vesa_windows:
 *  Checks the mode info block structure to determine which VESA windows
 *  should be used for reading and writing to video memory.
 */
static int sort_out_vesa_windows(BITMAP *b, void (*w1)(void), void (*w2)(void))
{
   if ((mode_info.WinAAttributes & 5) == 5)        /* write to window 1? */
      b->write_bank = w1;
   else if ((mode_info.WinBAttributes & 5) == 5)   /* write to window 2? */
      b->write_bank = w2;
   else
      return -1;

   if ((mode_info.WinAAttributes & 3) == 3)        /* read from window 1? */
      b->read_bank = w1;
   else if ((mode_info.WinBAttributes & 3) == 3)   /* read from window 2? */
      b->read_bank = w2;
   else
      return -1;

   /* are the windows at different places in memory? */
   _window_2_offset = (mode_info.WinBSegment - mode_info.WinASegment) * 16;

   /* is it safe to do screen->screen blits? */
   if (b->read_bank == b->write_bank)
      b->id |= BMP_ID_NOBLIT;

   return 0;
}



/* make_linear_bitmap:
 *  Creates a screen bitmap for a linear framebuffer mode, creating the
 *  required physical address mappings and allocating an LDT descriptor
 *  to access the memory.
 */
static BITMAP *make_linear_bitmap(GFX_DRIVER *driver, int width, int height, int color_depth, int bpl)
{
   unsigned long addr;
   int seg;
   BITMAP *b;

   #ifdef ALLEGRO_DJGPP

      /* use farptr access with djgpp */
      if (_create_physical_mapping(&lb_linear, &lb_segment, mode_info.PhysBasePtr, driver->vid_mem) != 0)
	 return NULL;

      addr = 0;
      seg = lb_segment;

   #else

      /* use nearptr access with Watcom */
      if (_create_linear_mapping(&lb_linear, mode_info.PhysBasePtr, driver->vid_mem) != 0)
	 return NULL;

      addr = lb_linear;
      seg = _default_ds();

   #endif

   b = _make_bitmap(width, height, addr, driver, color_depth, bpl);
   if (!b) {
      _remove_physical_mapping(&lb_linear, &lb_segment);
      return NULL;
   }

   driver->vid_phys_base = mode_info.PhysBasePtr;

   b->seg = seg;
   return b;
}



/* calc_crtc_timing:
 *  Calculates CRTC mode timings.
 */
static void calc_crtc_timing(CRTCInfoBlock *crtc, int xres, int yres, int xadjust, int yadjust)
{
   int HTotal, VTotal;
   int HDisp, VDisp;
   int HSS, VSS;
   int HSE, VSE;
   int HSWidth, VSWidth;
   int SS, SE;
   int doublescan = FALSE;

   if (yres < 400){
      doublescan = TRUE;
      yres *= 2;
   }

   HDisp = xres;
   HTotal = (int)(HDisp * 1.27) & ~0x7;
   HSWidth = (int)((HTotal - HDisp) / 5) & ~0x7;
   HSS = HDisp + 16;
   HSE = HSS + HSWidth;
   VDisp = yres;
   VTotal = VDisp * 1.07;
   VSWidth = (VTotal / 100) + 1;
   VSS = VDisp + ((int)(VTotal - VDisp) / 5) + 1;
   VSE = VSS + VSWidth;

   SS = HSS + xadjust;
   SE = HSE + xadjust;

   if (xadjust < 0) {
      if (SS < (HDisp + 8)) {
	 SS = HDisp + 8;
	 SE = SS + HSWidth;
      }
   }
   else {
      if ((HTotal - 24) < SE) {
	 SE = HTotal - 24;
	 SS = SE - HSWidth;
      }
   }

   HSS = SS;
   HSE = SE;

   SS = VSS + yadjust;
   SE = VSE + yadjust;

   if (yadjust < 0) {
      if (SS < (VDisp + 3)) {
	 SS = VDisp + 3;
	 SE = SS + VSWidth;
      }
   }
   else {
      if ((VTotal - 4) < SE) {
	 SE = VTotal - 4;
	 SS = SE - VSWidth;
      }
   }

   VSS = SS;
   VSE = SE;

   crtc->HorizontalTotal     = HTotal;
   crtc->HorizontalSyncStart = HSS;
   crtc->HorizontalSyncEnd   = HSE;
   crtc->VerticalTotal       = VTotal;
   crtc->VerticalSyncStart   = VSS;
   crtc->VerticalSyncEnd     = VSE;
   crtc->Flags               = HNEG | VNEG;

   if (doublescan)
      crtc->Flags |= DOUBLESCAN;
}



/* get_closest_pixel_clock:
 *  Uses VESA 3.0 function 0x4F0B to find the closest pixel clock to the
 *  requested value.
 */
static unsigned long get_closest_pixel_clock(int mode_no, unsigned long vclk)
{
   __dpmi_regs r;

   r.x.ax = 0x4F0B;
   r.h.bl = 0;
   r.d.ecx = vclk;
   r.x.dx = mode_no;

   __dpmi_int(0x10, &r);

   if (r.h.ah != 0)
      return 0;

   return r.d.ecx;
}



/* vesa_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it. Will use a linear framebuffer if one is available.
 */
static BITMAP *vesa_init(GFX_DRIVER *driver, int linear, int vbe_version, int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;
   int width, height;
   int vesa_mode;
   int bytes_per_scanline;
   void (*w1)(void) = _vesa_window_1;
   void (*w2)(void) = _vesa_window_2;
   int bpp = BYTES_PER_PIXEL(color_depth);
   int rs, gs, bs;

   vesa_mode = find_vesa_mode(w, h, color_depth, vbe_version);
   if (vesa_mode == 0)
      return NULL;

   LOCK_FUNCTION(_vesa_window_1);
   LOCK_FUNCTION(_vesa_window_2);
   LOCK_FUNCTION(_vesa_pm_window_1);
   LOCK_FUNCTION(_vesa_pm_window_2);
   LOCK_FUNCTION(_vesa_pm_es_window_1);
   LOCK_FUNCTION(_vesa_pm_es_window_2);
   LOCK_VARIABLE(_window_2_offset);
   LOCK_VARIABLE(_dpmi_reg);
   LOCK_VARIABLE(_pm_vesa_switcher);
   LOCK_VARIABLE(_mmio_segment);

   driver->vid_mem = vesa_info.TotalMemory << 16;

   if (vbe_version >= 3)
      linear = (mode_info.ModeAttributes & 0x80) ? TRUE : FALSE;

   if (linear) {
      /* linear framebuffer */
      if (mode_info.ModeAttributes & 0x80) { 
	 driver->linear = TRUE;
	 driver->bank_size = driver->bank_gran = 0;
      }
      else {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Linear framebuffer not available"));
	 return NULL;
      }
   }
   else {
      /* banked framebuffer */
      if (!(mode_info.ModeAttributes & 0x40)) { 
	 driver->linear = FALSE;
	 driver->bank_size = mode_info.WinSize * 1024;
	 driver->bank_gran = mode_info.WinGranularity * 1024;
      }
      else {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Banked framebuffer not available"));
	 return NULL;
      }
   }

   if (MAX(w, v_w) * MAX(h, v_h) * bpp > driver->vid_mem) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Insufficient video memory"));
      return NULL;
   }

   /* avoid bug in S3 Trio64 drivers (can't set SVGA without VGA first) */
   if (strstr(oem_string, "Trio64")) {
      _dpmi_reg.x.ax = 0x13; 
      __dpmi_int(0x10, &_dpmi_reg);
      evilness_flag = 1;
   }
   else
      evilness_flag = 0;

   /* set screen mode */
   _dpmi_reg.x.ax = 0x4F02; 
   _dpmi_reg.x.bx = vesa_mode;

   if (driver->linear)
      _dpmi_reg.x.bx |= 0x4000;

   if ((_refresh_rate_request > 0) && (vbe_version >= 3)) {
      /* VESA 3.0 stuff for controlling the refresh rate */
      CRTCInfoBlock crtc;
      unsigned long vclk;
      double f0;

      calc_crtc_timing(&crtc, w, h, 0, 0);

      vclk = (double)crtc.HorizontalTotal * crtc.VerticalTotal * _refresh_rate_request;
      vclk = get_closest_pixel_clock(vesa_mode, vclk);

      if (vclk != 0) {
	 f0 = (double)vclk / (crtc.HorizontalTotal * crtc.VerticalTotal);

	 _set_current_refresh_rate((int)(f0 + 0.5));

	 crtc.PixelClock  = vclk;
	 crtc.RefreshRate = _refresh_rate_request * 100;

	 dosmemput(&crtc, sizeof(CRTCInfoBlock), MASK_LINEAR(__tb));

	 _dpmi_reg.x.di = RM_OFFSET(__tb);
	 _dpmi_reg.x.es = RM_SEGMENT(__tb);
	 _dpmi_reg.x.bx |= 0x0800;
      }
   }

   __dpmi_int(0x10, &_dpmi_reg);

   if (_dpmi_reg.h.ah) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VESA function 0x4F02 failed"));
      return NULL;
   }

   if ((vbe_version >= 3) && (linear))
      bytes_per_scanline = mode_info.LinBytesPerScanLine;
   else
      bytes_per_scanline = mode_info.BytesPerScanLine;

   width = MAX(bytes_per_scanline, v_w*bpp);
   _sort_out_virtual_width(&width, driver);

   if (width <= bytes_per_scanline) {
      height = driver->vid_mem / width;
   }
   else { 
      /* increase logical width */
      _dpmi_reg.x.ax = 0x4F06;
      _dpmi_reg.x.bx = 0;
      _dpmi_reg.x.cx = width/bpp;
      __dpmi_int(0x10, &_dpmi_reg);

      if ((_dpmi_reg.h.ah) || (width > _dpmi_reg.x.bx)) {
	 /* Evil, evil, evil. I really wish I didn't have to do this, but 
	  * some crappy VESA implementations can't handle the set logical 
	  * width call, which Allegro depends on. This register write will 
	  * work on 99% of cards, if VESA lets me down.
	  */
	 if ((width != 1024) && (width != 2048)) {
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VESA function 0x4F06 failed"));
	    return NULL;
	 }

	 _dpmi_reg.x.ax = 0x4F02;
	 _dpmi_reg.x.bx = vesa_mode;

	 if (driver->linear)
	    _dpmi_reg.x.bx |= 0x4000;

	 __dpmi_int(0x10, &_dpmi_reg);

	 if (_dpmi_reg.h.ah) {
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VESA function 0x4F02 failed"));
	    return NULL;
	 }

	 _set_vga_virtual_width(bytes_per_scanline, width);

	 height = driver->vid_mem / width;
	 evilness_flag |= 2;
      }
      else {
	 /* Some VESA drivers don't report the available virtual height 
	  * properly, so we do a sanity check based on the total amount of 
	  * video memory.
	  */
	 width = _dpmi_reg.x.bx;
	 height = MIN(_dpmi_reg.x.dx, driver->vid_mem / width);
      }
   }

   if ((width/bpp < v_w) || (width/bpp < w) || (height < v_h) || (height < h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Virtual screen size too large"));
      return NULL;
   }

   if (vbe_version >= 2) {
      if (get_pmode_functions(&w1, &w2) != 0) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VESA protected mode interface not available"));
	 return NULL;
      }
   }

   if (driver->linear) {                        /* make linear bitmap? */
      b = make_linear_bitmap(driver, width/bpp, height, color_depth, width);
      if (!b) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't make linear screen bitmap"));
	 return NULL;
      }
   }
   else {                                       /* or make bank switcher */
      b = _make_bitmap(width/bpp, height, mode_info.WinASegment*16, driver, color_depth, width);
      if (!b)
	 return NULL;

      if (sort_out_vesa_windows(b, w1, w2) != 0) {
	 destroy_bitmap(b);
	 return NULL;
      }
   }

   #if (defined ALLEGRO_COLOR16) || (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

      if ((vbe_version >= 3) && (linear)) {
	 rs = mode_info.LinRedFieldPos; 
	 gs = mode_info.LinGreenFieldPos;
	 bs = mode_info.LinBlueFieldPos;
      }
      else {
	 rs = mode_info.RedMaskPos; 
	 gs = mode_info.GreenMaskPos;
	 bs = mode_info.BlueMaskPos;
      }

      switch (color_depth) {

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	       _rgb_r_shift_15 = rs; 
	       _rgb_g_shift_15 = gs;
	       _rgb_b_shift_15 = bs;
	       break;

	    case 16:
	       _rgb_r_shift_16 = rs; 
	       _rgb_g_shift_16 = gs;
	       _rgb_b_shift_16 = bs;
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       _rgb_r_shift_24 = rs; 
	       _rgb_g_shift_24 = gs;
	       _rgb_b_shift_24 = bs;
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       _rgb_r_shift_32 = rs; 
	       _rgb_g_shift_32 = gs;
	       _rgb_b_shift_32 = bs;
	       break;

	 #endif
      }

   #endif

   /* is triple buffering supported? */
   if (vbe_version >= 3) {
      if (mode_info.ModeAttributes & 0x400) {
	 driver->request_scroll = vesa_request_scroll;
	 driver->poll_scroll = vesa_poll_scroll;
      }
      else {
	 driver->request_scroll = NULL;
	 driver->poll_scroll = NULL;
      }
   }

   vesa_xscroll = vesa_yscroll = 0;

   driver->w = b->cr = w;
   driver->h = b->cb = h;

   setup_vesa_desc(driver, vbe_version, linear);

   return b;
}



/* vesa_1_init:
 *  Initialises a VESA 1.x screen mode.
 */
static BITMAP *vesa_1_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return vesa_init(&gfx_vesa_1, FALSE, 1, w, h, v_w, v_h, color_depth);
}



/* vesa_2b_init:
 *  Initialises a VESA 2.0 banked screen mode.
 */
static BITMAP *vesa_2b_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return vesa_init(&gfx_vesa_2b, FALSE, 2, w, h, v_w, v_h, color_depth);
}



/* vesa_2l_init:
 *  Initialises a VESA 2.0 linear framebuffer mode.
 */
static BITMAP *vesa_2l_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return vesa_init(&gfx_vesa_2l, TRUE, 2, w, h, v_w, v_h, color_depth);
}



/* vesa_3_init:
 *  Initialises a VESA 3.0 mode.
 */
static BITMAP *vesa_3_init(int w, int h, int v_w, int v_h, int color_depth)
{
   return vesa_init(&gfx_vesa_3, FALSE, 3, w, h, v_w, v_h, color_depth);
}



/* vesa_vsync:
 *  VBE 2.0 vsync routine, needed for cards that don't emulate the VGA 
 *  blanking registers. VBE doesn't provide a vsync function, but we 
 *  can emulate it by changing the display start with the vsync flag set.
 */
static void vesa_vsync(void)
{
   vesa_scroll(vesa_xscroll, vesa_yscroll);
}



/* vesa_scroll:
 *  Hardware scrolling routine.
 */
static int vesa_scroll(int x, int y)
{
   int ret, seg;
   long a;

   vesa_xscroll = x;
   vesa_yscroll = y;

   if (_pm_vesa_scroller) {            /* use protected mode interface? */
      seg = _mmio_segment ? _mmio_segment : _default_ds();

      a = ((x * BYTES_PER_PIXEL(screen->vtable->color_depth)) +
	   (y * ((unsigned long)screen->line[1] - (unsigned long)screen->line[0]))) / 4;

      #ifdef ALLEGRO_GCC

	 /* use gcc-style inline asm */
	 asm (
	    "  pushl %%ebp ; "
	    "  pushw %%es ; "
	    "  movw %w1, %%es ; "         /* set the IO segment */
	    "  call *%0 ; "               /* call the VESA function */
	    "  popw %%es ; "
	    "  popl %%ebp ; "

	 :                                /* no outputs */

	 : "S" (_pm_vesa_scroller),       /* function pointer in esi */
	   "a" (seg),                     /* IO segment in eax */
	   "b" (0x80),                    /* mode in ebx */
	   "c" (a&0xFFFF),                /* low word of address in ecx */
	   "d" (a>>16)                    /* high word of address in edx */

	 : "memory", "%edi", "%cc"        /* clobbers edi and flags */
	 );

      #elif defined ALLEGRO_WATCOM

	 /* use Watcom-style inline asm */
	 {
	    void _scroll(void *func, int seg, int mode, int addr1, int addr2);

	    #pragma aux _scroll =                  \
	       " push ebp "                        \
	       " push es "                         \
	       " mov es, ax "                      \
	       " call esi "                        \
	       " pop es "                          \
	       " pop ebp "                         \
						   \
	    parm [esi] [eax] [ebx] [ecx] [edx]     \
	    modify [edi];

	    _scroll(_pm_vesa_scroller, seg, 0x80, a&0xFFFF, a>>16);
	 }

      #else
	 #error unknown platform
      #endif

      ret = 0;
   }
   else {                              /* use a real mode interrupt call */
      _dpmi_reg.x.ax = 0x4F07;
      _dpmi_reg.x.bx = 0;
      _dpmi_reg.x.cx = x;
      _dpmi_reg.x.dx = y;

      __dpmi_int(0x10, &_dpmi_reg); 
      ret = _dpmi_reg.h.ah;

      _vsync_in();
   }

   return (ret ? -1 : 0); 
}



/* vesa_set_palette_range:
 *  Uses VESA function #9 (VBE 2.0 only) to set the palette.
 */
static void vesa_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   PALETTE tmp;
   int c, mode, seg;

   if (_pm_vesa_palette) {             /* use protected mode interface? */
      mode = (vsync) ? 0x80 : 0;
      seg = (_mmio_segment) ? _mmio_segment : _default_ds();

      /* swap the palette into the funny order VESA uses */
      for (c=from; c<=to; c++) {
	 tmp[c].r = p[c].b;
	 tmp[c].g = p[c].g;
	 tmp[c].b = p[c].r;
      }

      #ifdef ALLEGRO_GCC

	 /* use gcc-style inline asm */
	 asm (
	    "  pushl %%ebp ; "
	    "  pushw %%ds ; "
	    "  movw %w1, %%ds ; "         /* set the IO segment */
	    "  call *%0 ; "               /* call the VESA function */
	    "  popw %%ds ; "
	    "  popl %%ebp ; "

	 :                                /* no outputs */

	 : "S" (_pm_vesa_palette),        /* function pointer in esi */
	   "a" (seg),                     /* IO segment in eax */
	   "b" (mode),                    /* mode in ebx */
	   "c" (to-from+1),               /* how many colors in ecx */
	   "d" (from),                    /* first color in edx */
	   "D" (tmp+from)                 /* palette data pointer in edi */

	 : "memory", "%cc"                /* clobbers flags */
	 );

      #elif defined ALLEGRO_WATCOM

	 /* use Watcom-style inline asm */
	 {
	    void _pal(void *func, int seg, int mode, int count, int from, void *data);

	    #pragma aux _pal =                           \
	       " push ebp "                              \
	       " push ds "                               \
	       " mov ds, ax "                            \
	       " call esi "                              \
	       " pop ds "                                \
	       " pop ebp "                               \
							 \
	    parm [esi] [eax] [ebx] [ecx] [edx] [edi];

	    _pal(_pm_vesa_palette, seg, mode, to-from+1, from, tmp+from);
	 }

      #else
	 #error unknown platform
      #endif
   }
   else 
      _vga_set_palette_range(p, from, to, vsync);
}



/* vesa_request_scroll:
 *  VBE 3.0 triple buffering initiate routine.
 */
static int vesa_request_scroll(int x, int y)
{
   long a = (x * BYTES_PER_PIXEL(screen->vtable->color_depth)) +
	    (y * ((unsigned long)screen->line[1] - (unsigned long)screen->line[0]));

   vesa_xscroll = x;
   vesa_yscroll = y;

   _dpmi_reg.x.ax = 0x4F07;
   _dpmi_reg.x.bx = 2;
   _dpmi_reg.d.ecx = a;

   __dpmi_int(0x10, &_dpmi_reg);

   return (_dpmi_reg.h.ah ? -1 : 0);
}



/* vesa_poll_scroll:
 *  VBE 3.0 triple buffering test routine.
 */
static int vesa_poll_scroll(void)
{
   _dpmi_reg.x.ax = 0x4F07;
   _dpmi_reg.x.bx = 4;

   __dpmi_int(0x10, &_dpmi_reg);

   if ((_dpmi_reg.h.ah) || (_dpmi_reg.x.cx))
      return FALSE;

   return TRUE;
}



/* vesa_exit:
 *  Shuts down the VESA driver.
 */
static void vesa_exit(BITMAP *b)
{
   if (b->vtable->color_depth > 8) {
      /* workaround for some buggy VESA drivers that fail to clean up 
       * properly after using truecolor modes...
       */
      _dpmi_reg.x.ax = 0x4F02;
      _dpmi_reg.x.bx = 0x101;
      __dpmi_int(0x10, &_dpmi_reg);
   }

   _remove_physical_mapping(&lb_linear, &lb_segment);
   _remove_physical_mapping(&mmio_linear, &_mmio_segment);

   if (pm_info) {
      _AL_FREE(pm_info);
      pm_info = NULL;
   }

   _pm_vesa_switcher = _pm_vesa_scroller = _pm_vesa_palette = NULL;
}


