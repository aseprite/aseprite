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
 *      Joystick driver for the Microsoft Sidewinder.
 *
 *      Main code by S.Sakamaki, using information provided by Robert Grubbs.
 *
 *      Sidewinder detection routine (of aggressive mode) under Windows,
 *      comments and translation by Tomohiko Sugiura.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error Something is wrong with the makefile
#endif


/* driver functions */
static int sw_init(void);
static int sw_init_aggressive(void);
static void sw_exit(void);
static int sw_poll(void);

#define DETECT_TRIES 1


static int read_sidewinder(void);
static int change_mode_b(void);

static int g_aggressive = FALSE;

static int g_port = 0x201;
static int g_swbuff_index = 0;
static char g_swbuff[4096];
static char g_swdata[60];
static int g_swpaddata[4];
static int g_valid_count = 4096;
static int g_base_count  = 0;
static int g_sw_count    = 0;
static int g_sw_mode     = 0;
static int g_sw_status   = 0;
static int g_endstrobelimit = 0;

static int g_sw_raw_input[4];


#define SWMODE_A  1
#define SWMODE_B  2


static void _sw_reset_swbuff_index(void);
static void _sw_swbuff_index_dec(void);
static int _sw_poll(int gameport, int endstrobe_limit);

static int _sw_poll_b(int gameport, int endstrobelimit);

static int _sw_wait_strobe(char* data, int* length, char HL);
static int _sw_in(char* data);
static int _sw_mode_a_convert(char* data);
static int _sw_mode_b_convert(char* data);
static int _sw_convert(char* data, int* paddatabuff, int mode, int sw_count);
static int _sw_trace_data(char* data, int* sw_count, int* mode);
static int _sw_get_base_count(int* count);
static int _sw_check_parity(int data);
static void wait_for_sw_sleep(int gameport, int count);


JOYSTICK_DRIVER joystick_sw =
{
   JOY_TYPE_SIDEWINDER,
   empty_string,
   empty_string,
   "Sidewinder",
   sw_init,
   sw_exit,
   sw_poll,
   NULL, NULL,
   NULL, NULL
};


JOYSTICK_DRIVER joystick_sw_ag =
{
   JOY_TYPE_SIDEWINDER_AG,
   empty_string,
   empty_string,
   "Sidewinder Aggressive",
   sw_init_aggressive,
   sw_exit,
   sw_poll,
   NULL, NULL,
   NULL, NULL
};



/* sw_init_aggressive:
 *  Initializes the aggressive driver.
 */
static int sw_init_aggressive(void)
{
   g_aggressive = TRUE;
   return sw_init();
}



/* sw_init:
 *  Initializes the driver.
 */
static int sw_init(void)
{
   int i;
   int d0;
   int d1;
   int mode;
   int port[] = { 0x201, 0x200, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207 };
   int portindex;
   int success;
   int basecount;
   int validcount;

   d0 = 0;
   while (d0 < 4) {
      g_sw_raw_input[d0] = 0;
      d0 += 1;
   }

   if (g_aggressive) {
      portindex = 0;
      while (portindex < 8) {
	 g_port = port[portindex];
	 for (success=0, i=0; i<DETECT_TRIES; i++) {
	    /* Wait for a while to confirm sw is inactive */
	    wait_for_sw_sleep (g_port, 4096);
	    validcount = _sw_poll(g_port, 0);
	    if (!_sw_get_base_count(&basecount))
	       continue;
	    g_base_count = basecount;	/* length of first strobe */
	    g_endstrobelimit = basecount * 4;
	    _sw_reset_swbuff_index();

	    /* decode strobe data */
	    if (!_sw_trace_data(g_swdata, &g_sw_count, &mode))
	       continue;
	    success++;
	 }

	 if (success == DETECT_TRIES)
	    break;

	 portindex += 1;
      }

      if (success != DETECT_TRIES)
	 return -1;

      change_mode_b();
   }
   else {
      g_port = 0x201;

      /* Wait for a while to confirm sw is inactive */
      wait_for_sw_sleep(g_port, 4096);
      _sw_poll(g_port, 0);
      if (!_sw_get_base_count(&basecount))
	 return -1;
      g_base_count = basecount;			/* length of first strobe */
      g_endstrobelimit = basecount * 4;
      _sw_reset_swbuff_index();

      /* decode strobe data */
      if (!_sw_trace_data(g_swdata, &g_sw_count, &mode))
	 return -1;				/* detection failed */
   }

   num_joysticks = g_sw_count; 			/* numbers of detected sw */
   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL;

      joy[i].num_sticks = 1;
      joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = get_config_text("X");
      joy[i].stick[0].axis[1].name = get_config_text("Y");
      joy[i].stick[0].name = get_config_text("Pad");

      joy[i].num_buttons = 10;

      joy[i].button[0].name = get_config_text("A");
      joy[i].button[1].name = get_config_text("B");
      joy[i].button[2].name = get_config_text("C");
      joy[i].button[3].name = get_config_text("X");
      joy[i].button[4].name = get_config_text("Y");
      joy[i].button[5].name = get_config_text("Z");
      joy[i].button[6].name = get_config_text("L");
      joy[i].button[7].name = get_config_text("R");
      joy[i].button[8].name = get_config_text("Start");
      joy[i].button[9].name = get_config_text("M");
   }

   return 0;
}



