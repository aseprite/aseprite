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
 *      Video driver using the VBE/AF 2.0 hardware accelerator API,
 *      plus extensions added by the FreeBE/AF project.
 *
 *      This driver provides accelerated support for:
 *
 *       - scanline fills (solid, XOR, and patterned)
 *       - area fills (solid, XOR, and patterned)
 *       - line drawing (solid and XOR)
 *       - triangle drawing (solid and XOR)
 *       - monochrome character expansion
 *       - blitting within the video memory
 *       - masked blitting within the video memory
 *       - blits from system memory
 *       - masked blits from system memory
 *       - hardware mouse cursors
 *
 *      The FreeBE/AF project (http://www.talula.demon.co.uk/freebe/)
 *      has defined a number of API extensions, of which this driver
 *      implements:
 *
 *       FAFEXT_INIT  - extension init and new driver relocation mechanism
 *       FAFEXT_HWPTR - farptr access to video memory (no more Fat DS hack)
 *       FAFEXT_LIBC  - exports a bunch of callback functions from libc
 *       FAFEXT_PMODE - exports the SciTech pmode API callback functions
 *
 *      On the subject of VBE/AF in general:
 *
 *      Kendall Bennett and Tom Ryan of SciTech software deserve 
 *      significant thanks for coming up with such a magnificent API and 
 *      then helping me write this driver, after the VESA people (may they 
 *      be smitten with plagues of locusts and then develop subtle memory 
 *      leaks in their code :-) decided to charge stupid sums of money for 
 *      copies of the /AF specification. Unfortunately, SciTech have 
 *      subsequently abandoned VBE/AF in favour of a closed, proprietary 
 *      system called Nucleus. Ah well, I'm sure that this makes sense 
 *      for them in commercial terms, even if it is something of a 
 *      disappointment for us idealistic hacker types...
 *
 *      The problem with Nucleus is that the spec is only available under
 *      NDA, so I cannot support it directly in Allegro. The plan,
 *      therefore, is to write a Nucleus to VBE/AF wrapper driver.
 *      This requires a few callback routines that stock VBE/AF doesn't
 *      provide, hence the Nucleus-specific extensions in this file. It 
 *      doesn't provide direct Nucleus support, but hopefully will enable 
 *      someone (whether us or SciTech) to add that support in the future 
 *      via an external driver binary.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"

#if (defined ALLEGRO_DOS) || (defined ALLEGRO_LINUX_VBEAF)

#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

#if (defined ALLEGRO_DJGPP) && (!defined SCAN_DEPEND)
   #include <crt0.h>
   #include <sys/nearptr.h>
   #include <sys/exceptn.h>
#endif



/* main driver routines */
static BITMAP *vbeaf_init(int w, int h, int v_w, int v_h, int color_depth);
static void vbeaf_exit(BITMAP *b);
static void vbeaf_save(void);
static void vbeaf_restore(void);
static GFX_MODE_LIST *vbeaf_fetch_mode_list(void);
static void vbeaf_vsync(void);
static int vbeaf_scroll(int x, int y);
static void vbeaf_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync);
static int vbeaf_request_scroll(int x, int y);
static int vbeaf_poll_scroll(void);
static int vbeaf_set_mouse_sprite(BITMAP *sprite, int xfocus, int yfocus);
static int vbeaf_show_mouse(BITMAP *bmp, int x, int y);
static void vbeaf_hide_mouse(void);
static void vbeaf_move_mouse(int x, int y);
static void vbeaf_drawing_mode(void);
static int vbeaf_locate_driver(void);
static int vbeaf_lowlevel_init(void);


