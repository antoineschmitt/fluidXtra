/* FluidXtra
 *
 * Copyright (C) 2004  Antoine Schmitt, Hyptique, Peter Hanappe and others.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */
 
 
/* Does the bridge between libsndfile and fluid_ramsfont.
	So that fluid_ramsfont does not depend on libsndfile, nor the Xtra code.
	This way, this file may be later included in fluid.
	Antoine Schmitt November 2002
*/

#ifndef _FLUID_IMPORTFILE_H
#define _FLUID_IMPORTFILE_H

#include "fluidsynth.h"

// Gets the number of frames and channels in the file
// samples are expected to be 2 bytes (SHORT)
// a frame is channels samples
// bytes total is thus frames*channels*2
// Can fail if not shorts, or no file, or unknown format
int fluid_sample_import_compute_file_samples(char * filepath, long* nbFrames, int* nbChannels);

    /* 	Fills data and nbsamples
     On returns, data is be 16bits, 44.1KHz, mono
     data should be allocated !!
     if nbSamplesToLoad < 0, loads everything starting from seekPos.
     */
int fluid_sample_import_file(char * filepath, short *data, long seekPos, long nbFramesToLoad, long * nbFramesLoaded, int *samplerate, int *nbchannels);

short *fluidxtra_malloc_buffer(unsigned long bytesnb);
void fluidxtra_free_buffer(short *buf);

#endif // _FLUID_IMPORTFILE_H
