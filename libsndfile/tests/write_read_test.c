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
#include	<string.h>
#include	<unistd.h>
#include	<math.h>

#include	<sndfile.h>

#define	BUFFER_SIZE		(1<<15)

static	void	pcm_test_char	(char *str, char *filename, int typemajor, int typeminor) ;
static	void	pcm_test_short	(char *str, char *filename, int typemajor, int typeminor) ;
static	void	pcm_test_24bit	(char *str, char *filename, int typemajor, int typeminor) ;
static	void	pcm_test_int	(char *str, char *filename, int typemajor, int typeminor) ;

static	void	float_test	(char *str, char *filename, int typemajor) ;

/* Force the start of this buffer to be double aligned. Sparc-solaris will
** choke if its not.
*/
static	double	test_buffer [(BUFFER_SIZE/sizeof(double))+1] ;

int		main (int argc, char *argv[])
{	char	*filename ;
	int		bDoAll = 0 ;
	int		nTests = 0 ;

	if (argc != 2)
	{	printf ("Usage : %s <test>\n", argv [0]) ;
		printf ("    Where <test> is one of the following:\n") ;
		printf ("           wav       - test WAV file functions (little endian)\n") ;
		printf ("           wav_float - test WAV float file functions\n") ;
		printf ("           aiff      - test AIFF file functions (big endian)\n") ;
		printf ("           au        - test AU file functions (big endian)\n") ;
		printf ("           aule      - test little endian AU file functions\n") ;
		printf ("           raw       - test RAW header-less PCM file functions\n") ;
		printf ("           all       - perform all tests\n") ;
		exit (1) ;
		} ;

	bDoAll=!strcmp (argv [1], "all");
		
	if (bDoAll || ! strcmp (argv [1], "wav"))
	{	filename = "test.wav" ;
		pcm_test_char 	("wav", filename, SF_FORMAT_WAV, SF_FORMAT_PCM) ;
		pcm_test_short	("wav", filename, SF_FORMAT_WAV, SF_FORMAT_PCM) ;
		pcm_test_24bit	("wav", filename, SF_FORMAT_WAV, SF_FORMAT_PCM) ;
		pcm_test_int	("wav", filename, SF_FORMAT_WAV, SF_FORMAT_PCM) ;
		unlink (filename) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "wav_float"))
	{	filename = "test.wav" ;
		float_test	("wav_float", filename, SF_FORMAT_WAV) ;
		unlink (filename) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "aiff"))
	{	filename = "test.aiff" ;
		pcm_test_char  ("aiff", filename, SF_FORMAT_AIFF, SF_FORMAT_PCM) ;
		pcm_test_short ("aiff", filename, SF_FORMAT_AIFF, SF_FORMAT_PCM) ;
		pcm_test_24bit ("aiff", filename, SF_FORMAT_AIFF, SF_FORMAT_PCM) ;
		pcm_test_int   ("aiff", filename, SF_FORMAT_AIFF, SF_FORMAT_PCM) ;
		unlink (filename) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "au"))
	{	filename = "test.au" ;
		pcm_test_char  ("au", filename, SF_FORMAT_AU, SF_FORMAT_PCM) ;
		pcm_test_short ("au", filename, SF_FORMAT_AU, SF_FORMAT_PCM) ;
		pcm_test_24bit ("au", filename, SF_FORMAT_AU, SF_FORMAT_PCM) ;
		pcm_test_int   ("au", filename, SF_FORMAT_AU, SF_FORMAT_PCM) ;
		unlink (filename) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "aule"))
	{	filename = "test.au" ;
		pcm_test_char  ("aule", filename, SF_FORMAT_AULE, SF_FORMAT_PCM) ;
		pcm_test_short ("aule", filename, SF_FORMAT_AULE, SF_FORMAT_PCM) ;
		pcm_test_24bit ("aule", filename, SF_FORMAT_AULE, SF_FORMAT_PCM) ;
		pcm_test_int   ("aule", filename, SF_FORMAT_AULE, SF_FORMAT_PCM) ;
		unlink (filename) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "raw"))
	{	filename = "test.raw" ;
		pcm_test_char  ("raw-s8", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_S8) ;
		pcm_test_char  ("raw-u8", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_U8) ;
		pcm_test_short ("raw-le", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_LE) ;
		pcm_test_short ("raw-be", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_BE) ;
		pcm_test_24bit ("raw-le", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_LE) ;
		pcm_test_24bit ("raw-be", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_BE) ;
		pcm_test_int   ("raw-le", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_LE) ;
		pcm_test_int   ("raw-be", filename, SF_FORMAT_RAW, SF_FORMAT_RAW_BE) ;
		unlink (filename) ;
		nTests++ ;
		} ;

	if (nTests == 0)
	{	printf ("************************************\n") ;
		printf ("*  No '%s' test defined.\n", argv [1]) ;
		printf ("************************************\n") ;
		return 1 ;
		} ;

	return 0;
} /* main */

