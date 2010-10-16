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
 *      Ensoniq Soundscape driver.
 *
 *      By Andreas Kluge.
 *
 *      Based on code by Andrew P. Weir.
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



#define ODIE            0       /* ODIE gate array */
#define OPUS            1       /* OPUS gate array */
#define MMIC            2       /* MiMIC gate array */

static int soundscape_hw_ver = -1;      /* as reported by detection */

static char *ensoniq_gate_array[] = { "ODIE", "OPUS", "MiMIC" };


#define GA_HOSTCTL_OFF  2       /* host port ctrl/stat reg */
#define GA_ADDR_OFF     4       /* indirect address reg */
#define GA_DATA_OFF     5       /* indirect data reg */
#define GA_CODEC_OFF    8       /* for some boards CoDec is fixed from base */

#define GA_DMAB_REG     3       /* DMA chan B assign reg */
#define GA_INTCFG_REG   4       /* interrupt configuration reg */
#define GA_DMACFG_REG   5       /* DMA configuration reg */
#define GA_CDCFG_REG    6       /* CD-ROM/CoDec config reg */
#define GA_HMCTL_REG    9       /* host master control reg */

#define CD_ADDR_OFF     0       /* indirect address reg */
#define CD_DATA_OFF     1       /* indirect data reg */
#define CD_STATUS_OFF   2       /* status register */


#define OUT_TO_ADDR(n)  outportb(soundscape_waveport + CD_ADDR_OFF, n)

#define CODEC_MODE_CHANGE_ON()   OUT_TO_ADDR(0x40)
#define CODEC_MODE_CHANGE_OFF()  OUT_TO_ADDR(0x00)


#define CD_ADCL_REG     0       /* left DAC input control reg */
#define CD_ADCR_REG     1       /* right DAC input control reg */
#define CD_CDAUXL_REG   2       /* left DAC output control reg */
#define CD_CDAUXR_REG   3       /* right DAC output control reg */
#define CD_DACL_REG     6       /* left DAC output control reg */
#define CD_DACR_REG     7       /* right DAC output control reg */
#define CD_FORMAT_REG   8       /* clock and data format reg */
#define CD_CONFIG_REG   9       /* interface config register */
#define CD_PINCTL_REG   10      /* external pin control reg */
#define CD_UCOUNT_REG   14      /* upper count reg */
#define CD_LCOUNT_REG   15      /* lower count reg */
#define CD_XFORMAT_REG  28      /* extended format reg - 1845 record */
#define CD_XUCOUNT_REG  30      /* extended upper count reg - 1845 record */
#define CD_XLCOUNT_REG  31      /* extended lower count reg - 1845 record */


#define ENABLE_CODEC_IRQ()    cd_write(CD_PINCTL_REG, cd_read(CD_PINCTL_REG) | 0x02)
#define DISABLE_CODEC_IRQ()   cd_write(CD_PINCTL_REG, cd_read(CD_PINCTL_REG) & 0xFD);


static int soundscape_enabled = FALSE;
static int soundscape_mem_allocated = FALSE;
static int soundscape_dma_count = 0;
static int soundscape_dma;
static int soundscape_freq;
static int soundscape_baseport;              /* gate Array/MPU-401 port */
static int soundscape_waveport;              /* the AD-1848 base port */
static int soundscape_midiirq;               /* the MPU-401 IRQ */
static int soundscape_waveirq;               /* the PCM IRQ */

static int soundscape_detect(int input);
static int soundscape_init(int input, int voices);
static void soundscape_exit(int input);
static int soundscape_set_mixer_volume(int volume);
static int soundscape_buffer_size(void);

static int soundscape_int = -1;              /* interrupt vector */
static int soundscape_dma_size = -1;         /* size of dma transfer */

static volatile int soundscape_semaphore = FALSE;

