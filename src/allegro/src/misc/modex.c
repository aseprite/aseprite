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
 *      Video driver for VGA tweaked modes (aka mode-X).
 *
 *      By Shawn Hargreaves.
 *
 *      Jonathan Tarbox wrote the mode set code.
 *
 *      TBD/FeR added the 320x600 and 360x600 modes.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"

#ifdef ALLEGRO_GFX_HAS_VGA

#include "allegro/internal/aintern.h"
#include "allegro/internal/aintvga.h"

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

#include "modexsms.h"

#if (!defined ALLEGRO_LINUX) || ((defined ALLEGRO_LINUX_VGA) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE)))



void _x_draw_sprite_end(void);
void _x_blit_from_memory_end(void);
void _x_blit_to_memory_end(void);

static void really_split_modex_screen(int line);



/* table of functions for drawing onto the mode-X screen */
static GFX_VTABLE __modex_vtable =
{
   8,
   MASK_COLOR_8,
   _x_unbank_switch,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _x_getpixel,
   _x_putpixel,
   _x_vline,
   _x_hline,
   _x_hline,
   _normal_line,
   _fast_line,
   _normal_rectfill,
   _soft_triangle,
   _x_draw_sprite,
   _x_draw_sprite,
   _x_draw_sprite_v_flip,
   _x_draw_sprite_h_flip,
   _x_draw_sprite_vh_flip,
   _x_draw_trans_sprite,
   NULL,
   _x_draw_lit_sprite,
   _x_draw_rle_sprite,
   _x_draw_trans_rle_sprite,
   NULL,
   _x_draw_lit_rle_sprite,
   _x_draw_character,
   _x_draw_glyph,
   _x_blit_from_memory,
   _x_blit_to_memory,
   _x_blit_from_memory,
   _x_blit_to_memory,
   _x_blit,
   _x_blit_forward,
   _x_blit_backward,
   _blit_between_formats,
   _x_masked_blit,
   _x_clear_to_color,
   _pivot_scaled_sprite_flip,
   NULL,    /* AL_METHOD(void, do_stretch_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked)); */
   _soft_draw_gouraud_sprite,
   _x_draw_sprite_end,
   _x_blit_from_memory_end,
   _soft_polygon,
   _soft_rect,
   _soft_circle,
   _soft_circlefill,
   _soft_ellipse,
   _soft_ellipsefill,
   _soft_arc,
   _soft_spline,
   _soft_floodfill,

   _soft_polygon3d,
   _soft_polygon3d_f,
   _soft_triangle3d,
   _soft_triangle3d_f,
   _soft_quad3d,
   _soft_quad3d_f
};



static BITMAP *modex_init(int w, int h, int v_w, int v_h, int color_depth);
static void modex_exit(BITMAP *b);
static int modex_scroll(int x, int y);
static int request_modex_scroll(int x, int y);
static int poll_modex_scroll(void);
static void modex_enable_triple_buffer(void);
static GFX_MODE_LIST *modex_fetch_mode_list(void);



GFX_DRIVER gfx_modex =
{
   GFX_MODEX,
   empty_string,
   empty_string,
   "Mode-X",
   modex_init,
   modex_exit,
   modex_scroll,
   _vga_vsync,
   _vga_set_palette_range,
   NULL, NULL,                   /* no triple buffering (yet) */
   modex_enable_triple_buffer,
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   _save_vga_mode,
   _restore_vga_mode,
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   modex_fetch_mode_list,
   0, 0,
   TRUE,
   0, 0,
   0x40000,
   0,
   FALSE
};



static GFX_MODE modex_gfx_modes[] = {
   { 256, 200, 8 },
   { 256, 224, 8 },
   { 256, 240, 8 },
   { 256, 256, 8 },
   { 320, 200, 8 },
   { 320, 240, 8 },
   { 320, 350, 8 },
   { 320, 400, 8 },
   { 320, 480, 8 },
   { 320, 600, 8 },
   { 360, 200, 8 },
   { 360, 240, 8 },
   { 360, 270, 8 },
   { 360, 360, 8 },
   { 360, 400, 8 },
   { 360, 480, 8 },
   { 360, 600, 8 },
   { 376, 282, 8 },
   { 376, 308, 8 },
   { 376, 564, 8 },
   { 400, 150, 8 },
   { 400, 300, 8 },
   { 400, 600, 8 },
   { 0,   0,   0 }
};


#ifdef GFX_XTENDED


static BITMAP *xtended_init(int w, int h, int v_w, int v_h, int color_depth);
static GFX_MODE_LIST *xtended_fetch_mode_list(void);


GFX_DRIVER gfx_xtended =
{
   GFX_XTENDED,
   empty_string,
   empty_string,
   "Xtended mode",
   xtended_init,
   modex_exit,
   NULL,                         /* no hardware scrolling */
   _vga_vsync,
   _vga_set_palette_range,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   NULL, NULL,                   /* no state saving */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   xtended_fetch_mode_list,
   640, 400,
   TRUE,
   0, 0,
   0x40000,
   0,
   FALSE
};



GFX_MODE xtended_gfx_modes[] = {
   { 640, 400, 8 },
   { 0,   0,   0 }
};



#endif      /* GFX_XTENDED */



/* VGA register contents for the various tweaked modes */
typedef struct VGA_REGISTER
{
   unsigned short port;
   unsigned char index;
   unsigned char value;
} VGA_REGISTER;



