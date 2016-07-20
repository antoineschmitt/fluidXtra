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
	This way, this file may be later included in fluidsynth.
	Antoine Schmitt November 2002
*/


#include "fluidimportfile.h"
#include "fluidxtra.h" // for errors only
#include "sndfile.h"
#include <stdlib.h>

int fluid_sample_import_compute_file_samples(char * filepath, long* nbFrames, int* nbChannels) {
	SNDFILE *		infile = NULL;
	SF_INFO			sfinfo;
	int 			bytesPerSample;
	int err;

	// open file, read info
	infile = sf_open_read (filepath, &sfinfo) ;
	if (!infile) {
		return FLUIDXTRAERR_OPENREADFILE;
	}
    
	bytesPerSample = sfinfo.pcmbitwidth/8;
	if (bytesPerSample != 2) {
        if (infile) sf_close (infile);
        return FLUIDXTRAERR_BADFILEFORMAT;
    }
	
	*nbFrames = (long)sfinfo.samples;
    *nbChannels = sfinfo.channels;
    
    if (infile) sf_close (infile);

    err = 0;
    return err;
}

int fluid_sample_import_file(char * filepath, short *data, long seekPos, long nbFramesToLoad, long *nbFramesLoaded, int *samplerate, int *nbchannels)
{
	int err;
	int 			bytesPerSample;
	int				bytesPerFrame;
	unsigned long			fileBytes;
	long 			nbSamplesRead;
	short 			needsTwoComplementing;
	SNDFILE *		infile;
	SF_INFO			sfinfo;
	long            samplesToRead;
    
	// open file, read info
	infile = sf_open_read (filepath, &sfinfo) ;
	if (!infile) {
		return FLUIDXTRAERR_OPENREADFILE;
	}

	bytesPerSample = sfinfo.pcmbitwidth/8;
	if (bytesPerSample != 2) {
		err = FLUIDXTRAERR_BADFILEFORMAT;
		goto bail;
	}
	
	bytesPerFrame = sfinfo.channels*bytesPerSample;
	fileBytes = (unsigned long)sfinfo.samples*bytesPerFrame;
	needsTwoComplementing = (sfinfo.format & SF_FORMAT_AIFF) && (bytesPerSample == 1);

    if (seekPos > 0) {
        off_t res =	sf_seek(infile, seekPos, SEEK_SET);
        if (res < 0) {
            err = FLUIDXTRAERR_SEEKFILE;
            *nbFramesLoaded = 0;
            goto bail;
        }
    }
    
    samplesToRead = (nbFramesToLoad >= 0 ? nbFramesToLoad : sfinfo.samples)*sfinfo.channels;
	nbSamplesRead = sf_read_short (infile, data, samplesToRead);
	
	*nbFramesLoaded = nbSamplesRead/sfinfo.channels;
	*samplerate = sfinfo.samplerate;
	*nbchannels = sfinfo.channels;

	err = 0;
	
bail:
	if (infile) sf_close (infile);

	return err;
}


short *fluidxtra_malloc_buffer(unsigned long bytesnb) {
	// use the same functions as in fluidsynth (malloc and free)
	return (short *)malloc(bytesnb);
}

void fluidxtra_free_buffer(short *buf) {
	free(buf);
}