static int soundscape_sel;                   /* selector for the DMA buffer */
static unsigned long soundscape_buf[2];      /* pointers to the two buffers */
static int soundscape_bufnum = 0;            /* the one currently in use */

static void soundscape_lock_mem(void);

static char soundscape_desc[256] = EMPTY_STRING;

static int cd_cfg_save;             /* gate array register save area */
static int dma_cfg_save;            /* gate array register save area */
static int int_cfg_save;            /* gate array register save area */

static int dac_save_l;              /* DAC left volume save */
static int dac_save_r;              /* DAC right volume save */
static int cdx_save_l;              /* CD/Aux left volume save */
static int cdx_save_r;              /* CD/Aux right volume save */
static int adc_save_l;              /* ADC left volume save */
static int adc_save_r;              /* ADC right volume save */

static int ss_irqs[4] = { 9, 5, 7, 10 };
static int rs_irqs[4] = { 9, 7, 5, 15 };

static int *soundscape_irqset;      /* pointer to one of the IRQ sets */

static int soundscape_detected = FALSE; 


typedef struct                      /* DMA controller registers ... */
{
   unsigned char addr;              /* address register, lower/upper */
   unsigned char count;             /* address register, lower/upper */
   unsigned char status;            /* status register */
   unsigned char mask;              /* single channel mask register */
   unsigned char mode;              /* mode register */
   unsigned char clrff;             /* clear flip-flop register */
   unsigned char page;              /* fixed page register */
} DMAC_REGS;


static DMAC_REGS dmac_regs[4] =      /* the DMAC regs for chans 0-3 */
{ 
   { 0x00, 0x01, 0x08, 0x0A, 0x0B, 0x0C, 0x87 },
   { 0x02, 0x03, 0x08, 0x0A, 0x0B, 0x0C, 0x83 },
   { 0x04, 0x05, 0x08, 0x0A, 0x0B, 0x0C, 0x81 },
   { 0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0C, 0x82 }
};


static DMAC_REGS *dmac_reg_p;       /* a pointer to a DMAC reg struct */



DIGI_DRIVER digi_soundscape =
{
   DIGI_SOUNDSCAPE,
   empty_string,
   empty_string,
   "Soundscape",
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   soundscape_detect,
   soundscape_init,
   soundscape_exit,
   soundscape_set_mixer_volume,
   NULL,
   NULL,
   NULL,
   soundscape_buffer_size,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,
   _mixer_get_position,
   _mixer_set_position,
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,
   0, 0,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};



/* soundscape_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int soundscape_buffer_size(void)
{
   return soundscape_dma_size/4; /* convert bytes to stereo 16 bit samples */
}



/* cd_write:
 *  This function is used to write the indirect addressed registers in the
 *  Ad-1848 or compatible CoDec. It will preserve the special function bits
 *  in the upper-nibble of the indirect address register.
 */
static void cd_write(int rnum, int value)
{
   OUT_TO_ADDR((inportb(soundscape_waveport + CD_ADDR_OFF) & 0xF0) | rnum);
   outportb(soundscape_waveport + CD_DATA_OFF, value);
}



/* cd_read:
 *  This function is used to read the indirect addressed registers in the
 *  AD-1848 or compatible CoDec. It will preserve the special function bits
 *  in the upper-nibble of the indirect address register.
 */
static int cd_read(int rnum)
{
   OUT_TO_ADDR((inportb(soundscape_waveport + CD_ADDR_OFF) & 0xF0) | rnum);
   return inportb(soundscape_waveport + CD_DATA_OFF);
}



/* ga_write:
 *  This function is used to write the indirect addressed registers in the
 *  Ensoniq Soundscape gate array.
 */
static void ga_write(int rnum, int value)
{
   outportb(soundscape_baseport + GA_ADDR_OFF, rnum);
   outportb(soundscape_baseport + GA_DATA_OFF, value);
}



/* ga_read:
 *  This function is used to read the indirect addressed registers in the
 *  Ensoniq Soundscape gate array.
 */