static VGA_REGISTER mode_256x200[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4E },
   { 0x3D4, 0x5,  0x96 },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_256x224[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4A },
   { 0x3D4, 0x5,  0x9A },  { 0x3D4, 0x6,  0xB  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xDA },
   { 0x3D4, 0x11, 0x9C },  { 0x3D4, 0x12, 0xBF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xC7 },  { 0x3D4, 0x16, 0x4  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_256x240[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4E },
   { 0x3D4, 0x5,  0x96 },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_256x256[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x3F },
   { 0x3D4, 0x2,  0x40 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x4A },
   { 0x3D4, 0x5,  0x9A },  { 0x3D4, 0x6,  0x23 },  { 0x3D4, 0x7,  0xB2 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x61 },  { 0x3D4, 0x10, 0xA  },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xFF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x7  },  { 0x3D4, 0x16, 0x1A },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x200[] =
{
   { 0x3C2, 0x0,  0x63 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x240[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x400[] =
{
   { 0x3C2, 0x0,  0x63 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x480[] =
{
   { 0x3C2, 0x0,  0xE3 },  { 0x3D4, 0x0,  0x5F },  { 0x3D4, 0x1,  0x4F },
   { 0x3D4, 0x2,  0x50 },  { 0x3D4, 0x3,  0x82 },  { 0x3D4, 0x4,  0x54 },
   { 0x3D4, 0x5,  0x80 },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_320x600[] =
{
   { 0x3C2, 0x00, 0xE7 },  { 0x3D4, 0x00, 0x5F },  { 0x3D4, 0x01, 0x4F },
   { 0x3D4, 0x02, 0x50 },  { 0x3D4, 0x03, 0x82 },  { 0x3D4, 0x04, 0x54 },
   { 0x3D4, 0x05, 0x80 },  { 0x3D4, 0x06, 0x70 },  { 0x3D4, 0x07, 0xF0 },
   { 0x3D4, 0x08, 0x00 },  { 0x3D4, 0x09, 0x60 },  { 0x3D4, 0x10, 0x5B },
   { 0x3D4, 0x11, 0x8C },  { 0x3D4, 0x12, 0x57 },  { 0x3D4, 0x13, 0x28 },
   { 0x3D4, 0x14, 0x00 },  { 0x3D4, 0x15, 0x58 },  { 0x3D4, 0x16, 0x70 },
   { 0x3D4, 0x17, 0xE3 },  { 0x3C4, 0x01, 0x01 },  { 0x3C4, 0x04, 0x06 },
   { 0x3CE, 0x05, 0x40 },  { 0x3CE, 0x06, 0x05 },  { 0x3C0, 0x10, 0x41 },
   { 0x3C0, 0x13, 0x00 },  { 0,     0,    0    }
};



static VGA_REGISTER mode_360x200[] =
{
   { 0x3C2, 0x0,  0x67 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x240[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x41 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x270[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0x30 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x61 },  { 0x3D4, 0x10, 0x20 },
   { 0x3D4, 0x11, 0xA9 },  { 0x3D4, 0x12, 0x1B },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x1F },  { 0x3D4, 0x16, 0x2F },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x360[] =
{
   { 0x3C2, 0x0,  0x67 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x88 },
   { 0x3D4, 0x11, 0x85 },  { 0x3D4, 0x12, 0x67 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x6D },  { 0x3D4, 0x16, 0xBA },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x400[] =
{
   { 0x3C2, 0x0,  0x67 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xBF },  { 0x3D4, 0x7,  0x1F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x9C },
   { 0x3D4, 0x11, 0x8E },  { 0x3D4, 0x12, 0x8F },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x96 },  { 0x3D4, 0x16, 0xB9 },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x480[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6B },  { 0x3D4, 0x1,  0x59 },
   { 0x3D4, 0x2,  0x5A },  { 0x3D4, 0x3,  0x8E },  { 0x3D4, 0x4,  0x5E },
   { 0x3D4, 0x5,  0x8A },  { 0x3D4, 0x6,  0xD  },  { 0x3D4, 0x7,  0x3E },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0xEA },
   { 0x3D4, 0x11, 0xAC },  { 0x3D4, 0x12, 0xDF },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0xE7 },  { 0x3D4, 0x16, 0x6  },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_360x600[] =
{
   { 0x3C2, 0x00, 0xE7 },  { 0x3D4, 0x00, 0x5F },  { 0x3D4, 0x01, 0x59 },
   { 0x3D4, 0x02, 0x50 },  { 0x3D4, 0x03, 0x82 },  { 0x3D4, 0x04, 0x54 },
   { 0x3D4, 0x05, 0x80 },  { 0x3D4, 0x06, 0x70 },  { 0x3D4, 0x07, 0xF0 },
   { 0x3D4, 0x08, 0x00 },  { 0x3D4, 0x09, 0x60 },  { 0x3D4, 0x10, 0x5B },
   { 0x3D4, 0x11, 0x8C },  { 0x3D4, 0x12, 0x57 },  { 0x3D4, 0x13, 0x2D },
   { 0x3D4, 0x14, 0x00 },  { 0x3D4, 0x15, 0x58 },  { 0x3D4, 0x16, 0x70 },
   { 0x3D4, 0x17, 0xE3 },  { 0x3C4, 0x01, 0x01 },  { 0x3C4, 0x03, 0x00 },
   { 0x3C4, 0x04, 0x06 },  { 0x3CE, 0x05, 0x40 },  { 0x3CE, 0x06, 0x05 },
   { 0x3C0, 0x10, 0x41 },  { 0x3C0, 0x11, 0x00 },  { 0x3C0, 0x12, 0x0F },
   { 0x3C0, 0x13, 0x00 },  { 0x3C0, 0x14, 0x00 },  { 0,     0,    0    }
};



static VGA_REGISTER mode_376x282[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6E },  { 0x3D4, 0x1,  0x5D },
   { 0x3D4, 0x2,  0x5E },  { 0x3D4, 0x3,  0x91 },  { 0x3D4, 0x4,  0x62 },
   { 0x3D4, 0x5,  0x8F },  { 0x3D4, 0x6,  0x62 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x61 },  { 0x3D4, 0x10, 0x37 },
   { 0x3D4, 0x11, 0x89 },  { 0x3D4, 0x12, 0x33 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x3C },  { 0x3D4, 0x16, 0x5C },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_376x308[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6E },  { 0x3D4, 0x1,  0x5D },
   { 0x3D4, 0x2,  0x5E },  { 0x3D4, 0x3,  0x91 },  { 0x3D4, 0x4,  0x62 },
   { 0x3D4, 0x5,  0x8F },  { 0x3D4, 0x6,  0x62 },  { 0x3D4, 0x7,  0x0F },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x40 },  { 0x3D4, 0x10, 0x37 },
   { 0x3D4, 0x11, 0x89 },  { 0x3D4, 0x12, 0x33 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x3C },  { 0x3D4, 0x16, 0x5C },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_376x564[] =
{
   { 0x3C2, 0x0,  0xE7 },  { 0x3D4, 0x0,  0x6E },  { 0x3D4, 0x1,  0x5D },
   { 0x3D4, 0x2,  0x5E },  { 0x3D4, 0x3,  0x91 },  { 0x3D4, 0x4,  0x62 },
   { 0x3D4, 0x5,  0x8F },  { 0x3D4, 0x6,  0x62 },  { 0x3D4, 0x7,  0xF0 },
   { 0x3D4, 0x8,  0x0  },  { 0x3D4, 0x9,  0x60 },  { 0x3D4, 0x10, 0x37 },
   { 0x3D4, 0x11, 0x89 },  { 0x3D4, 0x12, 0x33 },  { 0x3D4, 0x14, 0x0  }, 
   { 0x3D4, 0x15, 0x3C },  { 0x3D4, 0x16, 0x5C },  { 0x3D4, 0x17, 0xE3 }, 
   { 0x3C4, 0x1,  0x1  },  { 0x3CE, 0x5,  0x40 },  { 0x3CE, 0x6,  0x5  }, 
   { 0x3C0, 0x10, 0x41 },  { 0,     0,    0    } 
};



static VGA_REGISTER mode_notweak[] =
{
   { 0,     0,    0    }
};



/* information about a mode-X resolution */
typedef struct TWEAKED_MODE
{
   int w, h;
   VGA_REGISTER *regs;
   int parent;
   int hrs;
   int shift;
   int repeat;
} TWEAKED_MODE;



static TWEAKED_MODE xmodes[] =
{
   {  256,  200,  mode_256x200,  0x13, 0, 0, 0  },
   {  256,  224,  mode_256x224,  0x13, 0, 0, 0  },
   {  256,  240,  mode_256x240,  0x13, 0, 0, 0  },
   {  256,  256,  mode_256x256,  0x13, 0, 0, 0  },
   {  320,  200,  mode_320x200,  0x13, 0, 0, 0  },
   {  320,  240,  mode_320x240,  0x13, 0, 0, 0  },
   {  320,  350,  mode_notweak,  0x10, 1, 1, 0  },
   {  320,  400,  mode_320x400,  0x13, 0, 0, 0  },
   {  320,  480,  mode_320x480,  0x13, 0, 0, 0  },
   {  320,  600,  mode_320x600,  0x13, 0, 0, 0  },
   {  360,  200,  mode_360x200,  0x13, 0, 0, 0  },
   {  360,  240,  mode_360x240,  0x13, 0, 0, 0  },
   {  360,  270,  mode_360x270,  0x13, 0, 0, 0  },
   {  360,  360,  mode_360x360,  0x13, 0, 0, 0  },
   {  360,  400,  mode_360x400,  0x13, 0, 0, 0  },
   {  360,  480,  mode_360x480,  0x13, 0, 0, 0  },
   {  360,  600,  mode_360x600,  0x13, 0, 0, 0  },
   {  376,  282,  mode_376x282,  0x13, 0, 0, 0  },
   {  376,  308,  mode_376x308,  0x13, 0, 0, 0  },
   {  376,  564,  mode_376x564,  0x13, 0, 0, 0  },
   {  400,  150,  mode_notweak,  0x6A, 1, 1, 3  },
   {  400,  300,  mode_notweak,  0x6A, 1, 1, 1  },
   {  400,  600,  mode_notweak,  0x6A, 1, 1, 0  },
   {  0,    0,    NULL,          0,    0, 0, 0  }
};



/* lookup tables for use by modexgfx.s */
char _x_left_mask_table[] = { 0x00, 0x08, 0x0C, 0x0E, 0x0F };
char _x_right_mask_table[] = { 0x00, 0x01, 0x03, 0x07, 0x0F };



/* memory buffer for emulating linear access to a mode-X screen */
unsigned long _x_magic_buffer_addr = 0;
unsigned long _x_magic_screen_addr = 0;
int _x_magic_screen_width = 0;

#ifdef ALLEGRO_DOS
   static int magic_sel = 0;
#endif



/* setup_x_magic:
 *  To make sure that things will go on working properly even if a few
 *  bits of code don't know about mode-X screens, we do special magic
 *  inside the bank switch functions to emulate a linear image surface.
 *  Whenever someone tries to access the screen directly, these routines
 *  copy the planar image into a temporary buffer, let the client write
 *  there, and then automatically refresh the screen with the updated
 *  graphics. Very slow, but it makes 100% sure that everything will work.
 */
static void setup_x_magic(BITMAP *b)
{
   #ifdef ALLEGRO_DOS

      /* DOS buffer has to go in conventional memory */
      int seg = __dpmi_allocate_dos_memory((b->w+15)/16, &magic_sel);

      if (seg > 0)
	 _x_magic_buffer_addr = seg*16;
      else
	 _x_magic_buffer_addr = 0;

   #else

      /* other platforms can put the buffer wherever they like */
      _x_magic_buffer_addr = (unsigned long)_AL_MALLOC(b->w);

   #endif

   _x_magic_screen_addr = 0;
   _x_magic_screen_width = 0;

   LOCK_VARIABLE(_x_magic_buffer_addr);
   LOCK_VARIABLE(_x_magic_screen_addr);
   LOCK_VARIABLE(_x_magic_screen_width);
   LOCK_FUNCTION(_x_bank_switch);

   b->write_bank = b->read_bank = _x_bank_switch;
}



/* modex_exit:
 *  Frees the magic bank switch buffer.
 */
static void modex_exit(BITMAP *b)
{
   #ifdef ALLEGRO_DOS

      /* free conventional memory buffer */
      if (_x_magic_buffer_addr) {
	 __dpmi_free_dos_memory(magic_sel);
	 magic_sel = 0;
	 _x_magic_buffer_addr = 0;
      }

   #else

      /* free normal memory buffer */
      if (_x_magic_buffer_addr) {
	 _AL_FREE((void *)_x_magic_buffer_addr);
	 _x_magic_buffer_addr = 0;
      }

   #endif
   _unset_vga_mode();

   /* see modexsms.c */
   _split_modex_screen_ptr = NULL;
}



/* modex_init:
 *  Selects a tweaked VGA mode and creates a screen bitmap for it.
 */
static BITMAP *modex_init(int w, int h, int v_w, int v_h, int color_depth)
{
   TWEAKED_MODE *mode = xmodes;
   VGA_REGISTER *reg;
   BITMAP *b;
   unsigned long addr;
   int c;

   /* Do not continue if this version of Allegro was built in C-only mode.
    * The bank switchers assume asm-mode calling conventions, but the
    * library would try to call them with C calling conventions.
    */
#ifdef ALLEGRO_NO_ASM
   return NULL;
#endif

   /* see modexsms.c */
   _split_modex_screen_ptr = really_split_modex_screen;

   /* check it is a valid resolution */
   if (color_depth != 8) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Mode-X only supports 8 bit color"));
      return NULL;
   }

   /* search the mode table for the requested resolution */
   while ((mode->regs) && ((mode->w != w) || (mode->h != h)))
      mode++;

   if (!mode->regs) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not a VGA mode-X resolution"));
      return NULL;
   }

   /* round up virtual size if required (v_w must be a multiple of eight) */
   v_w = MAX(w, (v_w+7) & 0xFFF8);
   v_h = MAX(h, v_h);

   if (v_h > 0x40000/v_w) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Virtual screen size too large"));
      return NULL;
   }

   /* calculate virtual height = vram / width */
   v_h = 0x40000 / v_w;

   /* lock everything that is used to draw mouse pointers */
   LOCK_VARIABLE(__modex_vtable); 
   LOCK_FUNCTION(_x_draw_sprite);
   LOCK_FUNCTION(_x_blit_from_memory);
   LOCK_FUNCTION(_x_blit_to_memory);

   /* set the base video mode, then start tweaking things */
   addr = _set_vga_mode(mode->parent);
   if (!addr)
      return NULL;

   outportw(0x3C4, 0x0100);                     /* synchronous reset */

   outportb(0x3D4, 0x11);                       /* enable crtc regs 0-7 */
   outportb(0x3D5, inportb(0x3D5) & 0x7F);

   outportw(0x3C4, 0x0604);                     /* disable chain-4 */

   for (reg=mode->regs; reg->port; reg++) {     /* set the VGA registers */
      if (reg->port == 0x3C0) {
	 inportb(0x3DA);
	 outportb(0x3C0, reg->index | 0x20);
	 outportb(0x3C0, reg->value);
      }
      else if (reg->port == 0x3C2) {
	 outportb(reg->port, reg->value);
      }
      else {
	 outportb(reg->port, reg->index); 
	 outportb(reg->port+1, reg->value);
      }
   }

   if (mode->hrs) {
      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)&0x7F);
      outportb(0x3D4, 0x04);
      outportb(0x3D5, inportb(0x3D5)+mode->hrs);
      outportb(0x3D4, 0x11);
      outportb(0x3D5,inportb(0x3D5)|0x80);
   }

   if (mode->shift) {
      outportb(0x3CE, 0x05);
      outportb(0x3CF, (inportb(0x3CF)&0x60)|0x40);

      inportb(0x3DA);
      outportb(0x3C0, 0x30);
      outportb(0x3C0, inportb(0x3C1)|0x40);

      for (c=0; c<16; c++) {
	 outportb(0x3C0, c);
	 outportb(0x3C0, c);
      }
      outportb(0x3C0, 0x20);
   }

   if (mode->repeat) {
      outportb(0x3D4, 0x09);
      outportb(0x3D5,(inportb(0x3D5)&0x60)|mode->repeat);
   }

   outportb(0x3D4, 0x13);                       /* set scanline length */
   outportb(0x3D5, v_w/8);

   outportw(0x3C4, 0x0300);                     /* restart sequencer */

   /* We only use 1/4th of the real width for the bitmap line pointers,
    * so they can be used directly for writing to vram, eg. the address
    * for pixel(x,y) is bmp->line[y]+(x/4). Divide the x coordinate, but
    * not the line pointer. The clipping position and bitmap width are 
    * stored in the linear pixel format, though, not as mode-X planes.
    */
   b = _make_bitmap(v_w/4, v_h, addr, &gfx_modex, 8, v_w/4);
   if (!b)
      return NULL;

   b->w = b->cr = v_w;
   b->vtable = &__modex_vtable;
   b->id |= BMP_ID_PLANAR;

   setup_x_magic(b);

   gfx_modex.w = w;
   gfx_modex.h = h;

   if (_timer_use_retrace) {
      gfx_modex.request_scroll = request_modex_scroll;
      gfx_modex.poll_scroll = poll_modex_scroll;
   }
   else {
      gfx_modex.request_scroll = NULL;
      gfx_modex.poll_scroll = NULL;
   }

   #ifdef ALLEGRO_LINUX

      b->vtable->acquire = __al_linux_acquire_bitmap;
      b->vtable->release = __al_linux_release_bitmap;

   #endif

   return b;
}



#ifdef GFX_XTENDED


/* xtended_init:
 *  Selects the unchained 640x400 mode.
 */
static BITMAP *xtended_init(int w, int h, int v_w, int v_h, int color_depth)
{
   unsigned long addr;
   BITMAP *b;

   /* Do not continue if this version of Allegro was built in C-only mode.
    * The bank switchers assume asm-mode calling conventions, but the
    * library would try to call them with C calling conventions.
    */
#ifdef ALLEGRO_NO_ASM
   return NULL;
#endif

   /* see modexsms.c */
   _split_modex_screen_ptr = really_split_modex_screen;

   /* check it is a valid resolution */
   if (color_depth != 8) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Xtended mode only supports 8 bit color"));
      return NULL;
   }

   if ((w != 640) || (h != 400) || (v_w > 640) || (v_h > 400)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Xtended mode only supports 640x400"));
      return NULL;
   }

   /* lock everything that is used to draw mouse pointers */
   LOCK_VARIABLE(__modex_vtable); 
   LOCK_FUNCTION(_x_draw_sprite);
   LOCK_FUNCTION(_x_blit_from_memory);
   LOCK_FUNCTION(_x_blit_to_memory);

   /* set VESA mode 0x100 */
   addr = _set_vga_mode(0x100);

   if (!addr) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VESA mode 0x100 not available"));
      return NULL;
   }

   outportw(0x3C4, 0x0604);         /* disable chain-4 */

   /* we only use 1/4th of the width for the bitmap line pointers */
   b = _make_bitmap(640/4, 400, addr, &gfx_xtended, 8, 640/4);
   if (!b)
      return NULL;

   b->w = b->cr = 640;
   b->vtable = &__modex_vtable;
   b->id |= BMP_ID_PLANAR;

   setup_x_magic(b);

   return b;
}



