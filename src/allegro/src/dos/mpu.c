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
 *      Direct output to an MPU-401 MIDI interface. 
 *
 *      By Shawn Hargreaves.
 *
 *      Marcel de Kogel fixed my original broken version, so that it now 
 *      actually works :-)
 *
 *      Input routines added by Ove Kaaven.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* external interface to the MPU-401 driver */
static int mpu_detect(int input);
static int mpu_init(int input, int voices);
static void mpu_exit(int input);
static void mpu_output(int data);

static char mpu_desc[256] = EMPTY_STRING;


MIDI_DRIVER midi_mpu401 =
{
   MIDI_MPU,
   empty_string,
   empty_string,
   "MPU-401", 
   0, 0, 0xFFFF, 0, -1, -1,
   mpu_detect,
   mpu_init,
   mpu_exit,
   NULL,
   NULL,
   mpu_output,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};



static int mpu_output_mode = FALSE;    /* are we active for output? */
static int mpu_input_mode = FALSE;     /* are we active for input? */

static int mpu_piggyback = FALSE;      /* are we having to share our IRQ? */

int _mpu_port = -1;                    /* where is the card? */
int _mpu_irq = -1;                     /* which IRQ? */

static int mpu_int = -1;               /* which interrupt? */



/* wait_for_mpu:
 *  Waits for the specified bit to clear on the specified port. Returns zero
 *  if it cleared, -1 if it timed out.
 */
static INLINE int wait_for_mpu(int flag, int port)
{
   int timeout;

   for (timeout=0; timeout<0x7FFF; timeout++)
      if (!(inportb(port) & flag))
	 return 0;

   return -1;
}



/* mpu_output:
 *  Writes a byte to the MPU-401 midi interface.
 */
static void mpu_output(int data)
{
   wait_for_mpu(0x40, _mpu_port+1);
   outportb(_mpu_port, data);
}

END_OF_STATIC_FUNCTION(mpu_output);



/* mpu_input:
 *  Reads a byte from the MPU-401 midi interface.
 */
static INLINE int mpu_input(void)
{
   if (wait_for_mpu(0x80, _mpu_port+1))
      return -1;

   return inportb(_mpu_port);
}



/* mpu_poll:
 *  Reads a byte from the MPU-401 interface, then timestamps and queues it.
 */
void _mpu_poll(void)
{
   int c = mpu_input();

   if ((c >= 0) && (midi_recorder))
      midi_recorder(c);
}

END_OF_FUNCTION(_mpu_poll);



/* mpu_interrupt:
 *  Handler for the MPU-401 input interrupt.
 */ 
static int mpu_interrupt(void)
{
   _mpu_poll();
   _eoi(_mpu_irq);
   return 0;
}

END_OF_STATIC_FUNCTION(mpu_interrupt);



/* mpu_detect:
 *  Detects the presence of an MPU-401 compatible midi interface.
 */
static int mpu_detect(int input)
{
   char *blaster = getenv("BLASTER");
   char tmp1[64], tmp2[64];
   int i;

   _mpu_port = get_config_hex(uconvert_ascii("sound", tmp1), uconvert_ascii("mpu_port", tmp2), -1);
   _mpu_irq = get_config_hex(uconvert_ascii("sound", tmp1), uconvert_ascii("mpu_irq", tmp2), -1);

   /* only bother with the detection if we aren't already active */
   if (!mpu_output_mode) {

      /* parse BLASTER env */
      if ((blaster) && (_mpu_port < 0)) { 
	 while (*blaster) {
	    while ((*blaster == ' ') || (*blaster == '\t'))
	       blaster++;

	    if (((*blaster == 'p') || (*blaster == 'P')) && (_mpu_port < 0))
	       _mpu_port = strtol(blaster+1, NULL, 16);

	    if (((*blaster == 'i') || (*blaster == 'I')) && (_mpu_irq < 0))
	       _mpu_irq = strtol(blaster+1, NULL, 10);

	    while ((*blaster) && (*blaster != ' ') && (*blaster != '\t'))
	       blaster++;
	 }
      }

      /* if that didn't work, guess :-) */
      if (_mpu_port < 0)
	 _mpu_port = 0x330;

      if (_mpu_irq < 0) {
	 if (_sound_irq > 0)
	    _mpu_irq = _sound_irq;
	 else
	    _mpu_irq = 7;
      }

      mpu_int = _map_irq(_mpu_irq);

      /* check whether the MPU is there */
      outportb(_mpu_port+1, 0xFF); 
      mpu_input();

      if (wait_for_mpu(0x40, _mpu_port+1) != 0) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("MPU-401 not found"));
	 return FALSE;
      }

      uszprintf(mpu_desc, sizeof(mpu_desc), get_config_text("MPU-401 MIDI interface on port %X, using IRQ %d"),
		_mpu_port, _mpu_irq);
      midi_mpu401.desc = mpu_desc;
   }

   /* can we handle input? */
   if (input) {
      static int conflicts[] =
      {
	 DIGI_SB10, DIGI_SB15, DIGI_SB20, DIGI_SBPRO, DIGI_SB16,
	 DIGI_AUDIODRIVE, 0
      };

      mpu_piggyback = FALSE;

      for (i=0; conflicts[i]; i++) {
	 if (digi_card == conflicts[i]) {
	    if (_sound_irq == _mpu_irq) {
	       mpu_piggyback = TRUE;
	       if (digi_card != DIGI_SB16) {
		  uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("MPU-401 and %s conflict over IRQ %d"),
			    digi_driver->name, _mpu_irq);
		  return FALSE;
	       }
	    }
	    break;
	 }
      }
   }

   return TRUE;
}



/* mpu_init:
 *  Initialises the MPU-401 midi interface.
 */
static int mpu_init(int input, int voices)
{
   if (!mpu_output_mode) {
      /* only do the hardware initialisation once */
      outportb(_mpu_port+1, 0x3F);
      mpu_input();

      LOCK_VARIABLE(midi_mpu401);
      LOCK_VARIABLE(_mpu_port);
      LOCK_VARIABLE(_mpu_irq);
      LOCK_FUNCTION(_mpu_poll);
      LOCK_FUNCTION(mpu_output);
      LOCK_FUNCTION(mpu_interrupt);
   }

   if (input) {
      /* only hook the interrupt if we have direct access to it */
      if (!mpu_piggyback) {
	 _enable_irq(_mpu_irq);
	 _install_irq(mpu_int, mpu_interrupt);
      }
      mpu_input_mode = TRUE;
   }
   else
      mpu_output_mode = TRUE;

   return 0;
}



/* mpu_exit:
 *  Resets the MPU-401 midi interface when we are finished.
 */
static void mpu_exit(int input)
{
   if (input) {
      /* only unhook the interrupt if we have direct access to it */
      if (!mpu_piggyback) {
	 _remove_irq(mpu_int);
	 _restore_irq(_mpu_irq);
      }
      mpu_input_mode = FALSE;
   }
   else
      mpu_output_mode = FALSE;

   if ((!mpu_input_mode) && (!mpu_output_mode)) {
      /* shut down the hardware once nothing is active */
      outportb(_mpu_port+1, 0xFF);
   }
}

