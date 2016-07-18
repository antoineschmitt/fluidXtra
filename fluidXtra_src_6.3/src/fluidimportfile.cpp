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

int fluid_sample_import_compute_file_data(char * filepath, unsigned long* bytes, int* nbChannels) {
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
	
	*bytes = (unsigned long)sfinfo.samples*sfinfo.channels*bytesPerSample;
    *nbChannels = sfinfo.channels;
    
    if (infile) sf_close (infile);

    err = 0;
    return err;
}

int fluid_sample_import_file(char * filepath, short *data, unsigned int *nbsamples, unsigned int *samplerate, int *nbchannels)
{
	int err;
	int 			bytesPerSample;
	int				bytesPerFrame;
	unsigned long			fileBytes;
	long 			nbShortsRead;
	short 			needsTwoComplementing;
	SNDFILE *		infile;
	SF_INFO			sfinfo;
	
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

	nbShortsRead = sf_read_short (infile, data, sfinfo.samples*sfinfo.channels);
	if (nbShortsRead != (long)sfinfo.samples*(long)sfinfo.channels) {
		err = FLUIDXTRAERR_READFILE;
		*nbsamples = 0;
		goto bail;
	}
	
	*nbsamples = sfinfo.samples;
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