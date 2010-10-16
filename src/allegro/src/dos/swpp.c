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
 *      Preliminary driver for Microsoft Sidewinder 3D/Precision/
 *      Force Feedback Pro Joysticks.
 *
 *      By Acho A. Tang.
 *
 *      Notes on the 3D Pro:
 *        - digital initialization is not 100% reliable,
 *        - toggling the TM/CH switch will put the joystick back
 *          in analogue mode,
 *        - the stick must remain in a near-neutral position during
 *          detection or the process will fail.
 *
 *      Notes on the Force Feedback Pro:
 *        - the driver has not been tested on a real FF Pro and it
 *          lacks force feedback support.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif


#define SWPP_DEBUG      0 /* Compiler switch for a debug-only build */
#define SWPP_VERBOSE    0 /* Compiler switch for debug messages */

#define SWPP_NPORTS     2 /* Number of game ports to scan */
#define SWPP_NSCANS     8 /* Number of scan attempts on each port */
#define SWPP_NSUCCESS   4 /* Number of successful attempts to pass detection */
#define SWPP_WAITID   800 /* Max wait for ID packet triplets [800 us](arbitrary) */
#define SWPP_START    400 /* Max wait for the first data triplet [400 us](arbitrary) */
#define SWPP_STROBE    50 /* Max wait for each successive data triplet [50 us](theoretical minimum [11 us]) */
#define SWPP_CMDELAY   80 /* Interrupt command delay [80 us](must be within X timer's period [354.2 us]) */
#define SWPP_MAXBITS  256 /* Max number of triplets */

#define NTSC_CLOCK 14318180.0
#define SW3D_POLLOUT 3000 /* Max analogue polling timeout [3 ms] */
#define SW3D_FID       32
#define SW3D_TSUB1     (115*2+(305-115)*2/SW3D_FID) /* SW3D A->D time constant 1 */
#define SW3D_TSUB2     (SW3D_TSUB1+697+775)         /* SW3D A->D time constant 2 */
#define SW3D_TSUB3     (SW3D_TSUB1+288+312)         /* SW3D A->D time constant 3 */

#define SWPP_NID       57 /* No. of triplets in a Precision Pro full packet */
#define SWFF_NID       30 /* No. of triplets in a Force Feedback Pro full packet *** UNCONFIRMED *** */
#define SW3D_NID      226 /* No. of triplets in a 3D Pro full packet */

#define SWPP_NDATA     16 /* No. of triplets in a Precision Pro data packet */
#define SWFF_NDATA     16 /* No. of triplets in a Force Feedback Pro data packet */
#define SW3D_NDATA     22 /* No. of triplets in a 3D Pro data packet */
#define SW3D_NSTREAM   (SW3D_NDATA*3)

enum { SWXX=0, SWPP, SWFF, SW3D };


/* driver functions */
static int swpp_init(void);
static void swpp_exit(void);
static int swpp_poll(void);

#if !SWPP_DEBUG
JOYSTICK_DRIVER joystick_sw_pp =
{
	JOY_TYPE_SIDEWINDER_PP,
	empty_string,
	empty_string,
	"Sidewinder Precision Pro",
	swpp_init,
	swpp_exit,
	swpp_poll,
	NULL,  /* AL_METHOD(int, save_data, (void)); */
	NULL,  /* AL_METHOD(int, load_data, (void)); */
	NULL,  /* AL_METHOD(AL_CONST char *, calibrate_name, (int n)); */
	NULL   /* AL_METHOD(int, calibrate, (int n)); */
};
#endif


/* global context */
static struct SWINF_TYPE
{
	unsigned int port, speed, mode, nid, ndata;
	unsigned int buttons;
	int hatx, haty;
	int x, y;
	int throttle, twist;
} swinf;

static int swpp_initialized = 0;


/* utility macros */
#define US2COUNT(u, s) ((u * s) >> 10)
#define WAITSWPP(p, c) do inportb(p); while (--c > 0)