/* accelerated drawing functions */
static void vbeaf_hline(BITMAP *bmp, int x1, int y, int x2, int color);
static void vbeaf_vline_a(BITMAP *bmp, int x, int y1, int y2, int color);
static void vbeaf_vline_b(BITMAP *bmp, int x, int y1, int y2, int color);
static void vbeaf_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
static void vbeaf_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
static void vbeaf_triangle(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color);
static void vbeaf_draw_glyph(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg);
static void vbeaf_draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y);
static void vbeaf_blit_from_memory(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void vbeaf_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void vbeaf_masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void vbeaf_clear_to_color(BITMAP *bitmap, int color);


#ifdef ALLEGRO_DOS
   static void vbeaf_move_mouse_end(void);
   static void vbeaf_draw_sprite_end(void);
   static void vbeaf_blit_from_memory_end(void);
#endif


/* original software drawing functions */
static void (*orig_vline)(BITMAP *bmp, int x, int y1, int y2, int color);
static void (*orig_hline)(BITMAP *bmp, int x1, int y, int x2, int color);
static void (*orig_line)(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
static void (*orig_rectfill)(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
static void (*orig_draw_glyph)(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg);
static void (*orig_draw_sprite)(BITMAP *bmp, BITMAP *sprite, int x, int y);
static void (*orig_masked_blit)(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);


static char vbeaf_desc[256] = EMPTY_STRING;



/* the video driver structure */
GFX_DRIVER gfx_vbeaf = 
{
   GFX_VBEAF,
   empty_string,
   empty_string,
   "VBE/AF",
   vbeaf_init,
   vbeaf_exit,
   vbeaf_scroll,
   vbeaf_vsync,
   vbeaf_set_palette_range,
   vbeaf_request_scroll,
   vbeaf_poll_scroll,
   NULL,                         /* enable triple buffer */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor (yet) */
   NULL,                         /* no drawing mode hook */
   vbeaf_save,
   vbeaf_restore,
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   vbeaf_fetch_mode_list,        /* fetch mode hook */
   0, 0, FALSE, 0, 0, 0, 0, FALSE
};



typedef struct AF_FIX_POINT      /* fixed point coordinate pair */
{
   fixed x;
   fixed y;
} AF_FIX_POINT;



typedef struct AF_TRAP           /* trapezium information block */
{
   long y;
   long count;
   fixed x1;
   fixed x2;
   fixed slope1;
   fixed slope2;
} AF_TRAP;



typedef struct AF_CURSOR         /* hardware cursor description */
{
   unsigned long xorMask[32];
   unsigned long andMask[32];
   unsigned long hotx;
   unsigned long hoty;
} AF_CURSOR;



typedef struct AF_PALETTE        /* color value (not in Allegro order) */
{
   unsigned char blue;
   unsigned char green;
   unsigned char red;
   unsigned char alpha;
} AF_PALETTE;



#ifdef ALLEGRO_GCC
   #define __PACKED__   __attribute__ ((packed))
#else
   #define __PACKED__
#endif

#ifdef ALLEGRO_WATCOM
   #pragma pack (1)
#endif



typedef struct AF_MODE_INFO      /* mode information structure */
{
   unsigned short Attributes              __PACKED__;
   unsigned short XResolution             __PACKED__;
   unsigned short YResolution             __PACKED__;
   unsigned short BytesPerScanLine        __PACKED__;
   unsigned short BitsPerPixel            __PACKED__;
   unsigned short MaxBuffers              __PACKED__;
   unsigned char  RedMaskSize;
   unsigned char  RedFieldPosition;
   unsigned char  GreenMaskSize;
   unsigned char  GreenFieldPosition;
   unsigned char  BlueMaskSize;
   unsigned char  BlueFieldPosition;
   unsigned char  RsvdMaskSize;
   unsigned char  RsvdFieldPosition;
   unsigned short MaxBytesPerScanLine     __PACKED__;
   unsigned short MaxScanLineWidth        __PACKED__;

   /* VBE/AF 2.0 extensions */
   unsigned short LinBytesPerScanLine     __PACKED__;
   unsigned char  BnkMaxBuffers;
   unsigned char  LinMaxBuffers;
   unsigned char  LinRedMaskSize;
   unsigned char  LinRedFieldPosition;
   unsigned char  LinGreenMaskSize;
   unsigned char  LinGreenFieldPosition;
   unsigned char  LinBlueMaskSize;
   unsigned char  LinBlueFieldPosition;
   unsigned char  LinRsvdMaskSize;
   unsigned char  LinRsvdFieldPosition;
   unsigned long  MaxPixelClock           __PACKED__;
   unsigned long  VideoCapabilities       __PACKED__;
   unsigned short VideoMinXScale          __PACKED__;
   unsigned short VideoMinYScale          __PACKED__;
   unsigned short VideoMaxXScale          __PACKED__;
   unsigned short VideoMaxYScale          __PACKED__;

   unsigned char  reserved[76];

} AF_MODE_INFO;



#define DC  struct AF_DRIVER *dc



typedef struct AF_DRIVER         /* VBE/AF driver structure */
{
   /* driver header */
   char           Signature[12];
   unsigned long  Version                 __PACKED__;
   unsigned long  DriverRev               __PACKED__;
   char           OemVendorName[80];
   char           OemCopyright[80];
   unsigned short *AvailableModes         __PACKED__;
   unsigned long  TotalMemory             __PACKED__;
   unsigned long  Attributes              __PACKED__;
   unsigned long  BankSize                __PACKED__;
   unsigned long  BankedBasePtr           __PACKED__;
   unsigned long  LinearSize              __PACKED__;
   unsigned long  LinearBasePtr           __PACKED__;
   unsigned long  LinearGranularity       __PACKED__;
   unsigned short *IOPortsTable           __PACKED__;
   unsigned long  IOMemoryBase[4]         __PACKED__;
   unsigned long  IOMemoryLen[4]          __PACKED__;
   unsigned long  LinearStridePad         __PACKED__;
   unsigned short PCIVendorID             __PACKED__;
   unsigned short PCIDeviceID             __PACKED__;
   unsigned short PCISubSysVendorID       __PACKED__;
   unsigned short PCISubSysID             __PACKED__;
   unsigned long  Checksum                __PACKED__;
   unsigned long  res2[6]                 __PACKED__;

   /* near pointers mapped by the application */
   void           *IOMemMaps[4]           __PACKED__;
   void           *BankedMem              __PACKED__;
   void           *LinearMem              __PACKED__;
   unsigned long  res3[5]                 __PACKED__;

   /* driver state variables */
   unsigned long  BufferEndX              __PACKED__;
   unsigned long  BufferEndY              __PACKED__;
   unsigned long  OriginOffset            __PACKED__;
   unsigned long  OffscreenOffset         __PACKED__;
   unsigned long  OffscreenStartY         __PACKED__;
   unsigned long  OffscreenEndY           __PACKED__;
   unsigned long  res4[10]                __PACKED__;

   /* relocatable 32 bit bank switch routine, for Windows (ugh!) */
   unsigned long  SetBank32Len            __PACKED__;
   void           *SetBank32              __PACKED__;

   /* callback functions provided by the application */
   void           *Int86                  __PACKED__;
   void           *CallRealMode           __PACKED__;

   /* main driver setup routine */
   void           *InitDriver             __PACKED__;

   /* VBE/AF 1.0 asm interface (obsolete and not supported by Allegro) */
   void           *af10Funcs[40]          __PACKED__;

   /* VBE/AF 2.0 extensions */
   void           *PlugAndPlayInit        __PACKED__;

   /* FreeBE/AF extension query function */
   void           *(*OemExt)(DC, unsigned long id);

   /* extension hook for implementing additional VESA interfaces */
   void           *SupplementalExt        __PACKED__;

   /* device driver functions */
   long  (*GetVideoModeInfo)(DC, short mode, AF_MODE_INFO *modeInfo);
   long  (*SetVideoMode)(DC, short mode, long virtualX, long virtualY, long *bytesPerLine, int numBuffers, void *crtc);
   void  (*RestoreTextMode)(DC);
   long  (*GetClosestPixelClock)(DC, short mode, unsigned long pixelClock);
   void  (*SaveRestoreState)(DC, int subfunc, void *saveBuf);
   void  (*SetDisplayStart)(DC, long x, long y, long waitVRT);
   void  (*SetActiveBuffer)(DC, long index);
   void  (*SetVisibleBuffer)(DC, long index, long waitVRT);
   int   (*GetDisplayStartStatus)(DC);
   void  (*EnableStereoMode)(DC, int enable);
   void  (*SetPaletteData)(DC, AF_PALETTE *pal, long num, long index, long waitVRT);
   void  (*SetGammaCorrectData)(DC, AF_PALETTE *pal, long num, long index);
   void  (*SetBank)(DC, long bank);

   /* hardware cursor functions */
   void  (*SetCursor)(DC, AF_CURSOR *cursor);
   void  (*SetCursorPos)(DC, long x, long y);
   void  (*SetCursorColor)(DC, unsigned char red, unsigned char green, unsigned char blue);
   void  (*ShowCursor)(DC, long visible);

   /* 2D rendering functions */
   void  (*WaitTillIdle)(DC);
   void  (*EnableDirectAccess)(DC);
   void  (*DisableDirectAccess)(DC);
   void  (*SetMix)(DC, long foreMix, long backMix);
   void  (*Set8x8MonoPattern)(DC, unsigned char *pattern);
   void  (*Set8x8ColorPattern)(DC, int index, unsigned long *pattern);
   void  (*Use8x8ColorPattern)(DC, int index);
   void  (*SetLineStipple)(DC, unsigned short stipple);
   void  (*SetLineStippleCount)(DC, unsigned long count);
   void  (*SetClipRect)(DC, long minx, long miny, long maxx, long maxy);
   void  (*DrawScan)(DC, long color, long y, long x1, long x2);
   void  (*DrawPattScan)(DC, long foreColor, long backColor, long y, long x1, long x2);
   void  (*DrawColorPattScan)(DC, long y, long x1, long x2);
   void  (*DrawScanList)(DC, unsigned long color, long y, long length, short *scans);
   void  (*DrawPattScanList)(DC, unsigned long foreColor, unsigned long backColor, long y, long length, short *scans);
   void  (*DrawColorPattScanList)(DC, long y, long length, short *scans);
   void  (*DrawRect)(DC, unsigned long color, long left, long top, long width, long height);
   void  (*DrawPattRect)(DC, unsigned long foreColor, unsigned long backColor, long left, long top, long width, long height);
   void  (*DrawColorPattRect)(DC, long left, long top, long width, long height);
   void  (*DrawLine)(DC, unsigned long color, fixed x1, fixed y1, fixed x2, fixed y2);
   void  (*DrawStippleLine)(DC, unsigned long foreColor, unsigned long backColor, fixed x1, fixed y1, fixed x2, fixed y2);
   void  (*DrawTrap)(DC, unsigned long color, AF_TRAP *trap);
   void  (*DrawTri)(DC, unsigned long color, AF_FIX_POINT *v1, AF_FIX_POINT *v2, AF_FIX_POINT *v3, fixed xOffset, fixed yOffset);
   void  (*DrawQuad)(DC, unsigned long color, AF_FIX_POINT *v1, AF_FIX_POINT *v2, AF_FIX_POINT *v3, AF_FIX_POINT *v4, fixed xOffset, fixed yOffset);
   void  (*PutMonoImage)(DC, long foreColor, long backColor, long dstX, long dstY, long byteWidth, long srcX, long srcY, long width, long height, unsigned char *image);
   void  (*PutMonoImageLin)(DC, long foreColor, long backColor, long dstX, long dstY, long byteWidth, long srcX, long srcY, long width, long height, long imageOfs);
   void  (*PutMonoImageBM)(DC, long foreColor, long backColor, long dstX, long dstY, long byteWidth, long srcX, long srcY, long width, long height, long imagePhysAddr);
   void  (*BitBlt)(DC, long left, long top, long width, long height, long dstLeft, long dstTop, long op);
   void  (*BitBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);
   void  (*BitBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);
   void  (*BitBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);
   void  (*SrcTransBlt)(DC, long left, long top, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*SrcTransBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*SrcTransBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*SrcTransBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBlt)(DC, long left, long top, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*StretchBlt)(DC, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*StretchBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*StretchBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*StretchBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*SrcTransStretchBlt)(DC, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*SrcTransStretchBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*SrcTransStretchBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*SrcTransStretchBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBlt)(DC, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);

   /* hardware video functions */
   void  (*SetVideoInput)(DC, long width, long height, long format);
   void *(*SetVideoOutput)(DC, long left, long top, long width, long height);
   void  (*StartVideoFrame)(DC);
   void  (*EndVideoFrame)(DC);

} AF_DRIVER;



#undef DC



/* FreeBE/AF API extensions use 32 bit magic numbers */
#define FAF_ID(a,b,c,d)    ((a<<24) | (b<<16) | (c<<8) | d)



/* ID code and magic return value for initialising the extensions */
#define FAFEXT_INIT     FAF_ID('I','N','I','T')
#define FAFEXT_MAGIC    FAF_ID('E','X', 0,  0)



/* function exporting extensions (needed for Nucleus compatibility) */
#define FAFEXT_LIBC     FAF_ID('L','I','B','C')
#define FAFEXT_PMODE    FAF_ID('P','M','O','D')



/* extension providing a hardware-specific way to access video memory */
#define FAFEXT_HWPTR    FAF_ID('H','P','T','R')


typedef struct FAF_HWPTR
{
   int sel;
   unsigned long offset;
} FAF_HWPTR;


typedef struct FAF_HWPTR_DATA
{
   FAF_HWPTR IOMemMaps[4];
   FAF_HWPTR BankedMem;
   FAF_HWPTR LinearMem;
} FAF_HWPTR_DATA;



#ifdef ALLEGRO_LINUX

   #include <sys/stat.h>
   #include <sys/mman.h>

   /* quick and dirty Linux emulation of the DOS memory mapping routines */
   #define _create_linear_mapping(_addr, _base, _len)       \
   ({                                                       \
      (_addr)->base = _base;                                \
      (_addr)->size = _len;                                 \
      (_addr)->perms = PROT_READ | PROT_WRITE;              \
      __al_linux_map_memory(_addr);                         \
   })

   #define _remove_linear_mapping(_addr)                    \
   {                                                        \
      if ((_addr)->data)                                    \
	 __al_linux_unmap_memory(_addr);                    \
   }

   #define _create_selector(sel, addr, len)  -1

   #define _remove_selector(sel)

   #define MMAP      struct MAPPED_MEMORY
   #define NOMM      { 0, 0, 0, 0 }
   #define MVAL(a)   a.data

#else

   /* DOS version */
   #define MMAP      unsigned long
   #define NOMM      0
   #define MVAL(a)   (void *)((a)-__djgpp_base_address)

#endif



static AF_DRIVER *af_driver = NULL;    /* the VBE/AF driver */

void *_accel_driver = NULL;            /* externally visible driver pointer */

int _accel_active = FALSE;             /* is the accelerator busy? */

void *_accel_set_bank;                 /* shortcuts to driver functions */
void *_accel_idle;

static int in_af_mode = FALSE;         /* true if VBE/AF is in use */

static int faf_id = FALSE;             /* true if FreeBE/AF detected */
static int faf_ext = 0;                /* FreeBE/AF extensions version */

static FAF_HWPTR_DATA *faf_farptr;     /* FreeBE/AF farptr data */

static int vbeaf_xscroll = 0;          /* current display start address */
static int vbeaf_yscroll = 0;

static int vbeaf_fg_mix = 0;           /* current hardware mix mode */
static int vbeaf_bg_mix = 0;

static int vbeaf_cur_c1 = -1;          /* hardware cursor colors */
static int vbeaf_cur_c2 = -1;

static BITMAP *vbeaf_cur_bmp = NULL;   /* bitmap showing the cursor */

static int vbeaf_cur_x;                /* cursor position */
static int vbeaf_cur_y;
static int vbeaf_cur_on;

static BITMAP *vbeaf_pattern = NULL;   /* currently loaded pattern bitmap */

static MMAP af_memmap[4] = { NOMM, NOMM, NOMM, NOMM };
static MMAP af_banked_mem = NOMM;
static MMAP af_linear_mem = NOMM;

#ifdef ALLEGRO_DOS
   static int vbeaf_nearptr = FALSE;   /* did we enable nearptrs ourselves? */
#endif

#ifdef ALLEGRO_LINUX
   int _vbeaf_selector = 0;            /* Linux version of __djgpp_ds_alias */
#endif

static int saved_mode;                 /* state info for console switches */
static int saved_vw;
static int saved_vh;
static int saved_wret;

extern void _af_int86(void), _af_call_rm(void), _af_wrapper(void), _af_wrapper_end(void);



#ifdef ALLEGRO_DJGPP

   /* djgpp wrapper to disable exceptions during critical operations */
   #define SAFE_CALL(FUNC)                \
   {                                      \
      int _ds, _es, _ss;                  \
					  \
      asm volatile (                      \
	 " movw %%ds, %w0 ; "             \
	 " movw %%es, %w1 ; "             \
	 " movw %%ss, %w2 ; "             \
	 " movw %w3, %%ds ; "             \
	 " movw %w3, %%es ; "             \
	 " movw %w3, %%ss ; "             \
					  \
      : "=&q" (_ds),                      \
	"=&q" (_es),                      \
	"=&q" (_ss)                       \
					  \
      : "q" (__djgpp_ds_alias)            \
      );                                  \
					  \
      FUNC                                \
					  \
      asm volatile (                      \
	 "movw %w0, %%ds ; "              \
	 "movw %w1, %%es ; "              \
	 "movw %w2, %%ss ; "              \
      :                                   \
      : "q" (_ds),                        \
	"q" (_es),                        \
	"q" (_ss)                         \
      );                                  \
   }

   #define SAFISH_CALL(FUNC)  FUNC

#elif defined ALLEGRO_LINUX

   /* Linux wrapper to disable console switches during critical operations */
   #define SAFE_CALL(FUNC)                \
   {                                      \
      __al_linux_switching_blocked++;     \
					  \
      FUNC                                \
					  \
      __al_linux_release_bitmap(NULL);    \
   }

   #define SAFISH_CALL(FUNC)  SAFE_CALL(FUNC)

#else

   #define SAFE_CALL(FUNC)    FUNC
   #define SAFISH_CALL(FUNC)  FUNC

#endif



/* bswap:
 *  Toggles the endianess of a 32 bit integer.
 */
static unsigned long bswap(unsigned long n)
{
   unsigned long a = n & 0xFF;
   unsigned long b = (n>>8) & 0xFF;
   unsigned long c = (n>>16) & 0xFF;
   unsigned long d = (n>>24) & 0xFF;

   return (a<<24) | (b<<16) | (c<<8) | d;
}



/* call_vbeaf_asm:
 *  Calls a VBE/AF function using the version 1.0 style asm interface.
 */
static int call_vbeaf_asm(void *proc)
{
   int ret;

   proc = (void *)((long)af_driver + (long)proc);

   #ifdef ALLEGRO_GCC

      /* use gcc-style inline asm */
      asm (
	 " pushl %%ebx ; "
	 " movl %%ecx, %%ebx ; "
	 " call *%%edx ; "
	 " popl %%ebx ; "

      : "=&a" (ret)                       /* return value in eax */

      : "c" (af_driver),                  /* VBE/AF driver in ds:ebx */
	"d" (proc)                        /* function ptr in edx */

      : "memory"                          /* assume everything is clobbered */
      );

   #elif defined ALLEGRO_WATCOM

      /* use Watcom-style inline asm */
      {
	 int _af(void *func, AF_DRIVER *driver);

	 #pragma aux _af =                \
	    " call esi "                  \
					  \
	 parm [esi] [ebx]                 \
	 modify [ecx edx edi]             \
	 value [eax];

	 ret = _af(proc, af_driver);
      }

   #else

      /* don't know what to do on this compiler */
      ret = -1;

   #endif

   return ret;
}



/* vbeaf_no_wait:
 *  Dummy wait-till-idle routine for non-accelerated drivers.
 */
static void vbeaf_no_wait(void)
{
}

END_OF_STATIC_FUNCTION(vbeaf_no_wait);



/* load_vbeaf_driver:
 *  Tries to load the specified VBE/AF driver file, returning TRUE on 
 *  success. Allocates memory and reads the driver into it.
 */
static int load_vbeaf_driver(AL_CONST char *filename)
{
   long size;
   PACKFILE *f;

   #ifdef ALLEGRO_LINUX

      /* on Linux. be paranoid and insist that vbeaf.drv belongs to root */
      char tmp[128];
      struct stat s;

      if (stat(uconvert_toascii(filename, tmp), &s) != 0)
	 return 0;

      if ((s.st_uid != 0) || (s.st_mode & (S_IWGRP | S_IWOTH))) {
	 uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s must only be writeable by root"), filename);
	 return -1;
      }

      size = s.st_size;
      if (size <= 0)
	 return 0;

   #else

      /* simple version for other platforms */
      size = file_size_ex(filename);
      if (size <= 0)
	 return 0;

   #endif

   f = pack_fopen(filename, F_READ);
   if (!f)
      return 0;

   af_driver = _accel_driver = _AL_MALLOC(size);

   if (pack_fread(af_driver, size, f) != size) {
      _AL_FREE(af_driver);
      af_driver = _accel_driver = NULL;
      return 0;
   }

   pack_fclose(f);

   LOCK_DATA(af_driver, size);

   return 1;
}



/* initialise_freebeaf_extensions:
 *  Prepares the FreeBE/AF extension functions.
 */
static void initialise_freebeaf_extensions(void)
{
   typedef unsigned long (*EXT_INIT_FUNC)(AF_DRIVER *af, unsigned long id);
   EXT_INIT_FUNC ext_init;
   unsigned long magic;
   int v1, v2;

   #ifdef ALLEGRO_DOS
      void *ptr;
   #endif

   /* safety check */
   if (!af_driver->OemExt) {
      faf_ext = 0;
      return;
   }

   /* call the extension init function */
   ext_init = (EXT_INIT_FUNC)((long)af_driver + (long)af_driver->OemExt);

   magic = ext_init(af_driver, FAFEXT_INIT);

   /* check that it returned a nice magic number */
   v1 = (magic>>8)&0xFF;
   v2 = magic&0xFF;

   if (((magic&0xFFFF0000) != FAFEXT_MAGIC) || (!uisdigit(v1)) || (!uisdigit(v2))) {
      faf_ext = 0;
      return;
   }

   faf_ext = (v1-'0')*10 + (v2-'0');

   /* export libc and pmode functions if the driver wants them */
   #ifdef ALLEGRO_DOS

      ptr = af_driver->OemExt(af_driver, FAFEXT_LIBC);
      if (ptr)
	 _fill_vbeaf_libc_exports(ptr);

      ptr = af_driver->OemExt(af_driver, FAFEXT_PMODE);
      if (ptr)
	 _fill_vbeaf_pmode_exports(ptr);

   #endif
}



/* initialise_vbeaf_driver:
 *  Sets up the DPMI memory mappings required by the VBE/AF driver, 
 *  returning zero on success.
 */
static int initialise_vbeaf_driver(void)
{
   int c;

   #ifdef ALLEGRO_DJGPP

      /* query driver for the FreeBE/AF farptr extension */
      if (faf_ext > 0)
	 faf_farptr = af_driver->OemExt(af_driver, FAFEXT_HWPTR);
      else
	 faf_farptr = NULL;

   #else

      /* don't use farptr on any other platforms */ 
      faf_farptr = NULL;

   #endif

   #ifdef ALLEGRO_DOS

      if (faf_farptr) {
	 /* use farptr access */
	 for (c=0; c<4; c++) {
	    faf_farptr->IOMemMaps[c].sel = 0; 
	    faf_farptr->IOMemMaps[c].offset = 0; 
	 }

	 faf_farptr->BankedMem.sel = 0; 
	 faf_farptr->BankedMem.offset = 0; 

	 faf_farptr->LinearMem.sel = 0; 
	 faf_farptr->LinearMem.offset = 0; 

	 vbeaf_nearptr = FALSE;
      }
      else {
	 /* enable nearptr access */
	 if (_crt0_startup_flags & _CRT0_FLAG_NEARPTR) {
	    vbeaf_nearptr = FALSE;
	 }
	 else {
	    if (__djgpp_nearptr_enable() == 0)
	       return -2;

	    vbeaf_nearptr = TRUE;
	 }
      }

   #endif

   /* create mapping for MMIO ports */
   for (c=0; c<4; c++) {
      if (af_driver->IOMemoryBase[c]) {
	 if (_create_linear_mapping(af_memmap+c, af_driver->IOMemoryBase[c], 
				    af_driver->IOMemoryLen[c]) != 0)
	    return -1;

	 if (faf_farptr) {
	    /* farptr IO mapping */
	    if (_create_selector(&faf_farptr->IOMemMaps[c].sel, af_memmap[c], 
				 af_driver->IOMemoryLen[c]) != 0) {
	       _remove_linear_mapping(af_memmap+c);
	       return -1;
	    }
	    faf_farptr->IOMemMaps[c].offset = 0;
	    af_driver->IOMemMaps[c] = NULL;
	 }
	 else {
	    /* nearptr IO mapping */
	    af_driver->IOMemMaps[c] = MVAL(af_memmap[c]);
	 }
      }
   }

   /* create mapping for banked video RAM */
   if (af_driver->BankedBasePtr) {
      if (_create_linear_mapping(&af_banked_mem, af_driver->BankedBasePtr, 0x10000) != 0)
	 return -1;

      if (faf_farptr) {
	 /* farptr banked vram mapping */
	 if (_create_selector(&faf_farptr->BankedMem.sel, af_banked_mem, 0x10000) != 0) {
	    _remove_linear_mapping(&af_banked_mem);
	    return -1;
	 }
	 faf_farptr->BankedMem.offset = 0;
	 af_driver->BankedMem = NULL;
      }
      else {
	 /* nearptr banked vram mapping */
	 af_driver->BankedMem = MVAL(af_banked_mem);
      }
   }

   /* create mapping for linear video RAM */
   if (af_driver->LinearBasePtr) {
      if (_create_linear_mapping(&af_linear_mem, af_driver->LinearBasePtr, af_driver->LinearSize*1024) != 0)
	 return -1;

      if (faf_farptr) {
	 /* farptr linear vram mapping */
	 if (_create_selector(&faf_farptr->LinearMem.sel, af_linear_mem, af_driver->LinearSize*1024) != 0) {
	    _remove_linear_mapping(&af_linear_mem);
	    return -1;
	 }
	 faf_farptr->LinearMem.offset = 0;
	 af_driver->LinearMem = NULL;
      }
      else {
	 /* nearptr linear vram mapping */
	 af_driver->LinearMem  = MVAL(af_linear_mem);
      }
   }

   /* callback functions: why are these needed? ugly, IMHO */
   af_driver->Int86 = _af_int86;
   af_driver->CallRealMode = _af_call_rm;

   return 0;
}



/* find_vbeaf_mode:
 *  Tries to find a VBE/AF mode number for the specified screen size.
 */
static int find_vbeaf_mode(int w, int h, int v_w, int v_h, int color_depth, AF_MODE_INFO *mode_info)
{
   unsigned short *mode;

   /* search the list of modes */
   for (mode = af_driver->AvailableModes; *mode != 0xFFFF; mode++) {

      /* retrieve the mode information block */
      if (af_driver->GetVideoModeInfo(af_driver, *mode, mode_info) == 0) {

	 /* check size and color depth */
	 if ((mode_info->XResolution == w) &&
	     (mode_info->YResolution == h) &&
	     (mode_info->BitsPerPixel == color_depth) &&
	     (mode_info->MaxScanLineWidth >= v_w)) {

	    /* make sure the mode supports scrolling */
	    if ((v_w > w) || (v_h > h)) {
	       if (!(mode_info->Attributes & 2)) {
		  ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Hardware scrolling not supported"));
		  return 0;
	       }
	    }

	    /* if it isn't FreeBE/AF, it must be UniVBE, so if we don't
	     * have hardware acceleration we may as well just fail this
	     * call and use VBE 3.0 instead.
	     */
	    if ((!(mode_info->Attributes & 16)) && (!faf_id)) {
	       ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Hardware acceleration not available"));
	       return 0;
	    }

	    return *mode;
	 }
      }
   }

   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
   return 0;
}



/* set_vbeaf_mode:
 *  Selects a VBE/AF graphics mode.
 */
static int set_vbeaf_mode(int mode, int w, int h, int v_w, int v_h, int *width, int *scrollable)
{
   long wret = *width;
   int ret;

   if (gfx_vbeaf.linear)
      mode |= 0x4000;

   /* try to set a large virtual screen */
   mode |= 0x1000;
   *scrollable = TRUE;

   saved_mode = mode;
   saved_vw = v_w;
   saved_vh = v_h;
   saved_wret = wret;

   ret = af_driver->SetVideoMode(af_driver, mode, v_w, v_h, &wret, 1, NULL);

   if ((ret != 0) && (v_w <= w) && (v_h <= h)) {
      /* if that didn't work, try to set a non-scrolling framebuffer */
      mode &= ~0x1000;
      *scrollable = FALSE;

      saved_mode = mode;
      saved_wret = wret;

      ret = af_driver->SetVideoMode(af_driver, mode, v_w, v_h, &wret, 1, NULL);
   }

   *width = wret;
   return ret;
}



/* vbeaf_locate_driver:
 *  locates and loads a VBE/AF driver.
 */
static int vbeaf_locate_driver(void)
{
   static char *possible_filenames[] =
   {
   #ifdef ALLEGRO_DOS
      "c:\\vbeaf.drv",
   #else
      "/usr/local/lib/vbeaf.drv",
      "/usr/lib/vbeaf.drv",
      "/lib/vbeaf.drv",
      "/vbeaf.drv",
   #endif
      NULL
   };

   char filename[1024], tmp1[1024], tmp2[128];
   AL_CONST char *p;
   int ret, i, attrib;

   /* look for driver in the config file location */
   p = get_config_string(uconvert_ascii("graphics", tmp1), uconvert_ascii("vbeaf_driver", tmp2), NULL);

   if ((p) && (ugetc(p))) {
      ustrzcpy(filename, sizeof(filename), p);

      if (ugetc(get_filename(filename)) == 0) {
	 append_filename(filename, filename, uconvert_ascii("vbeaf.drv", tmp1), sizeof(filename));
      }
      else {
	 if (file_exists(filename, FA_DIREC, &attrib)) {
	    if (attrib & FA_DIREC)
	       append_filename(filename, filename, uconvert_ascii("vbeaf.drv", tmp1), sizeof(filename));
	 }
      }

      ret = load_vbeaf_driver(filename);
      if (ret)
	 goto found_it;
   }

   /* look for driver in the same directory as the program */
   get_executable_name(tmp1, sizeof(tmp1));
   replace_filename(filename, tmp1, uconvert_ascii("vbeaf.drv", tmp2), sizeof(filename));
   ret = load_vbeaf_driver(filename);
   if (ret)
      goto found_it;

   /* look for driver in the default location */
   for (i=0; possible_filenames[i]; i++) {
      ret = load_vbeaf_driver(uconvert_ascii(possible_filenames[i], tmp1));
      if (ret)
	 goto found_it;
   }

   /* check the environment for a location */
   p = getenv("VBEAF_PATH");
   if (p) {
      append_filename(filename, uconvert_ascii(p, tmp1), uconvert_ascii("vbeaf.drv", tmp2), sizeof(filename));
      ret = load_vbeaf_driver(filename);
      if (ret)
	 goto found_it;
   }

   /* oops, no driver */
   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't find VBEAF.DRV"));
   ret = FALSE;

   /* got it! */ 
   found_it:

   return ret;
}



/*  vbeaf_lowlevel_init:
 *   loads the VBE/AF driver and initializes it.
 */
static int vbeaf_lowlevel_init(void)
{
   int ret;

   /* check the driver ID string */
   if (strcmp(af_driver->Signature, "VBEAF.DRV") != 0) {
      vbeaf_exit(NULL);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Bad VBE/AF driver ID string"));
      return FALSE;
   }

   /* check the VBE/AF version number */
   if (af_driver->Version < 0x200) {
      vbeaf_exit(NULL);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Obsolete VBE/AF version (need 2.0 or greater)"));
      return FALSE;
   }

   /* to run on Linux, we need to be God */
   #ifdef ALLEGRO_LINUX

      if (!__al_linux_have_ioperms) {
	 vbeaf_exit(NULL);
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("This driver needs root privileges"));
	 return FALSE;
      }

      _vbeaf_selector = _default_ds();

   #endif

   /* detect and initialise the FreeBE/AF extensions */
   if (strstr(af_driver->OemVendorName, "FreeBE")) {
      faf_id = TRUE;
      initialise_freebeaf_extensions();
   }
   else {
      faf_id = FALSE;
      faf_ext = 0;
   }

   /* special setup for Plug and Play hardware */
   if (call_vbeaf_asm(af_driver->PlugAndPlayInit) != 0) {
      vbeaf_exit(NULL);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VBE/AF Plug and Play initialisation failed"));
      return FALSE;
   }

   /* deal with all that DPMI memory mapping crap */
   ret = initialise_vbeaf_driver();
   if (ret != 0) {
      vbeaf_exit(NULL);
      if (ret == -2)
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VBE/AF nearptrs not supported on this platform"));
      else
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't map memory for VBE/AF"));
      return FALSE;
   }

   /* low level driver initialisation */
   if (call_vbeaf_asm(af_driver->InitDriver) != 0) {
      vbeaf_exit(NULL);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VBE/AF device not present"));
      return FALSE;
   }

   return TRUE;
}



/* vbeaf_init:
 *  Tries to enter the specified graphics mode, and makes a screen bitmap
 *  for it.
 */
static BITMAP *vbeaf_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;
   AF_MODE_INFO mode_info;
   int bpp = BYTES_PER_PIXEL(color_depth);
   int bytes_per_scanline, width, height;
   int scrollable, mode;
   int vseg;
   int rs, gs, bs;
   char tmp1[512];
   void *vaddr;

   /* Do not continue if this version of Allegro was built in C-only mode.
    * The bank switchers assume asm-mode calling conventions, but the
    * library would try to call them with C calling conventions.
    */
#ifdef ALLEGRO_NO_ASM
   return NULL;
#endif

   /* locate and load VBE/AF driver from disk */
   if (!vbeaf_locate_driver())
      return NULL;

   LOCK_VARIABLE(af_driver);
   LOCK_VARIABLE(_accel_driver);
   LOCK_VARIABLE(_accel_active);
   LOCK_VARIABLE(_accel_set_bank);
   LOCK_VARIABLE(_accel_idle);
   LOCK_VARIABLE(vbeaf_cur_on);
   LOCK_VARIABLE(vbeaf_cur_x);
   LOCK_VARIABLE(vbeaf_cur_y);
   LOCK_FUNCTION(_accel_bank_switch);
   LOCK_FUNCTION(_accel_bank_stub);
   LOCK_FUNCTION(_af_wrapper);
   LOCK_FUNCTION(vbeaf_no_wait);
   LOCK_FUNCTION(vbeaf_move_mouse);
   LOCK_FUNCTION(vbeaf_draw_sprite);
   LOCK_FUNCTION(vbeaf_blit_from_memory);

   /* init the currently loaded VBE/AF driver */
   if (!vbeaf_lowlevel_init())
      return NULL;

   /* bodge to work around bugs in the present (6.51) version of UniVBE */
   if ((v_w > w) && (!faf_id)) {
      vbeaf_exit(NULL);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("SciTech VBE/AF drivers do not support wide virtual screens"));
      return NULL;
   }

   /* get ourselves a mode number */
   mode = find_vbeaf_mode(w, h, v_w, v_h, color_depth, &mode_info);
   if (mode == 0) {
      vbeaf_exit(NULL);
      return NULL;
   }

   gfx_vbeaf.vid_mem = af_driver->TotalMemory * 1024;

   vaddr = NULL;

   if (mode_info.Attributes & 8) {
      /* linear framebuffer */
      gfx_vbeaf.linear = TRUE;
      gfx_vbeaf.bank_size = gfx_vbeaf.bank_gran = 0;
      gfx_vbeaf.vid_phys_base = af_driver->LinearBasePtr;
      bytes_per_scanline = mode_info.LinBytesPerScanLine;

      if (faf_farptr) {
	 vaddr = (void *)faf_farptr->LinearMem.offset;
	 vseg = faf_farptr->LinearMem.sel;
      }
      else {
	 vaddr = af_driver->LinearMem;
	 vseg = _default_ds();
      }
   }
   else {
      /* banked framebuffer */
      gfx_vbeaf.linear = FALSE;
      gfx_vbeaf.bank_size = 65536;
      gfx_vbeaf.bank_gran = af_driver->BankSize * 1024;
      gfx_vbeaf.vid_phys_base = af_driver->BankedBasePtr;
      bytes_per_scanline = mode_info.BytesPerScanLine;

      if (faf_farptr) {
	 vaddr = (void *)faf_farptr->BankedMem.offset;
	 vseg = faf_farptr->BankedMem.sel;
      }
      else {
	 vaddr = af_driver->BankedMem;
	 vseg = _default_ds();
      }
   }

   width = MAX(bytes_per_scanline, v_w*bpp);
   height = MAX(h, v_h);
   _sort_out_virtual_width(&width, &gfx_vbeaf);

   if (width * height > gfx_vbeaf.vid_mem) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Insufficient video memory"));
      vbeaf_exit(NULL);
      return NULL;
   }

   /* set the mode */
   if (set_vbeaf_mode(mode, w, h, width/bpp, height, &width, &scrollable) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to set VBE/AF mode"));
      vbeaf_exit(NULL);
      return NULL;
   }

   in_af_mode = TRUE;
   _accel_active = FALSE;

   if (scrollable)
      height = MAX(height, (int)af_driver->OffscreenEndY+1);

   if ((width/bpp < v_w) || (width/bpp < w) || (height < v_h) || (height < h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Virtual screen size too large"));
      vbeaf_exit(NULL);
      return NULL;
   }

   /* construct the screen bitmap */
   b = _make_bitmap(width/bpp, height, (unsigned long)vaddr, &gfx_vbeaf, color_depth, width);
   if (!b) {
      vbeaf_exit(NULL);
      return NULL;
   }

   b->seg = vseg;

   /* set up the /AF driver state */
   af_driver->SetActiveBuffer(af_driver, 0);
   af_driver->SetVisibleBuffer(af_driver, 0, 0);

   if (af_driver->EnableDirectAccess)
      af_driver->EnableDirectAccess(af_driver);

   if (af_driver->SetClipRect)
      af_driver->SetClipRect(af_driver, 0, 0, b->w-1, b->h-1);

   _accel_set_bank = af_driver->SetBank;

   if (af_driver->EnableDirectAccess)
      _accel_idle = af_driver->EnableDirectAccess;
   else if (af_driver->WaitTillIdle)
      _accel_idle = af_driver->WaitTillIdle;
   else
      _accel_idle = vbeaf_no_wait;

   if (gfx_vbeaf.linear) {
      if (_accel_idle == vbeaf_no_wait)
	 b->write_bank = b->read_bank = _stub_bank_switch;
      else
	 b->write_bank = b->read_bank = _accel_bank_stub;
   }
   else {
      b->write_bank = b->read_bank = _accel_bank_switch;
      b->id |= BMP_ID_NOBLIT;
   }

   vbeaf_xscroll = vbeaf_yscroll = 0;

   gfx_vbeaf.w = b->cr = w;
   gfx_vbeaf.h = b->cb = h;

   /* set up the truecolor pixel format */
   #if (defined ALLEGRO_COLOR16) || (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

      if (gfx_vbeaf.linear) {
	 rs = mode_info.LinRedFieldPosition; 
	 gs = mode_info.LinGreenFieldPosition;
	 bs = mode_info.LinBlueFieldPosition;
      }
      else {
	 rs = mode_info.RedFieldPosition; 
	 gs = mode_info.GreenFieldPosition;
	 bs = mode_info.BlueFieldPosition;
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

   orig_vline = _screen_vtable.vline;
   orig_hline = _screen_vtable.hline;
   orig_line = _screen_vtable.line;
   orig_rectfill = _screen_vtable.rectfill;
   orig_draw_glyph = _screen_vtable.draw_glyph;
   orig_draw_sprite = _screen_vtable.draw_sprite;
   orig_masked_blit = _screen_vtable.masked_blit;

   /* is triple buffering supported? */
   if (mode_info.Attributes & 2048) {
      gfx_vbeaf.request_scroll = vbeaf_request_scroll;
      gfx_vbeaf.poll_scroll = vbeaf_poll_scroll;
   }
   else {
      gfx_vbeaf.request_scroll = NULL;
      gfx_vbeaf.poll_scroll = NULL;
   }

   /* are hardware cursors supported? */
   if ((mode_info.Attributes & 64) && (af_driver->SetCursor)) {
      gfx_vbeaf.set_mouse_sprite = vbeaf_set_mouse_sprite;
      gfx_vbeaf.show_mouse = vbeaf_show_mouse;
      gfx_vbeaf.hide_mouse = vbeaf_hide_mouse;
      gfx_vbeaf.move_mouse = vbeaf_move_mouse;
   }
   else {
      gfx_vbeaf.set_mouse_sprite = NULL;
      gfx_vbeaf.show_mouse = NULL;
      gfx_vbeaf.hide_mouse = NULL;
      gfx_vbeaf.move_mouse = NULL;
   }

   /* accelerated scanline fills? */
   if (af_driver->DrawScan)
      gfx_capabilities |= (GFX_HW_HLINE | GFX_HW_HLINE_XOR);

   if (af_driver->DrawPattScan)
      gfx_capabilities |= GFX_HW_HLINE_SOLID_PATTERN;

   if (af_driver->DrawColorPattScan)
      gfx_capabilities |= GFX_HW_HLINE_COPY_PATTERN;

   /* accelerated line drawing? */
   if (af_driver->DrawLine)
      gfx_capabilities |= (GFX_HW_LINE | GFX_HW_LINE_XOR);

   /* accelerated rectangle fills? */
   if (af_driver->DrawRect) {
      _screen_vtable.clear_to_color = vbeaf_clear_to_color;
      gfx_capabilities |= (GFX_HW_FILL | GFX_HW_FILL_XOR);
   }

   if (af_driver->DrawPattRect)
      gfx_capabilities |= GFX_HW_FILL_SOLID_PATTERN;

   if (af_driver->DrawColorPattRect)
      gfx_capabilities |= GFX_HW_FILL_COPY_PATTERN;

   /* accelerated triangle drawing? */
   if (af_driver->DrawTrap)
      gfx_capabilities |= (GFX_HW_TRIANGLE | GFX_HW_TRIANGLE_XOR);

   /* accelerated monochrome text output? */
   if (af_driver->PutMonoImage) {
      _screen_vtable.draw_glyph = vbeaf_draw_glyph;
      gfx_capabilities |= GFX_HW_GLYPH;
   }

   /* accelerated video memory blits? */
   if (af_driver->BitBlt) {
      _screen_vtable.blit_to_self = vbeaf_blit_to_self;
      _screen_vtable.blit_to_self_forward = vbeaf_blit_to_self;
      _screen_vtable.blit_to_self_backward = vbeaf_blit_to_self;
      gfx_capabilities |= GFX_HW_VRAM_BLIT;
   }

   /* accelerated blits from system memory? */
   if (af_driver->BitBltSys) {
      _screen_vtable.blit_from_memory = vbeaf_blit_from_memory;
      _screen_vtable.blit_from_system = vbeaf_blit_from_memory;
      gfx_capabilities |= GFX_HW_MEM_BLIT;
   }

   /* accelerated masked blits? */
   if ((af_driver->SrcTransBlt) || (af_driver->SrcTransBltSys)) {
      _screen_vtable.masked_blit = vbeaf_masked_blit;
      _screen_vtable.draw_sprite = vbeaf_draw_sprite;

      if (_screen_vtable.draw_256_sprite == orig_draw_sprite)
	 _screen_vtable.draw_256_sprite = vbeaf_draw_sprite;

      if (af_driver->SrcTransBlt)
	 gfx_capabilities |= GFX_HW_VRAM_BLIT_MASKED;

      if (af_driver->SrcTransBltSys)
	 gfx_capabilities |= GFX_HW_MEM_BLIT_MASKED;
   }

   /* set up the VBE/AF description string */
   ustrzcpy(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(af_driver->OemVendorName, tmp1));

   if (gfx_vbeaf.linear)
      ustrzcat(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(", linear", tmp1));
   else
      ustrzcat(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(", banked", tmp1));

   if (faf_ext > 0)
      uszprintf(vbeaf_desc+ustrsize(vbeaf_desc), sizeof(vbeaf_desc) - ustrsize(vbeaf_desc),
		uconvert_ascii(", FreeBE ex%02d", tmp1), faf_ext);
   else if (faf_id)
      ustrzcat(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(", FreeBE noex", tmp1));

   if (faf_farptr)
      ustrzcat(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(", farptr", tmp1));

   /* is this an accelerated or dumb framebuffer mode? */
   if (mode_info.Attributes & 16) {
      gfx_vbeaf.drawing_mode = vbeaf_drawing_mode;
      vbeaf_drawing_mode();
      ustrzcat(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(", accel", tmp1));
   }
   else {
      gfx_vbeaf.drawing_mode = NULL;
      ustrzcat(vbeaf_desc, sizeof(vbeaf_desc), uconvert_ascii(", noaccel", tmp1));
   }

   gfx_vbeaf.desc = vbeaf_desc;

   #ifdef ALLEGRO_LINUX

      __al_linux_console_graphics();

      b->vtable->acquire = __al_linux_acquire_bitmap;
      b->vtable->release = __al_linux_release_bitmap;

   #endif

   return b;
}



/* vbeaf_exit:
 *  Shuts down the VBE/AF driver.
 */
static void vbeaf_exit(BITMAP *b)
{
   int c;

   /* shut down the driver */
   if (in_af_mode) {
      if (af_driver->EnableDirectAccess)
	 af_driver->EnableDirectAccess(af_driver);
      else if (af_driver->WaitTillIdle)
	 af_driver->WaitTillIdle(af_driver);

      af_driver->RestoreTextMode(af_driver);
      in_af_mode = FALSE;
   }

   /* undo memory mappings */
   if (faf_farptr) {
      for (c=0; c<4; c++)
	 _remove_selector(&faf_farptr->IOMemMaps[c].sel);

      _remove_selector(&faf_farptr->BankedMem.sel);
      _remove_selector(&faf_farptr->LinearMem.sel);
   }

   for (c=0; c<4; c++)
      _remove_linear_mapping(af_memmap+c);

   _remove_linear_mapping(&af_banked_mem);
   _remove_linear_mapping(&af_linear_mem);

   if (af_driver) {
      _AL_FREE(af_driver);
      af_driver = _accel_driver = NULL;
   }

   #ifdef ALLEGRO_DOS

      if (vbeaf_nearptr) {
	 __djgpp_nearptr_disable();
	 vbeaf_nearptr = FALSE;
      }

   #endif

   #ifdef ALLEGRO_LINUX

      __al_linux_console_text();

   #endif
}



/* vbeaf_save:
 *  Saves the graphics state.
 */
static void vbeaf_save(void)
{
   if (af_driver->EnableDirectAccess)
      af_driver->EnableDirectAccess(af_driver);
   else if (af_driver->WaitTillIdle)
      af_driver->WaitTillIdle(af_driver);

   af_driver->RestoreTextMode(af_driver);

   in_af_mode = FALSE;
}



/* vbeaf_restore:
 *  Restores the graphics state.
 */
static void vbeaf_restore(void)
{
   long wret = saved_wret;

   af_driver->SetVideoMode(af_driver, saved_mode, saved_vw, saved_vh, &wret, 1, NULL);

   af_driver->SetActiveBuffer(af_driver, 0);
   af_driver->SetVisibleBuffer(af_driver, 0, 0);

   if (af_driver->EnableDirectAccess)
      af_driver->EnableDirectAccess(af_driver);

   if (af_driver->SetClipRect)
      af_driver->SetClipRect(af_driver, 0, 0, SCREEN_W-1, SCREEN_H-1);

   if (gfx_vbeaf.drawing_mode)
      gfx_vbeaf.drawing_mode();

   vbeaf_scroll(vbeaf_xscroll, vbeaf_yscroll);

   in_af_mode = TRUE;
}



/* vbeaf_fetch_mode_list:
 *  Attempts to create a list of valid video modes for the VBE/AF driver.
 *  Returns the mode list on success or NULL on failure.
 */
static GFX_MODE_LIST *vbeaf_fetch_mode_list(void)
{
   GFX_MODE_LIST *gfx_mode_list;
   AF_MODE_INFO *mode_info;
   unsigned short *mode;
   int vbeaf_was_off;

   mode_info = _AL_MALLOC(sizeof(AF_MODE_INFO));
   if (!mode_info) return NULL;

   /* make sure VBE/AF interface is enabled! */
   if (!af_driver) {
      if (!vbeaf_locate_driver()) {
         if (mode_info) _AL_FREE(mode_info);
	 
         return NULL;
      }
      if (!vbeaf_lowlevel_init()) {
         if (mode_info) _AL_FREE(mode_info);
	 
         return NULL;
      }
      vbeaf_was_off = TRUE;
   }
   else
      vbeaf_was_off = FALSE;

   /* start building mode-list */
   gfx_mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!gfx_mode_list) {
      if (mode_info) _AL_FREE(mode_info);
      
      return NULL;
   }
   gfx_mode_list->mode = NULL;
   gfx_mode_list->num_modes = 0;

   /* create mode list */
   for (mode = af_driver->AvailableModes; *mode != 0xFFFF; mode++) {
      gfx_mode_list->mode = _al_sane_realloc(gfx_mode_list->mode, sizeof(GFX_MODE) * (gfx_mode_list->num_modes + 1));
      if (!gfx_mode_list->mode) {
         if (mode_info) _AL_FREE(mode_info);
         if (gfx_mode_list) _AL_FREE(gfx_mode_list);
         
         return NULL;
      }
      if (af_driver->GetVideoModeInfo(af_driver, *mode, mode_info) != 0) {
         if (mode_info) _AL_FREE(mode_info);
         if (gfx_mode_list->mode) _AL_FREE(gfx_mode_list->mode);
         if (gfx_mode_list) _AL_FREE(gfx_mode_list);
         
         return NULL;
      }

      gfx_mode_list->mode[gfx_mode_list->num_modes].width  = mode_info->XResolution;
      gfx_mode_list->mode[gfx_mode_list->num_modes].height = mode_info->YResolution;
      gfx_mode_list->mode[gfx_mode_list->num_modes].bpp    = mode_info->BitsPerPixel;

      gfx_mode_list->num_modes++;
   }

   /* terminate mode list */
   gfx_mode_list->mode = _al_sane_realloc(gfx_mode_list->mode, sizeof(GFX_MODE) * (gfx_mode_list->num_modes + 1));
   if (!gfx_mode_list->mode) {
      if (mode_info) _AL_FREE(mode_info);
      if (gfx_mode_list) _AL_FREE(gfx_mode_list);
      
      return NULL;
   }

   gfx_mode_list->mode[gfx_mode_list->num_modes].width  = 0;
   gfx_mode_list->mode[gfx_mode_list->num_modes].height = 0;
   gfx_mode_list->mode[gfx_mode_list->num_modes].bpp    = 0;

   /* shut down VBE/AF interface if it wasn't previously loaded */
   if (vbeaf_was_off)
      vbeaf_exit(NULL);

   if (mode_info) _AL_FREE(mode_info);

   return gfx_mode_list;
}