/* xtended_fetch_mode_list:
 *  Creates a list of of currently implemented Xtended modes.
 */
static GFX_MODE_LIST *xtended_fetch_mode_list(void)
{
   GFX_MODE_LIST *mode_list;

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list) return NULL;
   
   mode_list->mode = _AL_MALLOC(sizeof(xtended_gfx_modes));
   if (!mode_list->mode) return NULL;

   memcpy(mode_list->mode, xtended_gfx_modes, sizeof(xtended_gfx_modes));
   mode_list->num_modes = sizeof(xtended_gfx_modes) / sizeof(GFX_MODE) - 1;

   return mode_list;
}

#endif      /* GFX_XTENDED */



/* modex_scroll:
 *  Hardware scrolling routine for mode-X.
 */
static int modex_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   /* write low 2 bits to horizontal pan register */
   _write_hpp((a&3)<<1);

   return 0;
}



/* request_modex_scroll:
 *  Requests a scroll but doesn't wait for it to be competed: just writes
 *  to some VGA registers and some variables used by the vertical retrace
 *  interrupt simulator, and then returns immediately. The scroll will
 *  actually take place during the next vertical retrace, so this function
 *  can be used together with poll_modex_scroll to implement triple buffered
 *  animation systems (see examples/ex20.c).
 */