/*============================================================================================
 *	Here are the test functions.
 */ 

static	
void	pcm_test_char (char *str, char *filename, int typemajor, int typeminor)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	unsigned int	k, len ;
	short			*data ;

	printf ("    pcm_test_char  : %s ... ", str) ;

	len = 127 ;

	data = (short*) test_buffer ;
	for (k = 0 ; k < len ; k++)
		data [k] = k * ((k % 2) ? 1 : -1) ;
		
	sfinfo.samplerate  = 44100 ;
	sfinfo.samples     = 123456789 ; /* Wrong length. Library should correct this on sf_close. */
	sfinfo.channels    = 1 ;
	sfinfo.pcmbitwidth = 8 ;
	sfinfo.format 	   = (typemajor | typeminor) ;

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if ((k = sf_write_short (file, data, len)) != len)
	{	printf ("sf_write_short failed with short write (%d => %d).\n", len, k) ;
		exit (1) ;
		} ;
	sf_close (file) ;
	
	memset (data, 0, len * sizeof (short)) ;
	
	if (typemajor != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;
	
	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_read failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sfinfo.format != (typemajor | typeminor))
	{	printf ("Returned format incorrect (0x%08X => 0x%08X).\n", (typemajor | typeminor), sfinfo.format) ;
		exit (1) ;
		} ;
	
	if (sfinfo.samples != len)
	{	printf ("Incorrect number of samples in file. (%u => %u)\n", len, sfinfo.samples) ;
		exit (1) ;
		} ;
	
	if (sfinfo.channels != 1)
	{	printf ("Incorrect number of channels in file.\n") ;
		exit (1) ;
		} ;

	if (sfinfo.pcmbitwidth != 8)
	{	printf ("Incorrect bit width (%d).\n", sfinfo.pcmbitwidth) ;
		exit (1) ;
		} ;

	if ((k = sf_read_short (file, data, len)) != len)
	{	printf ("short read (%d).\n", k) ;
		exit (1) ;
		} ;
	sf_close (file) ;

	for (k = 0 ; k < len ; k++)
		if (data [k] != k * ((k % 2) ? 1 : -1))
		{	printf ("Incorrect sample (#%d : %d => %d).\n", k, k * ((k % 2) ? 1 : -1), data [k]) ;
			exit (1) ;
			} ;

	printf ("ok\n") ;
} /* pcm_test_char */

static	
void	pcm_test_short (char *str, char *filename, int typemajor, int typeminor)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	unsigned int	k, len ;
	short			*data ;

	printf ("    pcm_test_short : %s ... ", str) ;

	len = BUFFER_SIZE / sizeof (short) ;

	data = (short*) test_buffer ;
	for (k = 0 ; k < len ; k++)
		data [k] = k * ((k % 2) ? 1 : -1) ;
		
	sfinfo.samplerate  = 44100 ;
	sfinfo.samples     = len ;
	sfinfo.channels    = 1 ;
	sfinfo.pcmbitwidth = 16 ;
	sfinfo.format 	   = (typemajor | typeminor) ;

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if ((k = sf_write_short (file, data, len)) != len)
	{	printf ("sf_write_short failed with short write (%d => %d).\n", len, k) ;
		exit (1) ;
		} ;
	sf_close (file) ;
	
	memset (data, 0, len * sizeof (short)) ;

	if (typemajor != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;
	
	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_read failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sfinfo.format != (typemajor | typeminor))
	{	printf ("Returned format incorrect (0x%08X => 0x%08X).\n", (typemajor | typeminor), sfinfo.format) ;
		exit (1) ;
		} ;
	
	if (sfinfo.samples != len)
	{	printf ("Incorrect number of samples in file. (%d => %d)\n", len, sfinfo.samples) ;
		exit (1) ;
		} ;
	
	if (sfinfo.channels != 1)
	{	printf ("Incorrect number of channels in file.\n") ;
		exit (1) ;
		} ;

	if (sfinfo.pcmbitwidth != 16)
	{	printf ("Incorrect bit width (%d).\n", sfinfo.pcmbitwidth) ;
		exit (1) ;
		} ;

	if ((k = sf_read_short (file, data, len)) != len)
	{	printf ("short read (%d).\n", k) ;
		exit (1) ;
		} ;
	sf_close (file) ;

	for (k = 0 ; k < len ; k++)
		if (data [k] != k * ((k % 2) ? 1 : -1))
		{	printf ("Incorrect sample (#%d : %d => %d).\n", k, k * ((k % 2) ? 1 : -1), data [k]) ;
			exit (1) ;
			} ;

	printf ("ok\n") ;
} /* pcm_test_short */

