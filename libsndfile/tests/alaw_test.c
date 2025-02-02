/*
** Copyright (C) 1999 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/



#include	<stdio.h>
#include	<unistd.h>

#include	<sndfile.h>

#define	BUFFER_SIZE		(65536)

static unsigned char	alaw_encode (int sample) ;
static int				alaw_decode (unsigned int alawbyte) ;

static	short			short_buffer [BUFFER_SIZE] ;
static	unsigned char	alaw_buffer [BUFFER_SIZE] ;

int		main (int argc, char *argv[])
{	SNDFILE	*file ;
	SF_INFO sfinfo ;
	char	*filename ;
	int		k ;
	
	filename = "test.au" ;
	
	sfinfo.format      = SF_FORMAT_AU | SF_FORMAT_ALAW ;
	sfinfo.samplerate  = 44100 ;
	sfinfo.samples     = 123456789 ; /* Wrong length. Library should correct this on sf_close. */
	sfinfo.channels    = 1 ;
	sfinfo.pcmbitwidth = 16 ;

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
		
	/* Generate a file containing all possible 16 bit sample values 
	** and write it to disk as alaw encoded samples. 
	*/

	for (k = 0 ; k < 0x10000 ; k++)
		short_buffer [k] = k & 0xFFFF ;

	sf_write_short (file, short_buffer, BUFFER_SIZE) ;
	sf_close (file) ;
	
	/* Now open that file and compare the alaw encoded sample values 
	** with what they should be.
	*/

	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;

	if (sf_read_raw (file, alaw_buffer, BUFFER_SIZE) != BUFFER_SIZE)
	{	printf ("sf_read_raw : ") ;
		sf_perror (file) ;
		exit (1) ;
		} ;
	
	for (k = 0 ; k < 0x10000 ; k++)
		if (alaw_encode (short_buffer [k]) != alaw_buffer [k])
		{	printf ("Encoder error : sample #%d (0x%02X should be 0x%02X)\n", k, alaw_buffer [k], alaw_encode (short_buffer [k])) ;
			exit (1) ;
			} ;

	sf_close (file) ;
	
	printf ("    alaw_test : encoder ... ok\n") ;

	/* Now generate a file containing all possible 8 bit encoded
	** sample values and write it to disk as alaw encoded samples. 
	*/

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;

	for (k = 0 ; k < 256 ; k++)
		alaw_buffer [k] = k & 0xFF ;

	sf_write_raw (file, alaw_buffer, 256) ;
	sf_close (file) ;
	
	/* Now open that file and compare the alaw decoded sample values 
	** with what they should be.
	*/

	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;

	if (sf_read_short (file, short_buffer, 256) != 256)
	{	printf ("sf_read_short : ") ;
		sf_perror (file) ;
		exit (1) ;
		} ;
	
	
	for (k = 0 ; k < 256 ; k++)
		if (short_buffer [k] != alaw_decode (alaw_buffer [k]))
		{	printf ("Decoder error : sample #%d (0x%02X should be 0x%02X)\n", k, short_buffer [k], alaw_decode (alaw_buffer [k])) ;
			exit (1) ;
			} ;

	sf_close (file) ;
	
	printf ("    alaw_test : decoder ... ok\n") ;
	
	unlink (filename) ;

	return 0 ;
} /* main */


/*=================================================================================
**	The following routines came from the sox-12.15 (Sound eXcahcnge) distribution.
**
**	This code is not compiled into libsndfile. It is only used to test the 
**	libsndfile lookup tables for correctness.
**	
**	I have included the original authors comments.
*/

/*
** A-law routines by Graeme W. Gill.
** Date: 93/5/7
**
** References:
** 1) CCITT Recommendation G.711
**
*/

#define ACLIP 31744

static
unsigned char alaw_encode (int sample)
{	static int exp_lut[128] = {1,1,2,2,3,3,3,3,
                               4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7};

    int sign, exponent, mantissa;
    unsigned char Alawbyte;

    /* Get the sample into sign-magnitude. */
    sign = ((~sample) >> 8) & 0x80 ;			/* set aside the sign */
    if (sign == 0) 
		sample = -sample ;		/* get magnitude */
    if (sample > ACLIP) 
		sample = ACLIP ;						/* clip the magnitude */

    /* Convert from 16 bit linear to ulaw. */
    if (sample >= 256)
	{	exponent = exp_lut [(sample >> 8) & 0x7F] ;
		mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F ;
		Alawbyte = ((exponent << 4) | mantissa) ;
		}
    else
		Alawbyte = (sample >> 4) ;
    
	Alawbyte ^= (sign ^ 0x55) ;

    return Alawbyte ;
} /* alaw_encode */

static
int alaw_decode (unsigned int Alawbyte)
{	static int exp_lut[8] = { 0, 264, 528, 1056, 2112, 4224, 8448, 16896 } ;
    int sign, exponent, mantissa, sample ;

    Alawbyte ^= 0x55 ;
    sign = ( Alawbyte & 0x80 ) ;
    Alawbyte &= 0x7f ;			/* get magnitude */
    if (Alawbyte >= 16)
	{	exponent = (Alawbyte >> 4 ) & 0x07 ;
		mantissa = Alawbyte & 0x0F ;
		sample = exp_lut[exponent] + (mantissa << ( exponent + 3 )) ;
		}
    else
		sample = (Alawbyte << 4) + 8 ;
    if (sign == 0) 
		sample = -sample ;

    return sample ;
} /* alaw_decode */






















