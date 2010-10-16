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
 *      MacAllegro resources this file is requiered to all
 *      applications linked with allegro.x!!!
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "SysTypes.r"
#include "Types.r"
#include "allegro/internal/macdef.h"

/*You Can modify this item*/
/*Control of Version*/

resource 'vers'(1){0x01, 0x00, release, 0x00,verUS, 
	"1.00","1.00, Copyright © ????.???? ??????"		/*Which look at GetInfo from Finder*/
	};

/*Please not modify the itens bellow*/
/*Definition of macallegro menssage box*/

resource 'ALRT' (rmac_message, purgeable){	
	{0, 0, 320, 480},rmac_message,{
		OK, visible, silent,
		OK, visible, silent,
		OK, visible, silent,
		OK, visible, silent
		},centerMainScreen
	};
resource 'DITL' (rmac_message, purgeable) {
	{
		{280, 360, 300, 460},Button {enabled,"OK"},
		{10, 60, 260, 460},StaticText {disabled,"^0"},
		{8, 8, 40, 40},Icon {disabled,2}
		}
	};


/*Error strings not used yet*/
resource 'STR#' (rerror_str, purgeable) {
	{	"Failed memory allocation";
		"undefined error";
		}
	};

/*The main application resource*/
resource 'SIZE' (-1) {
	dontSaveScreen,
	acceptSuspendResumeEvents,
	enableOptionSwitch,
	canBackground,
	multiFinderAware,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	reserved, reserved, reserved, reserved,
	reserved, reserved, reserved,
	kprefsize*1024,		/*pref size of memory required kprefsize is in defined macdef.h and is used in macallegro.c*/
	kminsize*1024		/*min size of memory required kminsize is defined in macdef.h and is used in macallegro.c*/
	};
