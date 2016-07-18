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
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>

#ifdef _WINDOWS
#include <Windows.h>
#include <wchar.h>
#endif

/* For the Metrowerks CodeWarrior Pro Compiler (mainly MacOS) */

#if	(defined (__MWERKS__))
#include	<stat.h>
#else
#include	<sys/stat.h>
#endif

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"

#define	SHORT_STR_LEN	10

typedef struct
{	int 	error ;
	char	*str ;
} ErrorStruct ;

static
ErrorStruct SndfileErrors [] = 
{ 
	{	SFE_NO_ERROR			, "No Error." },
	{	SFE_BAD_FILE			, "File does not exist or is not a regular file (possibly a pipe?)." },
	{	SFE_OPEN_FAILED			, "Could not open file." },
	{	SFE_BAD_OPEN_FORMAT		, "Bad format specified for file open." },
	{	SFE_BAD_SNDFILE_PTR		, "Not a valid SNDFILE* pointer." },
	{	SFE_BAD_SF_INFO_PTR		, "NULL SF_INFO pointer passed to libsndfile." },
	{	SFE_BAD_INT_FD			, "Bad file descriptor." },
	{	SFE_BAD_INT_PTR			, "Internal error, Bad pointer." },
	{	SFE_MALLOC_FAILED		, "Internal malloc () failed." },
	{	SFE_BAD_SEEK			, "Internal fseek() failed." },
	{	SFE_NOT_SEEKABLE		, "Seek attempted on unseekable file type." },
	{	SFE_UNIMPLEMENTED		, "File contains data in an unimplemented format." },
	{	SFE_BAD_READ_ALIGN  	, "Attempt to read a non-integer number of channels." },
	{	SFE_BAD_WRITE_ALIGN 	, "Attempt to write a non-integer number of channels." },
	{	SFE_UNKNOWN_FORMAT		, "File contains data in an unknown format." },
	{	SFE_NOT_READMODE		, "Read attempted on file currently open for write." },
	{	SFE_NOT_WRITEMODE		, "Write attempted on file currently open for read." },
	{	SFE_BAD_SF_INFO			, "SF_INFO struct incomplete." },

	{	SFE_SHORT_READ			, "Short read error." },
	{	SFE_SHORT_WRITE			, "Short write error." },
	
	{	SFE_WAV_NO_RIFF			, "Error in WAV file. No 'RIFF' chunk marker." },
	{	SFE_WAV_NO_WAVE			, "Error in WAV file. No 'WAVE' chunk marker." },
	{	SFE_WAV_NO_FMT			, "Error in WAV file. No 'fmt ' chunk marker." },
	{	SFE_WAV_FMT_SHORT		, "Error in WAV file. Short 'fmt ' chunk." },

	{	SFE_WAV_FMT_TOO_BIG		, "Error in WAV file. 'fmt ' chunk too large." },

	{	SFE_WAV_BAD_FORMAT		, "Error in WAV file. Errors in 'fmt ' chunk." },
	{	SFE_WAV_BAD_BLOCKALIGN	, "Error in WAV file. Block alignment in 'fmt ' chunk is incorrect." },
	{	SFE_WAV_NO_DATA			, "Error in WAV file. No 'data' chunk marker." },
	{	SFE_WAV_UNKNOWN_CHUNK	, "Error in WAV file. File contains an unknown chunk marker." },
	
	{	SFE_WAV_ADPCM_NOT4BIT	, "Error in ADPCM WAV file. Invalid bit width."},
	{	SFE_WAV_ADPCM_CHANNELS	, "Error in ADPCM WAV file. Invalid number of channels."},
  
	{	SFE_AIFF_NO_FORM		, "Error in AIFF file, bad 'FORM' chunk."},
	{	SFE_AIFF_UNKNOWN_CHUNK	, "Error in AIFF file, unknown chunk."},
	{	SFE_COMM_CHUNK_SIZE		, "Error in AIFF file, bad 'COMM' chunk size."},
	{	SFE_AIFF_NO_SSND		, "Error in AIFF file, bad 'SSND' chunk."},
	{	SFE_AIFF_NO_DATA		, "Error in AIFF file, no sound data."},
	
	{	SFE_AU_UNKNOWN_FORMAT	, "Error in AU file, unknown format."},

	{	SFE_RAW_READ_BAD_SPEC	, "Error while opening RAW file for read. Must specify format, pcmbitwidth and channels."},
	{	SFE_RAW_BAD_BITWIDTH	, "RAW file bitwidth must be a multiple of 8."},

	{	SFE_MAX_ERROR			, "Maximum error number." }
} ;

