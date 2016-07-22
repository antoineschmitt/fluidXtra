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

/* fluid_portaudio.c
 *
 * Drivers for the PortAudio API : www.portaudio.com
 * Implementation files for PortAudio on each platform have to be added
 *
 * Stephane Letz  (letz@grame.fr)  Grame
 * 12/20/01 Adapdation for new audio drivers
 *
 * Josh Green <jgreen@users.sourceforge.net>
 * 2009-01-28 Overhauled for PortAudio 19 API and current FluidSynth API (was broken)
 */

#include "fluid_synth.h"
#include "fluid_sys.h"
#include "fluid_settings.h"
#include "fluid_adriver.h"

#if PORTAUDIO_SUPPORT

#include <fcntl.h>
//#include <unistd.h>
#include <errno.h>
#include <portaudio.h>

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

/** fluid_portaudio_driver_t
 *
 * This structure should not be accessed directly. Use audio port
 * functions instead.
 */
typedef struct
{
  fluid_audio_driver_t driver;
  PaStream *stream;
  int chanL, chanR, chansOpen;
  void* data;
  fluid_audio_func_t callback;
  unsigned int buffer_size;
  float* buffers[2];
} fluid_portaudio_driver_t;

static int
fluid_portaudio_run (const void *input, void *output, unsigned long frameCount,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags, void *userData);
int delete_fluid_portaudio_driver (fluid_audio_driver_t *p);

fluid_audio_driver_t *
new_fluid_portaudio_driver2 (fluid_settings_t *settings, fluid_audio_func_t func, void* data);

#define PORTAUDIO_DEFAULT_DEVICE "PortAudio Default"

void
fluid_portaudio_driver_settings (fluid_settings_t *settings)
{
  const PaDeviceInfo *deviceInfo;
  int numDevices;
  PaError err;
  int i;
  char tmp_setting__name[1024];

  fluid_settings_register_str (settings, "audio.portaudio.device", PORTAUDIO_DEFAULT_DEVICE, 0, NULL, NULL);
  fluid_settings_add_option (settings, "audio.portaudio.device", PORTAUDIO_DEFAULT_DEVICE);
  snprintf(tmp_setting__name, sizeof(tmp_setting__name), "audio.portaudio.%s.channels", PORTAUDIO_DEFAULT_DEVICE);
  fluid_settings_register_int (settings, tmp_setting__name, 2, 0, 32, 0, NULL, NULL);
  fluid_settings_register_int (settings, "audio.portaudio.channelL", 0, 0, 32, 0, NULL, NULL);
  fluid_settings_register_int (settings, "audio.portaudio.channelR", 1, 0, 32, 1, NULL, NULL);

  err = Pa_Initialize();

  if (err != paNoError)
  {
    FLUID_LOG (FLUID_ERR, "Error initializing PortAudio driver: %s",
               Pa_GetErrorText (err));
    return;
  }

  numDevices = Pa_GetDeviceCount();

  if (numDevices < 0)
  {
    FLUID_LOG (FLUID_ERR, "PortAudio returned unexpected device count %d", numDevices);
    return;
  }

  for (i = 0; i < numDevices; i++)
  {
    deviceInfo = Pa_GetDeviceInfo (i);
    if ( deviceInfo->maxOutputChannels >= 2 )
      fluid_settings_add_option (settings, "audio.portaudio.device",
                                 deviceInfo->name);
		{
			int nb_outputs = deviceInfo->maxOutputChannels;
		  snprintf(tmp_setting__name, sizeof(tmp_setting__name), "audio.portaudio.%s.channels", deviceInfo->name);
		  fluid_settings_register_int (settings, tmp_setting__name, nb_outputs, 0, 32, 0, NULL, NULL);
		}
  }

  /* done with PortAudio for now, may get reopened later */
  err = Pa_Terminate();

  if (err != paNoError)
    printf ("PortAudio termination error: %s\n", Pa_GetErrorText (err) );
}

fluid_audio_driver_t *
new_fluid_portaudio_driver (fluid_settings_t *settings, fluid_synth_t *synth) {
  return new_fluid_portaudio_driver2 ( settings,
                                        NULL,
                                        (void*) synth );}