static int ga_read(int rnum)
{
   outportb(soundscape_baseport + GA_ADDR_OFF, rnum);
   return inportb(soundscape_baseport + GA_DATA_OFF);
}



/* set_dac_vol:
 *  This function sets the left and right DAC output level in the CoDec.
 */
static void set_dac_vol(int lvol, int rvol)
{
   cd_write(CD_DACL_REG, ~(lvol >> 2) & 0x3F);
   cd_write(CD_DACR_REG, ~(rvol >> 2) & 0x3F);
}



/* resume_codec:
 *  This function will resume the CoDec auto-restart DMA process.
 */
static void resume_codec(int direction)
{
   cd_write(CD_CONFIG_REG, direction ? 0x02 : 0x01);
}



/* set_format:
 *  This function sets the CoDec audio data format for record or playback.
 */
static int set_format(int srate, int stereo, int size16bit, int direction)
{
   int format = 0;
   int i;

   /* first, find the sample rate ... */
   switch (srate) {

      case 5512:  format = 0x01; soundscape_dma_size = 512;  break;
      case 6615:  format = 0x0F; soundscape_dma_size = 512;  break;
      case 8000:  format = 0x00; soundscape_dma_size = 512;  break;
      case 9600:  format = 0x0E; soundscape_dma_size = 512;  break;
      case 11025: format = 0x03; soundscape_dma_size = 512;  break; /* 11363 */
      case 16000: format = 0x02; soundscape_dma_size = 512;  break; /* 17046 */
      case 18900: format = 0x05; soundscape_dma_size = 1024; break;
      case 22050: format = 0x07; soundscape_dma_size = 1024; break; /* 22729 */
      case 27428: format = 0x04; soundscape_dma_size = 1024; break;
      case 32000: format = 0x06; soundscape_dma_size = 2048; break;
      case 33075: format = 0x0D; soundscape_dma_size = 2048; break;
      case 37800: format = 0x09; soundscape_dma_size = 2048; break;
      case 44100: format = 0x0B; soundscape_dma_size = 2048; break; /* 44194 */
      case 48000: format = 0x0C; soundscape_dma_size = 2048; break;

      default:
	 return FALSE;
   }

   /* set other format bits ... */
   if (stereo)
      format |= 0x10;

   if (size16bit) {
      format |= 0x40;
      soundscape_dma_size *= 2;
   }

   soundscape_freq = srate;

   CODEC_MODE_CHANGE_ON();

   /* and write the format register */
   cd_write(CD_FORMAT_REG, format);

   /* if not using ODIE and recording, setup extended format register */
   if ((soundscape_hw_ver) != ODIE && (direction))
      cd_write(CD_XFORMAT_REG, format & 0x70);

   /* delay for internal re-sync */
   for (i=0; i<200000; i++)
      inportb(soundscape_baseport + GA_ADDR_OFF);

   CODEC_MODE_CHANGE_OFF();

   return TRUE;
}



/* stop_codec:
 *  This function will stop the CoDec auto-restart DMA process.
 */
static void stop_codec(void)
{
   int i;

   cd_write(CD_CONFIG_REG, cd_read(CD_CONFIG_REG) & 0xFC);

   /* let the CoDec receive its last DACK(s). The DMAC must not be */
   /* masked while the CoDec has DRQs pending. */
   for (i=0; i<64; i++) {
      if (!(inportb(dmac_reg_p->status) & (0x10 << soundscape_dma)))
	 break;
   }
}



/* get_ini_config_entry:
 *  This function parses a file (SNDSCAPE.INI) for a left-hand string and,
 *  if found, writes its associated right-hand value to a destination buffer.
 *  This function is case-insensitive.
 */
