/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

/* fluid_coreaudio.c
 *
 * Driver for the Apple's CoreAudio on MacOS X
 *
 */

#include "fluid_synth.h"
#include "fluid_midi.h"
#include "fluid_adriver.h"
#include "fluid_mdriver.h"
#include "fluid_settings.h"

#include "config.h"

#if COREAUDIO_SUPPORT
#include <CoreServices/CoreServices.h>
#include <CoreAudio/AudioHardware.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioUnit/AudioUnit.h>

/*
 * fluid_core_audio_driver_t
 *
 */
typedef struct {
  fluid_audio_driver_t driver;
  AudioUnit outputUnit;
  AudioStreamBasicDescription format;
  fluid_audio_func_t callback;
  void* data;
  unsigned int buffer_size;
  float* buffers[2];
  double phase; // ??
  int chanL, chanR, chansOpen;
} fluid_core_audio_driver_t;

fluid_audio_driver_t* new_fluid_core_audio_driver (fluid_settings_t* settings, fluid_synth_t* synth);

fluid_audio_driver_t* new_fluid_core_audio_driver2 (fluid_settings_t* settings,
                                                    fluid_audio_func_t func,
                                                    void* data);

OSStatus fluid_core_audio_callback (void *data,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp *inTimeStamp,
                                    UInt32 inBusNumber,
                                    UInt32 inNumberFrames,
                                    AudioBufferList *ioData);

int delete_fluid_core_audio_driver (fluid_audio_driver_t* p);


/**************************************************************
 *
 *        CoreAudio audio driver
 *
 */

#define OK(x) (x == noErr)

int
get_num_outputs (AudioDeviceID deviceID)
{
  int i, total = 0;
  UInt32 size;
  AudioObjectPropertyAddress pa;
  pa.mSelector = kAudioDevicePropertyStreamConfiguration;
  pa.mScope = kAudioDevicePropertyScopeOutput;
  pa.mElement = kAudioObjectPropertyElementMaster;

  if (OK (AudioObjectGetPropertyDataSize (deviceID, &pa, 0, 0, &size))) {
    int num = size / (int) sizeof (AudioBufferList);
    AudioBufferList bufList[num];
    if (OK (AudioObjectGetPropertyData (deviceID, &pa, 0, 0, &size, bufList))) {
      int numStreams = bufList->mNumberBuffers;
      for (i = 0; i < numStreams; ++i) {
        AudioBuffer b = bufList->mBuffers[i];
        total += b.mNumberChannels;
      }
    }
  }
  return total;
}

void
fluid_core_audio_driver_settings(fluid_settings_t* settings)
{
  char tmp_setting__name[1024];
  int i;
  UInt32 size;
  AudioObjectPropertyAddress pa;
  pa.mSelector = kAudioHardwarePropertyDevices;
  pa.mScope = kAudioObjectPropertyScopeWildcard;
  pa.mElement = kAudioObjectPropertyElementMaster;

  fluid_settings_register_int (settings, "audio.coreaudio.channelL", 0, 0, 32, 0, NULL, NULL);
  fluid_settings_register_int (settings, "audio.coreaudio.channelR", 1, 0, 32, 1, NULL, NULL);

  fluid_settings_register_str (settings, "audio.coreaudio.device", "default", 0, NULL, NULL);
  fluid_settings_add_option (settings, "audio.coreaudio.device", "default");
  snprintf(tmp_setting__name, sizeof(tmp_setting__name), "audio.coreaudio.%s.channels", "default");
  fluid_settings_register_int (settings, tmp_setting__name, 2, 0, 32, 0, NULL, NULL);
  
  if (OK (AudioObjectGetPropertyDataSize (kAudioObjectSystemObject, &pa, 0, 0, &size))) {
    int num = size / (int) sizeof (AudioDeviceID);
    AudioDeviceID devs [num];
    if (OK (AudioObjectGetPropertyData (kAudioObjectSystemObject, &pa, 0, 0, &size, devs))) {
      for (i = 0; i < num; ++i) {
        char name [1024];
        size = sizeof (name);
        pa.mSelector = kAudioDevicePropertyDeviceName;
        if (OK (AudioObjectGetPropertyData (devs[i], &pa, 0, 0, &size, name))) {
          int nb_outputs = get_num_outputs (devs[i]);
          if ( nb_outputs > 0) {
            fluid_settings_add_option (settings, "audio.coreaudio.device", name);
            
            snprintf(tmp_setting__name, sizeof(tmp_setting__name), "audio.coreaudio.%s.channels", name);
            fluid_settings_register_int (settings, tmp_setting__name, nb_outputs, 0, 32, 0, NULL, NULL);
          }
        }
      }
    }
  }
}

/*
 * new_fluid_core_audio_driver
 */
fluid_audio_driver_t*
new_fluid_core_audio_driver(fluid_settings_t* settings, fluid_synth_t* synth)
{
  return new_fluid_core_audio_driver2 ( settings,
                                        (fluid_audio_func_t) fluid_synth_process,
                                        (void*) synth );
}

