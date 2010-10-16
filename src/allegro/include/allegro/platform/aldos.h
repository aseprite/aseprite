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
 *      DOS-specific header defines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DOS
   #error bad include
#endif


/********************************************/
/************ Platform-specifics ************/
/********************************************/

#define SYSTEM_DOS               AL_ID('D','O','S',' ')

AL_VAR(SYSTEM_DRIVER, system_dos);

AL_VAR(int, i_love_bill);


#define KEYDRV_PCDOS             AL_ID('P','C','K','B')

AL_VAR(KEYBOARD_DRIVER, keydrv_pcdos);


#define TIMEDRV_FIXED_RATE       AL_ID('F','I','X','T')
#define TIMEDRV_VARIABLE_RATE    AL_ID('V','A','R','T')

AL_VAR(TIMER_DRIVER, timedrv_fixed_rate);
AL_VAR(TIMER_DRIVER, timedrv_variable_rate);


#define MOUSEDRV_MICKEYS         AL_ID('M','I','C','K')
#define MOUSEDRV_INT33           AL_ID('I','3','3',' ')
#define MOUSEDRV_POLLING         AL_ID('P','O','L','L')
#define MOUSEDRV_WINNT           AL_ID('W','N','T',' ')
#define MOUSEDRV_WIN2K           AL_ID('W','2','K',' ')

AL_VAR(MOUSE_DRIVER, mousedrv_mickeys);
AL_VAR(MOUSE_DRIVER, mousedrv_int33);
AL_VAR(MOUSE_DRIVER, mousedrv_polling);
AL_VAR(MOUSE_DRIVER, mousedrv_winnt);
AL_VAR(MOUSE_DRIVER, mousedrv_win2k);



/*******************************************/
/************ Joystick routines ************/
/*******************************************/

#define JOY_TYPE_STANDARD        AL_ID('S','T','D',' ')
#define JOY_TYPE_2PADS           AL_ID('2','P','A','D')
#define JOY_TYPE_4BUTTON         AL_ID('4','B','U','T')
#define JOY_TYPE_6BUTTON         AL_ID('6','B','U','T')
#define JOY_TYPE_8BUTTON         AL_ID('8','B','U','T')
#define JOY_TYPE_FSPRO           AL_ID('F','P','R','O')
#define JOY_TYPE_WINGEX          AL_ID('W','I','N','G')
#define JOY_TYPE_SIDEWINDER      AL_ID('S','W',' ',' ')
#define JOY_TYPE_SIDEWINDER_AG   AL_ID('S','W','A','G')
#define JOY_TYPE_SIDEWINDER_PP   AL_ID('S','W','P','P')
#define JOY_TYPE_GAMEPAD_PRO     AL_ID('G','P','R','O')
#define JOY_TYPE_GRIP            AL_ID('G','R','I','P')
#define JOY_TYPE_GRIP4           AL_ID('G','R','I','4')
#define JOY_TYPE_SNESPAD_LPT1    AL_ID('S','N','E','1')
#define JOY_TYPE_SNESPAD_LPT2    AL_ID('S','N','E','2')
#define JOY_TYPE_SNESPAD_LPT3    AL_ID('S','N','E','3')
#define JOY_TYPE_PSXPAD_LPT1     AL_ID('P','S','X','1')
#define JOY_TYPE_PSXPAD_LPT2     AL_ID('P','S','X','2')
#define JOY_TYPE_PSXPAD_LPT3     AL_ID('P','S','X','3')
#define JOY_TYPE_N64PAD_LPT1     AL_ID('N','6','4','1')
#define JOY_TYPE_N64PAD_LPT2     AL_ID('N','6','4','2')
#define JOY_TYPE_N64PAD_LPT3     AL_ID('N','6','4','3')
#define JOY_TYPE_DB9_LPT1        AL_ID('D','B','9','1')
#define JOY_TYPE_DB9_LPT2        AL_ID('D','B','9','2')
#define JOY_TYPE_DB9_LPT3        AL_ID('D','B','9','3')
#define JOY_TYPE_TURBOGRAFX_LPT1 AL_ID('T','G','X','1')
#define JOY_TYPE_TURBOGRAFX_LPT2 AL_ID('T','G','X','2')
#define JOY_TYPE_TURBOGRAFX_LPT3 AL_ID('T','G','X','3')
#define JOY_TYPE_IFSEGA_ISA      AL_ID('S','E','G','I')
#define JOY_TYPE_IFSEGA_PCI      AL_ID('S','E','G','P')
#define JOY_TYPE_IFSEGA_PCI_FAST AL_ID('S','G','P','F')
#define JOY_TYPE_WINGWARRIOR     AL_ID('W','W','A','R')


