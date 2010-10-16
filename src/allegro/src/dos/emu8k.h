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
 *      Low-level functions for the AWE32 driver, by George Foot.
 *
 *      Information taken primarily from "AWE32/EMU8000 Programmer's Guide"
 *      (AEPG) by Dave Rossum. The AEPG is part of the AWE32 Developers'
 *      Information Pack (ADIP) from Creative Labs.
 *
 *      I/O port verification technique taken from "The Un-official Sound
 *      Blaster AWE32 Programming Guide" by Vince Vu a.k.a. Judge Dredd
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DOS_EMU8K_H
#define ALLEGRO_DOS_EMU8K_H


typedef struct envparms_t 
{
   int envvol,  envval,  dcysus,  atkhldv;
   int lfo1val, atkhld,  lfo2val, ip;
   int ifatn,   pefe,    fmmod,   tremfrq;
   int fm2frq2, dcysusv, ptrx,    psst;
   int csl,     ccca,    rootkey, ipbase;
   int ipscale, minkey,  maxkey,  minvel;
   int maxvel,  key,     vel,     exc;
   int keyMEH,  keyMED,  keyVEH,  keyVED;
   int filter,  atten,   smpmode, volrel;
   int modrel,  pan,     loopst,  reverb;
   int chorus,  volsust, modsust, pitch;
   int sampend;
} envparms_t;


#define sfgen_startAddrsOffset                  0
#define sfgen_endAddrsOffset                    1
#define sfgen_startloopAddrsOffset              2
#define sfgen_endloopAddrsOffset                3
#define sfgen_startAddrsCoarseOffset            4
#define sfgen_modLfoToPitch                     5
#define sfgen_vibLfoToPitch                     6
#define sfgen_modEnvToPitch                     7
#define sfgen_initialFilterFc                   8
#define sfgen_initialFilterQ                    9
#define sfgen_modLfoToFilterFc                  10
#define sfgen_modEnvToFilterFc                  11
#define sfgen_endAddrsCoarseOffset              12
#define sfgen_modLfoToVolume                    13
#define sfgen_chorusEffectsSend                 15
#define sfgen_reverbEffectsSend                 16
#define sfgen_pan                               17
#define sfgen_delayModLFO                       21
#define sfgen_freqModLFO                        22
#define sfgen_delayVibLFO                       23
#define sfgen_freqVibLFO                        24
#define sfgen_delayModEnv                       25
#define sfgen_attackModEnv                      26
#define sfgen_holdModEnv                        27
#define sfgen_decayModEnv                       28
#define sfgen_sustainModEnv                     29
#define sfgen_releaseModEnv                     30
#define sfgen_keynumToModEnvHold                31
#define sfgen_keynumToModEnvDecay               32
#define sfgen_delayVolEnv                       33
#define sfgen_attackVolEnv                      34
#define sfgen_holdVolEnv                        35
#define sfgen_decayVolEnv                       36
#define sfgen_sustainVolEnv                     37
#define sfgen_releaseVolEnv                     38
#define sfgen_keynumToVolEnvHold                39
#define sfgen_keynumToVolEnvDecay               40
#define sfgen_instrument                        41
#define sfgen_keyRange                          43
#define sfgen_velRange                          44
#define sfgen_startloopAddrsCoarseOffset        45
#define sfgen_keynum                            46
#define sfgen_velocity                          47
#define sfgen_initialAttenuation                48
#define sfgen_endloopAddrsCoarseOffset          50
#define sfgen_coarseTune                        51
#define sfgen_fineTune                          52
#define sfgen_sampleID                          53
#define sfgen_sampleModes                       54
#define sfgen_initialPitch                      55
#define sfgen_scaleTuning                       56
#define sfgen_exclusiveClass                    57
#define sfgen_overridingRootKey                 58

#define gfgen_startAddrs                        59
#define gfgen_startloopAddrs                    60
#define gfgen_endloopAddrs                      61
#define gfgen_endAddrs                          62

#define SOUNDFONT_NUM_GENERATORS                63

typedef int generators_t[SOUNDFONT_NUM_GENERATORS];

/*extern envparms_t env_default;*/
extern int _emu8k_baseport;
extern int _emu8k_numchannels;

void emu8k_init(void);
void emu8k_startsound(int channel,struct envparms_t *envparms);
void emu8k_releasesound(int channel,struct envparms_t *envparms);
void emu8k_terminatesound(int channel);
int  emu8k_detect(void);

void emu8k_modulate_atten (int channel, int atten);
void emu8k_modulate_ip (int channel, int ip);
void emu8k_modulate_pan (int channel, int pan);

envparms_t *emu8k_createenvelope(generators_t);
void emu8k_destroyenvelope(envparms_t *);

void emu8k_lock(void);

void *_lock_malloc(size_t size);                /* malloc and lock */


#endif