/*------------------------------------------------------------------------------
** Private (static) variables.
*/

static	int		sf_errno = 0 ;
static	char	sf_strbuffer [SF_BUFFER_LEN] = { 0 } ;

/*------------------------------------------------------------------------------
** Private static functions.
*/

#define	VALIDATE_SNDFILE_AND_ASSIGN_PSF(a,b)		\
		{	if (! (a))								\
				return SFE_BAD_SNDFILE_PTR ;		\
			(b) = (SF_PRIVATE*) (a) ;				\
			if (!((b)->file))						\
				return SFE_BAD_INT_FD ;				\
			if ((b)->Magick != 0xFEEDC0DE)			\
				return	SFE_BAD_SNDFILE_PTR ;		\
			(b)->error = 0 ;						\
			} 

static
int is_real_file (const char *filename)
{	struct stat statbuf ;

// ASjun12 : on Windows, the _wstat function only accepts widechars
#ifdef _WINDOWS
	wchar_t tmpw[1024];
	int len = MultiByteToWideChar(CP_UTF8, 0, filename, strlen(filename), tmpw, 1024);
	tmpw[len] = 0;
	if (_wstat (tmpw, &statbuf))
#else
	if (stat (filename, &statbuf))
#endif
		return 0 ;
	
	if (! S_ISREG (statbuf.st_mode))
		return 0 ;
		
	return 1 ;
} /* is_real_file */

static
int guess_file_type (const char *filename)
{	char *ptr, buffer [SHORT_STR_LEN] ;

	if (! (ptr = (char *)strrchr (filename, '.'))) 		/* Find the file extension. */
		return	0 ;
	ptr ++ ;									/* Move past dot. */
	
	if (strlen (ptr) > SHORT_STR_LEN - 1)
		return	0 ;
		
	strncpy (buffer, ptr, SHORT_STR_LEN - 1) ;
	buffer [SHORT_STR_LEN - 1] = 0 ;
	
	for (ptr = buffer ; *ptr ; ptr ++)
		*ptr = tolower (*ptr) ;
	
	if (! strcmp (buffer, "wav"))
		return 1 ;
	if (! strcmp (buffer, "aiff") || ! strcmp (buffer, "aif") || ! strcmp (buffer, "aifc"))
		return 2 ;
	if (! strcmp (buffer, "au") || ! strcmp (buffer, "snd"))
		return 3 ;
	if (! strcmp (buffer, "raw") || ! strcmp (buffer, "pcm"))
		return 4 ;
	
	return 0 ;
} /* guess_file_type */


static
int validate_sfinfo (SF_INFO *sfinfo)
{	if (! sfinfo->samplerate)
		return 0 ;	
	if (! sfinfo->samples)
		return 0 ;	
	if (! sfinfo->channels)
		return 0 ;	
	if (! sfinfo->pcmbitwidth)
		return 0 ;	
	if (! sfinfo->format)
		return 0 ;	
	if (! sfinfo->sections)
		return 0 ;	
	return 1 ;
} /* validate_sfinfo */

static
int validate_psf (SF_PRIVATE *psf)
{	if (! psf->blockwidth)
		return 0 ;	
	if (! psf->bytewidth)
		return 0 ;	
	if (! psf->datalength)
		return 0 ;
	if (psf->blockwidth != psf->sf.channels * psf->bytewidth)
		return 0 ;	
	return 1 ;
} /* validate_psf */

static
void save_header_info (SF_PRIVATE *psf)
{	strcpy (sf_strbuffer, psf->strbuffer) ;
} /* validate_psf */


/*------------------------------------------------------------------------------
**	Public functions.
*/

