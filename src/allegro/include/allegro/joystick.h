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
 *      Joystick routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_JOYSTICK_H
#define ALLEGRO_JOYSTICK_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define JOY_TYPE_AUTODETECT      -1
#define JOY_TYPE_NONE            0


#define MAX_JOYSTICKS            8
#define MAX_JOYSTICK_AXIS        3
#define MAX_JOYSTICK_STICKS      5
#define MAX_JOYSTICK_BUTTONS     32


/* information about a single joystick axis */
typedef struct JOYSTICK_AXIS_INFO
{
   int pos;
   int d1, d2;
   AL_CONST char *name;
} JOYSTICK_AXIS_INFO;


/* information about one or more axis (a slider or directional control) */
typedef struct JOYSTICK_STICK_INFO
{
   int flags;
   int num_axis;
   JOYSTICK_AXIS_INFO axis[MAX_JOYSTICK_AXIS];
   AL_CONST char *name;
} JOYSTICK_STICK_INFO;


/* information about a joystick button */
typedef struct JOYSTICK_BUTTON_INFO
{
   int b;
   AL_CONST char *name;
} JOYSTICK_BUTTON_INFO;


/* information about an entire joystick */
typedef struct JOYSTICK_INFO
{
   int flags;
   int num_sticks;
   int num_buttons;
   JOYSTICK_STICK_INFO stick[MAX_JOYSTICK_STICKS];
   JOYSTICK_BUTTON_INFO button[MAX_JOYSTICK_BUTTONS];
} JOYSTICK_INFO;


/* joystick status flags */
#define JOYFLAG_DIGITAL             1
#define JOYFLAG_ANALOGUE            2
#define JOYFLAG_CALIB_DIGITAL       4
#define JOYFLAG_CALIB_ANALOGUE      8
#define JOYFLAG_CALIBRATE           16
#define JOYFLAG_SIGNED              32
#define JOYFLAG_UNSIGNED            64


/* alternative spellings */
#define JOYFLAG_ANALOG              JOYFLAG_ANALOGUE
#define JOYFLAG_CALIB_ANALOG        JOYFLAG_CALIB_ANALOGUE


/* global joystick information */
AL_ARRAY(JOYSTICK_INFO, joy);
AL_VAR(int, num_joysticks);


typedef struct JOYSTICK_DRIVER         /* driver for reading joystick input */
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(int, init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(int, poll, (void));
   AL_METHOD(int, save_data, (void));
   AL_METHOD(int, load_data, (void));
   AL_METHOD(AL_CONST char *, calibrate_name, (int n));
   AL_METHOD(int, calibrate, (int n));
} JOYSTICK_DRIVER;


AL_VAR(JOYSTICK_DRIVER, joystick_none);
AL_VAR(JOYSTICK_DRIVER *, joystick_driver);
AL_ARRAY(_DRIVER_INFO, _joystick_driver_list);


/* macros for constructing the driver list */
#define BEGIN_JOYSTICK_DRIVER_LIST                             \
   _DRIVER_INFO _joystick_driver_list[] =                      \
   {

#define END_JOYSTICK_DRIVER_LIST                               \
      {  JOY_TYPE_NONE,    &joystick_none,      TRUE  },       \
      {  0,                NULL,                0     }        \
   };


AL_FUNC(int, install_joystick, (int type));
AL_FUNC(void, remove_joystick, (void));

AL_FUNC(int, poll_joystick, (void));

AL_FUNC(int, save_joystick_data, (AL_CONST char *filename));
AL_FUNC(int, load_joystick_data, (AL_CONST char *filename));

AL_FUNC(AL_CONST char *, calibrate_joystick_name, (int n));
AL_FUNC(int, calibrate_joystick, (int n));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_JOYSTICK_H */