/* vbeaf_vsync:
 *  VBE/AF vsync routine, needed for cards that don't emulate the VGA 
 *  blanking registers. VBE/AF doesn't provide a vsync function, but we 
 *  can emulate it by altering the display start address with the vsync 
 *  flag set.
 */
static void vbeaf_vsync(void)
{
   vbeaf_scroll(vbeaf_xscroll, vbeaf_yscroll);
}



/* vbeaf_scroll:
 *  Hardware scrolling routine.
 */
static int vbeaf_scroll(int x, int y)
{
   int moved = (vbeaf_xscroll != x) || (vbeaf_yscroll != y);

   vbeaf_xscroll = x;
   vbeaf_yscroll = y;

   SAFISH_CALL(
      if (_wait_for_vsync && af_driver->WaitTillIdle)
	 af_driver->WaitTillIdle(af_driver);

      af_driver->SetDisplayStart(af_driver, x, y, 1);
   );

   if ((vbeaf_cur_bmp) && (moved))
      vbeaf_move_mouse(vbeaf_cur_x, vbeaf_cur_y);

   return 0;
}



/* vbeaf_set_palette_range:
 *  Uses VBE/AF functions to set the palette.
 */
static void vbeaf_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   AF_PALETTE tmp[256];
   int c;

   for (c=from; c<=to; c++) {
      tmp[c-from].red = p[c].r << 2;
      tmp[c-from].green = p[c].g << 2;
      tmp[c-from].blue = p[c].b << 2;
      tmp[c-from].alpha = 0;
   }

   SAFISH_CALL(
      af_driver->SetPaletteData(af_driver, tmp, to-from+1, from, (vsync ? 1 : 0));
   );
}



