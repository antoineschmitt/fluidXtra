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
#include "fluidnotif.h"

/**********************************************/
/************** Notification  *****************/
/**********************************************/

BEGIN_DEFINE_CLASS_INTERFACE(FLUIDNotif, IMoaNotificationClient)
END_DEFINE_CLASS_INTERFACE

/* interface creation/destruction */
FLUIDNotif_IMoaNotificationClient::FLUIDNotif_IMoaNotificationClient(MoaError FAR * pErr)
	{ *pErr = (kMoaErr_NoErr); }
FLUIDNotif_IMoaNotificationClient::~FLUIDNotif_IMoaNotificationClient() {}

/* instance creation/destruction */
STDMETHODIMP 
MoaCreate_FLUIDNotif(FLUIDNotif FAR * This)
{ MoaError err = kMoaErr_NoErr; return(err); }

STDMETHODIMP_(void) 
MoaDestroy_FLUIDNotif(FLUIDNotif FAR * This)
{ return; }

MoaError
FLUIDNotif_IMoaNotificationClient::Notify(ConstPMoaNotifyID nid, PMoaVoid pNData, PMoaVoid pRefCon)
{
	FLUIDXtra FAR * xtraObj = (FLUIDXtra FAR *)pRefCon;
	if (!xtraObj) return(kMoaErr_NoErr);
	
	poll(xtraObj);
	
	return(kMoaErr_NoErr);
}

