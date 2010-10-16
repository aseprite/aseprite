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
 *      Some definitions for internal use by the Linux console code.
 *
 *      By George Foot.
 * 
 *      See readme.txt for copyright information.
 */

#ifndef AINTLNX_H
#define AINTLNX_H

#ifdef __cplusplus
extern "C" {
#endif


/**************************************/
/************ Driver lists ************/
/**************************************/

extern _DRIVER_INFO _linux_gfx_driver_list[];
extern _DRIVER_INFO _linux_keyboard_driver_list[];
extern _DRIVER_INFO _linux_mouse_driver_list[];
extern _DRIVER_INFO _linux_timer_driver_list[];
/* _linux_joystick_driver_list is in aintunix.h */


/****************************************/
/************ Memory mapping ************/ /* (src/linux/lmemory.c) */
/****************************************/

/* struct MAPPED_MEMORY:  Used to describe a block of memory mapped 
 *  into our address space (in particular, the video memory).
 */
struct MAPPED_MEMORY {
	unsigned int base, size;      /* linear address and size of block */
	int perms;                    /* PROT_READ | PROT_WRITE, etc */
	void *data;                   /* pointer to block after mapping */
};

extern int __al_linux_have_ioperms;

int __al_linux_init_memory (void);
int __al_linux_shutdown_memory (void);
int __al_linux_map_memory (struct MAPPED_MEMORY *info);
int __al_linux_unmap_memory (struct MAPPED_MEMORY *info);


/******************************************/
/************ Standard drivers ************/ /* (src/linux/lstddrv.c) */
/******************************************/

/* This "standard drivers" business is mostly a historical artifact.
 * It was highly over-engineered, now simplified.  It has been mostly
 * superseded by the newer, shinier, better bg_man.  But hey, at least
 * it didn't suffer the fate of its cousin lasyncio.c, now dead,
 * buried, and without a tombstone to show for it. --pw
 */

typedef struct STD_DRIVER {
   unsigned    type; /* One of the below STD_ constants */

   int (*update) (void);
   void (*resume) (void);
   void (*suspend) (void);

   int         fd;   /* Descriptor of the opened device */
} STD_DRIVER;

#define STD_MOUSE            0
#define STD_KBD              1

#define N_STD_DRIVERS        2

/* List of standard drivers */
extern STD_DRIVER *__al_linux_std_drivers[];

/* Exported functions */
int  __al_linux_add_standard_driver (STD_DRIVER *spec);
int  __al_linux_remove_standard_driver (STD_DRIVER *spec);
void __al_linux_update_standard_drivers (int threaded);
void __al_linux_suspend_standard_drivers (void);
void __al_linux_resume_standard_drivers (void);


/******************************************/
/************ Console routines ************/ /* (src/linux/lconsole.c) */
/******************************************/

#define N_CRTC_REGS  24
#define N_ATC_REGS   21
#define N_GC_REGS    9
#define N_SEQ_REGS   5

#define MISC_REG_R       0x03CC
#define MISC_REG_W       0x03C2
#define ATC_REG_IW       0x03C0
#define ATC_REG_R        0x03C1
#define GC_REG_I         0x03CE
#define GC_REG_RW        0x03CF
#define SEQ_REG_I        0x03C4
#define SEQ_REG_RW       0x03C5
#define PEL_REG_IW       0x03C8
#define PEL_REG_IR       0x03C7
#define PEL_REG_D        0x03C9

#define _is1             0x03DA

#define ATC_DELAY        10 /* microseconds - for usleep() */

#define VGA_MEMORY_BASE  0xA0000
#define VGA_MEMORY_SIZE  0x10000
#define VGA_FONT_SIZE    0x02000

/* This structure is also used for video state saving/restoring, therefore it
 * contains fields that are used only when saving/restoring the text mode. */
typedef struct MODE_REGISTERS
{
   unsigned char crt[N_CRTC_REGS];
   unsigned char seq[N_SEQ_REGS];
   unsigned char atc[N_ATC_REGS];
   unsigned char gc[N_GC_REGS];
   unsigned char misc;
   unsigned char *ext;
   unsigned short ext_count;
   unsigned char *text_font1;
   unsigned char *text_font2;
   unsigned long flags;
   union {
      unsigned char vga[768];
      PALETTE allegro;
   } palette;
} MODE_REGISTERS;

extern int __al_linux_vt;
extern int __al_linux_console_fd;
extern int __al_linux_prev_vt;
extern int __al_linux_got_text_message;
extern struct termios __al_linux_startup_termio;
extern struct termios __al_linux_work_termio;

int __al_linux_use_console (void);
int __al_linux_leave_console (void);

int __al_linux_console_graphics (void);
int __al_linux_console_text (void);

int __al_linux_wait_for_display (void);


/*************************************/
/************ VGA helpers ************/ /* (src/linux/lvgahelp.c) */
/*************************************/

int __al_linux_init_vga_helpers (void);
int __al_linux_shutdown_vga_helpers (void);

void __al_linux_screen_off (void);
void __al_linux_screen_on (void);

void __al_linux_clear_vram (void);

void __al_linux_set_vga_regs (MODE_REGISTERS *regs);
void __al_linux_get_vga_regs (MODE_REGISTERS *regs);

void __al_linux_save_gfx_mode (void);
void __al_linux_restore_gfx_mode (void);
void __al_linux_save_text_mode (void);
void __al_linux_restore_text_mode (void);

#define __just_a_moment() usleep(ATC_DELAY)


/**************************************/
/************ VT switching ************/ /* (src/linux/vtswitch.c) */
/**************************************/

/* signals for VT switching */
#define SIGRELVT        SIGUSR1
#define SIGACQVT        SIGUSR2

int __al_linux_init_vtswitch (void);
int __al_linux_done_vtswitch (void);

void __al_linux_acquire_bitmap (BITMAP *bmp);
void __al_linux_release_bitmap (BITMAP *bmp);

int __al_linux_set_display_switch_mode (int mode);
void __al_linux_display_switch_lock (int lock, int foreground);

extern volatile int __al_linux_switching_blocked;


/**************************************/
/************ Mode setting ************/ /* (src/linux/lgraph.c) */
/**************************************/

typedef struct GFX_MODE_INFO {
	int w,h,c;   /* width, height, colour depth */
	int id;      /* ID code, for driver's reference */
	void *data;  /* data for driver's use in setting the mode */
} GFX_MODE_INFO;

BITMAP *__al_linux_gfx_mode_set_helper (
	int w, int h, int v_w, int v_h, int c,
	GFX_DRIVER *driver, GFX_MODE_INFO *mode,
	int (*set_video_mode) (GFX_MODE_INFO *mode),
	void (*set_width) (int w)
);


/*******************************/
/************ Mouse ************/ /* (src/linux/lmouse.c) */
/*******************************/

typedef struct INTERNAL_MOUSE_DRIVER {
   int device;
   int (*process) (unsigned char *buf, int buf_size);
   int num_buttons;
} INTERNAL_MOUSE_DRIVER;

int  __al_linux_mouse_init (INTERNAL_MOUSE_DRIVER *drv);
void __al_linux_mouse_exit (void);
void __al_linux_mouse_position (int x, int y);
void __al_linux_mouse_set_range (int x1, int y_1, int x2, int y2);
void __al_linux_mouse_set_speed (int xspeed, int yspeed);
void __al_linux_mouse_get_mickeys (int *mickeyx, int *mickeyy);
void __al_linux_mouse_handler (int x, int y, int z, int b);


#ifdef __cplusplus
}
#endif

/* VGA register access helpers */
/* This is conditional because configure may have disabled VGA support */
#ifdef ALLEGRO_LINUX_VGA
   #include "allegro/internal/aintern.h"
   #include "allegro/internal/aintvga.h"
#endif

/* Functions for querying the framebuffer, for the fbcon driver */
#if (defined ALLEGRO_LINUX_FBCON) && (!defined ALLEGRO_WITH_MODULES)
   extern int __al_linux_get_fb_color_depth(void);
   extern int __al_linux_get_fb_resolution(int *width, int *height);
#endif

#endif /* ifndef AINTLNX_H */