/* vbeaf_request_scroll:
 *  Triple buffering initiate routine.
 */
static int vbeaf_request_scroll(int x, int y)
{
   vbeaf_xscroll = x;
   vbeaf_yscroll = y;

   SAFISH_CALL(
      if (af_driver->WaitTillIdle)
	 af_driver->WaitTillIdle(af_driver);

      af_driver->SetDisplayStart(af_driver, x, y, 0);
   );

   if (vbeaf_cur_bmp)
      vbeaf_move_mouse(vbeaf_cur_x, vbeaf_cur_y);

   return 0;
}



/* vbeaf_poll_scroll:
 *  Triple buffering test routine.
 */
static int vbeaf_poll_scroll(void)
{
   int ret;

   SAFISH_CALL(
      ret = af_driver->GetDisplayStartStatus(af_driver);
   );

   return (ret) ? FALSE : TRUE;
}



/* vbeaf_set_mouse_sprite:
 *  Attempts to download a new hardware cursor graphic, returning zero on
 *  success or non-zero if this image is unsuitable.
 */
static int vbeaf_set_mouse_sprite(BITMAP *sprite, int xfocus, int yfocus)
{
   AF_CURSOR cursor;
   int bpp = bitmap_color_depth(sprite);
   int c, x, y;
   int iszero;

   /* hardware cursors cannot be larger than 32x32 */
   if ((sprite->w > 32) || (sprite->h > 32))
      return -1;

   /* clear the cursor data */
   for (c=0; c<32; c++) {
      cursor.andMask[c] = 0;
      cursor.xorMask[c] = 0;
   }

   vbeaf_cur_c1 = -1;
   vbeaf_cur_c2 = -1;

   /* scan through the pointer image */
   for (y=0; y<sprite->h; y++) {
      for (x=0; x<sprite->w; x++) {

	 c = getpixel(sprite, x, y);

	 if (c == bitmap_mask_color(sprite)) {
	    /* skip masked pixels */
	 }
	 else if (c == vbeaf_cur_c1) {
	    /* color #1 pixel */
	    cursor.andMask[y] |= 0x80000000>>x;
	    cursor.xorMask[y] |= 0x80000000>>x;
	 }
	 else if (c == vbeaf_cur_c2) {
	    /* color #2 pixel */
	    cursor.andMask[y] |= 0x80000000>>x;
	 }
	 else {
	    /* check whether this pixel value is zero */
	    if (bpp == 8) {
	       iszero = ((getr_depth(bpp, c) == getr_depth(bpp, 0)) &&
			 (getg_depth(bpp, c) == getg_depth(bpp, 0)) &&
			 (getb_depth(bpp, c) == getb_depth(bpp, 0)));
	    }
	    else {
	       iszero = ((getr_depth(bpp, c) == 0) &&
			 (getg_depth(bpp, c) == 0) &&
			 (getb_depth(bpp, c) == 0));
	    }

	    if ((vbeaf_cur_c2 < 0) && (iszero)) {
	       /* zero pixel value = color #2 */
	       vbeaf_cur_c2 = c;
	       cursor.andMask[y] |= 0x80000000>>x;
	    }
	    else if (vbeaf_cur_c1 < 0) {
	       /* other pixel value = color #1 */
	       vbeaf_cur_c1 = c;
	       cursor.andMask[y] |= 0x80000000>>x;
	       cursor.xorMask[y] |= 0x80000000>>x;
	    }
	    else {
	       /* it's not cool if there are more than two colors! */
	       return -1;
	    }
	 }
      }
   }

   /* flip the endianess */
   for (c=0; c<32; c++) {
      cursor.andMask[c] = bswap(cursor.andMask[c]);
      cursor.xorMask[c] = bswap(cursor.xorMask[c]);
   }

   cursor.hotx = xfocus;
   cursor.hoty = yfocus;

   /* download to the hardware */
   SAFISH_CALL(
      af_driver->SetCursor(af_driver, &cursor);

      if ((bpp > 8) && (vbeaf_cur_c1 >= 0)) {
	 af_driver->SetCursorColor(af_driver, 
				   getr_depth(bpp, vbeaf_cur_c1),
				   getg_depth(bpp, vbeaf_cur_c1),
				   getb_depth(bpp, vbeaf_cur_c1));
      }
   );

   return 0;
}



