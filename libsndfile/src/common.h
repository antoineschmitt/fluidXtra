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

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#ifdef _WIN32
	#pragma pack(push,1)
#endif


#define	SF_BUFFER_LEN	(4096)

#define		BITWIDTH2BYTES(x)	(((x) + 7) / 8)

enum
{	SF_MODE_READ = 1, 
	SF_MODE_WRITE = 2
} ; 

enum
{	SF_ENDIAN_LITTLE = 100, 
	SF_ENDIAN_BIG = 200
} ; 

enum
{	SF_FALSE = 0, 
	SF_TRUE = 1
} ; 

typedef	int	(*func_seek) 	(void*, int, int) ;

typedef	int	(*func_short)	(void*, short *ptr, unsigned int len) ;
typedef	int	(*func_int)		(void*, int *ptr, unsigned int len) ;
typedef	int	(*func_double)	(void*, double *ptr, unsigned int len, int normalize) ;

typedef	int	(*func_close)	(void*) ;

typedef struct
{	/* Force the compiler to double align the start of buffer. */
	double		buffer		[SF_BUFFER_LEN/sizeof(double)] ;
	char		strbuffer	[SF_BUFFER_LEN] ;
	int			Magick ;
	int			strindex ;
	FILE 		*file ;
	int			mode ;
	int			filelength ;
	int			error ;
	int			current ;
	int			endian ;
	
	SF_INFO		sf ; 
	
	unsigned int	dataoffset ;		/* Offset in number of bytes from beginning of file. */
	unsigned int	datalength ;		/* Length in bytes of the audio data. */
	unsigned int	blockwidth ;		/* Size in bytes of one set of interleaved samples. */
 	unsigned int 	bytewidth ;			/* Size in bytes of one sample (one channel). */

	void		*fdata ;
	
	double		normfactor ;

	func_seek	seek_func ;

	func_short		read_short ;
	func_int		read_int ;
	func_double		read_double ;

	func_short		write_short ;
	func_int		write_int ;
	func_double		write_double ;

	func_close	close ;

} SF_PRIVATE ;

enum
{	SFE_NO_ERROR	= 0,

	SFE_BAD_FILE,
	SFE_OPEN_FAILED,
	SFE_BAD_OPEN_FORMAT,
	SFE_BAD_SNDFILE_PTR,
	SFE_BAD_SF_INFO_PTR,
	SFE_BAD_INT_FD,
	SFE_BAD_INT_PTR,
	SFE_MALLOC_FAILED, 
	SFE_BAD_SEEK, 
	SFE_NOT_SEEKABLE,
	SFE_UNIMPLEMENTED,
	SFE_BAD_READ_ALIGN,
	SFE_BAD_WRITE_ALIGN,
	SFE_UNKNOWN_FORMAT,
	SFE_NOT_READMODE,
	SFE_NOT_WRITEMODE,
	SFE_BAD_SF_INFO,
	SFE_SHORT_READ,
	SFE_SHORT_WRITE,
	
	SFE_WAV_NO_RIFF,
	SFE_WAV_NO_WAVE,
	SFE_WAV_NO_FMT,
	SFE_WAV_FMT_SHORT,
	SFE_WAV_FMT_TOO_BIG,
	SFE_WAV_BAD_FORMAT,
	SFE_WAV_BAD_BLOCKALIGN,
	SFE_WAV_NO_DATA,
	SFE_WAV_ADPCM_NOT4BIT,
	SFE_WAV_ADPCM_CHANNELS,
	SFE_WAV_UNKNOWN_CHUNK,

	SFE_AIFF_NO_FORM,
	SFE_AIFF_UNKNOWN_CHUNK,
	SFE_COMM_CHUNK_SIZE,
	SFE_AIFF_NO_SSND,
	SFE_AIFF_NO_DATA,

	SFE_AU_UNKNOWN_FORMAT,
	
	SFE_RAW_READ_BAD_SPEC,
	SFE_RAW_BAD_BITWIDTH,
	
	SFE_MAX_ERROR			/* This must be last in list. */
} ;

void	__endswap_short_array	(short *ptr, int len) ;
void	__endswap_int_array 	(int *ptr, int len) ;

void	__psf_sprintf		(SF_PRIVATE *psf, char *format, ...) ;

int		__aiff_open_read	(SF_PRIVATE *psf) ;
int		__aiff_open_write	(SF_PRIVATE *psf) ;

int		__au_open_read		(SF_PRIVATE *psf) ;
int		__au_open_write		(SF_PRIVATE *psf) ;

int		__wav_open_read		(SF_PRIVATE *psf) ;
int		__wav_open_write	(SF_PRIVATE *psf) ;

int		__raw_open_read		(SF_PRIVATE *psf) ;
int		__raw_open_write	(SF_PRIVATE *psf) ;

#ifdef _WIN32
	#pragma pack(pop,1)
#endif

#endif /* COMMON_H_INCLUDED */