SNDFILE* 	sf_open_read	(const char *path, SF_INFO *sfinfo)
{	SF_PRIVATE 	*psf ;
	int			filetype ;
#ifdef _WINDOWS
	wchar_t tmpw[1024];
	int wlen;
#endif

	if (! sfinfo)
	{	sf_errno = SFE_BAD_SF_INFO_PTR ;
		return	NULL ;
		} ;

	sf_errno = 0 ;
	sf_strbuffer [0] = 0 ;
	
	if (! is_real_file (path))
	{	sf_errno = SFE_BAD_FILE ;
		return	NULL ;
		} ;
	
	psf = (SF_PRIVATE*) malloc (sizeof (SF_PRIVATE)) ; 
	if (! psf)
	{	sf_errno = SFE_MALLOC_FAILED ;
		return	NULL ;
		} ;

	memset (psf, 0, sizeof (SF_PRIVATE)) ;
	psf->Magick = 0xFEEDC0DE ;

	/* fopen with 'b' means binary file mode for Win32 systems. */
	
#ifdef _WINDOWS
	// ASjun12 : on Windows, the _wfopen function only accepts widechars
	wlen = MultiByteToWideChar(CP_UTF8, 0, path, strlen(path), tmpw, 1024);
	tmpw[wlen] = 0;
	if (! (psf->file = _wfopen (tmpw, L"rb")))
#else
	if (! (psf->file = fopen (path, "rb")))
#endif
	{	sf_errno = SFE_OPEN_FAILED ;
		free (psf) ;
		return NULL ;
		} ;
	
	psf->mode = SF_MODE_READ ;
	
	filetype = guess_file_type (path) ;
	
	fseek (psf->file, 0, SEEK_END) ;
	psf->filelength = ftell (psf->file) ;
	fseek (psf->file, 0, SEEK_SET) ;

	__psf_sprintf (psf, "%s\nsize : %d\n", path, psf->filelength) ;
	
	switch (filetype)
	{	case	1 :
				sf_errno = __wav_open_read (psf) ;
				break ;

		case	2 :
				sf_errno = __aiff_open_read (psf) ;
				break ;

		case	3 :
				sf_errno = __au_open_read (psf) ;
				break ;

		case	4 :
				/* For RAW files, need the sfinfo struct data to
				** figure out the bitwidth, endian-ness etc.
				*/
				memcpy (&(psf->sf), sfinfo, sizeof (SF_INFO)) ;
				sf_errno = __raw_open_read (psf) ;
				break ;

		default :	
				sf_errno = SFE_UNKNOWN_FORMAT ;
		} ;
		
	if (sf_errno)
	{	save_header_info (psf) ;
		free (psf) ;
		return NULL ;
		} ;

	if (! validate_sfinfo (&(psf->sf)))
	{	save_header_info (psf) ;
		sf_errno = SFE_BAD_SF_INFO ;
		free (psf) ;
		return NULL ;
		} ;
		
	if (! validate_psf (psf))
	{	save_header_info (psf) ;
		sf_errno = 666 ;
		free (psf) ;
		return NULL ;
		} ;
		
	memcpy (sfinfo, &(psf->sf), sizeof (SF_INFO)) ;

	return (SNDFILE*) psf ;
} /* sf_open_read */

/*------------------------------------------------------------------------------
*/

SNDFILE* 	sf_open_write	(const char *path, const SF_INFO *sfinfo)
{	SF_PRIVATE 	*psf ;
#ifdef _WINDOWS
	WCHAR tmpw[1024];
	int wlen;
#endif

	if (! sfinfo)
	{	sf_errno = SFE_BAD_SF_INFO_PTR ;
		return	NULL ;
		} ;
		
	if (! sf_format_check (sfinfo))
	{	sf_errno = SFE_BAD_OPEN_FORMAT ;
		return	NULL ;
		} ;

	sf_errno = 0 ;
	sf_strbuffer [0] = 0 ;
	
	psf = (SF_PRIVATE*) malloc (sizeof (SF_PRIVATE)) ; 
	if (! psf)
	{	sf_errno = SFE_MALLOC_FAILED ;
		return	NULL ;
		} ;
	
	memset (psf, 0, sizeof (SF_PRIVATE)) ;
	memcpy (&(psf->sf), sfinfo, sizeof (SF_INFO)) ;

	psf->Magick = 0xFEEDC0DE ;

	/* fopen with 'b' means binary file mode for Win32 systems. */
	
#ifdef _WINDOWS
	// ASjun12 : on Windows, the _wfopen function only accepts widechars
	wlen = MultiByteToWideChar(CP_UTF8, 0, path, strlen(path), tmpw, 1024);
	tmpw[wlen] = 0;
	if (! (psf->file = _wfopen (tmpw, L"wb")))
#else
	if (! (psf->file = fopen (path, "wb")))
#endif
	{	sf_errno = SFE_OPEN_FAILED ;
		free (psf) ;
		return NULL ;
		} ;
		
	psf->mode = SF_MODE_WRITE ;
	
	psf->filelength = ftell (psf->file) ;
	fseek (psf->file, 0, SEEK_SET) ;

	switch (sfinfo->format & SF_FORMAT_TYPEMASK)
	{	case	SF_FORMAT_WAV :
				if ((sf_errno = __wav_open_write (psf)))
				{	free (psf) ;
					return NULL ;
					} ;
				break ;

		case	SF_FORMAT_AIFF :
				if ((sf_errno = __aiff_open_write (psf)))
				{	free (psf) ;
					return NULL ;
					} ;
				break ;

		case	SF_FORMAT_AU :
		case	SF_FORMAT_AULE :
				if ((sf_errno = __au_open_write (psf)))
				{	free (psf) ;
					return NULL ;
					} ;
				break ;
				
		case    SF_FORMAT_RAW :
				if ((sf_errno = __raw_open_write (psf)))
				{	free (psf) ;
					return NULL ;
					} ;
				break ;

		default :	
				sf_errno = SFE_UNKNOWN_FORMAT ;
				free (psf) ;
				return NULL ;
		} ;				

	return (SNDFILE*) psf ;
} /* sf_open_write */