/* sw_exit:
 *  Shuts down the driver.
 */
static void sw_exit(void)
{
   g_aggressive = FALSE;
}



/* sw_poll:
 *  Updates the joystick status variables.
 */
static int sw_poll(void)
{
   int i, b, mask;

   g_sw_status = read_sidewinder();

   for (i=0; i<num_joysticks; i++) {
      joy[i].stick[0].axis[0].d1 = ((g_sw_raw_input[i] & 0x10) != 0);
      joy[i].stick[0].axis[0].d2 = ((g_sw_raw_input[i] & 0x08) != 0);
      joy[i].stick[0].axis[1].d1 = ((g_sw_raw_input[i] & 0x02) != 0);
      joy[i].stick[0].axis[1].d2 = ((g_sw_raw_input[i] & 0x04) != 0);

      for (b=0; b<2; b++) {
	 if (joy[i].stick[0].axis[b].d1)
	    joy[i].stick[0].axis[b].pos = -128;
	 else if (joy[i].stick[0].axis[b].d2)
	    joy[i].stick[0].axis[b].pos = 128;
	 else
	    joy[i].stick[0].axis[b].pos = 0;
      }

      mask = 0x20;

      for (b=0; b<10; b++) {
	 joy[i].button[b].b = ((g_sw_raw_input[i] & mask) != 0);
	 mask <<= 1;
      }
   }

   return 0;
}



static int read_sidewinder(void)
{
   int d0;
   int c_sw_count;
   int mode;
   int result;
   int try;

   if (g_aggressive) {
      try = 3;
      while (try > 0) {
	 _sw_poll(g_port, g_endstrobelimit); 	/* read sw data */
	 result = TRUE;
	 /* decode sw data */
	 if (!_sw_trace_data(g_swdata, &c_sw_count, &mode))
	    result = FALSE;
	 if (result) {
	    if (c_sw_count != g_sw_count)
	       result = FALSE;			/* error if numbers of sw is not equal to initialized one. */
	 }
	 if (result) {
	    /* convert 5(modeB) or 15(modeA) chars to 1 int */
	    if (!_sw_convert(g_swdata, g_swpaddata, mode, c_sw_count))
	       result = FALSE;
	 }
	 if (result)
	    break;
	 try -= 1;
	 /* Wait for a while to confirm sw is inactive */
	 if (try != 0)
	    wait_for_sw_sleep(g_port, g_base_count);
      }
   }
   else {
      wait_for_sw_sleep(g_port, g_base_count);	/* Wait for a while to confirm sw is inactive */
      _sw_poll(g_port, g_endstrobelimit); 	/* read sw data */
      result = TRUE;
      /* decode sw data */
      if (!_sw_trace_data(g_swdata, &c_sw_count, &mode))
	 result = FALSE;
      if (result) {
	 if (c_sw_count != g_sw_count)
	    result = FALSE;			/* error if numbers of sw is not equal to initialized one. */
      }
      if (result) {
	 /* convert 5(modeB) or 15(modeA) chars to 1 int */
	 if (!_sw_convert(g_swdata, g_swpaddata, mode, c_sw_count))
	    result = FALSE;
      }
   }

   if (result) {
      d0 = 0;
      while (d0 < c_sw_count) {
	 g_sw_raw_input[d0] = g_swpaddata[d0] << 1;	/* store final sw status */
	 d0 += 1;
      }
   }

   return result;
}