/*
 * new_fluid_core_audio_driver2
 */
fluid_audio_driver_t*
new_fluid_core_audio_driver2(fluid_settings_t* settings, fluid_audio_func_t func, void* data)
{
  char* devname = NULL;
  fluid_core_audio_driver_t* dev = NULL;
  int period_size, periods;
  double sample_rate;
  OSStatus status;
  UInt32 size;
  int i;

  dev = FLUID_NEW(fluid_core_audio_driver_t);
  if (dev == NULL) {
    FLUID_LOG(FLUID_ERR, "Out of memory");
    return NULL;
  }
  FLUID_MEMSET(dev, 0, sizeof(fluid_core_audio_driver_t));

  dev->callback = func;
  dev->data = data;

  // Open the default output unit
  AudioComponentDescription desc;
  bzero(&desc, sizeof(desc));
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent comp = AudioComponentFindNext(NULL, &desc);
  if (comp == NULL) {
    FLUID_LOG(FLUID_ERR, "Failed to get the default audio device");
    goto error_recovery;
  }

  status = AudioComponentInstanceNew(comp, &dev->outputUnit);
  if (status != noErr) {
    FLUID_LOG(FLUID_ERR, "Failed to open the default audio device. Status=%ld\n", (long int)status);
    goto error_recovery;
  }

  // Set up a callback function to generate output
  AURenderCallbackStruct render;
  render.inputProc = fluid_core_audio_callback;
  render.inputProcRefCon = (void *) dev;
  status = AudioUnitSetProperty (dev->outputUnit,
                                 kAudioUnitProperty_SetRenderCallback,
                                 kAudioUnitScope_Input,
                                 0,
                                 &render,
                                 sizeof(render));
  if (status != noErr) {
    FLUID_LOG (FLUID_ERR, "Error setting the audio callback. Status=%ld\n", (long int)status);
    goto error_recovery;
  }

  fluid_settings_getnum(settings, "synth.sample-rate", &sample_rate);
  fluid_settings_getint(settings, "audio.periods", &periods);
  fluid_settings_getint(settings, "audio.period-size", &period_size);

  /* get the selected device name. if none is specified, use NULL for the default device. */
  int numOutputs = 2;
  if (fluid_settings_dupstr(settings, "audio.coreaudio.device", &devname)  /* alloc device name */
      && devname && strlen (devname) > 0) {
    AudioObjectPropertyAddress pa;
    pa.mSelector = kAudioHardwarePropertyDevices;
    pa.mScope = kAudioObjectPropertyScopeWildcard;
    pa.mElement = kAudioObjectPropertyElementMaster;
    if (OK (AudioObjectGetPropertyDataSize (kAudioObjectSystemObject, &pa, 0, 0, &size))) {
      int num = size / (int) sizeof (AudioDeviceID);
      AudioDeviceID devs [num];
      if (OK (AudioObjectGetPropertyData (kAudioObjectSystemObject, &pa, 0, 0, &size, devs))) {
        for (i = 0; i < num; ++i) {
          char name [1024];
          size = sizeof (name);
          pa.mSelector = kAudioDevicePropertyDeviceName;
          if (OK (AudioObjectGetPropertyData (devs[i], &pa, 0, 0, &size, name))) {
            if (get_num_outputs (devs[i]) > 0 && strcasecmp(devname, name) == 0) {
              AudioDeviceID selectedID = devs[i];
              status = AudioUnitSetProperty (dev->outputUnit,
                                             kAudioOutputUnitProperty_CurrentDevice,
                                             kAudioUnitScope_Global,
                                             0,
                                             &selectedID,
                                             sizeof(AudioDeviceID));
              if (status != noErr) {
                FLUID_LOG (FLUID_ERR, "Error setting the selected output device. Status=%ld\n", (long int)status);
                goto error_recovery;
              }
              numOutputs = get_num_outputs (devs[i]);
            }
          }
        }
      }
    }
  }

  if (devname)
    FLUID_FREE (devname); /* free device name */

  fluid_settings_getint(settings, "audio.coreaudio.channelL", &(dev->chanL));
  fluid_settings_getint(settings, "audio.coreaudio.channelR", &(dev->chanR));
  dev->chansOpen = 1 + ((dev->chanL > dev->chanR) ? dev->chanL : dev->chanR);
  if (dev->chansOpen > numOutputs) {
    // error
    FLUID_LOG (FLUID_ERR, "One specified channel is greater than the maximum number of channels : L=%d, R=%d, max=%d\n", dev->chanL, dev->chanR, numOutputs-1);
    goto error_recovery;
  }
  dev->buffer_size = period_size * periods;

  // The DefaultOutputUnit should do any format conversions
  // necessary from our format to the device's format.
  dev->format.mSampleRate = sample_rate; // sample rate of the audio stream
  dev->format.mFormatID = kAudioFormatLinearPCM; // encoding type of the audio stream
  dev->format.mFormatFlags = kLinearPCMFormatFlagIsFloat;
  dev->format.mChannelsPerFrame = dev->chansOpen;
  dev->format.mBytesPerFrame = dev->format.mChannelsPerFrame*sizeof(float);
  dev->format.mFramesPerPacket = 1;
  dev->format.mBytesPerPacket = dev->format.mBytesPerFrame;
  dev->format.mBitsPerChannel = 8*sizeof(float);

  FLUID_LOG (FLUID_DBG, "mSampleRate %g", dev->format.mSampleRate);
  FLUID_LOG (FLUID_DBG, "mFormatFlags %08X", dev->format.mFormatFlags);
  FLUID_LOG (FLUID_DBG, "mBytesPerPacket %d", dev->format.mBytesPerPacket);
  FLUID_LOG (FLUID_DBG, "mFramesPerPacket %d", dev->format.mFramesPerPacket);
  FLUID_LOG (FLUID_DBG, "mChannelsPerFrame %d", dev->format.mChannelsPerFrame);
  FLUID_LOG (FLUID_DBG, "mBytesPerFrame %d", dev->format.mBytesPerFrame);
  FLUID_LOG (FLUID_DBG, "mBitsPerChannel %d", dev->format.mBitsPerChannel);

  status = AudioUnitSetProperty (dev->outputUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input,
                                 0,
                                 &dev->format,
                                 sizeof(AudioStreamBasicDescription));
  if (status != noErr) {
    FLUID_LOG (FLUID_ERR, "Error setting the audio format. Status=%ld\n", (long int)status);
    goto error_recovery;
  }

  status = AudioUnitSetProperty (dev->outputUnit,
                                 kAudioUnitProperty_MaximumFramesPerSlice,
                                 kAudioUnitScope_Input,
                                 0,
                                 &dev->buffer_size,
                                 sizeof(unsigned int));
  if (status != noErr) {
    FLUID_LOG (FLUID_ERR, "Failed to set the MaximumFramesPerSlice. Status=%ld\n", (long int)status);
    goto error_recovery;
  }
  FLUID_LOG (FLUID_DBG, "MaximumFramesPerSlice = %d", dev->buffer_size);

  dev->buffers[0] = FLUID_ARRAY(float, dev->buffer_size);
  dev->buffers[1] = FLUID_ARRAY(float, dev->buffer_size);

  // Initialize the audio unit
  status = AudioUnitInitialize(dev->outputUnit);
  if (status != noErr) {
    FLUID_LOG (FLUID_ERR, "Error calling AudioUnitInitialize(). Status=%ld\n", (long int)status);
    goto error_recovery;
  }

  // Start the rendering
  status = AudioOutputUnitStart (dev->outputUnit);
  if (status != noErr) {
    FLUID_LOG (FLUID_ERR, "Error calling AudioOutputUnitStart(). Status=%ld\n", (long int)status);
    goto error_recovery;
  }

  return (fluid_audio_driver_t*) dev;

error_recovery:

  delete_fluid_core_audio_driver((fluid_audio_driver_t*) dev);
  return NULL;
}

