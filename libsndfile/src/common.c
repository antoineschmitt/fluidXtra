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

#include	<stdarg.h>

#include	"sndfile.h"
#include	"common.h"

/*-----------------------------------------------------------------------------------------------
 */

void	__endswap_short_array (short *ptr, int len)
{	int k ;
	for (k = 0 ; k < len ; k++)
		ptr[k] = ((((ptr[k])>>8)&0xFF)|(((ptr[k])&0xFF)<<8)) ;
} /* __endswap_short_array */

void	__endswap_int_array (int *ptr, int len)
{	int k ;
	for (k = 0 ; k < len ; k++)
		ptr[k] = ((((ptr[k])>>24)&0xFF)|(((ptr[k])>>8)&0xFF00)|
					(((ptr[k])&0xFF00)<<8)|(((ptr[k])&0xFF)<<24)) ;		
} /* __endswap_int_array */

/*-----------------------------------------------------------------------------------------------
 */

#define __psf_putchar(a,b)									\
			if ((a)->strindex < SF_BUFFER_LEN - 1)			\
			{	(a)->strbuffer [(a)->strindex++] = (b) ;	\
				(a)->strbuffer [(a)->strindex] = 0 ;		\
				} ;

void __psf_sprintf (SF_PRIVATE *psf, char *format, ...)
{	va_list	ap ;
	int     d, tens, shift ;
	char    c, *strptr, istr [5] ;

	va_start(ap, format);
	
	/* printf ("__psf_sprintf : %s\n", format) ; */
	
	while ((c = *format++))
	{	if (c != '%')
		{	__psf_putchar (psf, c) ;
			continue ;
			} ;
		
		switch((c = *format++)) 
		{	case 's': /* string */
					strptr = va_arg (ap, char *) ;
					while (*strptr)
						__psf_putchar (psf, *strptr++) ;
					break;
		    
			case 'd': /* int */
					d = va_arg (ap, int) ;

					if (d == 0)
					{	__psf_putchar (psf, '0') ;
						break ;
						} 
					if (d < 0)
					{	__psf_putchar (psf, '-') ;
						d = -d ;
						} ;
					tens = 1 ;
					while (d / tens >= 10) 
						tens *= 10 ;
					while (tens > 0)
					{	__psf_putchar (psf, '0' + d / tens) ;
						d %= tens ;
						tens /= 10 ;
						} ;
					break;
					
			case 'X': /* hex */
					d = va_arg (ap, int) ;
					
					if (d == 0)
					{	__psf_putchar (psf, '0') ;
						break ;
						} ;
					shift = 28 ;
					while (! ((0xF << shift) & d))
						shift -= 4 ;
					while (shift >= 0)
					{	c = (d >> shift) & 0xF ;
						__psf_putchar (psf, (c > 9) ? c + 'A' - 10 : c + '0') ;
						shift -= 4 ;
						} ;
					break;
					
			case 'c': /* char */
					c = va_arg (ap, char) ;
					__psf_putchar (psf, c);
					break;
					
			case 'D': /* int2str */
					d = va_arg (ap, int);
					*((int*) istr) = d ;
					istr [4] = 0 ;
					strptr = &(istr [0]) ;
					while (*strptr)
					{	c = *strptr++ ;
						__psf_putchar (psf, c) ;
						} ;
					break;
					
			default :
					__psf_putchar (psf, '?') ;
					__psf_putchar (psf, c) ;
					__psf_putchar (psf, '?') ;
					break ;
			} /* switch */
		} /* while */

	va_end(ap);
	return ;
} /* __psf_sprintf */

/*-----------------------------------------------------------------------------------------------
 */