static int get_ini_config_entry(char *entry, char *dest, unsigned int dest_size, FILE *fp)
{
   char str[83];
   char tokstr[33];
   char *p;

   /* make a local copy of the entry, upper-case it */
   _al_sane_strncpy(tokstr, entry, sizeof(tokstr));
   strupr(tokstr);

   /* rewind the file and try to find it... */
   rewind(fp);

   for (;;) {
      /* get the next string from the file */
      fgets(str, 83, fp);
      if (feof(fp)) {
	 fclose(fp);
	 return -1;
      }

      /* properly terminate the string */
      for (p=str; *p; p++) {
	 if (uisspace(*p)) {
	    *p = 0;
	    break;
	 }
      }

      /* see if it's an 'equate' string; if so, zero the '=' */
      p = strchr(str, '=');
      if (!p)
	 continue;

      *p = 0;

      /* upper-case the current string and test it */
      strupr(str);

      if (strcmp(str, tokstr))
	 continue;

      /* it's our string - copy the right-hand value to buffer */
      _al_sane_strncpy(dest, str+strlen(str)+1, dest_size);
      break;
   }

   return 0;
}



/* get_init_config:
 *  This function gets all parameters from a file (SNDSCAPE.INI)
 */
static int get_init_config(void)
{
   FILE *fp = NULL;
   char str[78];
   char *ep;

   /* get the environment var and build the filename, then open it */
   if (!(ep = getenv("SNDSCAPE")))
      return FALSE;

   _al_sane_strncpy(str, ep, sizeof(str));

   if (str[strlen(str)-1] == '\\')
      str[strlen(str)-1] = 0;

   strncat(str, "\\SNDSCAPE.INI", sizeof(str)-1);

   if (!(fp = fopen(str, "r")))
      return FALSE;

   /* read all of the necessary config info ... */
   if (get_ini_config_entry("Product", str, sizeof(str), fp)) {
      fclose(fp);
      return FALSE;
   }

   /* if an old product name is read, set the IRQs accordingly */
   strupr(str);
   if (strstr(str, "SOUNDFX") || strstr(str, "MEDIA_FX"))
      soundscape_irqset = rs_irqs;
   else
      soundscape_irqset = ss_irqs;

   if (get_ini_config_entry("Port", str, sizeof(str), fp)) {
      fclose(fp);
      return FALSE;
   }

   soundscape_baseport = strtol(str, NULL, 16);

   if (get_ini_config_entry("WavePort", str, sizeof(str), fp)) {
      fclose(fp);
      return FALSE;
   }

   soundscape_waveport = strtol(str, NULL, 16);

   if (get_ini_config_entry("IRQ", str, sizeof(str), fp)) {
      fclose(fp);
      return FALSE;
   }

   soundscape_midiirq = strtol(str, NULL, 10);

   if (soundscape_midiirq == 2)
      soundscape_midiirq = 9;

   if (get_ini_config_entry("SBIRQ", str, sizeof(str), fp)) {
      fclose(fp);
      return FALSE;
   }

   soundscape_waveirq = strtol(str, NULL, 10);

   if (soundscape_waveirq == 2)
      soundscape_waveirq = 9;

   if (get_ini_config_entry("DMA", str, sizeof(str), fp)) {
      fclose(fp);
      return FALSE;
   }

   soundscape_dma = strtol(str, NULL, 10);

   fclose(fp);
   return TRUE;
}



/* detect_soundscape:
 *  This function is used to detect the presence of a Soundscape card in a
 *  system. It will read the hardware config info from the SNDSCAPE.INI file,
 *  the path to which is indicated by the SNDSCAPE environment variable. This
 *  config info will be stored in global variable space for the other driver
 *  functions to reference. Once the config settings have been determined, a
 *  hardware test will be performed to see if the Soundscape card is actually
 *  present. If this function is not explicitly called by the application, it
 *  it will be called by the OpenSoundscape function.
 */
