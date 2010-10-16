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
 *      MacOs header file for the Allegro library.
 *      This should be included by everyone and everything.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */


#ifndef MAC_ALLEGRO_H
#define MAC_ALLEGRO_H

#ifndef ALLEGRO_H
#error Please include allegro.h before macalleg.h!
#endif

#if (!defined SCAN_EXPORT) && (!defined SCAN_DEPEND)
#endif

#include <MacTypes.h>		//theses includes aways are needed in normal mac programs
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Controls.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Devices.h>
#include <Events.h> 
#include <Scrap.h>
#include <Errors.h>
#include <errno.h>
#include <Script.h>
#include <Memory.h>
#include <Events.h>
#include <Sound.h>

#include <ToolUtils.h>
#include <TextUtils.h>
#include <NumberFormatting.h>
#include <FixMath.h>
#include <fp.h>
#include <Traps.h>

#include <stdio.h>

#include <Files.h>
#include <StandardFile.h>
#include <Timer.h>
#include <DrawSprocket.h>
#include <Sound.h>

#include "allegro/platform/macdef.h"


#ifdef __cplusplus
   extern "C" {
#endif

typedef BITMAP BITMAP;

typedef struct{
	CGrafPtr cg;
	long	rowBytes;
	unsigned char* first;
	unsigned char* last;
	long	flags;
	int		cachealigned;
	Ptr		data;
	}mac_bitmap;

#define MEMORY_ALIGN 4
#define GETMACBITMAP(bmp)((mac_bitmap*)(bmp->extra))

#pragma options align=mac68k
typedef struct mac_sample{
	short		formatType;
	short		numberSyns;
	short		idFirstSynth;
	long		INIToptions;
	short		numCommands;
	short		command1;
	short		param1;
	long		param2;
	SoundHeader	s;
	}mac_sample;
typedef mac_sample **hmac_sample;
#pragma options align=reset

typedef struct mac_voice{
	SndChannelPtr channel;
	SndListHandle sample;
	short headsize;
	long headeroff;
	int loop;
	int loop_start;
	int loop_end;
	int vol;
	int pan;
	int frequency;
	SAMPLE * al_sample;
	}mac_voice;


AL_VAR(QDGlobals , qd);
AL_VAR(GFX_VTABLE,__mac_sys_vtable8);
AL_VAR(GFX_VTABLE,__mac_sys_vtable15);
AL_VAR(GFX_VTABLE,__mac_sys_vtable24);

extern void _mac_lock(void *address, unsigned long size);
extern void _mac_unlock(void *address, unsigned long size);

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef MAC_ALLEGRO_H */


