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
 *      Mac-specific header defines.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */


/*******************************************/
/************* system drivers **************/
/*******************************************/
#define SYSTEM_MACOS           AL_ID('M','C','O','S')
AL_VAR(SYSTEM_DRIVER, system_macos);

/*******************************************/
/************** timer drivers **************/
/*******************************************/
#define TIMER_MACOS           AL_ID('M','C','O','S')
AL_VAR(TIMER_DRIVER, timer_macos);

/*******************************************/
/************ keyboard drivers *************/
/*******************************************/
#define KEYBOARD_MACOS       AL_ID('M','C','O','S')
AL_VAR(KEYBOARD_DRIVER, keyboard_macos);
#define KEYBOARD_ADB         AL_ID('M','A','D','B')
AL_VAR(KEYBOARD_DRIVER, keyboard_adb);

/*******************************************/
/************* mouse drivers ***************/
/*******************************************/
#define MOUSE_MACOS          AL_ID('M','C','O','S')
AL_VAR(MOUSE_DRIVER, mouse_macos);
#define MOUSE_ADB            AL_ID('M','A','D','B')
AL_VAR(MOUSE_DRIVER, mouse_adb);


/*******************************************/
/*************** gfx drivers ***************/
/*******************************************/
#define GFX_DRAWSPROCKET     AL_ID('D','S','P',' ')
AL_VAR(GFX_DRIVER,gfx_drawsprocket);


/********************************************/
/*************** sound drivers **************/
/********************************************/
#define DIGI_MACOS            AL_ID('M','C','O','S')
AL_VAR(DIGI_DRIVER, digi_macos);

#define MIDI_QUICKTIME        AL_ID('Q','T',' ',' ')
AL_VAR(MIDI_DRIVER, midi_quicktime);

/*******************************************/
/************ joystick drivers *************/
/*******************************************/
/*no yet */

