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
 *      Internal header file for the PSP Allegro library port.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTPSP_H
#define AINTPSP_H


/* For accelerated blitting support .*/
AL_FUNC(BITMAP *, psp_create_bitmap, (int color_depth, int width, int height));
AL_FUNC(int, psp_destroy_bitmap, (BITMAP *bitmap));

/* Extra bitmap info. */
#define BMP_EXTRA(bmp)         ((BMP_EXTRA_INFO *)((bmp)->extra))

typedef struct BMP_EXTRA_INFO
{
   int pitch;
   /* For video bitmaps. */
   int size;
   uintptr_t hw_addr;
} BMP_EXTRA_INFO;

#define ALIGN_TO(v,n)          ((v + n-1) & ~(n-1))


/* For 8 bpp support. */
AL_VAR(GFX_VTABLE, psp_vtable8);
AL_FUNC(void, psp_do_stretch_blit8, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked));

/* The video bitmap actually displayed on the PSP. */
BITMAP *displayed_video_bitmap;

AL_FUNC(void, psp_draw_to_screen, (void));


/* Video memory manager stuff. */
#define VMM_NO_MEM             ((uintptr_t)0)

typedef struct VRAM_HOLE
{
   uintptr_t h_base;
   unsigned int h_len;
   struct VRAM_HOLE *h_next;
} VRAM_HOLE;

AL_FUNC(void, vmm_init, (uintptr_t base, unsigned int available_vram));
AL_FUNC(uintptr_t, vmm_alloc_mem, (unsigned int size));
AL_FUNC(void, vmm_free_mem, (uintptr_t base, unsigned int size));


/* PSP controller stuff. */
#define SAMPLING_CYCLE 0
#define SAMPLING_MODE  PSP_CTRL_MODE_DIGITAL

AL_FUNC(void, _psp_init_controller, (int cycle, int mode));

#endif