/*------------------------------------------------------------------------------
*/

int	sf_perror	(SNDFILE *sndfile)
{	SF_PRIVATE 	*psf ;
	int 		k, errnum ;

	if (! sndfile)
	{	errnum = sf_errno ;
		}
	else
	{	VALIDATE_SNDFILE_AND_ASSIGN_PSF(sndfile,psf) ;
		errnum = psf->error ;
		} ;
		
	errnum = (errnum >= SFE_MAX_ERROR || errnum < 0) ? 0 : errnum ;

	for (k = 0 ; SndfileErrors[k].str ; k++)
		if (errnum == SndfileErrors[k].error)
		{	printf ("%s\n", SndfileErrors[k].str) ;
			return SFE_NO_ERROR ;
			} ;
	
	printf ("No error string for error number %d.\n", errnum) ;
	return SFE_NO_ERROR ;
} /* sf_perror */


/*------------------------------------------------------------------------------
*/

int	sf_error_str	(SNDFILE *sndfile, char *str, size_t maxlen)
{	SF_PRIVATE 	*psf ;
	int 		errnum, k ;

	if (! sndfile)
	{	errnum = sf_errno ;
		}
	else
	{	VALIDATE_SNDFILE_AND_ASSIGN_PSF(sndfile,psf) ;
		errnum = psf->error ;
		} ;
		
	errnum = (errnum >= SFE_MAX_ERROR || errnum < 0) ? 0 : errnum ;

	for (k = 0 ; SndfileErrors[k].str ; k++)
		if (errnum == SndfileErrors[k].error)
		{	strncpy (str, SndfileErrors [errnum].str, maxlen) ;
			str [maxlen-1] = 0 ;
			return SFE_NO_ERROR ;
			} ;
			
	strncpy (str, "No error defined for this error number. This is a bug in libsndfile.", maxlen) ;		
	str [maxlen-1] = 0 ;
			
	return SFE_NO_ERROR ;
} /* sf_error_str */

/*------------------------------------------------------------------------------
*/

int	sf_error_number	(int errnum, char *str, size_t maxlen)
{	int 		k ;

	errnum = (errnum >= SFE_MAX_ERROR || errnum < 0) ? 0 : errnum ;

	for (k = 0 ; SndfileErrors[k].str ; k++)
		if (errnum == SndfileErrors[k].error)
		{	strncpy (str, SndfileErrors [errnum].str, maxlen) ;
			str [maxlen-1] = 0 ;
			return SFE_NO_ERROR ;
			} ;
			
	strncpy (str, "No error defined for this error number. This is a bug in libsndfile.", maxlen) ;		
	str [maxlen-1] = 0 ;
			
	return SFE_NO_ERROR ;
} /* sf_error_number */

/*------------------------------------------------------------------------------
*/