/* estimates game port speed in queries/ms (WARNING: timer 2 will be reprogrammed) */
extern void _swpp_get_portspeed(unsigned int port, unsigned int npasses, unsigned int *nqueries, unsigned int *elapsed);

static unsigned int get_portspeed(unsigned int port)
{
#define NQUERIES 1024

	unsigned int elapsed[5], nqueries[5]={1,NQUERIES+1,NQUERIES+1,NQUERIES+1,1};
	unsigned int emin=-1, emax=0, e, imin=0, imax=0, i;
	double e64, r64;
	unsigned char ah, al;

	ah = al = inportb(0x61); al &= ~2; al |= 1; outportb(0x61, al); /* enable timer channel 2 */
	outportb(0x43, 0xb4); /* wake timer */


	_enter_critical();

	_swpp_get_portspeed(port, 5, nqueries, elapsed);

	_exit_critical();


	outportb(0x61, ah);

	for (i=1; i<4; i++)
	{
		e = elapsed[i];

		if (e <  emin) { emin = e; imin = i; }
		if (e >= emax) { emax = e; imax = i; }
	}

	e = elapsed[6-imin-imax] - elapsed[4];

	if ((int)e > 0)
	{
		r64 = (double)e * 1000000;
		e64 = 1193182.0 * NQUERIES * 1024;
		e64 /= r64;

		e = (unsigned int)e64;
	}
	else e = 0;

#if SWPP_VERBOSE
	printf("SWPP get_portspeed(%xh): %u queries/ms\n", port, e);
#endif

	return(e);

#undef NQUERIES
}


/* attempts to switch the Sidewinder 3D Pro into digital mode
 *
 * WARNING: No time-out during polling. Computer may freeze if timer gets
 *          stuck although it rarely happens. A better solution is to use
 *          timer 0 interrupt provided that it's not being used by Allegro.
 */
extern void _swpp_init_digital(unsigned int port, unsigned int ncmds, unsigned char *cmdiv, unsigned int *timeout);

static int init_digital(unsigned int port, unsigned int speed)
{
	unsigned char cmdiv[6];
	unsigned char ah, al;
	unsigned int i;

	ah = al = inportb(0x61); al &= ~2; al |= 1; outportb(0x61, al); /* enable timer channel 2 */
	outportb(0x43, 0xb0); /* wake timer */

	i = NTSC_CLOCK * SW3D_TSUB1 / 24000000; /* clock divisor for command 1 delay */
	cmdiv[0] = i & 0xff;
	cmdiv[1] = i>>8;

	i = NTSC_CLOCK * SW3D_TSUB2 / 24000000; /* clock divisor for command 2 delay */
	cmdiv[2] = i & 0xff;
	cmdiv[3] = i>>8;

	i = NTSC_CLOCK * SW3D_TSUB3 / 24000000; /* clock divisor for command 3 delay */
	cmdiv[4] = i & 0xff;
	cmdiv[5] = i>>8;

	i = US2COUNT(SW3D_POLLOUT * 3, speed); /* axis 0 timeout */


	_enter_critical();

	_swpp_init_digital(port, 3, cmdiv, &i);

	_exit_critical();


	outportb(0x61, ah);

#if SWPP_VERBOSE
	printf("SWPP init_digital(%xh): D1=%x%02x D2=%x%02x D3=%x%02x\n",
			port, cmdiv[1], cmdiv[0], cmdiv[3], cmdiv[2], cmdiv[5], cmdiv[4]);
#endif

	return(i);
}


/* standard 4-axis polling */
extern void _swpp_read_analogue(unsigned int port, unsigned int *outbuf, unsigned int pollout);

