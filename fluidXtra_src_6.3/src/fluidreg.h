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

#ifndef _FLUIDREG_H
#define _FLUIDREG_H

#include "moaxtra.h"
#include "fluidxtra.h"

DEFINE_GUID(CLSID(FLUIDReg), 0x2146E290L, 0x5EE8, 0x11D7, 0xA4, 0x5E, 0x00, 0x50, 0xE4, 0xCE, 0xC9, 0x7C);

/**************************************************
 *
 *   FLUIDReg
 */
EXTERN_BEGIN_DEFINE_CLASS_INSTANCE_VARS(FLUIDReg) 
EXTERN_END_DEFINE_CLASS_INSTANCE_VARS

EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(FLUIDReg, IMoaRegister) 
EXTERN_DEFINE_METHOD(MoaError, Register, (PIMoaCache, PIMoaDict))
EXTERN_END_DEFINE_CLASS_INTERFACE

#endif /* _FLUIDREG_H */