static int _sw_convert(char* data, int* paddatabuff, int mode, int sw_count)
{
   int d0;
   int d1;

   d0 = 0;
   while (d0 < sw_count) {
      switch (mode) {
	 case 5:
	    d1 = _sw_mode_b_convert(data + (d0*mode));
	    g_sw_mode = SWMODE_B;
	    break;
	 case 15:
	    d1 = _sw_mode_a_convert(data + (d0*mode));
	    g_sw_mode = SWMODE_A;
	    break;
	 default:
	    return FALSE;
      }
      if (_sw_check_parity(d1) == 0)
	 return FALSE;
      *paddatabuff++ = d1;
      d0 += 1;
   }

   return TRUE;
}



/* _sw_check_parity:
 *  Parity bit is 0 if odd button is pressed, else 1.
 */
static int _sw_check_parity(int data)
{
   /*
    * valid lower 16bit
    */
   int d0;

   d0 = 8;
   while (d0 > 0) {
      data = data ^ (data >> d0);
      d0 = d0 >> 1;
   }
   d0 = data & 0x01;

   return d0;					/* return 1 if parity check passed. */
}



/* _sw_mode_a_convert:
 *
 *           xxUxxxxx
 *           xxDxxxxx
 *           xxRxxxxx
 *           xxLxxxxx
 * converts  xx0xxxxx   to  xPMS76543210LRDU
 *           xx1xxxxx       MLB is not used
 *           xx2xxxxx
 *           xx3xxxxx
 *           xx4xxxxx
 *           xx5xxxxx
 *           xx6xxxxx
 *           xx7xxxxx
 *           xxSxxxxx
 *           xxMxxxxx
 *           xxPxxxxx
 */
static int _sw_mode_a_convert(char* data)
{
   int d0;
   int d1;

   d0 = 14;
   d1 = 0;
   while (d0 >= 0) {
      d1 = d1 << 1;
      d1 |= ( *(data + d0) >> 5 ) & 0x01;
      d0 -= 1;
   }

   return d1;					/* 15 buttons status */
}



/*_sw_mode_b_convert:
 *           RDUxxxxx
 *           10Lxxxxx
 * converts  432xxxxx   to  xPMS76543210LRDU
 *           765xxxxx       MLB is not used
 *           PMSxxxxx
 */
static int _sw_mode_b_convert(char* data)
{
   int d0;
   int d1;

   d0 = 4;
   d1 = 0;
   while (d0 >= 0) {
      d1 = d1 << 3;
      d1 |= ( *(data + d0) >> 5 ) & 0x07;
      d0 -= 1;
   }

   return d1;					/* 15 buttons status */
}



static int _sw_trace_data(char* data, int* sw_count, int* mode)
{
   char swbyte;
   int  length;
   int  d2;
   int  d3;
   int  data_index = 0;

   *mode = 0;
   *sw_count = 0;
   d2 = 0;
   if (!_sw_wait_strobe(&swbyte, &length, 1))
      return FALSE;				/* wait strobe 1 */
   while (1) {
      if (!_sw_wait_strobe(&swbyte, &length, 0))
	 return FALSE;				/* wait strobe 0 */
      d2 += 1;
      if (data_index < 60)
	 data[data_index++] = swbyte;
      if (!_sw_wait_strobe(&swbyte, &length, 1))
	 return FALSE;
      if (length >= g_base_count) { 		/* finish one pad */
	 /* if continued strobe 1 more than base_count */
	 if ((d2 == 5) || (d2 == 15)) {
	    *mode = d2; 			/* data length is 5 on modeBC15 on modeA */
	    *sw_count += 1;
	    if (length >= g_endstrobelimit)
	       return TRUE;
	    /* finish all pad */
	    /* if continued strobe 1 more than base_count*4 */
	    break;
	 }
	 else {					/* if data length is not 5 or 15 ,sw data is destroyed */
	    return FALSE;
	 }
      }
   }
   d2 = 0;
   while (1) {
      if (!_sw_wait_strobe(&swbyte, &length, 0))
	 return FALSE;
      d2 += 1;
      if (data_index < 60)
	 data[data_index++] = swbyte;
      if (!_sw_wait_strobe(&swbyte, &length, 1))
	 return FALSE;
      if (length >= g_base_count) { 		/* finish one pad */
	 /* if continued strobe 1 more than base_count */

	 if ( d2 == *mode) {			/* All sw must be same mode(A or B) */
	    /* if recieved data is valid, */
	    d2 = 0;
	    *sw_count += 1;
	    if (length >= g_endstrobelimit)
	       break;	/* finish all pad */
	    /* if continued strobe 1 more than base_count*4 */
	 }
	 else {
	    return FALSE;
	 }
      }
   }

   return TRUE;
}