static	
void	pcm_test_24bit (char *str, char *filename, int typemajor, int typeminor)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	unsigned int	k, len ;
	int				*data ;

	printf ("    pcm_test_24bit : %s ... ", str) ;

	len = BUFFER_SIZE / sizeof (int) ;

	data = (int*) test_buffer ;
	for (k = 0 ; k < len ; k++)
		data [k] = k * 256 * ((k % 2) ? 1 : -1) ;
		
	sfinfo.samplerate  = 44100 ;
	sfinfo.samples     = len ;
	sfinfo.channels    = 1 ;
	sfinfo.pcmbitwidth = 24 ;
	sfinfo.format 	   = (typemajor | typeminor) ;

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sf_write_int (file, data, len) != len)
	{	printf ("sf_write_int failed with error : ") ;
		sf_perror (file) ;
		exit (1) ;
		} ;
		
	sf_close (file) ;
	
	memset (data, 0, len * 3) ;

	if (typemajor != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;
	
	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_read failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sfinfo.format != (typemajor | typeminor))
	{	printf ("Returned format incorrect (0x%08X => 0x%08X).\n", (typemajor | typeminor), sfinfo.format) ;
		exit (1) ;
		} ;
	
	if (sfinfo.samples != len)
	{	printf ("Incorrect number of samples in file. (%d => %d)\n", len, sfinfo.samples) ;
		exit (1) ;
		} ;
	
	if (sfinfo.channels != 1)
	{	printf ("Incorrect number of channels in file.\n") ;
		exit (1) ;
		} ;

	if (sfinfo.pcmbitwidth != 24)
	{	printf ("Incorrect bit width (%d).\n", sfinfo.pcmbitwidth) ;
		exit (1) ;
		} ;

	if ((k = sf_read_int (file, data, len)) != len)
	{	printf ("short read (%d).\n", k) ;
		exit (1) ;
		} ;
	sf_close (file) ;

	for (k = 0 ; k < len ; k++)
		if (data [k] != k * 256 * ((k % 2) ? 1 : -1))
		{	printf ("Incorrect sample (#%d : %d => %d).\n", k, k * 256 * ((k % 2) ? 1 : -1), data [k]) ;
			exit (1) ;
			} ;
		
	printf ("ok\n") ;
} /* pcm_test_24bit */