static int request_modex_scroll(int x, int y)
{
   long a = x + (y * VIRTUAL_W);

   DISABLE();

   _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, (a>>2) & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>10) & 0xFF);

   ENABLE();

   if (_timer_use_retrace) {
      /* store low 2 bits where the timer handler will find them */
      _retrace_hpp_value = (a&3)<<1;
   }
   else {
      /* change instantly */
      _write_hpp((a&3)<<1);
   }

   return 0;
}



/* poll_modex_scroll:
 *  Returns non-zero if there is a scroll waiting to take place, previously
 *  set by request_modex_scroll(). Only works if vertical retrace interrupts 
 *  are enabled.
 */
static int poll_modex_scroll(void)
{
   if ((_retrace_hpp_value < 0) || (!_timer_use_retrace))
      return FALSE;

   return TRUE;
}



/* modex_enable_triple_buffer:
 *  Tries to turn on triple buffering mode, if that is currently possible.
 */
static void modex_enable_triple_buffer(void)
{
   if ((!_timer_use_retrace) && (timer_can_simulate_retrace()))
      timer_simulate_retrace(TRUE);

   if (_timer_use_retrace) {
      gfx_modex.request_scroll = request_modex_scroll;
      gfx_modex.poll_scroll = poll_modex_scroll;
   }
}