AL_VAR(JOYSTICK_DRIVER, joystick_standard);
AL_VAR(JOYSTICK_DRIVER, joystick_2pads);
AL_VAR(JOYSTICK_DRIVER, joystick_4button);
AL_VAR(JOYSTICK_DRIVER, joystick_6button);
AL_VAR(JOYSTICK_DRIVER, joystick_8button);
AL_VAR(JOYSTICK_DRIVER, joystick_fspro);
AL_VAR(JOYSTICK_DRIVER, joystick_wingex);
AL_VAR(JOYSTICK_DRIVER, joystick_sw);
AL_VAR(JOYSTICK_DRIVER, joystick_sw_ag);
AL_VAR(JOYSTICK_DRIVER, joystick_sw_pp);
AL_VAR(JOYSTICK_DRIVER, joystick_gpro);
AL_VAR(JOYSTICK_DRIVER, joystick_grip);
AL_VAR(JOYSTICK_DRIVER, joystick_grip4);
AL_VAR(JOYSTICK_DRIVER, joystick_sp1);
AL_VAR(JOYSTICK_DRIVER, joystick_sp2);
AL_VAR(JOYSTICK_DRIVER, joystick_sp3);
AL_VAR(JOYSTICK_DRIVER, joystick_psx1);
AL_VAR(JOYSTICK_DRIVER, joystick_psx2);
AL_VAR(JOYSTICK_DRIVER, joystick_psx3);
AL_VAR(JOYSTICK_DRIVER, joystick_n641);
AL_VAR(JOYSTICK_DRIVER, joystick_n642);
AL_VAR(JOYSTICK_DRIVER, joystick_n643);
AL_VAR(JOYSTICK_DRIVER, joystick_db91);
AL_VAR(JOYSTICK_DRIVER, joystick_db92);
AL_VAR(JOYSTICK_DRIVER, joystick_db93);
AL_VAR(JOYSTICK_DRIVER, joystick_tgx1);
AL_VAR(JOYSTICK_DRIVER, joystick_tgx2);
AL_VAR(JOYSTICK_DRIVER, joystick_tgx3);
AL_VAR(JOYSTICK_DRIVER, joystick_sg1);
AL_VAR(JOYSTICK_DRIVER, joystick_sg2);
AL_VAR(JOYSTICK_DRIVER, joystick_sg2f);
AL_VAR(JOYSTICK_DRIVER, joystick_ww);


#define JOYSTICK_DRIVER_STANDARD                                  \
      { JOY_TYPE_STANDARD,       &joystick_standard,  TRUE  },    \
      { JOY_TYPE_2PADS,          &joystick_2pads,     FALSE },    \
      { JOY_TYPE_4BUTTON,        &joystick_4button,   FALSE },    \
      { JOY_TYPE_6BUTTON,        &joystick_6button,   FALSE },    \
      { JOY_TYPE_8BUTTON,        &joystick_8button,   FALSE },    \
      { JOY_TYPE_FSPRO,          &joystick_fspro,     FALSE },    \
      { JOY_TYPE_WINGEX,         &joystick_wingex,    FALSE },

#define JOYSTICK_DRIVER_SIDEWINDER                                \
      { JOY_TYPE_SIDEWINDER,     &joystick_sw,        TRUE  },    \
      { JOY_TYPE_SIDEWINDER_AG,  &joystick_sw_ag,     TRUE  },    \
      { JOY_TYPE_SIDEWINDER_PP,  &joystick_sw_pp,     TRUE  },

#define JOYSTICK_DRIVER_GAMEPAD_PRO                               \
      { JOY_TYPE_GAMEPAD_PRO,    &joystick_gpro,      TRUE  },

#define JOYSTICK_DRIVER_GRIP                                      \
      { JOY_TYPE_GRIP,           &joystick_grip,      TRUE  },    \
      { JOY_TYPE_GRIP4,          &joystick_grip4,     TRUE  },

#define JOYSTICK_DRIVER_SNESPAD                                   \
      { JOY_TYPE_SNESPAD_LPT1,   &joystick_sp1,       FALSE },    \
      { JOY_TYPE_SNESPAD_LPT2,   &joystick_sp2,       FALSE },    \
      { JOY_TYPE_SNESPAD_LPT3,   &joystick_sp3,       FALSE },

#define JOYSTICK_DRIVER_PSXPAD                                    \
      { JOY_TYPE_PSXPAD_LPT1,    &joystick_psx1,      FALSE },    \
      { JOY_TYPE_PSXPAD_LPT2,    &joystick_psx2,      FALSE },    \
      { JOY_TYPE_PSXPAD_LPT3,    &joystick_psx3,      FALSE },