static int detect_soundscape(void)
{
   int tmp;

   if (!get_init_config())
      return FALSE;

   /* see if Soundscape is there by reading HW ... */
   if ((inportb(soundscape_baseport + GA_HOSTCTL_OFF) & 0x78) != 0x00)
      return FALSE;

   if ((inportb(soundscape_baseport + GA_ADDR_OFF) & 0xF0) == 0xF0)
      return FALSE;

   outportb(soundscape_baseport + GA_ADDR_OFF, 0xF5);
   tmp = inportb(soundscape_baseport + GA_ADDR_OFF);

   if ((tmp & 0xF0) == 0xF0)
      return FALSE;

   if ((tmp & 0x0F) != 0x05)
      return FALSE;

   /* formulate the chip ID */
   if ((tmp & 0x80) != 0x00)
      soundscape_hw_ver = MMIC;
   else if ((tmp & 0x70) != 0x00)
      soundscape_hw_ver = OPUS;
   else
      soundscape_hw_ver = ODIE;

   /* now do a quick check to make sure the CoDec is there too */
   if ((inportb(soundscape_waveport) & 0x80) != 0x00)
      return FALSE;

   soundscape_detected = TRUE;
   return TRUE;
}



/* soundscape_set_mixer_volume:
 *  Sets the Soundscape mixer volume for playing digital samples.
 */
static int soundscape_set_mixer_volume(int volume)
{
   if (volume >= 0)
      set_dac_vol(volume, volume);

   return 0;
}



/* soundscape_interrupt:
 *  The Soundscape end-of-buffer interrupt handler.
 */
static int soundscape_interrupt(void)
{
   /* if the CoDec is interrupting ... */
   if (inportb(soundscape_waveport + CD_STATUS_OFF) & 0x01) {

      /* clear the AD-1848 interrupt */
      outportb(soundscape_waveport + CD_STATUS_OFF, 0x00);

      soundscape_dma_count++;
      if (soundscape_dma_count > 16) {
	 soundscape_bufnum = (_dma_todo(soundscape_dma) > (unsigned)soundscape_dma_size) ? 1 : 0;
	 soundscape_dma_count = 0;
      }

      if (!(soundscape_semaphore)) {
	 soundscape_semaphore = TRUE;

	 /* mix some more samples */
	 ENABLE(); 
	 _mix_some_samples(soundscape_buf[soundscape_bufnum], _dos_ds, TRUE);
	 DISABLE();

	 soundscape_semaphore = FALSE;
      }

      soundscape_bufnum = 1 - soundscape_bufnum;
   }

   /* acknowledge interrupt */
   _eoi(soundscape_waveirq);
   return 0;
}

END_OF_STATIC_FUNCTION(soundscape_interrupt);



/* soundscape_detect:
 *  This function is used to detect the presence of a Soundscape card.
 */
static int soundscape_detect(int input)
{
   char tmp[64];

   /* input isn't supported yet */
   if (input)
      return FALSE;

   if (!detect_soundscape()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Soundscape not found"));
      return FALSE;
   } 

   /* figure out the hardware interrupt number */
   soundscape_int = _map_irq(soundscape_waveirq);

   /* get desired frequency from _sound variable */
   soundscape_freq = _sound_freq;
   if (soundscape_freq == -1)
      soundscape_freq = 48000;

   /* adjust SB-frequencies for this CoDec */
   if (soundscape_freq < 12000)              /* 11906 -> 11025 */
      soundscape_freq = 11025;
   else if (soundscape_freq < 18000)         /* 16129 -> 16000 */
      soundscape_freq = 16000;
   else if (soundscape_freq < 24000)         /* 22727 -> 22050 */
      soundscape_freq = 22050;
   else                                      /* 45454 -> 48000 */
      soundscape_freq = 48000;

   /* set up the card description */
   uszprintf(soundscape_desc, sizeof(soundscape_desc),
             get_config_text("Soundscape %s (%d hz) on port %X, using IRQ %d, DMA channel %d and waveport %X"),
             uconvert_ascii(ensoniq_gate_array[soundscape_hw_ver], tmp), 
	     soundscape_freq, soundscape_baseport, soundscape_waveirq, soundscape_dma, soundscape_waveport);

   digi_soundscape.desc = soundscape_desc;

   return TRUE;
}