/* x_write:
 *  Inline helper for the C drawing functions: writes a pixel onto a
 *  mode-X bitmap, performing clipping but ignoring the drawing mode.
 */
static INLINE void x_write(BITMAP *bmp, int x, int y, int color)
{
   if ((x >= bmp->cl) && (x < bmp->cr) && (y >= bmp->ct) && (y < bmp->cb)) {
      outportw(0x3C4, (0x100<<(x&3))|2);
      bmp_write8((unsigned long)bmp->line[y]+(x>>2), color);
   }
}



/* _x_vline:
 *  Draws a vertical line onto a mode-X screen.
 */
void _x_vline(BITMAP *bmp, int x, int y1, int y2, int color)
{
   int c;

   if (y1 > y2) {
      c = y1;
      y1 = y2;
      y2 = c;
   }

   for (c=y1; c<=y2; c++)
      _x_putpixel(bmp, x, c, color);
}



/* _x_draw_sprite_v_flip:
 *  Draws a vertically flipped sprite onto a mode-X screen.
 */
void _x_draw_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int c1, c2;

   bmp_select(bmp);

   for (c1=0; c1<sprite->h; c1++)
      for (c2=0; c2<sprite->w; c2++)
	 if (sprite->line[sprite->h-1-c1][c2])
	    x_write(bmp, x+c2, y+c1, sprite->line[sprite->h-1-c1][c2]);
}



