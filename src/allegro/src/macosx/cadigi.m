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
 *      MacOS X Core Audio digital sound driver.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif


static int ca_detect(int);
static int ca_init(int, int);
static void ca_exit(int);
static int ca_buffer_size();
static int ca_set_mixer_volume(int);


static AUGraph graph;
static AudioUnit output_unit, converter_unit;
static int audio_buffer_size = 0;
static char ca_desc[512] = EMPTY_STRING;


DIGI_DRIVER digi_core_audio =
{
   DIGI_CORE_AUDIO,
   empty_string,
   empty_string,
   "CoreAudio",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   ca_detect,
   ca_init,
   ca_exit,
   ca_set_mixer_volume,
   NULL,

   NULL,
   NULL,
   ca_buffer_size,
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
   0,
   0,
   0,
   0,
   0,
   0
};



static OSStatus render_callback(void *inRefCon, AudioUnitRenderActionFlags *inActionFlags,
   const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumFrames, AudioBufferList *ioData)
{
   _mix_some_samples((unsigned long)ioData->mBuffers[0].mData, 0, TRUE);
   return 0;
}



static int ca_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }
   if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_1)
      return FALSE;
   return TRUE;
}



static int ca_init(int input, int voices)
{
   AudioDeviceID audio_device;
   ComponentDescription desc;
   AUNode output_node, converter_node;
   AURenderCallbackStruct render_cb;
   UInt32 property_size, buffer_size;
   AudioStreamBasicDescription input_format_desc, output_format_desc;
   char device_name[64], manufacturer[64];
   UInt32 device_name_size, manufacturer_size;
   char tmp1[256], tmp2[256], tmp3[256];

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }
   if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_1) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("MacOS X.2 or newer required by this driver"));
      return -1;
   }
   
   NewAUGraph(&graph);

   desc.componentType = kAudioUnitType_FormatConverter;
   desc.componentSubType = kAudioUnitSubType_AUConverter;
   desc.componentManufacturer = kAudioUnitManufacturer_Apple;
   desc.componentFlags = 0;        
   desc.componentFlagsMask = 0;   
   AUGraphNewNode(graph, &desc, 0, NULL, &converter_node);
   
   desc.componentType = kAudioUnitType_Output;
   desc.componentSubType = kAudioUnitSubType_DefaultOutput;
   desc.componentManufacturer = kAudioUnitManufacturer_Apple;
   desc.componentFlags = 0;        
   desc.componentFlagsMask = 0;   
   AUGraphNewNode(graph, &desc, 0, NULL, &output_node);
   
   AUGraphOpen(graph);
   AUGraphInitialize(graph);
   
   AUGraphGetNodeInfo(graph, output_node, NULL, NULL, NULL, &output_unit);
   AUGraphGetNodeInfo(graph, converter_node, NULL, NULL, NULL, &converter_unit);

   property_size = sizeof(output_format_desc);
   if (AudioUnitGetProperty(output_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &output_format_desc, &property_size)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot detect output audio format"));
      ca_exit(FALSE);
      return -1;
   }
   
   input_format_desc.mSampleRate = output_format_desc.mSampleRate;
   input_format_desc.mFormatID = kAudioFormatLinearPCM;
#ifdef ALLEGRO_BIG_ENDIAN
   input_format_desc.mFormatFlags = kAudioFormatFlagIsBigEndian | kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
#else
   input_format_desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
#endif
   input_format_desc.mBytesPerPacket = 4;
   input_format_desc.mFramesPerPacket = 1;
   input_format_desc.mBytesPerFrame = 4;
   input_format_desc.mChannelsPerFrame = 2;
   input_format_desc.mBitsPerChannel = 16;
   
   if (AudioUnitSetProperty(converter_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &output_format_desc, sizeof(output_format_desc)) ||
       AudioUnitSetProperty(converter_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &input_format_desc, sizeof(input_format_desc))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot configure format converter audio unit"));
      ca_exit(FALSE);
      return -1;
   }
   
   _sound_bits = 16;
   _sound_stereo = TRUE;
   _sound_freq = (int)output_format_desc.mSampleRate;
   
   AUGraphConnectNodeInput(graph, converter_node, 0, output_node, 0);
   
   render_cb.inputProc = render_callback;
   render_cb.inputProcRefCon = NULL;
   if (AudioUnitSetProperty(converter_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &render_cb, sizeof(render_cb))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot set audio rendering callback"));
      ca_exit(FALSE);
      return -1;
   }

   property_size = sizeof(audio_device);
   if (AudioUnitGetProperty(output_unit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Output, 0, &audio_device, &property_size)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot get CoreAudio device"));
      ca_exit(FALSE);
      return -1;
   }
   
   property_size = sizeof(buffer_size);
   if (AudioDeviceGetProperty(audio_device, 0, false, kAudioDevicePropertyBufferSize, &property_size, &buffer_size)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot get CoreAudio device buffer size"));
      ca_exit(FALSE);
      return -1;
   }
   audio_buffer_size = buffer_size / sizeof(float) * sizeof(short);
   digi_core_audio.voices = voices;
   if (_mixer_init(audio_buffer_size / sizeof(short), _sound_freq, _sound_stereo, TRUE, &digi_core_audio.voices)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Error initializing mixer"));
      ca_exit(FALSE);
      return -1;
   }
   
   AUGraphStart(graph);
   
   device_name_size = sizeof(device_name);
   manufacturer_size = sizeof(manufacturer);
   if (!AudioDeviceGetProperty(audio_device, 0, false, kAudioDevicePropertyDeviceName, &device_name_size, device_name) &&
       !AudioDeviceGetProperty(audio_device, 0, false, kAudioDevicePropertyDeviceManufacturer, &manufacturer_size, manufacturer)) {
      uszprintf(ca_desc, sizeof(ca_desc), get_config_text("%s (%s), 16 bits (%d real), %d bps, %s"),
         uconvert_ascii(device_name, tmp1), uconvert_ascii(manufacturer, tmp2), output_format_desc.mBitsPerChannel,
	 _sound_freq, uconvert_ascii(_sound_stereo ? "stereo" : "mono", tmp3));
   }
   else {
      uszprintf(ca_desc, sizeof(ca_desc), get_config_text("16 bits (%d real), %d bps, %s"), output_format_desc.mBitsPerChannel,
	 _sound_freq, uconvert_ascii(_sound_stereo ? "stereo" : "mono", tmp3));
   }
   digi_core_audio.desc = ca_desc;
   
   return 0;
}



static void ca_exit(int input)
{
   if (input)
      return;
   AUGraphStop(graph);
   AUGraphUninitialize(graph);
   AUGraphClose(graph);
   DisposeAUGraph(graph);
   _mixer_exit();
}



static int ca_buffer_size()
{
   return audio_buffer_size;
}



static int ca_set_mixer_volume(int volume)
{
   float value = (float)volume / 255.0;

   return AudioUnitSetParameter(converter_unit, kAudioUnitParameterUnit_LinearGain, kAudioUnitScope_Output, 0, value, 0);
}

