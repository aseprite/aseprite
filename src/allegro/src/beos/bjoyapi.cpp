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
 *      Joystick driver routines for BeOS.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif            

static BJoystick *be_joy = NULL;
static int32 num_devices, num_axes, num_hats, num_buttons;
static int16 *axis_value = NULL;
static int8 *hat_value = NULL;



/* be_joy_init:
 *  Initializes BeOS joystick driver.
 */
extern "C" int be_joy_init(void)
{
   const char *device_config;
   char device_name[B_OS_NAME_LENGTH + 1];
   static char desc[30];
   static char name_x[10];
   static char name_y[10];
   static char name_stick[] = "stick";
   static char name_hat[] = "hat";
   static char name_throttle[] = "throttle";
   static char name_hat_lr[] = "left/right";
   static char name_hat_ud[] = "up/down";
   static char *name_b[] =
   {"B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8"};
   int32 i, j, stick;
   BString *temp;
   char *name;
   
   temp = new BString;
   be_joy = new BJoystick;
   num_devices = be_joy->CountDevices();
   
   /* no joysticks available */
   if (num_devices == 0) {
      goto cleanup;
   }
   
   /* Scans if the joystick_device config variable is set */
   device_config = get_config_string("joystick", "joystick_device", "");
   
   /* Let's try to open selected device */
   if ((device_config[0] == '\0') || (be_joy->Open(device_config) < 0)) {
      /* ok, let's try to open first available device */
      if (be_joy->GetDeviceName(0, device_name) != B_OK) {
         goto cleanup;
      }
      if (be_joy->Open(device_name) == B_ERROR) {
         goto cleanup;
      }
   }
   be_joy->GetControllerName(temp);
   name = temp->LockBuffer(0);
   _al_sane_strncpy(desc, name, sizeof(desc));
   temp->UnlockBuffer();
   joystick_beos.desc = desc;

   num_axes = be_joy->CountAxes();
   num_hats = be_joy->CountHats();
   num_buttons = be_joy->CountButtons();
   if (num_axes) {
      axis_value = (int16 *)malloc(sizeof(int16) * num_axes);
      hat_value = (int8 *)malloc(sizeof(int8) * num_axes);
   }
   
   num_joysticks = be_joy->CountSticks();
   for (i = 0; i < num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE;
      
      stick = 0;
      if (num_axes >= 2) {
         joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
         joy[i].stick[0].num_axis = 2;
         joy[i].stick[0].name = name_stick;
         be_joy->GetAxisNameAt(0, temp);
         name = temp->LockBuffer(0);
         _al_sane_strncpy(name_x, name, sizeof(name_x));
         temp->UnlockBuffer();
         joy[i].stick[0].axis[0].name = name_x;
         be_joy->GetAxisNameAt(1, temp);
         name = temp->LockBuffer(0);
         _al_sane_strncpy(name_y, name, sizeof(name_y));
         temp->UnlockBuffer();
         joy[i].stick[0].axis[1].name = name_y;
         
         stick++;
         for (j = 2; j < num_axes; j++) {
            joy[i].stick[stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
            joy[i].stick[stick].num_axis = 1;
            joy[i].stick[stick].axis[0].name = "";
            joy[i].stick[stick].name = name_throttle;
            stick++;
         }
         for (j = 0; j < num_hats; j++) {
            joy[i].stick[stick].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
            joy[i].stick[stick].num_axis = 2;
            joy[i].stick[stick].axis[0].name = name_hat_lr;
            joy[i].stick[stick].axis[1].name = name_hat_ud;
            joy[i].stick[stick].name = name_hat;
            stick++;
         }
      }
      joy[i].num_sticks = stick;
      joy[i].num_buttons = num_buttons;
      
      for (j = 0; j < num_buttons; j++)
         joy[i].button[j].name = name_b[j];
   }
   
   delete temp;
   be_joy_poll();
   return 0;
   
   cleanup: {
       delete temp;
       delete be_joy;
       return -1;
   } 
}



/* be_joy_exit:
 *  Closes joystick driver.
 */
extern "C" void be_joy_exit(void)
{
   if (be_joy) {
      delete be_joy;
      be_joy = NULL;
   }
   if (axis_value) {
      free(axis_value);
      axis_value = NULL;
   }
   if (hat_value) {
      free(hat_value);
      hat_value = NULL;
   }
}



/* be_joy_poll:
 *  Polls joysticks status.
 */
extern "C" int be_joy_poll(void)
{
   int32 i, j, k;
   int32 axis, hat, stick, buttons;
   
   be_joy->Update();
   
   for (i = 0; i < num_joysticks; i++) {
      axis = hat = stick = 0;
      be_joy->GetAxisValues(axis_value, i);
      for (j = 0; j < joy[i].num_sticks; j++) {
         for (k = 0; k < joy[i].stick[j].num_axis; k++) {
            axis_value[axis] /= 256;
            if (joy[i].stick[j].flags & JOYFLAG_SIGNED)
               joy[i].stick[j].axis[k].pos = axis_value[axis];
            else
               joy[i].stick[j].axis[k].pos = axis_value[axis] + 128;
            joy[i].stick[j].axis[k].d1 = (axis_value[axis] < -64 ? 1 : 0);
            joy[i].stick[j].axis[k].d2 = (axis_value[axis] > 64 ? 1 : 0);
            axis++;
         }
         stick++;
         if (axis >= num_axes) break;
      }
      for (j = 0; j < num_hats; j++) {
         switch (hat_value[hat]) {
            case 0:  /* centered */
               joy[i].stick[stick].axis[0].pos = 0;
               joy[i].stick[stick].axis[0].d1 = 0;
               joy[i].stick[stick].axis[0].d2 = 0;
               joy[i].stick[stick].axis[1].pos = 0;
               joy[i].stick[stick].axis[1].d1 = 0;
               joy[i].stick[stick].axis[1].d2 = 0;
               break;
            case 1:  /* up */
               joy[i].stick[stick].axis[0].pos = 0;
               joy[i].stick[stick].axis[0].d1 = 0;
               joy[i].stick[stick].axis[0].d2 = 0;
               joy[i].stick[stick].axis[1].pos = 128;
               joy[i].stick[stick].axis[1].d1 = 0;
               joy[i].stick[stick].axis[1].d2 = 1;
            case 2:  /* up and right */
               joy[i].stick[stick].axis[0].pos = 128;
               joy[i].stick[stick].axis[0].d1 = 0;
               joy[i].stick[stick].axis[0].d2 = 1;
               joy[i].stick[stick].axis[1].pos = 128;
               joy[i].stick[stick].axis[1].d1 = 0;
               joy[i].stick[stick].axis[1].d2 = 1;
            case 3:  /* right */
               joy[i].stick[stick].axis[0].pos = 128;
               joy[i].stick[stick].axis[0].d1 = 0;
               joy[i].stick[stick].axis[0].d2 = 1;
               joy[i].stick[stick].axis[1].pos = 0;
               joy[i].stick[stick].axis[1].d1 = 0;
               joy[i].stick[stick].axis[1].d2 = 0;
            case 4:  /* down and right */
               joy[i].stick[stick].axis[0].pos = 128;
               joy[i].stick[stick].axis[0].d1 = 0;
               joy[i].stick[stick].axis[0].d2 = 1;
               joy[i].stick[stick].axis[1].pos = -128;
               joy[i].stick[stick].axis[1].d1 = 1;
               joy[i].stick[stick].axis[1].d2 = 0;
            case 5:  /* down */
               joy[i].stick[stick].axis[0].pos = 0;
               joy[i].stick[stick].axis[0].d1 = 0;
               joy[i].stick[stick].axis[0].d2 = 0;
               joy[i].stick[stick].axis[1].pos = -128;
               joy[i].stick[stick].axis[1].d1 = 1;
               joy[i].stick[stick].axis[1].d2 = 0;
            case 6:  /* down and left */
               joy[i].stick[stick].axis[0].pos = -128;
               joy[i].stick[stick].axis[0].d1 = 1;
               joy[i].stick[stick].axis[0].d2 = 0;
               joy[i].stick[stick].axis[1].pos = -128;
               joy[i].stick[stick].axis[1].d1 = 1;
               joy[i].stick[stick].axis[1].d2 = 0;
            case 7:  /* left */
               joy[i].stick[stick].axis[0].pos = -128;
               joy[i].stick[stick].axis[0].d1 = 1;
               joy[i].stick[stick].axis[0].d2 = 0;
               joy[i].stick[stick].axis[1].pos = 0;
               joy[i].stick[stick].axis[1].d1 = 0;
               joy[i].stick[stick].axis[1].d2 = 0;
            case 8:  /* up and left */
               joy[i].stick[stick].axis[0].pos = -128;
               joy[i].stick[stick].axis[0].d1 = 1;
               joy[i].stick[stick].axis[0].d2 = 0;
               joy[i].stick[stick].axis[1].pos = 128;
               joy[i].stick[stick].axis[1].d1 = 0;
               joy[i].stick[stick].axis[1].d2 = 1;
         }
         hat++;
         stick++;
      }
      buttons = be_joy->ButtonValues(i);
      for (j = 0; j < num_buttons; j++) {
         joy[i].button[j].b = buttons & 1;
         buttons >>= 1;
      }
   }
   return 0;
}
