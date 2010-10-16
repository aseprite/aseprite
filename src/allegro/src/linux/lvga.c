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
 *      Helpers for setting VGA modes in Linux.
 *
 *      By George Foot, based on code by Marek Habersack.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "allegro.h"

#if ((defined ALLEGRO_LINUX_VGA) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE)))

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintvga.h"



static MODE_REGISTERS mode13h = /* 320x200x8bpp NI */
{
		/* CRTC registers (24: 0-23) */
	{       0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 
		0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3 },
		/* Sequencer registers (5: 54-58) */
	{                                           0x03, 0x01,
		0x0F, 0x00, 0x0E },
		/* ATC registers (21: 24-44) */
	{       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x41, 0x00, 0x0F, 0x00, 0x00 },
		/* GC registers (9: 45-53) */
	{                                     0x00, 0x00, 0x00,
		0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF },
		/* misc (1: 59-59) */
				  0x63,
		/* null pointer to extra register block; extra reg count */
	NULL, 0, 
		/* text font data */
	NULL, NULL,
		/* flags */
	0L,
		/* palette (unused) */
	{ { 0 } }
};


static MODE_REGISTERS mode10h = /* 640x350x4bpp NI */
{
		/* CRTC registers (24: 0-23) */
	{       0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 
		0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x83, 0x85, 0x5D, 0x28, 0x0F, 0x63, 0xBA, 0xE3 },
		/* Sequencer registers (5: 54-58) */
	{                                           0x03, 0x01, 
		0x0F, 0x00, 0x06 },
		/* ATC registers (21: 24-44) */
	{       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x01, 0x00, 0x0F, 0x00, 0x00 },
		/* GC registers (9: 45-53) */
	{                                     0x00, 0x0F, 0x00, 
		0x20, 0x00, 0x00, 0x05, 0x0F, 0xFF },
		/* misc (1: 59-59) */
				  0xA3,
		/* null pointer to extra register block; extra reg count */
	NULL, 0,
		/* text font data */
	NULL, NULL,
		/* flags */
	0L,
		/* palette (unused) */
	{ { 0 } }
};


static MODE_REGISTERS mode0Dh = /* 320x200x4bpp NI */
{
		/* CRTC registers (24: 0-23) */
	{       0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F, 
		0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x9C, 0x8E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xE3 },
		/* Sequencer registers (5: 54-58) */
	{                                           0x03, 0x09, 
		0x0F, 0x00, 0x06 },
		/* ATC registers (21: 24-44) */
	{       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
		0x01, 0x00, 0x0F, 0x00, 0x00 },
		/* GC registers (9: 45-53) */
	{                                     0x00, 0x0F, 0x00, 
		0x20, 0x00, 0x00, 0x05, 0x0F, 0xFF },
		/* misc (1: 59-59) */
				  0x63,
		/* null pointer to extra register block; extra reg count */
	NULL, 0,
		/* text font data */
	NULL, NULL,
		/* flags */
	0L,
		/* palette (unused) */
	{ { 0 } }
};



static struct MAPPED_MEMORY vram = {
	0xa0000, 0x10000,
	PROT_READ | PROT_WRITE,
	NULL
};


/* _set_vga_mode:
 *  Helper for the VGA and mode-X drivers to set a video mode.
 */
uintptr_t _set_vga_mode (int modenum)
{
	MODE_REGISTERS *regs;

	if (!__al_linux_have_ioperms) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("This driver needs root privileges"));
		return 0;
	}

	switch (modenum) {
		case 0x13: regs = &mode13h; break;
		case 0x10: regs = &mode10h; break;
		case 0x0D: regs = &mode0Dh; break;
		default:
			ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Required VGA mode not supported"));
			return 0;
	}

	if (__al_linux_map_memory (&vram)) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to map video memory"));
		return 0;
	}

	__al_linux_screen_off();
	if (__al_linux_console_graphics()) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Error setting VGA video mode"));
		__al_linux_screen_on();
		__al_linux_unmap_memory (&vram);
		return 0;
	}

	__al_linux_save_text_mode();
	__al_linux_set_vga_regs (regs);
	__al_linux_clear_vram();
	__al_linux_screen_on();
	return (unsigned long)vram.data;
}


/* _unset_vga_mode:
 *  Helper for the VGA and mode-X drivers to unset the video mode.
 */
void _unset_vga_mode (void)
{
	__al_linux_unmap_memory (&vram);
	__al_linux_screen_off();
	__al_linux_restore_text_mode();
	__al_linux_console_text();
	__al_linux_screen_on();
}


static MODE_REGISTERS gfx_regs;

void _save_vga_mode (void)
{
   __al_linux_screen_off();
   get_palette (gfx_regs.palette.allegro);
   __al_linux_get_vga_regs (&gfx_regs);
   __al_linux_restore_text_mode();
   __al_linux_console_text();
   __al_linux_screen_on();
}

void _restore_vga_mode (void)
{
   __al_linux_screen_off();
   __al_linux_save_text_mode();
   __al_linux_console_graphics();
   __al_linux_set_vga_regs (&gfx_regs);
   set_palette (gfx_regs.palette.allegro);
   __al_linux_screen_on();
}



#endif      /* if ((defined ALLEGRO_LINUX_VGA) && ... */