/*
 * delete_fluid_core_audio_driver
 */
int
delete_fluid_core_audio_driver(fluid_audio_driver_t* p)
{
  fluid_core_audio_driver_t* dev = (fluid_core_audio_driver_t*) p;

  if (dev == NULL) {
    return FLUID_OK;
  }

  AudioComponentInstanceDispose (dev->outputUnit);

  if (dev->buffers[0]) {
    FLUID_FREE(dev->buffers[0]);
  }
  if (dev->buffers[1]) {
    FLUID_FREE(dev->buffers[1]);
  }

  FLUID_FREE(dev);

  return FLUID_OK;
}

OSStatus
fluid_core_audio_callback ( void *data,
                            AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp,
                            UInt32 inBusNumber,
                            UInt32 inNumberFrames,
                            AudioBufferList *ioData)
{
  int i, l, r;
  fluid_core_audio_driver_t* dev = (fluid_core_audio_driver_t*) data;
  int len = inNumberFrames;
  float* buffer = ioData->mBuffers[0].mData;

  if (dev->callback)
  {
    // callback fills dev->buffers
    (*dev->callback)(dev->data, len, 0, NULL, 2, dev->buffers);

    float* left = dev->buffers[0];
    float* right = dev->buffers[1];
    
    bzero(buffer, dev->chansOpen*len*sizeof(float));
    for (i = 0, l = dev->chanL, r = dev->chanR; i < len; i++, l += dev->chansOpen, r += dev->chansOpen) {
      buffer[l] = left[i];
      buffer[r] = right[i];
    }
  }
  else fluid_synth_write_float((fluid_synth_t*) dev->data, len, buffer, dev->chanL, dev->chansOpen,
                               buffer, dev->chanR, dev->chansOpen);
  return noErr;
}


#endif /* #if COREAUDIO_SUPPORT */