#define JOYSTICK_DRIVER_N64PAD                                    \
      { JOY_TYPE_N64PAD_LPT1,    &joystick_n641,      FALSE },    \
      { JOY_TYPE_N64PAD_LPT2,    &joystick_n642,      FALSE },    \
      { JOY_TYPE_N64PAD_LPT3,    &joystick_n643,      FALSE },

#define JOYSTICK_DRIVER_DB9                                       \
      { JOY_TYPE_DB9_LPT1,       &joystick_db91,      FALSE },    \
      { JOY_TYPE_DB9_LPT2,       &joystick_db92,      FALSE },    \
      { JOY_TYPE_DB9_LPT3,       &joystick_db93,      FALSE },

#define JOYSTICK_DRIVER_TURBOGRAFX                                \
      { JOY_TYPE_TURBOGRAFX_LPT1,&joystick_tgx1,      FALSE },    \
      { JOY_TYPE_TURBOGRAFX_LPT2,&joystick_tgx2,      FALSE },    \
      { JOY_TYPE_TURBOGRAFX_LPT3,&joystick_tgx3,      FALSE },

#define JOYSTICK_DRIVER_IFSEGA_ISA                                \
      { JOY_TYPE_IFSEGA_ISA,     &joystick_sg1,       FALSE },

#define JOYSTICK_DRIVER_IFSEGA_PCI                                \
      { JOY_TYPE_IFSEGA_PCI,     &joystick_sg2,       FALSE },

#define JOYSTICK_DRIVER_IFSEGA_PCI_FAST                           \
      { JOY_TYPE_IFSEGA_PCI_FAST,&joystick_sg2f,      FALSE },

#define JOYSTICK_DRIVER_WINGWARRIOR                               \
      { JOY_TYPE_WINGWARRIOR,    &joystick_ww,        TRUE  },


#define joy_FSPRO_trigger     joy_b1
#define joy_FSPRO_butleft     joy_b2
#define joy_FSPRO_butright    joy_b3
#define joy_FSPRO_butmiddle   joy_b4

#define joy_WINGEX_trigger    joy_b1
#define joy_WINGEX_buttop     joy_b2
#define joy_WINGEX_butthumb   joy_b3
#define joy_WINGEX_butmiddle  joy_b4


AL_FUNC(int, calibrate_joystick_tl, (void));
AL_FUNC(int, calibrate_joystick_br, (void));
AL_FUNC(int, calibrate_joystick_throttle_min, (void));
AL_FUNC(int, calibrate_joystick_throttle_max, (void));
AL_FUNC(int, calibrate_joystick_hat, (int direction));



/*******************************************/
/************ Graphics routines ************/
/*******************************************/

#define ALLEGRO_GFX_HAS_VGA
#define ALLEGRO_GFX_HAS_VBEAF

#define GFX_VGA                  AL_ID('V','G','A',' ')
#define GFX_MODEX                AL_ID('M','O','D','X')
#define GFX_VESA1                AL_ID('V','B','E','1')
#define GFX_VESA2B               AL_ID('V','B','2','B')
#define GFX_VESA2L               AL_ID('V','B','2','L')
#define GFX_VESA3                AL_ID('V','B','E','3')
#define GFX_VBEAF                AL_ID('V','B','A','F')
#define GFX_XTENDED              AL_ID('X','T','N','D')

AL_VAR(GFX_DRIVER, gfx_vga);
AL_VAR(GFX_DRIVER, gfx_modex);
AL_VAR(GFX_DRIVER, gfx_vesa_1);
AL_VAR(GFX_DRIVER, gfx_vesa_2b);
AL_VAR(GFX_DRIVER, gfx_vesa_2l);
AL_VAR(GFX_DRIVER, gfx_vesa_3);
AL_VAR(GFX_DRIVER, gfx_vbeaf);
AL_VAR(GFX_DRIVER, gfx_xtended);


#define GFX_DRIVER_VGA                                            \
   {  GFX_VGA,          &gfx_vga,            TRUE  },

#define GFX_DRIVER_MODEX                                          \
   {  GFX_MODEX,        &gfx_modex,          TRUE  },

#define GFX_DRIVER_VBEAF                                          \
   {  GFX_VBEAF,        &gfx_vbeaf,          TRUE   },

#define GFX_DRIVER_VESA3                                          \
   {  GFX_VESA3,        &gfx_vesa_3,         TRUE   },

#define GFX_DRIVER_VESA2L                                         \
   {  GFX_VESA2L,       &gfx_vesa_2l,        TRUE   },

