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
 *      PSP specific header defines.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALPSP_H
#define ALPSP_H

#ifndef ALLEGRO_PSP
   #error bad include
#endif


/* System driver */
#define SYSTEM_PSP              AL_ID('P','S','P',' ')
AL_VAR(SYSTEM_DRIVER, system_psp);

/* Timer driver */
#define TIMER_PSP               AL_ID('P','S','P','T')
AL_VAR(TIMER_DRIVER, timer_psp);

/* Keyboard driver */
#define KEYSIM_PSP              AL_ID('P','S','P','K')
AL_VAR(KEYBOARD_DRIVER, keybd_simulator_psp);

/* Mouse drivers */
#define MOUSE_PSP               AL_ID('P','S','P','M')
AL_VAR(MOUSE_DRIVER, mouse_psp);

/* Gfx driver */
#define GFX_PSP                 AL_ID('P','S','P','G')
AL_VAR(GFX_DRIVER, gfx_psp);

/* Digital sound driver */
#define DIGI_PSP                AL_ID('P','S','P','S')
AL_VAR(DIGI_DRIVER, digi_psp);

/* Joystick drivers */
#define JOYSTICK_PSP            AL_ID('P','S','P','J')
AL_VAR(JOYSTICK_DRIVER, joystick_psp);

#endif