static int read_analogue(unsigned int port, unsigned int speed, unsigned char *buf)
{
	unsigned int outbuf[6];
	unsigned int pollout, i;

	if (inportb(port))
	{
		pollout = US2COUNT(SW3D_POLLOUT, speed);


		_enter_critical();

		_swpp_read_analogue(port, outbuf, pollout);

		_exit_critical();


		if (!outbuf[5])
			for (i=1; i<5; i++) { if (outbuf[i] >= pollout) outbuf[i] = -1; }
	}
	else
	{
		for (i=0; i<6; i++) outbuf[i] = -1;
		pollout = -1;
	}

#if SWPP_VERBOSE
	i = outbuf[0];

	printf("SWPP read_analogue(%xh): limit=%d A0=%d A1=%d A2=%d A3=%d B=%04x\n", port, pollout,
			outbuf[1], outbuf[2], outbuf[3], outbuf[4], (i&1)|((i&2)<<3)|((i&4)<<6)|((i&8)<<9));
#endif

	for (i=0; i<6; i++) buf[i] = (unsigned char)outbuf[i];

	return(~outbuf[0] & 0x10);
}


/* thanks to Vojtech (vojtech@suse.cz) for the technical infos */
static unsigned int read_packet(unsigned int port, unsigned int speed, unsigned char *buf, int length, int id)
{
	int timeout, start, strobe, cmdelay, cmout=0, count=0, i;
	unsigned char ah, al=0;

	if (id)
	{
		timeout = start = US2COUNT(SWPP_WAITID, speed);
		cmdelay = US2COUNT(SWPP_CMDELAY, speed);

		_enter_critical();

		outportb(port, 0);
		al = inportb(port);
		do
		{
			ah = al;
			al = inportb(port);

			/* rising edge on button 0 */
			if (al & ~ah & 0x10)
			{
				buf[count++] = al >> 5;
				if (count >= length) break;
				timeout = start;
			}

			/* falling edge on axis 0 */
			if (cmdelay && ~al & ah & 0x01) cmout = 1;
			if (cmout && !(--cmdelay))
			{
				cmout = 0;
				outportb(port, 0);
			}
		}
		while (--timeout);

		_exit_critical();

	}
	else
	{
		timeout = start = US2COUNT(SWPP_START, speed);
		strobe = US2COUNT(SWPP_STROBE, speed);

		_enter_critical();

		outportb(port, 0);
		al = inportb(port);
		do
		{
			ah = al;
			al = inportb(port);

			/* rising edge on button 0 */
			if (al & ~ah & 0x10)
			{
				buf[count++] = al >> 5;
				if (count >= length) break;
				timeout = strobe;
			}
		}
		while (--timeout);

		_exit_critical();
	}

#if SWPP_VERBOSE
	printf("SWPP read_packet(%xh): %d triplets [", port, count);
	for (i=0; i<count; i++) printf("%1x", buf[i]); printf("]\n");
#endif

	return(count);
}


/* compares packet parity against the parity bit */
static unsigned int check_parity(unsigned char *buf, unsigned int length)
{
	unsigned char xbits[8]={0,1,1,0,1,0,0,1};
	unsigned char soft_parity, hard_parity, i, j;

	j = (unsigned char)length;
	length--;

	soft_parity = 1;
	hard_parity = buf[length] >> 2;
	buf[length] &= 3;

	for (i=0; i<j; i++) soft_parity ^= xbits[buf[i]];

	return(soft_parity == hard_parity);
}