/* vbeaf_show_mouse:
 *  Turns on the hardware cursor, returning zero on success.
 */
static int vbeaf_show_mouse(BITMAP *bmp, int x, int y)
{
   if (bitmap_color_depth(bmp) == 8) {
      if (vbeaf_cur_c2 >= 0) {
	 /* check that the palette contains a valid zero color */
	 if ((getr_depth(8, vbeaf_cur_c2) != getr_depth(8, 0)) ||
	     (getg_depth(8, vbeaf_cur_c2) != getg_depth(8, 0)) ||
	     (getb_depth(8, vbeaf_cur_c2) != getb_depth(8, 0)))
	    return -1;
      }

      /* set the cursor color */
      if (vbeaf_cur_c1 >= 0) {
	 SAFISH_CALL(
	    af_driver->SetCursorColor(af_driver, vbeaf_cur_c1, 0, 0);
	 );
      }
   }

   vbeaf_cur_bmp = bmp;
   vbeaf_cur_on = FALSE;

   vbeaf_move_mouse(x, y);

   return 0;
}



/* vbeaf_hide_mouse:
 *  Gets rid of the hardware cursor.
 */
static void vbeaf_hide_mouse(void)
{
   SAFISH_CALL(
      af_driver->ShowCursor(af_driver, 0);
   );

   vbeaf_cur_bmp = NULL;
   vbeaf_cur_on = FALSE;
}