/* soundscape_init:
 *  This function opens the Soundscape driver. It will setup the Soundscape
 *  hardware for Native PCM mode.
 */
static int soundscape_init(int input, int voices)
{
   int windx, mindx, tmp;

   /* see if we need to detect first */
   if (!soundscape_detected)
      if (!soundscape_detect(0))
	 return -1;

   /* set DMA controller register set pointer based on channel */
   dmac_reg_p = &dmac_regs[soundscape_dma];

   /* in case the CoDec is running, stop it */
   stop_codec();

   /* clear possible CoDec and SoundBlaster emulation interrupts */
   outportb(soundscape_waveport + CD_STATUS_OFF, 0x00);
   inportb(0x22E);

   /* input isn't supported yet, don't init twice without exit */
   if ((input) || (soundscape_enabled))
      return -1;

   /* set format, always stereo, 16 bit */
   set_format(soundscape_freq, 1, 1, 0);

   if (_dma_allocate_mem(soundscape_dma_size * 2, &soundscape_sel, &soundscape_buf[0]) != 0)
      return -1;

   soundscape_mem_allocated = TRUE;

   soundscape_buf[1] = soundscape_buf[0] + soundscape_dma_size;

   soundscape_lock_mem();

   digi_soundscape.voices = voices;

   if (_mixer_init(soundscape_dma_size/2, soundscape_freq, TRUE, TRUE, &digi_soundscape.voices) != 0) {
      soundscape_exit(0);
      return -1;
   }

   _mix_some_samples(soundscape_buf[0], _dos_ds, TRUE);
   _mix_some_samples(soundscape_buf[1], _dos_ds, TRUE);
   soundscape_bufnum = 0;

   _enable_irq(soundscape_waveirq);
   _install_irq(soundscape_int, soundscape_interrupt);

   if (soundscape_hw_ver != MMIC) {
      /* derive the MIDI and Wave IRQ indices (0-3) for reg writes */
      for (mindx=0; mindx<4; mindx++)
	 if (soundscape_midiirq == *(soundscape_irqset + mindx))
	    break;

      for (windx=0; windx<4; windx++)
	 if (soundscape_waveirq == *(soundscape_irqset + windx))
	    break;

      /* setup the CoDec DMA polarity */
      ga_write(GA_DMACFG_REG, 0x50);

      /* give the CoDec control of the DMA and Wave IRQ resources */
      cd_cfg_save = ga_read(GA_CDCFG_REG);
      ga_write(GA_CDCFG_REG, 0x89 | (soundscape_dma << 4) | (windx << 1));

      /* pull the Sound Blaster emulation off of those resources */
      dma_cfg_save = ga_read(GA_DMAB_REG);
      ga_write(GA_DMAB_REG, 0x20);
      int_cfg_save = ga_read(GA_INTCFG_REG);
      ga_write(GA_INTCFG_REG, 0xF0 | (mindx << 2) | mindx);
   }

   /* save all volumes that we might use */
   dac_save_l = cd_read(CD_DACL_REG);
   dac_save_r = cd_read(CD_DACR_REG);
   cdx_save_l = cd_read(CD_CDAUXL_REG);
   cdx_save_r = cd_read(CD_CDAUXL_REG);
   adc_save_l = cd_read(CD_ADCL_REG);
   adc_save_r = cd_read(CD_ADCR_REG);

   set_dac_vol(127, 127);

   /* select the mic/line input to the record mux */
   /* if not ODIE, set the mic gain bit too */
   cd_write(CD_ADCL_REG, (cd_read(CD_ADCL_REG) & 0x3F) | (soundscape_hw_ver == ODIE ? 0x80 : 0xA0));
   cd_write(CD_ADCR_REG, (cd_read(CD_ADCR_REG) & 0x3F) | (soundscape_hw_ver == ODIE ? 0x80 : 0xA0));

   /* put the CoDec into mode change state */
   CODEC_MODE_CHANGE_ON();

   /* setup CoDec mode - single DMA chan, AutoCal on */
   cd_write(CD_CONFIG_REG, 0x0c);

   ENABLE_CODEC_IRQ();

   _dma_start(soundscape_dma, soundscape_buf[0], soundscape_dma_size * 2, TRUE, FALSE) ;

   /* write the CoDec interrupt count - sample frames per half-buffer. */
   tmp = soundscape_dma_size / 4 - 1;
   cd_write(CD_LCOUNT_REG, tmp);
   cd_write(CD_UCOUNT_REG, tmp >> 8);

   CODEC_MODE_CHANGE_OFF();

   /* start the CoDec for output */
   resume_codec(0); 
   rest(100);

   soundscape_enabled = TRUE;

   return 0;
}