static	
void	pcm_test_int (char *str, char *filename, int typemajor, int typeminor)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	unsigned int	k, len ;
	int				sign, *data ;

	printf ("    pcm_test_int   : %s ... ", str) ;

	len = BUFFER_SIZE / sizeof (int) ;

	data = (int*) test_buffer ;
	for (sign = 1, k = 0 ; k < len ; k++)
		data [k] = k * 200000 * ((k % 2) ? 1 : -1) ;
		
	sfinfo.samplerate  = 44100 ;
	sfinfo.samples     = len ;
	sfinfo.channels    = 1 ;
	sfinfo.pcmbitwidth = 32 ;
	sfinfo.format 	   = (typemajor | typeminor) ;

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sf_write_int (file, data, len) != len)
	{	printf ("sf_write_int failed with error : ") ;
		sf_perror (file) ;
		exit (1) ;
		} ;
	sf_close (file) ;
	
	memset (data, 0, len * sizeof (short)) ;

	if (typemajor != SF_FORMAT_RAW)
		memset (&sfinfo, 0, sizeof (sfinfo)) ;
	
	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_read failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sfinfo.format != (typemajor | typeminor))
	{	printf ("Returned format incorrect (0x%08X => 0x%08X).\n", (typemajor | typeminor), sfinfo.format) ;
		exit (1) ;
		} ;
	
	if (sfinfo.samples != len)
	{	printf ("Incorrect number of samples in file. (%d => %d)\n", len, sfinfo.samples) ;
		exit (1) ;
		} ;
	
	if (sfinfo.channels != 1)
	{	printf ("Incorrect number of channels in file.\n") ;
		exit (1) ;
		} ;

	if (sfinfo.pcmbitwidth != 32)
	{	printf ("Incorrect bit width (%d).\n", sfinfo.pcmbitwidth) ;
		exit (1) ;
		} ;

	if ((k = sf_read_int (file, data, len)) != len)
	{	printf ("short read (%d).\n", k) ;
		exit (1) ;
		} ;
	sf_close (file) ;

	for (sign = 1, k = 0 ; k < len ; k++)
		if (data [k] != k * 200000 * ((k % 2) ? 1 : -1))
		{	printf ("Incorrect sample (#%d : %d => %d).\n", k, (k % 2) ? 1 : -1, data [k]) ;
			exit (1) ;
			} ;

	printf ("ok\n") ;
} /* pcm_test_int */

static	
void	float_test (char *str, char *filename, int	typemajor)
{	SNDFILE			*file ;
	SF_INFO			sfinfo ;
	unsigned int	k, len ;
	int				sign ;
	double			*data, error ;

	printf ("    float_test     : %s ... ", str) ;

	len = BUFFER_SIZE / sizeof (double) ;

	data = (double*) test_buffer ;
	for (sign = 1, k = 0 ; k < len ; k++)
		data [k] = ((double) k) / 100.0 * (sign *= -1) ;
		
	sfinfo.samplerate = 44100 ;
	sfinfo.samples    = len ;
	sfinfo.channels   = 1 ;
	sfinfo.pcmbitwidth   = 32 ;
	sfinfo.format 	  = (typemajor | SF_FORMAT_FLOAT) ;

	if (! (file = sf_open_write (filename, &sfinfo)))
	{	printf ("sf_open_write failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sf_write_double (file, data, len, 0) != len)
	{	printf ("sf_write_int failed with error : ") ;
		sf_perror (file) ;
		exit (1) ;
		} ;
	sf_close (file) ;
	
	memset (data, 0, len * sizeof (double)) ;
	memset (&sfinfo, 0, sizeof (sfinfo)) ;
	
	if (! (file = sf_open_read (filename, &sfinfo)))
	{	printf ("sf_open_read failed with error : ") ;
		sf_perror (NULL) ;
		exit (1) ;
		} ;
	
	if (sfinfo.format != (typemajor | SF_FORMAT_FLOAT))
	{	printf ("Returned format incorrect (0x%08X => 0x%08X).\n", (typemajor | SF_FORMAT_FLOAT), sfinfo.format) ;
		exit (1) ;
		} ;
	
	if (sfinfo.samples != len)
	{	printf ("Incorrect number of samples in file. (%d => %d)\n", len, sfinfo.samples) ;
		exit (1) ;
		} ;
	
	if (sfinfo.channels != 1)
	{	printf ("Incorrect number of channels in file.\n") ;
		exit (1) ;
		} ;

	if (sfinfo.pcmbitwidth != 32)
	{	printf ("Incorrect bit width (%d).\n", sfinfo.pcmbitwidth) ;
		exit (1) ;
		} ;

	if ((k = sf_read_double (file, data, len, 0)) != len)
	{	printf ("short read (%d).\n", k) ;
		exit (1) ;
		} ;
	sf_close (file) ;

	for (sign = 1, k = 0 ; k < len ; k++)
	{	error = fabs (data [k] - ((double) k) / 100.0 * (sign *= -1)) ;
		if (fabs (data [k]) > 1e-100 && fabs (error / data [k]) > 1e-5)
		{	printf ("Incorrect sample (#%d : %f => %f).\n", k, ((double) k) / 100.0, data [k]) ;
			exit (1) ;
			} ;
		} ;

	printf ("ok\n") ;
} /* float_test */