/* vbeaf_move_mouse:
 *  Sets the hardware cursor position.
 */
static void vbeaf_move_mouse(int x, int y)
{
   int hx, hy;
   int onscreen;

   hx = x + vbeaf_cur_bmp->x_ofs - vbeaf_xscroll;
   hy = y + vbeaf_cur_bmp->y_ofs - vbeaf_yscroll;

   onscreen = (hx >= 0) && (hx < SCREEN_W) && (hy >= 0) && (hy < SCREEN_H);

   SAFISH_CALL(
      if (onscreen) {
	 af_driver->SetCursorPos(af_driver, hx, hy);

	 if (!vbeaf_cur_on) {
	    af_driver->ShowCursor(af_driver, 1);
	    vbeaf_cur_on = TRUE;
	 }
      }
      else {
	 if (vbeaf_cur_on) {
	    af_driver->ShowCursor(af_driver, 0);
	    vbeaf_cur_on = FALSE;
	 }
      }
   );

   vbeaf_cur_x = x;
   vbeaf_cur_y = y;
}

END_OF_STATIC_FUNCTION(vbeaf_move_mouse);



/* vbeaf_drawing_mode:
 *  Hook to tell us that the drawing mode has been changed, so we may need
 *  to alter some of our vtable entries.
 */
static void vbeaf_drawing_mode(void)
{
   vbeaf_pattern = NULL;

   if ((_drawing_mode == DRAW_MODE_SOLID) || (_drawing_mode == DRAW_MODE_XOR)) {
      /* easy, everything supports solid and XOR drawing */
      _screen_vtable.hline = (af_driver->DrawScan) ? vbeaf_hline : orig_hline;
      _screen_vtable.vline = (af_driver->DrawLine) ? vbeaf_vline_a : ((af_driver->DrawRect) ? vbeaf_vline_b : orig_vline);
      _screen_vtable.line = (af_driver->DrawLine) ? vbeaf_line : orig_line;
      _screen_vtable.rectfill = (af_driver->DrawRect) ? vbeaf_rectfill : orig_rectfill;
      _screen_vtable.triangle = (af_driver->DrawTrap) ? vbeaf_triangle : NULL;

      vbeaf_fg_mix = vbeaf_bg_mix = (_drawing_mode == DRAW_MODE_XOR) ? 3 : 0;

      SAFISH_CALL(
	 af_driver->SetMix(af_driver, vbeaf_fg_mix, vbeaf_bg_mix);
      );
      return;
   }

   if ((_drawing_mode == DRAW_MODE_COPY_PATTERN) &&
       (_drawing_pattern->w <= 8) && (_drawing_pattern->h <= 8)) {
      /* color patterns can be done in hardware if they are small enough */
      _screen_vtable.hline = (af_driver->DrawColorPattScan) ? vbeaf_hline : orig_hline;
      _screen_vtable.vline = (af_driver->DrawColorPattRect) ? vbeaf_vline_b : orig_vline;
      _screen_vtable.line = orig_line;
      _screen_vtable.rectfill = (af_driver->DrawColorPattRect) ? vbeaf_rectfill : orig_rectfill;
      _screen_vtable.triangle = NULL;

      vbeaf_fg_mix = vbeaf_bg_mix = 0;

      SAFISH_CALL(
	 af_driver->SetMix(af_driver, vbeaf_fg_mix, vbeaf_bg_mix);
      );
      return;
   }

   if (((_drawing_mode == DRAW_MODE_SOLID_PATTERN) || (_drawing_mode == DRAW_MODE_MASKED_PATTERN)) &&
       (_drawing_pattern->w <= 8) && (_drawing_pattern->h <= 8)) {
      /* mono patterns can be done in hardware if they are small enough */
      _screen_vtable.hline = (af_driver->DrawPattScan) ? vbeaf_hline : orig_hline;
      _screen_vtable.vline = (af_driver->DrawPattRect) ? vbeaf_vline_b : orig_vline;
      _screen_vtable.line = orig_line;
      _screen_vtable.rectfill = (af_driver->DrawPattRect) ? vbeaf_rectfill : orig_rectfill;
      _screen_vtable.triangle = NULL;

      vbeaf_fg_mix = 0;
      vbeaf_bg_mix = (_drawing_mode == DRAW_MODE_MASKED_PATTERN) ? 4 : 0;

      SAFISH_CALL(
	 af_driver->SetMix(af_driver, vbeaf_fg_mix, vbeaf_bg_mix);
      );
      return;
   }

   /* urgh, have to use software drawing functions for this mode */
   _screen_vtable.vline = orig_vline;
   _screen_vtable.hline = orig_hline;
   _screen_vtable.line = orig_line;
   _screen_vtable.rectfill = orig_rectfill;
   _screen_vtable.triangle = NULL;

   vbeaf_fg_mix = vbeaf_bg_mix = 0;

   SAFISH_CALL(
      af_driver->SetMix(af_driver, vbeaf_fg_mix, vbeaf_bg_mix);
   );
}



