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


int		__alaw_read_alaw2s (SF_PRIVATE *psf, short *ptr, unsigned int len) ;
int		__alaw_read_alaw2i (SF_PRIVATE *psf, int *ptr, unsigned int len) ; /* Antoine Schmitt 27.11.99 */
int		__alaw_read_alaw2d (SF_PRIVATE *psf, double *ptr, unsigned int len, int normalize) ; /* Antoine Schmitt 27.11.99 */

int		__alaw_write_s2alaw (SF_PRIVATE *psf, short *ptr, unsigned int len) ;
int		__alaw_write_i2alaw (SF_PRIVATE *psf, int *ptr, unsigned int len) ; /* Antoine Schmitt 27.11.99 */
int		__alaw_write_d2alaw (SF_PRIVATE *psf, double *ptr, unsigned int len, int normalize) ; /* Antoine Schmitt 27.11.99 */