fluid_audio_driver_t *
new_fluid_portaudio_driver2 (fluid_settings_t *settings, fluid_audio_func_t func, void* data)
{
  fluid_portaudio_driver_t *dev = NULL;
  PaStreamParameters outputParams;
  char *device = NULL;
  double sample_rate;
  int period_size;
  PaError err;
  int numOutputs = 2;

  dev = FLUID_NEW (fluid_portaudio_driver_t);

  if (dev == NULL)
  {
    FLUID_LOG (FLUID_ERR, "Out of memory");
    return NULL;
  }

  err = Pa_Initialize ();

  if (err != paNoError)
  {
    FLUID_LOG (FLUID_ERR, "Error initializing PortAudio driver: %s",
               Pa_GetErrorText (err));
    FLUID_FREE (dev);
    return NULL;
  }

  FLUID_MEMSET (dev, 0, sizeof (fluid_portaudio_driver_t));
  dev->data = data;
  dev->callback = func;

  bzero (&outputParams, sizeof (outputParams));

  fluid_settings_getint (settings, "audio.period-size", &period_size);
  fluid_settings_getnum (settings, "synth.sample-rate", &sample_rate);
  fluid_settings_dupstr(settings, "audio.portaudio.device", &device);   /* ++ alloc device name */

  /* Locate the device if specified */

  if (strcmp (device, PORTAUDIO_DEFAULT_DEVICE) != 0)
  {
    const PaDeviceInfo *deviceInfo;
    int numDevices;
    int i;

    numDevices = Pa_GetDeviceCount ();

    if (numDevices < 0)
    {
      FLUID_LOG (FLUID_ERR, "PortAudio returned unexpected device count %d", numDevices);
      goto error_recovery;
    }

    for (i = 0; i < numDevices; i++)
    {
      deviceInfo = Pa_GetDeviceInfo (i);

      if (strcmp (device, deviceInfo->name) == 0)
      {
        outputParams.device = i;
		numOutputs = deviceInfo->maxOutputChannels;
        break;
      }
    }

    if (i == numDevices)
    {
      FLUID_LOG (FLUID_ERR, "PortAudio device '%s' was not found", device);
      goto error_recovery;
    }
  }
  else outputParams.device = Pa_GetDefaultOutputDevice();

  fluid_settings_getint(settings, "audio.portaudio.channelL", &(dev->chanL));
  fluid_settings_getint(settings, "audio.portaudio.channelR", &(dev->chanR));
  dev->chansOpen = 1 + ((dev->chanL > dev->chanR) ? dev->chanL : dev->chanR);
  if (dev->chansOpen > numOutputs) {
    // error
    FLUID_LOG (FLUID_ERR, "One specified channel is greater than the maximum number of channels: L=%d, R=%d, max=%d\n", dev->chanL, dev->chanR, numOutputs-1);
    goto error_recovery;
  }
  dev->buffer_size = period_size;
  dev->buffers[0] = FLUID_ARRAY(float, dev->buffer_size);
  dev->buffers[1] = FLUID_ARRAY(float, dev->buffer_size);

  outputParams.channelCount = dev->chansOpen;
  outputParams.suggestedLatency = (PaTime)period_size / sample_rate;

  // force float format
  outputParams.sampleFormat = paFloat32;

  /* PortAudio section */

  /* Open an audio I/O stream. */
  err = Pa_OpenStream (&dev->stream,
                       NULL,              /* Input parameters */
                       &outputParams,
                       sample_rate,
                       period_size,
                       paNoFlag,
                       fluid_portaudio_run,
                       dev);

  if (err != paNoError)
  {
    FLUID_LOG (FLUID_ERR, "Error opening PortAudio stream: %s",
               Pa_GetErrorText (err));
    goto error_recovery;
  }

  err = Pa_StartStream (dev->stream);

  if (err != paNoError)
  {
    FLUID_LOG (FLUID_ERR, "Error starting PortAudio stream: %s",
               Pa_GetErrorText (err));
    goto error_recovery;
  }

  if (device) FLUID_FREE (device);      /* -- free device name */
  
  return (fluid_audio_driver_t *)dev;

error_recovery:
  if (device) FLUID_FREE (device);      /* -- free device name */
  delete_fluid_portaudio_driver ((fluid_audio_driver_t *)dev);
  return NULL;
}

/* PortAudio callback
 * fluid_portaudio_run
 */
static int
fluid_portaudio_run (const void *input, void *output, unsigned long frameCount,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags, void *userData)
{
  fluid_portaudio_driver_t *dev = (fluid_portaudio_driver_t *)userData;

  if (dev->callback) {
    float* left = dev->buffers[0];
    float* right = dev->buffers[1];
	int i, r, l;
	long len = frameCount;
    float* buffer = (float *)output;

	// callback fills dev->buffers
    (*dev->callback)(dev->data, len, 0, NULL, 2, dev->buffers);

    bzero(buffer, dev->chansOpen*len*sizeof(float));
    for (i = 0, l = dev->chanL, r = dev->chanR; i < len; i++, l += dev->chansOpen, r += dev->chansOpen) {
      buffer[l] = left[i];
      buffer[r] = right[i];
    }
  } else
	/* it's as simple as that: */
	fluid_synth_write_float ((fluid_synth_t *)dev->data, frameCount, output, dev->chanL, dev->chansOpen, output, dev->chanR, dev->chansOpen);
  return 0;
}

/*
 * delete_fluid_portaudio_driver
 */
int
delete_fluid_portaudio_driver(fluid_audio_driver_t *p)
{
  fluid_portaudio_driver_t* dev;
  PaError err;

  dev = (fluid_portaudio_driver_t*)p;
  if (dev == NULL) return FLUID_OK;

  /* PortAudio section */
  if (dev->stream) Pa_CloseStream (dev->stream);

  err = Pa_Terminate();

  if (err != paNoError)
    printf ("PortAudio termination error: %s\n", Pa_GetErrorText (err) );

  if (dev->buffers[0]) {
    FLUID_FREE(dev->buffers[0]);
  }
  if (dev->buffers[1]) {
    FLUID_FREE(dev->buffers[1]);
  }

  FLUID_FREE (dev);
  return FLUID_OK;
}

#endif /*#if PORTAUDIO_SUPPORT */