#define GFX_DRIVER_VESA2B                                         \
   {  GFX_VESA2B,       &gfx_vesa_2b,        TRUE   },

#define GFX_DRIVER_XTENDED                                        \
   {  GFX_XTENDED,      &gfx_xtended,        FALSE  },

#define GFX_DRIVER_VESA1                                          \
   {  GFX_VESA1,        &gfx_vesa_1,         TRUE   },


AL_FUNC_DEPRECATED(void, split_modex_screen, (int lyne));


AL_INLINE(void, _set_color, (int index, AL_CONST RGB *p),
{
   outportb(0x3C8, index);
   outportb(0x3C9, p->r);
   outportb(0x3C9, p->g);
   outportb(0x3C9, p->b);

   _current_palette[index] = *p;
})



/****************************************/
/************ Sound routines ************/
/****************************************/

#define DIGI_SB10             AL_ID('S','B','1','0')
#define DIGI_SB15             AL_ID('S','B','1','5')
#define DIGI_SB20             AL_ID('S','B','2','0')
#define DIGI_SBPRO            AL_ID('S','B','P',' ')
#define DIGI_SB16             AL_ID('S','B','1','6')
#define DIGI_AUDIODRIVE       AL_ID('E','S','S',' ')
#define DIGI_SOUNDSCAPE       AL_ID('E','S','C',' ')
#define DIGI_WINSOUNDSYS      AL_ID('W','S','S',' ')

#define MIDI_OPL2             AL_ID('O','P','L','2')
#define MIDI_2XOPL2           AL_ID('O','P','L','X')
#define MIDI_OPL3             AL_ID('O','P','L','3')
#define MIDI_SB_OUT           AL_ID('S','B',' ',' ')
#define MIDI_MPU              AL_ID('M','P','U',' ')
#define MIDI_AWE32            AL_ID('A','W','E',' ')


AL_VAR(DIGI_DRIVER, digi_sb10);
AL_VAR(DIGI_DRIVER, digi_sb15);
AL_VAR(DIGI_DRIVER, digi_sb20);
AL_VAR(DIGI_DRIVER, digi_sbpro);
AL_VAR(DIGI_DRIVER, digi_sb16);
AL_VAR(DIGI_DRIVER, digi_audiodrive);
AL_VAR(DIGI_DRIVER, digi_soundscape);
AL_VAR(DIGI_DRIVER, digi_wss);

AL_VAR(MIDI_DRIVER, midi_opl2);
AL_VAR(MIDI_DRIVER, midi_2xopl2);
AL_VAR(MIDI_DRIVER, midi_opl3);
AL_VAR(MIDI_DRIVER, midi_sb_out);
AL_VAR(MIDI_DRIVER, midi_mpu401);
AL_VAR(MIDI_DRIVER, midi_awe32);


#define DIGI_DRIVER_WINSOUNDSYS                                   \
      {  DIGI_WINSOUNDSYS, &digi_wss,           FALSE  },

#define DIGI_DRIVER_AUDIODRIVE                                    \
      {  DIGI_AUDIODRIVE,  &digi_audiodrive,    TRUE   },

#define DIGI_DRIVER_SOUNDSCAPE                                    \
      {  DIGI_SOUNDSCAPE,  &digi_soundscape,    TRUE   },

#define DIGI_DRIVER_SB                                            \
      {  DIGI_SB16,        &digi_sb16,          TRUE   },         \
      {  DIGI_SBPRO,       &digi_sbpro,         TRUE   },         \
      {  DIGI_SB20,        &digi_sb20,          TRUE   },         \
      {  DIGI_SB15,        &digi_sb15,          TRUE   },         \
      {  DIGI_SB10,        &digi_sb10,          TRUE   },

#define MIDI_DRIVER_AWE32                                         \
      {  MIDI_AWE32,       &midi_awe32,         TRUE   },

#define MIDI_DRIVER_ADLIB                                         \
      {  MIDI_OPL3,        &midi_opl3,          TRUE   },         \
      {  MIDI_2XOPL2,      &midi_2xopl2,        TRUE   },         \
      {  MIDI_OPL2,        &midi_opl2,          TRUE   },

#define MIDI_DRIVER_SB_OUT                                        \
      {  MIDI_SB_OUT,      &midi_sb_out,        FALSE  },

#define MIDI_DRIVER_MPU                                           \
      {  MIDI_MPU,         &midi_mpu401,        FALSE  },


#ifdef __cplusplus
extern "C" {
#endif

AL_FUNC(int, load_ibk, (AL_CONST char *filename, int drums));

#ifdef __cplusplus
}
#endif