static int _sw_get_base_count(int* count)
{
   char swbyte;
   int  length;

   if (!_sw_wait_strobe(&swbyte, &length, 0))
      return FALSE;
   if (!_sw_wait_strobe(&swbyte, &length, 1))
      return FALSE;
   *count	= length;
   if (!_sw_wait_strobe(&swbyte, &length, 0))
      return FALSE;
   *count += length;

   return TRUE;
}



static int _sw_wait_strobe(char* data, int* length, char HL)
{
   /* HL is 0 or 1 */
   char d0;
   char d1;

   *length = 0;
   while (1) {
      if (_sw_in(data))
	 d1 = (*data & 0x10) >> 4;
      else
	 return FALSE;
      if (d1 == HL)
	 break; 				/* upclock or downclock */
   }
   *length += 1;

   /* search next upclock or downclock point */
   while (1) {
      if (_sw_in(&d0))
	 d1 = (d0 & 0x10) >> 4;
      else
	 break;
      if (d1 != HL) {
	 _sw_swbuff_index_dec();
	 break;
      }
      *length += 1;
   }

   return TRUE;
}



static void _sw_reset_swbuff_index(void)
{
   g_swbuff_index = 0;
}



static void _sw_swbuff_index_dec(void)
{
   g_swbuff_index -= 1;
}



/* _sw_in:
 *  Reads memory of sw data buffer.
 */
static int _sw_in(char* data)
{
   if (g_swbuff_index == g_valid_count)
      return FALSE;
   g_swbuff_index += 1;
   *data = g_swbuff[g_swbuff_index] & 0xF0;
   return TRUE;
}



/* _sw_poll:
 *  Polls sw data from gameport.
 */
static int _sw_poll(int gameport, int endstrobelimit)
{
   unsigned char* buff = (unsigned char *)g_swbuff;
   int bufflength = sizeof(g_swbuff);
   int read_count;
   int fill;
   int d0;
   int d1;
   int *a0;
   int strobecount;

   if (endstrobelimit == 0)
      endstrobelimit = bufflength;
   fill = 512;
   if (bufflength < 512)
      fill = bufflength;
   fill = (fill / sizeof(int)) - 2;

   _enter_critical();

   a0 = (int*)buff;
   d0 = 0;
   while (d0 < fill) {
      *(a0 + d0) = *(a0 + d0 + 1);
      d0 += 1;
   }

   strobecount = 0;
   d0 = bufflength;
   outportb(gameport, 0x00);
   while (d0 > 0) {
      d1 = inportb(gameport);
      d1 = ~d1;
      *buff++ = (unsigned char)d1;
      d1 = (d1 >> 4) & 0x01;
      strobecount += d1;
      strobecount &= d1 * -1;
      d0 -= 1;
      if (strobecount == endstrobelimit)
	 break;
   }

   _exit_critical();

   g_valid_count  = bufflength - d0;
   _sw_reset_swbuff_index();

   return g_valid_count;
}



static void wait_for_sw_sleep(int gameport, int count)
{
   int limit;
   int d0;

   d0 = 0;
   limit = 0;
   while (limit < 8192) {
      if ((inportb(gameport) & 0x10) != 0) {
	 d0 += 1;
      }
      else {
	 d0 = 0;
      }
      if (d0 >= count)
	 break;
      limit += 1;
   }
}



static int change_mode_b(void)
{
   int loop;
   int d0;
   int validcount;
   int limit;

   wait_for_sw_sleep(g_port, 4096);
   validcount = _sw_poll(g_port, g_endstrobelimit);

   limit = 256;
   loop = 0;
   while (loop < limit) {
      wait_for_sw_sleep(g_port, 4096);

      _enter_critical();

      outportb(g_port, 0x00);

      d0 = 0;
      while (d0 < (g_base_count * (4 + loop))) {
	 inportb(g_port);
	 d0 += 1;
      }
      d0 = 0;
      while (d0 < validcount) {
	 outportb(g_port, 0x00);
	 d0 += 1;
      }

      _exit_critical();

      wait_for_sw_sleep(g_port, 4096);
      sw_poll();
      if (g_sw_status && (g_sw_mode == SWMODE_B))
	 break;
      loop += 1;
   }

   if (loop >= limit)
      return FALSE;

   return TRUE;
}