/* prepare_color_pattern:
 *  Sets up the hardware ready for a colored pattern drawing operation.
 */
static void prepare_color_pattern(BITMAP *bmp)
{
   static unsigned long pattern[64];
   int x, y, xx, yy, xo, yo;

   if (vbeaf_pattern != bmp) {
      xo = _drawing_x_anchor + bmp->x_ofs;
      yo = _drawing_y_anchor + bmp->y_ofs;

      switch (bitmap_color_depth(bmp)) {

	 #ifdef ALLEGRO_COLOR8

	    case 8:
	       for (y=0; y<8; y++) {
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     pattern[y*8+x] = _drawing_pattern->line[yy][xx];
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	    case 16:
	       for (y=0; y<8; y++) {
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     pattern[y*8+x] = ((unsigned short *)_drawing_pattern->line[yy])[xx];
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       for (y=0; y<8; y++) {
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     pattern[y*8+x] = *((unsigned long *)(_drawing_pattern->line[yy]+xx*3)) & 0xFFFFFF;
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       for (y=0; y<8; y++) {
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     pattern[y*8+x] = ((unsigned long *)_drawing_pattern->line[yy])[xx];
		  }
	       }
	       break;

	 #endif
      }

      af_driver->Set8x8ColorPattern(af_driver, 0, pattern);
      af_driver->Use8x8ColorPattern(af_driver, 0);

      vbeaf_pattern = bmp;
   }
}



/* prepare_mono_pattern:
 *  Sets up the hardware ready for a mono pattern drawing operation.
 */
static void prepare_mono_pattern(BITMAP *bmp)
{
   static unsigned char pattern[8];
   int x, y, xx, yy, xo, yo;

   if (vbeaf_pattern != bmp) {
      xo = _drawing_x_anchor + bmp->x_ofs;
      yo = _drawing_y_anchor + bmp->y_ofs;

      switch (bitmap_color_depth(bmp)) {

	 #ifdef ALLEGRO_COLOR16

	    case 8:
	       for (y=0; y<8; y++) {
		  pattern[y] = 0;
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     if (_drawing_pattern->line[yy][xx])
			pattern[y] |= (0x80>>x);
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	    case 16:
	       for (y=0; y<8; y++) {
		  pattern[y] = 0;
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     if (((unsigned short *)_drawing_pattern->line[yy])[xx] != bitmap_mask_color(bmp))
			pattern[y] |= (0x80>>x);
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       for (y=0; y<8; y++) {
		  pattern[y] = 0;
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     if ((*((unsigned long *)(_drawing_pattern->line[yy]+xx*3)) & 0xFFFFFF) != MASK_COLOR_24)
			pattern[y] |= (0x80>>x);
		  }
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       for (y=0; y<8; y++) {
		  pattern[y] = 0;
		  for (x=0; x<8; x++) {
		     xx = (x-xo) & _drawing_x_mask;
		     yy = (y-yo) & _drawing_y_mask;
		     if (((unsigned long *)_drawing_pattern->line[yy])[xx] != MASK_COLOR_32)
			pattern[y] |= (0x80>>x);
		  }
	       }
	       break;

	 #endif
      }

      af_driver->Set8x8MonoPattern(af_driver, pattern);

      vbeaf_pattern = bmp;
   }
}



/* go_accel:
 *  Prepares the hardware for an accelerated drawing operation.
 */
static INLINE void go_accel(void)
{
   /* turn on the accelerator */
   if (!_accel_active) {
      if (af_driver->DisableDirectAccess)
	 af_driver->DisableDirectAccess(af_driver);

      _accel_active = TRUE;
   }
}



/* vbeaf_hline:
 *  Accelerated scanline filling routine.
 */
static void vbeaf_hline(BITMAP *bmp, int x1, int y, int x2, int color)
{
   if (x1 > x2) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (bmp->clip) {
      if ((y < bmp->ct) || (y >= bmp->cb))
	 return;

      if (x1 < bmp->cl)
	 x1 = bmp->cl;

      if (x2 >= bmp->cr)
	 x2 = bmp->cr-1;

      if (x2 < x1)
	 return;
   }

   SAFISH_CALL(
      go_accel();

      switch (_drawing_mode) {

	 case DRAW_MODE_SOLID:
	 case DRAW_MODE_XOR:
	    /* normal scanline fill */
	    af_driver->DrawScan(af_driver, color, y+bmp->y_ofs, 
				x1+bmp->x_ofs, x2+bmp->x_ofs+1);
	    break;

	 case DRAW_MODE_COPY_PATTERN:
	    /* colored pattern scanline fill */
	    prepare_color_pattern(bmp);

	    af_driver->DrawColorPattScan(af_driver, y+bmp->y_ofs, 
					 x1+bmp->x_ofs, x2+bmp->x_ofs+1);
	    break;

	 case DRAW_MODE_SOLID_PATTERN:
	 case DRAW_MODE_MASKED_PATTERN:
	    /* mono pattern scanline fill */
	    prepare_mono_pattern(bmp);

	    af_driver->DrawPattScan(af_driver, color, 0, y+bmp->y_ofs, 
				    x1+bmp->x_ofs, x2+bmp->x_ofs+1);
	    break;
      }
   );
}



/* vbeaf_vline_a:
 *  Accelerated vertical line drawer, using the VBE/AF line primitive.
 */
static void vbeaf_vline_a(BITMAP *bmp, int x, int y1, int y2, int color)
{
   if (y1 > y2) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
      if ((x < bmp->cl) || (x >= bmp->cr))
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;
   }

   SAFISH_CALL(
      go_accel();

      af_driver->DrawLine(af_driver, color,
			  itofix(x+bmp->x_ofs), itofix(y1+bmp->y_ofs),
			  itofix(x+bmp->x_ofs), itofix(y2+bmp->y_ofs));
   );
}



/* vbeaf_vline_b:
 *  Accelerated vertical line drawer, using the VBE/AF rectangle primitive.
 */
static void vbeaf_vline_b(BITMAP *bmp, int x, int y1, int y2, int color)
{
   if (y1 > y2) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
      if ((x < bmp->cl) || (x >= bmp->cr))
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;
   }

   SAFISH_CALL(
      go_accel();

      switch (_drawing_mode) {

	 case DRAW_MODE_SOLID:
	 case DRAW_MODE_XOR:
	    /* mono vertical line */
	    af_driver->DrawRect(af_driver, color, 
				x+bmp->x_ofs, y1+bmp->y_ofs, 1, y2-y1+1);
	    break;

	 case DRAW_MODE_COPY_PATTERN:
	    /* colored pattern vertical line */
	    prepare_color_pattern(bmp);

	    af_driver->DrawColorPattRect(af_driver,
					 x+bmp->x_ofs, y1+bmp->y_ofs, 1, y2-y1+1);
	    break;

	 case DRAW_MODE_SOLID_PATTERN:
	 case DRAW_MODE_MASKED_PATTERN:
	    /* mono pattern vertical line */
	    prepare_mono_pattern(bmp);

	    af_driver->DrawPattRect(af_driver, color, 0,
				    x+bmp->x_ofs, y1+bmp->y_ofs, 1, y2-y1+1);
	    break;
      }
   );
}



/* vbeaf_line:
 *  Accelerated line drawer.
 */
static void vbeaf_line(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int sx, sy, dx, dy, t;

   if (bmp->clip) {
      sx = x1;
      sy = y1;
      dx = x2;
      dy = y2;

      if (sx > dx) {
	 t = sx;
	 sx = dx;
	 dx = t;
      }

      if (sy > dy) {
	 t = sy;
	 sy = dy;
	 dy = t;
      }

      if ((sx >= bmp->cr) || (sy >= bmp->cb) || (dx < bmp->cl) || (dy < bmp->ct))
	 return;

      if ((sx < bmp->cl) || (sy < bmp->ct) || (dx >= bmp->cr) || (dy >= bmp->cb)) {
	 orig_line(bmp, x1, y1, x2, y2, color);
	 return;
      }
   }

   SAFISH_CALL(
      go_accel();

      af_driver->DrawLine(af_driver, color,
			  itofix(x1+bmp->x_ofs), itofix(y1+bmp->y_ofs),
			  itofix(x2+bmp->x_ofs), itofix(y2+bmp->y_ofs));
   );
}



/* vbeaf_rectfill:
 *  Accelerated rectangle filling routine.
 */
static void vbeaf_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   if (x2 < x1) {
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (y2 < y1) {
      int tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
      if (x1 < bmp->cl)
	 x1 = bmp->cl;

      if (x2 >= bmp->cr)
	 x2 = bmp->cr-1;

      if (x2 < x1)
	 return;

      if (y1 < bmp->ct)
	 y1 = bmp->ct;

      if (y2 >= bmp->cb)
	 y2 = bmp->cb-1;

      if (y2 < y1)
	 return;
   }

   SAFISH_CALL(
      go_accel();

      switch (_drawing_mode) {

	 case DRAW_MODE_SOLID:
	 case DRAW_MODE_XOR:
	    /* mono rectangle fill */
	    af_driver->DrawRect(af_driver, color, 
				x1+bmp->x_ofs, y1+bmp->y_ofs, 
				x2-x1+1, y2-y1+1);
	    break;

	 case DRAW_MODE_COPY_PATTERN:
	    /* colored pattern rectangle fill */
	    prepare_color_pattern(bmp);

	    af_driver->DrawColorPattRect(af_driver,
					 x1+bmp->x_ofs, y1+bmp->y_ofs,
					 x2-x1+1, y2-y1+1);
	    break;

	 case DRAW_MODE_SOLID_PATTERN:
	 case DRAW_MODE_MASKED_PATTERN:
	    /* mono pattern rectangle fill */
	    prepare_mono_pattern(bmp);

	    af_driver->DrawPattRect(af_driver, color, 0,
				    x1+bmp->x_ofs, y1+bmp->y_ofs,
				    x2-x1+1, y2-y1+1);
	    break;
      }
   );
}



/* vbeaf_triangle:
 *  Hardware accelerated triangle drawing function.
 */
static void vbeaf_triangle(BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
   AF_TRAP trap;

   /* bounding box test */
   if (bmp->clip) {
      if ((x1 < bmp->cl) || (x2 < bmp->cl) || (x3 < bmp->cl)) {
         _soft_triangle(bmp, x1, y1, x2, y2, x3, y3, color);
	 return;
      }
      
      if ((y1 < bmp->ct) || (y2 < bmp->ct) || (y3 < bmp->ct)) {
         _soft_triangle(bmp, x1, y1, x2, y2, x3, y3, color);
	 return;
      }

      if ((x1 >= bmp->cr) || (x2 >= bmp->cr) || (x3 >= bmp->cr)) {
         _soft_triangle(bmp, x1, y1, x2, y2, x3, y3, color);
	 return;
      }

      if ((y1 >= bmp->cb) || (y2 >= bmp->cb) || (y3 >= bmp->cb)) {
         _soft_triangle(bmp, x1, y1, x2, y2, x3, y3, color);
	 return;
      }
   }

   /* sort along the vertical axis */
   #define TRI_SWAP(a, b)     \
   {                          \
      int tmp = x##a;         \
      x##a = x##b;            \
      x##b = tmp;             \
			      \
      tmp = y##a;             \
      y##a = y##b;            \
      y##b = tmp;             \
   }

   if (y2 < y1)
      TRI_SWAP(1, 2);

   if (y3 < y1) {
      TRI_SWAP(1, 3);
      TRI_SWAP(2, 3);
   }
   else if (y3 < y2)
      TRI_SWAP(2, 3);

   SAFISH_CALL(
      go_accel();

      if (y2 > y1) {
	 /* draw the top half of the triangle as one trapezoid */
	 trap.y = y1+bmp->y_ofs;
	 trap.count = y2-y1;
	 trap.x1 = trap.x2 = itofix(x1+bmp->x_ofs);
	 trap.slope1 = itofix(x2-x1) / (y2-y1);
	 trap.slope2 = itofix(x3-x1) / (y3-y1);

	 if (trap.slope1 < 0)
	    trap.x1 += MIN(trap.slope1+itofix(1), 0);

	 if (trap.slope2 < 0)
	    trap.x2 += MIN(trap.slope2+itofix(1), 0);

	 if (trap.slope1 > trap.slope2)
	    trap.x1 += MAX(ABS(trap.slope1), itofix(1));
	 else
	    trap.x2 += MAX(ABS(trap.slope2), itofix(1));

	 af_driver->DrawTrap(af_driver, color, &trap);

	 if (y3 > y2) {
	    /* draw the bottom half as a second trapezoid */
	    if (trap.slope1 < 0)
	       trap.x1 -= MIN(trap.slope1+itofix(1), 0);

	    if (trap.slope1 > trap.slope2)
	       trap.x1 -= MAX(ABS(trap.slope1), itofix(1));

	    trap.count = y3-y2;
	    trap.slope1 = itofix(x3-x2) / (y3-y2);

	    if (trap.slope1 < 0)
	       trap.x1 += MIN(trap.slope1+itofix(1), 0);

	    if (trap.x1 > trap.x2)
	       trap.x1 += MAX(ABS(trap.slope1), itofix(1));

	    af_driver->DrawTrap(af_driver, color, &trap);
	 }
      }
      else if (y3 > y2) {
	 /* we can draw the entire thing with a single trapezoid */
	 trap.y = y1+bmp->y_ofs;
	 trap.count = y3-y2;
	 trap.x1 = itofix(x2+bmp->x_ofs);
	 trap.x2 = itofix(x1+bmp->x_ofs);
	 trap.slope1 = itofix(x3-x2) / (y3-y2);
	 trap.slope2 = (y3 > y1) ? (itofix(x3-x1) / (y3-y1)) : 0;

	 if (trap.slope1 < 0)
	    trap.x1 += MIN(trap.slope1+itofix(1), 0);

	 if (trap.slope2 < 0)
	    trap.x2 += MIN(trap.slope2+itofix(1), 0);

	 if (trap.x1 > trap.x2)
	    trap.x1 += MAX(ABS(trap.slope1), itofix(1));
	 else
	    trap.x2 += MAX(ABS(trap.slope2), itofix(1));

	 af_driver->DrawTrap(af_driver, color, &trap);
      }
   );
}



/* vbeaf_draw_glyph:
 *  Monochrome text plotter.
 */
static void vbeaf_draw_glyph(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg)
{
   AL_CONST unsigned char *data = glyph->dat;
   int w = glyph->w;
   int h = glyph->h;
   int stride = (w+7)/8;
   int d;

   /* give up if we can't draw this */
   if ((x < bmp->cl) || (x+w >= bmp->cr) || ((w&7) && (bg >= 0))) {
      orig_draw_glyph(bmp, glyph, x, y, color, bg);
      return;
   }

   /* clip the top */
   if (y < bmp->ct) {
      d = bmp->ct - y;

      h -= d;
      if (h <= 0)
	 return;

      data += d*stride;
      y = bmp->ct;
   }

   /* clip the bottom */
   if (y+h >= bmp->cb) {
      h = bmp->cb - y;
      if (h <= 0)
	 return;
   }

   SAFE_CALL(
      /* set the mix mode */
      if (bg >= 0) {
	 af_driver->SetMix(af_driver, 0, 0);
	 d = bg;
      }
      else {
	 af_driver->SetMix(af_driver, 0, 4);
	 d = 0;
      }

      /* draw the letter */
      go_accel();

      af_driver->PutMonoImage(af_driver, color, d,
			      x+bmp->x_ofs, y+bmp->y_ofs,
			      stride, 0, 0, stride*8, h, (unsigned char *)data);

      af_driver->SetMix(af_driver, vbeaf_fg_mix, vbeaf_bg_mix);
   );
}



/* vbeaf_draw_sprite:
 *  Accelerated sprite drawing routine.
 */
static void vbeaf_draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int sx, sy, w, h;
   int pitch;

   if (((sprite->vtable == &_screen_vtable) && (af_driver->SrcTransBlt)) ||
       ((sprite->vtable != &_screen_vtable) && (af_driver->SrcTransBltSys))) {

      /* this sprite can be drawn in hardware */
      sx = sprite->x_ofs;
      sy = sprite->y_ofs;
      w = sprite->w;
      h = sprite->h;

      if (bmp->clip) {
	 if (x < bmp->cl) {
	    sx += bmp->cl-x;
	    w -= bmp->cl-x;
	    x = bmp->cl;
	 }

	 if (y < bmp->ct) {
	    sy += bmp->ct-y;
	    h -= bmp->ct-y;
	    y = bmp->ct;
	 }

	 if (x+w > bmp->cr)
	    w = bmp->cr-x;

	 if (w <= 0)
	    return;

	 if (y+h > bmp->cb)
	    h = bmp->cb-y;

	 if (h <= 0)
	    return;
      }

      if (sprite->vtable == &_screen_vtable) {
	 /* hardware blit within the video memory */
	 SAFISH_CALL(
	    go_accel();

	    af_driver->SrcTransBlt(af_driver, sx, sy, w, h,
				   x+bmp->x_ofs, y+bmp->y_ofs,
				   0, sprite->vtable->mask_color);
	 );
      }
      else {
	 /* hardware blit from system memory */
	 if (sprite->h > 1)
	    pitch = (long)sprite->line[1] - (long)sprite->line[0];
	 else
	    pitch = sprite->w;

	 SAFE_CALL(
	    go_accel();

	    af_driver->SrcTransBltSys(af_driver, 
				      sprite->line[0], pitch,
				      sx, sy, w, h,
				      x+bmp->x_ofs, y+bmp->y_ofs,
				      0, sprite->vtable->mask_color);
	 );
      }
   }
   else {
      /* have to use the original software version */
      orig_draw_sprite(bmp, sprite, x, y);
   }
}

END_OF_STATIC_FUNCTION(vbeaf_draw_sprite);



/* vbeaf_blit_from_memory:
 *  Accelerated system memory -> vram blitting routine.
 */
static void vbeaf_blit_from_memory(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   int pitch;

   if (source->h > 1)
      pitch = (long)source->line[1] - (long)source->line[0];
   else
      pitch = source->w;

   SAFE_CALL(
      go_accel();

      af_driver->BitBltSys(af_driver,
			   source->line[0], pitch,
			   source_x, source_y,
			   width, height,
			   dest_x+dest->x_ofs,
			   dest_y+dest->y_ofs,
			   0);
   );
}

END_OF_STATIC_FUNCTION(vbeaf_blit_from_memory);



/* vbeaf_blit_to_self:
 *  Accelerated vram -> vram blitting routine.
 */
static void vbeaf_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   SAFISH_CALL(
      go_accel();

      af_driver->BitBlt(af_driver,
			source_x+source->x_ofs, 
			source_y+source->y_ofs,
			width, height,
			dest_x+dest->x_ofs, 
			dest_y+dest->y_ofs,
			0);
   );
}