int		sf_format_check	(const SF_INFO *info)
{	int	subformat = info->format & SF_FORMAT_SUBMASK ;

	if (info->channels < 1 || info->channels > 256)
		return 0 ;

	switch (info->format & SF_FORMAT_TYPEMASK)
	{	case SF_FORMAT_WAV :
				if (subformat == SF_FORMAT_PCM && (info->pcmbitwidth >= 8 && info->pcmbitwidth <= 32))
					return 1 ;
				if (subformat == SF_FORMAT_FLOAT && info->pcmbitwidth == 32)
					return 1 ;
				if (subformat == SF_FORMAT_IMA_ADPCM && info->pcmbitwidth == 16 && info->channels <= 2)
					return 1 ;
				if (subformat == SF_FORMAT_MS_ADPCM && info->pcmbitwidth == 16 && info->channels <= 2)
					return 1 ;
				if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
					return 1 ;
				break ;
				
		case SF_FORMAT_AIFF :
				if (subformat == SF_FORMAT_PCM && (info->pcmbitwidth >= 8 && info->pcmbitwidth <= 32))
					return 1 ;
				break ;
				
		case SF_FORMAT_AU :
				if (subformat == SF_FORMAT_PCM && (info->pcmbitwidth >= 8 && info->pcmbitwidth <= 32))
					return 1 ;
				if (subformat == SF_FORMAT_ULAW)
					return 1 ;
				if (subformat == SF_FORMAT_ALAW)
					return 1 ;
				break ;
				
		case SF_FORMAT_AULE :
				if (subformat == SF_FORMAT_PCM && (info->pcmbitwidth >= 8 && info->pcmbitwidth <= 32))
					return 1 ;
				if (subformat == SF_FORMAT_ULAW)
					return 1 ;
				if (subformat == SF_FORMAT_ALAW)
					return 1 ;
				break ;
				
		case SF_FORMAT_RAW :
				if (subformat == SF_FORMAT_RAW_S8 && info->pcmbitwidth == 8)
					return 1 ;
				if (subformat == SF_FORMAT_RAW_U8 && info->pcmbitwidth == 8)
					return 1 ;
				if (subformat != SF_FORMAT_RAW_BE && subformat != SF_FORMAT_RAW_LE)
					break ;
				if (info->pcmbitwidth % 8 || info->pcmbitwidth > 32)
					break ;
				return 1 ;
				break ;
				
		default : break ;
		} ;
	return 0 ;
} /* sf_format_check */

/*------------------------------------------------------------------------------
*/

size_t	sf_get_header_info	(SNDFILE *sndfile, char *buffer, size_t bufferlen, size_t offset)
{	SF_PRIVATE	*psf ;
	int			len ;
	
	if (! sndfile)
	{	strncpy (buffer, sf_strbuffer, bufferlen - 1) ;
		buffer [bufferlen - 1] = 0 ;
		return strlen (sf_strbuffer) ;
		} ;
	

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;
	
	len = strlen (psf->strbuffer) ;
	if (offset < 0 || offset >= len)
		return 0 ;
	
	strncpy (buffer, psf->strbuffer, bufferlen - 1) ;
	buffer [bufferlen - 1] = 0 ;
	
	return strlen (psf->strbuffer) ;
} /* sf_format_check */

/*------------------------------------------------------------------------------
*/

double  sf_signal_max   (SNDFILE *sndfile)
{	SF_PRIVATE 		*psf ;
	off_t			position ;
	unsigned int	k, len, readcount ;
	double 			max = 0.0, *data, temp ;
	
	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;
	
	if (! psf->sf.seekable)
	{	psf->error = SFE_NOT_SEEKABLE ;
		return	0.0 ;
		} ;
		
	
	if (! psf->read_double)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	-1 ;
		} ;
		
	position = sf_seek (sndfile, 0, SEEK_CUR) ; /* Get current position in file */
	sf_seek (sndfile, 0, SEEK_SET) ;			/* Go to start of file. */
	
	len = psf->sf.channels * 1024 ;
	
	data = (double*) malloc (len * sizeof (double)) ;
	readcount = len ;
	while (readcount == len)
	{	readcount = psf->read_double (psf, data, len, 0) ;
		for (k = 0 ; k < len ; k++)
		{	temp = data [k] ;
			temp = temp < 0.0 ? -temp : temp ;
			max  = temp > max ? temp : max ;
			} ;
		} ;
	free (data) ;
	
	sf_seek (sndfile, position, SEEK_SET) ;		/* Return to original position. */
	
	return	max ;
} /* sf_signal_max */

/*------------------------------------------------------------------------------
*/