/* _x_draw_sprite_h_flip:
 *  Draws a horizontally flipped sprite onto a mode-X screen.
 */
void _x_draw_sprite_h_flip(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int c1, c2;

   bmp_select(bmp);

   for (c1=0; c1<sprite->h; c1++)
      for (c2=0; c2<sprite->w; c2++)
	 if (sprite->line[c1][sprite->w-1-c2])
	    x_write(bmp, x+c2, y+c1, sprite->line[c1][sprite->w-1-c2]);
}



/* _x_draw_sprite_vh_flip:
 *  Draws a diagonally flipped sprite onto a mode-X screen.
 */
void _x_draw_sprite_vh_flip(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int c1, c2;

   bmp_select(bmp);

   for (c1=0; c1<sprite->h; c1++)
      for (c2=0; c2<sprite->w; c2++)
	 if (sprite->line[sprite->h-1-c1][sprite->w-1-c2])
	    x_write(bmp, x+c2, y+c1, sprite->line[sprite->h-1-c1][sprite->w-1-c2]);
}



/* _x_draw_trans_sprite:
 *  Draws a translucent sprite onto a mode-X screen.
 */
void _x_draw_trans_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int sx, sy, sy2;
   int dx, dy;
   int xlen, ylen;
   int plane;
   unsigned char *src;
   unsigned long addr;

   sx = sy = 0;

   if (bmp->clip) {
      if (x < bmp->cl) {
	 sx = bmp->cl - x;
	 x = bmp->cl;
      }
      if (y < bmp->ct) {
	 sy = bmp->ct - y;
	 y = bmp->ct;
      }
      xlen = MIN(sprite->w - sx, bmp->cr - x);
      ylen = MIN(sprite->h - sy, bmp->cb - y);
   }
   else {
      xlen = sprite->w;
      ylen = sprite->h;
   }

   if ((xlen <= 0) || (ylen <= 0))
      return;

   bmp_select(bmp);

   sy2 = sy;

   for (plane=0; plane<4; plane++) {
      sy = sy2;

      outportw(0x3C4, (0x100<<((x+plane)&3))|2);
      outportw(0x3CE, (((x+plane)&3)<<8)|4);

      for (dy=0; dy<ylen; dy++) {
	 src = sprite->line[sy]+sx+plane;
	 addr = (unsigned long)bmp->line[y+dy]+((x+plane)>>2);

	 for (dx=plane; dx<xlen; dx+=4) {
	    bmp_write8(addr, color_map->data[*src][bmp_read8(addr)]);

	    addr++;
	    src+=4;
	 }

	 sy++;
      }
   }
}



/* _x_draw_lit_sprite:
 *  Draws a lit sprite onto a mode-X screen.
 */
void _x_draw_lit_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int color)
{
   int sx, sy, sy2;
   int dx, dy;
   int xlen, ylen;
   int plane;
   unsigned char *src;
   unsigned long addr;

   sx = sy = 0;

   if (bmp->clip) {
      if (x < bmp->cl) {
	 sx = bmp->cl - x;
	 x = bmp->cl;
      }
      if (y < bmp->ct) {
	 sy = bmp->ct - y;
	 y = bmp->ct;
      }
      xlen = MIN(sprite->w - sx, bmp->cr - x);
      ylen = MIN(sprite->h - sy, bmp->cb - y);
   }
   else {
      xlen = sprite->w;
      ylen = sprite->h;
   }

   if ((xlen <= 0) || (ylen <= 0))
      return;

   bmp_select(bmp);

   sy2 = sy;

   for (plane=0; plane<4; plane++) {
      sy = sy2;

      outportw(0x3C4, (0x100<<((x+plane)&3))|2);

      for (dy=0; dy<ylen; dy++) {
	 src = sprite->line[sy]+sx+plane;
	 addr = (unsigned long)bmp->line[y+dy]+((x+plane)>>2);

	 for (dx=plane; dx<xlen; dx+=4) {
	    if (*src)
	       bmp_write8(addr, color_map->data[color][*src]);

	    addr++;
	    src+=4;
	 }

	 sy++;
      }
   }
}



/* _x_draw_rle_sprite:
 *  Draws an RLE sprite onto a mode-X screen.
 */
