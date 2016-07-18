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


#include	"config.h"

#define		ENDSWAP_SHORT(x)			((((x)>>8)&0xFF)|(((x)&0xFF)<<8))
#define		ENDSWAP_INT(x)				((((x)>>24)&0xFF)|(((x)>>8)&0xFF00)|(((x)&0xFF00)<<8)|(((x)&0xFF)<<24))

#if (__LITTLE_ENDIAN__ == 1)

#	define	__CPU_IS_LITTLE_ENDIAN__		1
#	define	__CPU_IS_BIG_ENDIAN__			0

#	define	H2LE_SHORT(x)				(x)
#	define	H2LE_INT(x)					(x)
#	define	LE2H_SHORT(x)				(x)
#	define	LE2H_INT(x)					(x)

#	define	BE2H_INT(x)					ENDSWAP_INT(x)
#	define	BE2H_SHORT(x)				ENDSWAP_SHORT(x)
#	define	H2BE_INT(x)					ENDSWAP_INT(x)
#	define	H2BE_SHORT(x)				ENDSWAP_SHORT(x)

#elif (__BIG_ENDIAN__ == 1)

#	define	__CPU_IS_LITTLE_ENDIAN__		0
#	define	__CPU_IS_BIG_ENDIAN__			1

#	define	H2LE_SHORT(x)				ENDSWAP_SHORT(x)
#	define	H2LE_INT(x)					ENDSWAP_INT(x)
#	define	LE2H_SHORT(x)				ENDSWAP_SHORT(x)
#	define	LE2H_INT(x)					ENDSWAP_INT(x)

#	define	BE2H_INT(x)					(x)
#	define	BE2H_SHORT(x)				(x)
#	define	H2BE_INT(x)					(x)
#	define	H2BE_SHORT(x)				(x)

#else
#	error "Cannot determine endian-ness of processor."
#endif