off_t	sf_seek	(SNDFILE *sndfile, off_t offset, int whence)
{	SF_PRIVATE 	*psf ;
	off_t		realseek, position ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;
	
	if (! psf->sf.seekable)
	{	psf->error = SFE_NOT_SEEKABLE ;
		return	((off_t) -1) ;
		} ;
	
	if (psf->seek_func)
		return	psf->seek_func (psf, offset, whence) ;
		
	if (! (psf->blockwidth && psf->datalength && psf->dataoffset))
	{	psf->error = SFE_BAD_SEEK ;
		return	((off_t) -1) ;
		} ;
	
	switch (whence)
	{	case SEEK_SET :
				if (offset < 0 || offset * psf->blockwidth > psf->datalength)
				{	psf->error = SFE_BAD_SEEK ;
					return	((off_t) -1) ;
					} ;
				realseek = psf->dataoffset + offset * psf->blockwidth ;
				fseek (psf->file, realseek, SEEK_SET) ;
				position = ftell (psf->file) ;
				break ;
				
		case SEEK_CUR :
				realseek = offset * psf->blockwidth ;
				position = ftell (psf->file) - psf->dataoffset ;
				if (position + realseek > psf->datalength || position + realseek < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((off_t) -1) ;
					} ;
				fseek (psf->file, realseek, SEEK_CUR) ;
				position = ftell (psf->file) ;
				break ;
				
		case SEEK_END :
				if (offset > 0 || psf->sf.samples + offset < 0)
				{	psf->error = SFE_BAD_SEEK ;
					return	((off_t) -1) ;
					} ;
				realseek = (psf->sf.samples + offset) * psf->blockwidth + psf->dataoffset ;

				fseek (psf->file, realseek, SEEK_SET) ;
				position = ftell (psf->file) ;
				break ;
				
		default : 
				psf->error = SFE_BAD_SEEK ;
				return	((off_t) -1) ;
		} ;

	psf->current = (position - psf->dataoffset) / psf->blockwidth ;
	return psf->current ;
} /* sf_seek */

/*------------------------------------------------------------------------------
*/

size_t		sf_read_raw		(SNDFILE *sndfile, void *ptr, size_t bytes)
{	SF_PRIVATE 	*psf ;
	size_t		count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;
	
	if (psf->mode != SF_MODE_READ)
	{	psf->error = SFE_NOT_READMODE ;
		return	((size_t) -1) ;
		} ;
	
	if (psf->current >= psf->datalength)
	{	memset (ptr, 0, bytes) ;
		return 0 ;
		} ;
	
	if (bytes % (psf->sf.channels * psf->bytewidth))
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return (size_t) -1 ;
		} ;
	
	count = fread (ptr, 1, bytes, psf->file) ;
		
	if (count < bytes)
		memset (((char*)ptr) + count, 0, bytes - count) ;

	psf->current += count / psf->blockwidth ;
	
	return count ;
} /* sf_read_raw */

/*------------------------------------------------------------------------------
*/

size_t	sf_read_short		(SNDFILE *sndfile, short *ptr, size_t len)
{	SF_PRIVATE 	*psf ;
	size_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;
	
	if (psf->mode != SF_MODE_READ)
	{	psf->error = SFE_NOT_READMODE ;
		return (size_t) -1 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_READ_ALIGN ;
		return (size_t) -1 ;
		} ;
	
	if (psf->current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (short)) ;
		return 0 ; /* End of file. */
		} ;

	if (! psf->read_short)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	(size_t) -1 ;
		} ;
		
	count = psf->read_short (psf, ptr, len) ;
	
	if (psf->current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (short)) ;
		psf->current = psf->sf.samples ;
		} ;
	
	psf->current += count / psf->sf.channels ;
	
	return count ;
} /* sf_read_short */

/*------------------------------------------------------------------------------
*/

size_t	sf_read_int		(SNDFILE *sndfile, int *ptr, size_t len)
{	SF_PRIVATE 	*psf ;
	size_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF   (sndfile,psf) ;

	if (psf->mode != SF_MODE_READ)
		return SFE_NOT_READMODE ;
	
	if (len % psf->sf.channels)
		return	(psf->error = SFE_BAD_READ_ALIGN) ;
	
	if (psf->current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (int)) ;
		return 0 ;
		} ;

	if (! psf->read_int)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	(size_t) -1 ;
		} ;
		
	count = psf->read_int (psf, ptr, len) ;
	
	if (psf->current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (int)) ;
		psf->current = psf->sf.samples ;
		} ;
	
	psf->current += count / psf->sf.channels ;
	
	return count ;
} /* sf_read_int */