void _x_draw_rle_sprite(BITMAP *bmp, AL_CONST RLE_SPRITE *sprite, int x, int y)
{
   AL_CONST signed char *p = sprite->dat;
   int c;
   int x_pos, y_pos;
   int lgap, width;
   unsigned long addr;

   bmp_select(bmp);
   y_pos = 0;

   /* clip on the top */
   while (y+y_pos < bmp->ct) {
      y_pos++;
      if ((y_pos >= sprite->h) || (y+y_pos >= bmp->cb))
	 return;

      while (*p)
	 p++;

      p++; 
   }

   /* x axis clip */
   lgap = MAX(bmp->cl-x, 0);
   width = MIN(sprite->w, bmp->cr-x);
   if (width <= lgap)
      return;

   /* draw it */
   while ((y_pos<sprite->h) && (y+y_pos < bmp->cb)) {
      addr = (unsigned long)bmp->line[y+y_pos];
      x_pos = 0;
      c = *(p++);

      /* skip pixels on the left */
      while (x_pos < lgap) {
	 if (c > 0) {
	    /* skip a run of solid pixels */
	    if (c > lgap-x_pos) {
	       p += lgap-x_pos;
	       c -= lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos += c;
	    p += c;
	 }
	 else {
	    /* skip a run of zeros */
	    if (-c > lgap-x_pos) {
	       c += lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos -= c;
	 }

	 c = *(p++);
      }

      /* draw a line of the sprite */
      for (;;) {
	 if (c > 0) {
	    /* draw a run of solid pixels */
	    c = MIN(c, width-x_pos);
	    while (c > 0) {
	       outportw(0x3C4, (0x100<<((x+x_pos)&3))|2);
	       bmp_write8(addr+((x+x_pos)>>2), *p);
	       x_pos++;
	       p++;
	       c--;
	    }
	 }
	 else {
	    /* skip a run of zeros */
	    x_pos -= c;
	 }

	 if (x_pos < width)
	    c = (*p++);
	 else
	    break;
      }

      /* skip pixels on the right */
      if (x_pos < sprite->w) {
	 while (*p)
	    p++;

	 p++;
      }

      y_pos++;
   }
}



/* _x_draw_trans_rle_sprite:
 *  Draws an RLE sprite onto a mode-X screen.
 */
void _x_draw_trans_rle_sprite(BITMAP *bmp, AL_CONST RLE_SPRITE *sprite, int x, int y)
{
   AL_CONST signed char *p = sprite->dat;
   int c;
   int x_pos, y_pos;
   int lgap, width;
   unsigned long addr, a;

   bmp_select(bmp);
   y_pos = 0;

   /* clip on the top */
   while (y+y_pos < bmp->ct) {
      y_pos++;
      if ((y_pos >= sprite->h) || (y+y_pos >= bmp->cb))
	 return;

      while (*p)
	 p++;

      p++; 
   }

   /* x axis clip */
   lgap = MAX(bmp->cl-x, 0);
   width = MIN(sprite->w, bmp->cr-x);
   if (width <= lgap)
      return;

   /* draw it */
   while ((y_pos<sprite->h) && (y+y_pos < bmp->cb)) {
      addr = (unsigned long)bmp->line[y+y_pos];
      x_pos = 0;
      c = *(p++);

      /* skip pixels on the left */
      while (x_pos < lgap) {
	 if (c > 0) {
	    /* skip a run of solid pixels */
	    if (c > lgap-x_pos) {
	       p += lgap-x_pos;
	       c -= lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos += c;
	    p += c;
	 }
	 else {
	    /* skip a run of zeros */
	    if (-c > lgap-x_pos) {
	       c += lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos -= c;
	 }

	 c = *(p++);
      }

      /* draw a line of the sprite */
      for (;;) {
	 if (c > 0) {
	    /* draw a run of solid pixels */
	    c = MIN(c, width-x_pos);
	    while (c > 0) {
	       outportw(0x3C4, (0x100<<((x+x_pos)&3))|2);
	       outportw(0x3CE, (((x+x_pos)&3)<<8)|4);
	       a = addr+((x+x_pos)>>2);
	       bmp_write8(a, color_map->data[*p][bmp_read8(a)]);
	       x_pos++;
	       p++;
	       c--;
	    }
	 }
	 else {
	    /* skip a run of zeros */
	    x_pos -= c;
	 }

	 if (x_pos < width)
	    c = (*p++);
	 else
	    break;
      }

      /* skip pixels on the right */
      if (x_pos < sprite->w) {
	 while (*p)
	    p++;

	 p++;
      }

      y_pos++;
   }
}



/* _x_draw_lit_rle_sprite:
 *  Draws a tinted RLE sprite onto a mode-X screen.
 */
void _x_draw_lit_rle_sprite(BITMAP *bmp, AL_CONST RLE_SPRITE *sprite, int x, int y, int color)
{
   AL_CONST signed char *p = sprite->dat;
   int c;
   int x_pos, y_pos;
   int lgap, width;
   unsigned long addr;

   bmp_select(bmp);
   y_pos = 0;

   /* clip on the top */
   while (y+y_pos < bmp->ct) {
      y_pos++;
      if ((y_pos >= sprite->h) || (y+y_pos >= bmp->cb))
	 return;

      while (*p)
	 p++;

      p++; 
   }

   /* x axis clip */
   lgap = MAX(bmp->cl-x, 0);
   width = MIN(sprite->w, bmp->cr-x);
   if (width <= lgap)
      return;

   /* draw it */
   while ((y_pos<sprite->h) && (y+y_pos < bmp->cb)) {
      addr = (unsigned long)bmp->line[y+y_pos];
      x_pos = 0;
      c = *(p++);

      /* skip pixels on the left */
      while (x_pos < lgap) {
	 if (c > 0) {
	    /* skip a run of solid pixels */
	    if (c > lgap-x_pos) {
	       p += lgap-x_pos;
	       c -= lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos += c;
	    p += c;
	 }
	 else {
	    /* skip a run of zeros */
	    if (-c > lgap-x_pos) {
	       c += lgap-x_pos;
	       x_pos = lgap;
	       break;
	    }

	    x_pos -= c;
	 }

	 c = *(p++);
      }

      /* draw a line of the sprite */
      for (;;) {
	 if (c > 0) {
	    /* draw a run of solid pixels */
	    c = MIN(c, width-x_pos);
	    while (c > 0) {
	       outportw(0x3C4, (0x100<<((x+x_pos)&3))|2);
	       bmp_write8(addr+((x+x_pos)>>2), color_map->data[color][*p]);
	       x_pos++;
	       p++;
	       c--;
	    }
	 }
	 else {
	    /* skip a run of zeros */
	    x_pos -= c;
	 }

	 if (x_pos < width)
	    c = (*p++);
	 else
	    break;
      }

      /* skip pixels on the right */
      if (x_pos < sprite->w) {
	 while (*p)
	    p++;

	 p++;
      }

      y_pos++;
   }
}



/* _x_draw_character:
 *  Draws a character from a proportional font onto a mode-X screen.
 */
void _x_draw_character(BITMAP *bmp, BITMAP *sprite, int x, int y, int color, int bg)
{
   int c1, c2;

   bmp_select(bmp);

   for (c1=0; c1<sprite->h; c1++) {
      for (c2=0; c2<sprite->w; c2++) {
	 if (sprite->line[c1][c2])
	    x_write(bmp, x+c2, y+c1, color);
	 else {
	    if (bg >= 0)
	       x_write(bmp, x+c2, y+c1, bg);
	 }
      }
   }
}



/* _x_draw_glyph:
 *  Draws monochrome text onto a mode-X screen.
 */
void _x_draw_glyph(BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color, int bg)
{
   AL_CONST unsigned char *data = glyph->dat;
   AL_CONST unsigned char *dat;
   unsigned long addr;
   int w = glyph->w;
   int h = glyph->h;
   int stride = (w+7)/8;
   int lgap = 0;
   int plane, d, i, j, k;

   if (bmp->clip) {
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

      /* clip the left */
      if (x < bmp->cl) {
	 d = bmp->cl - x;

	 w -= d;
	 if (w <= 0)
	    return;

	 data += d/8;
	 lgap = d&7;
	 x = bmp->cl;
      }

      /* clip the right */
      if (x+w >= bmp->cr) {
	 w = bmp->cr - x;
	 if (w <= 0)
	    return;
      }
   }

   /* draw it */
   bmp_select(bmp);

   for (plane=0; plane < MIN(w, 4); plane++) {
      outportw(0x3C4, (0x100<<((x+plane)&3))|2);
      dat = data;

      for (j=0; j<h; j++) {
	 addr = (unsigned long)bmp->line[y+j]+((x+plane)>>2);
	 d = dat[(plane+lgap)/8];
	 k = 0x80 >> ((plane+lgap)&7);
	 i = plane;

	 for (;;) {
	    if (d & k)
	       bmp_write8(addr, color);
	    else if (bg >= 0)
	       bmp_write8(addr, bg);

	    i += 4;
	    if (i >= w)
	       break;

	    k >>= 4;
	    if (!k) {
	       d = dat[(i+lgap)/8];
	       k = 0x80 >> ((i+lgap)&7);
	    }

	    addr++;
	 }

	 dat += stride;
      }
   }
}



/* modex_fetch_mode_list:
 *  Creates a list of of currently implemented ModeX modes.
 */
static GFX_MODE_LIST *modex_fetch_mode_list(void)
{
   GFX_MODE_LIST *mode_list;

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list) return NULL;

   mode_list->mode = _AL_MALLOC(sizeof(modex_gfx_modes));
   if (!mode_list->mode) return NULL;

   memcpy(mode_list->mode, modex_gfx_modes, sizeof(modex_gfx_modes));
   mode_list->num_modes = sizeof(modex_gfx_modes) / sizeof(GFX_MODE) - 1;

   return mode_list;
}



/* really_split_modex_screen:
 *  Enables a horizontal split screen at the specified line.
 *  Based on code from Paul Fenwick's XLIBDJ, which was in turn based 
 *  on Michael Abrash's routines in PC Techniques, June 1991.
 */
static void really_split_modex_screen(int line)
{
   if (gfx_driver != &gfx_modex)
      return;

   if (line < 0)
      line = 0;
   else if (line >= SCREEN_H)
      line = 0;

   _screen_split_position = line;

   /* adjust the line for double-scanned modes */
   if (SCREEN_H <= 150)
      line <<= 2;
   else if (SCREEN_H <= 300)
      line <<= 1;

   /* disable panning of the split screen area */
   _alter_vga_register(0x3C0, 0x30, 0x20, 0x20);

   /* set the line compare registers */
   _write_vga_register(0x3D4, 0x18, (line-1) & 0xFF);
   _alter_vga_register(0x3D4, 7, 0x10, ((line-1) & 0x100) >> 4);
   _alter_vga_register(0x3D4, 9, 0x40, ((line-1) & 0x200) >> 3);
}



#endif      /* (!defined ALLEGRO_LINUX) || ((defined ALLEGRO_LINUX_VGA) && ... */


#if (defined ALLEGRO_LINUX_VGA) && (defined ALLEGRO_MODULE)

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init_modex(int system_driver)
{
   if (system_driver == SYSTEM_LINUX)
      _unix_register_gfx_driver(GFX_MODEX, &gfx_modex, TRUE, FALSE);
}

#endif	    /* (defined ALLEGRO_LINUX_VGA) && (defined ALLEGRO_MODULE) */


#endif	    /* ALLEGRO_GFX_HAS_VGA */