/* verifies sync parity, compares checksum, and bit-packs good SW3D triplets */
static unsigned int pack_sw3d(unsigned char *buf, unsigned int length)
{
	unsigned char *workbuf;
	unsigned long pack64[2];
	unsigned long temp32;
	int sum, shift, n, i;

	if ((i = length - SW3D_NSTREAM) < 0) return(0);
	workbuf = buf + i + SW3D_NSTREAM + SW3D_NDATA;

	for (n=-SW3D_NSTREAM; n; n+=SW3D_NDATA)
	{
		pack64[0] = pack64[1] = 0;
		sum = shift = 0;

		for (i=-SW3D_NDATA; i; i++)
		{
			temp32 = (unsigned long)workbuf[n+i];

			if (shift > 29 && shift < 32)
			{
				pack64[0] |= temp32<<shift;
				pack64[1] |= temp32>>(32-shift);
			}
			else
				pack64[shift>>5 & 1] |= temp32<<(shift & 0x1f);

			shift += 3;
		}

		if ((pack64[0]^0x80) & 0x80808080 || pack64[1] & 0x80808080) continue;

		temp32 = pack64[0];
		do sum += temp32 & 0x0f; while (temp32 >>= 4);

		temp32 = pack64[1];
		do sum += temp32 & 0x0f; while (temp32 >>= 4);

		if (!(sum & 0x0f))
		{
			((unsigned long *)buf)[0] = pack64[0];
			((unsigned long *)buf)[1] = pack64[1];
			return(1);
		}
	}

	return(0);
}


/* parses SWPP triplets into meaningful values */
static void decode_swpp(struct SWINF_TYPE *inf, unsigned char *buf)
{
	int data, pos;

	data = buf[0] | buf[1]<<3 | buf[2]<<6;
	inf->buttons = ~data & 0x1ff;

	data = buf[3] | buf[4]<<3 | buf[5]<<6 | (buf[6]&1)<<9;
	inf->x = (data >> 2) - 128;

	data = (buf[6]&6)>>1 | buf[7]<<2 | buf[8]<<5 | (buf[9]&3)<<8;
	inf->y = (data >> 2) - 128;

	data = (buf[9]&4)>>2 | buf[10]<<1 | buf[11]<<4;
	data = ~data & 0x7f;
	inf->throttle = (data << 1) + (data >> 6) - 128;

	data = buf[12] | buf[13]<<3;
	inf->twist = (data << 2) + (data >> 4) - 128;

	data = buf[14] | (buf[15]&1)<<3;
	data = (1 << (data - 1)) & 0xff;

	if (data & 0xe0) pos = -128; else
	if (data & 0x0e) pos = 127; else
	pos = 0;
	inf->hatx = pos;

	if (data & 0x83) pos = -128; else
	if (data & 0x38) pos = 127; else
	pos = 0;
	inf->haty = pos;

#if SWPP_VERBOSE
	printf("SWPP decode_swpp(): x=%d y=%d tw=%d th=%d hx=%d hy=%d B=%03x\n",
			inf->x, inf->y, inf->twist, inf->throttle, inf->hatx, inf->haty, inf->buttons);
#endif
}


/* parse packed SW3D triplets into meaningful values */
static void decode_sw3d(struct SWINF_TYPE *inf, unsigned char *buf)
{
	int data, pos;

	data = (buf[1]&0x7f) | (buf[4]&0x40)<<1;
	inf->buttons = (~data & 0xff) | (buf[4]&0x20)<<3;

	data = (buf[0]&0x38)<<4 | (buf[2]&0x7f);
	inf->x = (data >> 2) - 128;

	data = (buf[0]&0x07)<<7 | (buf[3]&0x7f);
	inf->y = (data >> 2) - 128;

	data = (buf[4]&0x18)<<4 | (buf[5]&0x7f);
	inf->twist = (data >> 1) - 128;

	data = (buf[4]&0x07)<<7 | (buf[6]&0x7f);
	data = ~data & 0x3ff;
	inf->throttle = (data >> 2) -128;

	data = (buf[0]&0x40)>>3 | (buf[7]&0x70)>>4;
	data = (1 << (data - 1)) & 0xff;

	if (data & 0xe0) pos = -128; else
	if (data & 0x0e) pos = 127; else
	pos = 0;
	inf->hatx = pos;

	if (data & 0x83) pos = -128; else
	if (data & 0x38) pos = 127; else
	pos = 0;
	inf->haty = pos;

#if SWPP_VERBOSE
	printf("SWPP decode_sw3d(): x=%d y=%d tw=%d th=%d hx=%d hy=%d B=%03x\n",
			inf->x,inf->y,inf->twist,inf->throttle,inf->hatx,inf->haty,inf->buttons);
#endif
}