/*------------------------------------------------------------------------------
*/

size_t	sf_read_double	(SNDFILE *sndfile, double *ptr, size_t len, int normalize)
{	SF_PRIVATE 	*psf ;
	size_t		count, extra ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile, psf) ;
	
	if (psf->mode != SF_MODE_READ)
		return SFE_NOT_READMODE ;
	
	if (len % psf->sf.channels)
		return	(psf->error = SFE_BAD_READ_ALIGN) ;
	
	if (psf->current >= psf->sf.samples)
	{	memset (ptr, 0, len * sizeof (double)) ;
		return 0 ;
		} ;
		
	if (! psf->read_double)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return	(size_t) -1 ;
		} ;
		
	count = psf->read_double (psf, ptr, len, normalize) ;
	
	if (psf->current + count / psf->sf.channels > psf->sf.samples)
	{	count = (psf->sf.samples - psf->current) * psf->sf.channels ;
		extra = len - count ;
		memset (ptr + count, 0, extra * sizeof (double)) ;
		psf->current = psf->sf.samples ;
		} ;
	
	psf->current += count / psf->sf.channels ;
	
	return count ;
} /* sf_read_double */

/*------------------------------------------------------------------------------
*/

size_t	sf_write_raw	(SNDFILE *sndfile, void *ptr, size_t len)
{	SF_PRIVATE 	*psf ;
	size_t		count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;

	if (psf->mode != SF_MODE_WRITE)
		return SFE_NOT_WRITEMODE ;
	
	if (len % (psf->sf.channels * psf->bytewidth))
		return	(psf->error = SFE_BAD_WRITE_ALIGN) ;
	
	count = fwrite (ptr, 1, len, psf->file) ;
	
	psf->current += count / psf->blockwidth ;
	
	return 0 ;
} /* sf_write_raw */

/*------------------------------------------------------------------------------
*/

size_t	sf_write_short		(SNDFILE *sndfile, short *ptr, size_t len)
{	SF_PRIVATE 	*psf ;
	size_t		count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;

	if (psf->mode != SF_MODE_WRITE)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return (size_t) -1 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return (size_t) -1 ;
		} ;
	
	if (! psf->write_short)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return (size_t) -1 ;
		} ;
		
	count = psf->write_short (sndfile, ptr, len) ;
	
	psf->current += count / psf->sf.channels ;
	
	return count ;
} /* sf_write_short */

/*------------------------------------------------------------------------------
*/

size_t	sf_write_int		(SNDFILE *sndfile, int *ptr, size_t len)
{	SF_PRIVATE 	*psf ;
	size_t		count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;

	if (psf->mode != SF_MODE_WRITE)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return (size_t) -1 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return (size_t) -1 ;
		} ;
	
	if (! psf->write_int)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return (size_t) -1 ;
		} ;
		
	count = psf->write_int (sndfile, ptr, len) ;
	
	psf->current += count / psf->sf.channels ;
	
	return count ;
} /* sf_write_int */

/*------------------------------------------------------------------------------
*/

size_t	sf_write_double		(SNDFILE *sndfile, double *ptr, size_t len, int normalize)
{	SF_PRIVATE 	*psf ;
	size_t		count ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;

	if (psf->mode != SF_MODE_WRITE)
	{	psf->error = SFE_NOT_WRITEMODE ;
		return (size_t) -1 ;
		} ;
	
	if (len % psf->sf.channels)
	{	psf->error = SFE_BAD_WRITE_ALIGN ;
		return	(size_t) -1 ;
		} ;
		
	if (! psf->write_double)
	{	psf->error = SFE_UNIMPLEMENTED ;
		return (size_t) -1 ;
		} ;
		
	count = psf->write_double (sndfile, ptr, len, normalize) ;
	
	psf->current += count / psf->sf.channels ;
	
	return count ;
} /* sf_write_double */

/*------------------------------------------------------------------------------
*/

int	sf_close	(SNDFILE *sndfile)
{	SF_PRIVATE  *psf ;
	int			error ;

	VALIDATE_SNDFILE_AND_ASSIGN_PSF (sndfile,psf) ;

	if (psf->close)
		error = psf->close (psf) ;
	
	fclose (psf->file) ;
	memset (psf, 0, sizeof (SF_PRIVATE)) ;
		
	free (psf) ;

	return 0 ;
} /* sf_close */


/*=========================================================================
** Private functions.
*/