/* soundscape_exit:
 *  Soundscape driver cleanup routine, removes ints, stops dma,
 *  frees buffers, etc.
 */
static void soundscape_exit(int input)
{
   if (soundscape_enabled) {

      /* in case the CoDec is running, stop it */
      stop_codec();

      /* stop dma transfer */
      _dma_stop(soundscape_dma);

      /* disable the CoDec interrupt pin */
      DISABLE_CODEC_IRQ();

      /* restore all volumes ... */
      cd_write(CD_DACL_REG, dac_save_l);
      cd_write(CD_DACR_REG, dac_save_r);
      cd_write(CD_CDAUXL_REG, cdx_save_l);
      cd_write(CD_CDAUXL_REG, cdx_save_r);
      cd_write(CD_ADCL_REG, adc_save_l);
      cd_write(CD_ADCR_REG, adc_save_r);

      /* if necessary, restore gate array resource registers */
      if (soundscape_hw_ver != MMIC) {
	 ga_write(GA_INTCFG_REG, int_cfg_save);
	 ga_write(GA_DMAB_REG, dma_cfg_save);
	 ga_write(GA_CDCFG_REG, cd_cfg_save);
      }

      _remove_irq(soundscape_int);              /* restore interrupts */
      _restore_irq(soundscape_waveirq);         /* reset PIC channels */
   }

   if (soundscape_mem_allocated)
      __dpmi_free_dos_memory(soundscape_sel);   /* free DMA memory buffer */

   _mixer_exit();
   soundscape_hw_ver = -1;
   soundscape_detected = FALSE;
   soundscape_detected = FALSE;
   soundscape_enabled = FALSE;
}



/* soundscape_lock_mem:
 *  Locks all the memory touched by parts of the Soundscape code that are
 *  executed in an interrupt context.
 */
static void soundscape_lock_mem(void)
{
   LOCK_VARIABLE(digi_soundscape);
   LOCK_VARIABLE(soundscape_freq);
   LOCK_VARIABLE(soundscape_baseport);
   LOCK_VARIABLE(soundscape_waveport);
   LOCK_VARIABLE(soundscape_dma);
   LOCK_VARIABLE(soundscape_midiirq);
   LOCK_VARIABLE(soundscape_waveirq);
   LOCK_VARIABLE(soundscape_int);
   LOCK_VARIABLE(soundscape_hw_ver);
   LOCK_VARIABLE(soundscape_dma_size);
   LOCK_VARIABLE(soundscape_sel);
   LOCK_VARIABLE(soundscape_buf);
   LOCK_VARIABLE(soundscape_bufnum);
   LOCK_VARIABLE(soundscape_dma_count);
   LOCK_VARIABLE(soundscape_semaphore);
   LOCK_FUNCTION(soundscape_interrupt);

   _dma_lock_mem();
}