/* initializes the driver */
static int swpp_init(void)
{
	unsigned char buf[SWPP_MAXBITS];
	unsigned int portlist[SWPP_NPORTS]={0x201,0x200};
	unsigned int port=-1, speed=-1, mode=-1, ndata=-1, nid=-1, avgspeed=0;
	unsigned int tries=SWPP_NSCANS, lockport=0, success=0, counter, i;

#if !SWPP_DEBUG
	/* fill joystick info */
	num_joysticks = 1;

	joy[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	joy[0].num_sticks = 4;
	joy[0].num_buttons = 9;

	joy[0].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	joy[0].stick[0].num_axis = 2;
	joy[0].stick[0].name = get_config_text("Stick");
	joy[0].stick[0].axis[0].name = get_config_text("X");
	joy[0].stick[0].axis[1].name = get_config_text("Y");

	joy[0].stick[1].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	joy[0].stick[1].num_axis = 1;
	joy[0].stick[1].name = get_config_text("Twist");
	joy[0].stick[1].axis[0].name = get_config_text("");

	joy[0].stick[2].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	joy[0].stick[2].num_axis = 1;
	joy[0].stick[2].name = get_config_text("Throttle");
	joy[0].stick[2].axis[0].name = get_config_text("");

	joy[0].stick[3].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	joy[0].stick[3].num_axis = 2;
	joy[0].stick[3].name = get_config_text("Hat");
	joy[0].stick[3].axis[0].name = get_config_text("X");
	joy[0].stick[3].axis[1].name = get_config_text("Y");

	joy[0].button[0].name = get_config_text("B1");
	joy[0].button[1].name = get_config_text("B2");
	joy[0].button[2].name = get_config_text("B3");
	joy[0].button[3].name = get_config_text("B4");
	joy[0].button[4].name = get_config_text("B5");
	joy[0].button[5].name = get_config_text("B6");
	joy[0].button[6].name = get_config_text("B7");
	joy[0].button[7].name = get_config_text("B8");
	joy[0].button[8].name = get_config_text("B9");
#endif

	if (swpp_initialized)
	{
		read_packet(swinf.port, swinf.speed, buf, SWPP_MAXBITS, 1);
		counter = US2COUNT(50000, swinf.speed); WAITSWPP(swinf.port, counter);

		return(0);
	}

	/* do a simple detection run on each port */
	do
	{
		i = 0;
		do
		{
			if (!lockport) port = portlist[i];

			/* check port speed */
			if (!(speed = get_portspeed(port))) continue;
			counter = US2COUNT(50000, speed); WAITSWPP(port, counter);

			/* check X timer */
			if (!read_analogue(port, speed, buf)) continue;
			counter = US2COUNT(50000, speed); WAITSWPP(port, counter);

			/* check ID */
			if (!(nid = read_packet(port, speed, buf, SWPP_MAXBITS, 1)))
			{
				/* initiate digital mode and recheck ID */
				if (!init_digital(port, speed)) continue;
				counter = US2COUNT(300000, speed); WAITSWPP(port, counter);

				if (!(nid = read_packet(port, speed, buf, SWPP_MAXBITS, 1))) continue;
			}
			counter = US2COUNT(50000, speed); WAITSWPP(port, counter);
			lockport = 1;

			/* check packet length and parity */
			ndata = read_packet(port, speed, buf, SWPP_MAXBITS, 0);
			counter = US2COUNT(50000, speed); WAITSWPP(port, counter);

			mode = SWXX;

			if (nid == SWPP_NID)
			{
				if (ndata != SWPP_NDATA || !check_parity(buf, ndata)) continue;
				mode = SWPP;
			}
			else if (nid >= SW3D_NID)
			{
				if (ndata < SW3D_NSTREAM || !pack_sw3d(buf, ndata)) continue;
				mode = SW3D; nid = SW3D_NID; ndata = SW3D_NSTREAM;
			}
			else
			{
				if (ndata != SWFF_NDATA || !check_parity(buf, ndata)) continue;
				mode = SWFF;
			}

			avgspeed += speed;
			success++;
		}
		while (!lockport && ++i < SWPP_NPORTS);
	}
	while (success < SWPP_NSUCCESS && --tries + success >= SWPP_NSUCCESS);

	if (success < SWPP_NSUCCESS) { swinf.mode = mode; return(-1); }

	avgspeed /= SWPP_NSUCCESS;

	memset(&swinf, 0, sizeof(struct SWINF_TYPE));
	swinf.port = port;
	swinf.speed = avgspeed;
	swinf.mode = mode;
	swinf.nid = nid;
	swinf.ndata = ndata;

	swpp_initialized = 1;

#if SWPP_VERBOSE
	printf("SWPP swpp_init(): port=%xh speed=%d mode=%d nid=%d ndata=%d\n",port, avgspeed, mode, nid, ndata);
#endif

	return(0);
}


/* polls the joystick state */
static int swpp_poll(void)
{
	unsigned char buf[SWPP_MAXBITS];
	unsigned int i;
	int d;

	switch (swinf.mode)
	{
		case SWPP:
		case SWFF:
			if (read_packet(swinf.port, swinf.speed, buf, swinf.ndata, 0) == swinf.ndata)
				if (check_parity(buf, swinf.ndata)) decode_swpp(&swinf, buf);
		break;

		case SW3D:
			if ((i = read_packet(swinf.port, swinf.speed, buf, swinf.ndata+1, 0)) >= swinf.ndata)
				if (pack_sw3d(buf, i)) decode_sw3d(&swinf, buf);
		break;
	}

#if !SWPP_DEBUG
	d = swinf.x;
	joy[0].stick[0].axis[0].pos = d;
	joy[0].stick[0].axis[0].d1  = d <= -64;
	joy[0].stick[0].axis[0].d2  = d >= 64;

	d = swinf.y;
	joy[0].stick[0].axis[1].pos = d;
	joy[0].stick[0].axis[1].d1  = d <= -64;
	joy[0].stick[0].axis[1].d2  = d >= 64;

	d = swinf.twist;
	joy[0].stick[1].axis[0].pos = d;
	joy[0].stick[1].axis[0].d1  = d <= -64;
	joy[0].stick[1].axis[0].d2  = d >= 64;

	d = swinf.throttle;
	joy[0].stick[2].axis[0].pos = d;
	joy[0].stick[2].axis[0].d1  = d <= -64;
	joy[0].stick[2].axis[0].d2  = d >= 64;

	d = swinf.hatx;
	joy[0].stick[3].axis[0].pos = d;
	joy[0].stick[3].axis[0].d1 = d <= -64;
	joy[0].stick[3].axis[0].d2 = d >= 64;

	d = swinf.haty;
	joy[0].stick[3].axis[1].pos = d;
	joy[0].stick[3].axis[1].d1 = d <= -64;
	joy[0].stick[3].axis[1].d2 = d >= 64;

	d = swinf.buttons;
	for (i=0; i<9; i++) joy[0].button[i].b = d & (1 << i);
#endif

	return(0);
}


/* shuts down the driver */
static void swpp_exit(void) {}


#if SWPP_DEBUG
int main(void)
{
	swpp_init();

	if (swinf.mode != -1)
	{
		printf("\nFound ");
		switch (swinf.mode)
		{
			default:
			case SWXX: printf("unsupported Sidewinder input device\n"); break;
			case SWPP: printf("Sidewinder Precision Pro joystick\n"); break;
			case SWFF: printf("Sidewinder Force Feedback Pro joystick\n"); break;
			case SW3D: printf("Sidewinder 3D Pro joystick\n");
		}
	}
	else printf("\nNo Sidewinder joystick found\n");

	return(0);
}
#endif
