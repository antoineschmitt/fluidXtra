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

//****************************
// Notification class : FLUIDNotif
//****************************

#ifndef _FLUIDNOTIF_H
#define _FLUIDNOTIF_H

#include "moaxtra.h"
#include "mmixscrp.h"
#include "moafile2.h"
#include "moastr2.h"
#include "driservc.h"

#include <stdio.h>

#ifdef MACINTOSH
#include <windows.h>
#include <string.h>
#endif

#ifdef WINDOWS
#define snprintf _snprintf
#endif

DEFINE_GUID(CLSID(FLUIDNotif), 0x37468802L, 0xAD40, 0x11D6, 0x92, 0x37, 0x00, 0x50, 0xE4, 0xCE, 0xC9, 0x7C);

EXTERN_BEGIN_DEFINE_CLASS_INSTANCE_VARS(FLUIDNotif)
EXTERN_END_DEFINE_CLASS_INSTANCE_VARS

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(FLUIDNotif, IMoaNotificationClient)
	EXTERN_DEFINE_METHOD(MoaError, Notify, (ConstPMoaNotifyID, PMoaVoid, PMoaVoid))
EXTERN_END_DEFINE_CLASS_INTERFACE

#endif /* _FLUIDNOTIF_H */


