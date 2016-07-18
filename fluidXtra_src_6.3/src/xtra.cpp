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

#include "fluidxtra.h"
#include "fluidreg.h"
#include "fluidnotif.h"

#include "dversion.h"
#include "xclassver.h"


BEGIN_XTRA
	BEGIN_XTRA_DEFINES_CLASS(FLUIDReg, XTRA_CLASS_VERSION)
		CLASS_DEFINES_INTERFACE(FLUIDReg, IMoaRegister, XTRA_CLASS_VERSION)
	END_XTRA_DEFINES_CLASS
	
	BEGIN_XTRA_DEFINES_CLASS(FLUIDXtra, XTRA_CLASS_VERSION)
		CLASS_DEFINES_INTERFACE(FLUIDXtra, IMoaMmXScript, XTRA_CLASS_VERSION)
	END_XTRA_DEFINES_CLASS

	BEGIN_XTRA_DEFINES_CLASS(FLUIDNotif, XTRA_CLASS_VERSION)
		CLASS_DEFINES_INTERFACE(FLUIDNotif, IMoaNotificationClient, XTRA_CLASS_VERSION)	
	END_XTRA_DEFINES_CLASS
END_XTRA