/* vbeaf_masked_blit:
 *  Accelerated masked blitting routine.
 */
static void vbeaf_masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   int pitch;

   if ((source->vtable == &_screen_vtable) && (af_driver->SrcTransBlt)) {
      /* hardware blit within the video memory */
      SAFISH_CALL(
	 go_accel();

	 af_driver->SrcTransBlt(af_driver,
				source_x+source->x_ofs,
				source_y+source->y_ofs,
				width, height,
				dest_x+dest->x_ofs,
				dest_y+dest->y_ofs,
				0, source->vtable->mask_color);
      );
   }
   else if (af_driver->SrcTransBltSys) {
      /* hardware blit from system memory */
      if (source->h > 1)
	 pitch = (long)source->line[1] - (long)source->line[0];
      else
	 pitch = source->w;

      SAFE_CALL(
	 go_accel();

	 af_driver->SrcTransBltSys(af_driver,
				   source->line[0], pitch,
				   source_x, source_y,
				   width, height,
				   dest_x+dest->x_ofs,
				   dest_y+dest->y_ofs,
				   0, source->vtable->mask_color);
      );
   }
   else {
      /* have to use the original software version */
      orig_masked_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);
   }
}



/* vbeaf_clear_to_color:
 *  Accelerated screen clear routine.
 */
static void vbeaf_clear_to_color(BITMAP *bitmap, int color)
{
   SAFISH_CALL(
      go_accel();

      if ((vbeaf_fg_mix != 0) || (vbeaf_bg_mix != 0))
	 af_driver->SetMix(af_driver, 0, 0);

      af_driver->DrawRect(af_driver, color, 
			  bitmap->cl+bitmap->x_ofs, 
			  bitmap->ct+bitmap->y_ofs, 
			  bitmap->cr-bitmap->cl, 
			  bitmap->cb-bitmap->ct);

      if ((vbeaf_fg_mix != 0) || (vbeaf_bg_mix != 0))
	 af_driver->SetMix(af_driver, vbeaf_fg_mix, vbeaf_bg_mix);
   );
}



#endif      /* ifdef VBE/AF is cool on this platform */
