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

#ifndef INITGUID
#define INITGUID
#endif          

#include "fluidreg.h"
#include "fluidnotif.h"

#include "dversion.h"
#include "xclassver.h"

BEGIN_DEFINE_CLASS_INTERFACE(FLUIDReg, IMoaRegister) 
END_DEFINE_CLASS_INTERFACE

/* msgtable : Microsoft Visual C has a limit of 2048 characters for a static string.
If you reach this limit, you must store your msgTable in a resource. Your registration
code must then load the resource, put its content into the registry, then release the resource */

const static char msgTable1[] = { 
  "xtra fluidsynth\n"
  "-- Version " VER_VERSION_STRING "\n"
  "new object me, * -- create a new synthesizer. Optional plist argument defines settings\n"
  "free object me -- explicitely frees the synth and cleans up all data structures\n"
  "reset object me -- resets the synthsizer. Turns all notes off and resets all controls\n"
  "getSetting object me, string setting -- Returns the value of the given setting\n"
  "+getSetting object xtra, string setting -- Called on the Xtra object. Returns the default value for this setting\n"
  "+getSettingsOptions object xtra, string setting -- Called on the Xtra object. Returns a comma separated string of the list of possible default options for this setting\n"
  "+getSettingsOptionsList object xtra, string setting -- Called on the Xtra object. Returns the list of possible default options for this setting\n"
  "getChannelsCount object me\n"
  "getMasterGain object me\n"
  "setMasterGain object me, float gain\n"
  "-- \n"
  "-- SoundFonts are stacked, and have a unique stackID. \n"
  "-- When omitted in a function call, stackID defaults to the last loaded SoundFont\n"
  "-- \n"
  "loadSoundFont object me, string filepathOrName -- load a soundfont\n"
  "createSoundFont object me, * -- creates an empty memory soundfont\n"
  "unloadSoundFont object me, * -- unloads the soundfont, arg stackID is optional\n" 
  "reloadSoundFont object me, * -- reloads the soundfont, arg stackID is optional\n"
  "getSoundFontsStack object me -- returns list of current stackIDs\n"
  "getSoundFontInfo object me, * -- returns info about SoundFont, arg stackID is optional\n"
  "loadSampleFile object me, string filepath, int soundFontID, * -- loads a sample from a file into a memory soundfont\n"
  "loadSampleFileExtract object me, string filepath, int startSample, int nbSamples, int soundFontID, * -- loads a sample from an extract of a file into a memory soundfont\n"
  "getSampleFileSamples object me, string filepath -- returns the number of samples in a file\n"
  "loadSampleMember object me, * -- loads a member into a memory soundfont\n"
  "deleteSample object me, int sampleID -- deletes the sample\n"
  "getSampleName object me, int sampleID -- returns the name of the sample\n"
  "getRootKey object me, int sampleID -- returns the rootkey of the sample\n"
  "getKeyRange object me, int sampleID -- returns the keyrange of the sample\n"
  "getFrameCount object me, int sampleID -- returns the number of frames of the sample\n"
  "getFrameRate object me, int sampleID -- returns the framerate of the sample\n"
  "setLoop object me, int sampleID, int isLooped -- specifies if the sample should be looped\n"
  "getLoop object me, int sampleID -- is the sample looped\n"
  "setLoopPoints object me, int sampleID, int firstframe, int lastframe -- sets the loop points\n"
  "getLoopPoints object me, int sampleID -- gets the loop points list\n"
};
const static char msgTable2[] = { 
    "setEnvelope object me, int sampleID, * -- sets the envelope\n"
    "getEnvelope object me, int sampleID -- gets the envelope list\n"
  "-- Reverb/chorus\n"
  "setReverb object me, int\n"
  "getReverb object me\n"
  "setReverbProp object me, symbol, *\n"
  "getReverbProp object me, symbol\n"
  "setChorus object me, int\n"
  "getChorus object me\n"
  "setChorusProp object me, symbol, *\n"
  "getChorusProp object me, symbol\n"
  "-- Sequencer\n"
  "getDestinations object me\n"
  "setTimeUnit object me, int ticksPerSecond\n"
  "getTimeUnit object me -- default = 60\n"
  "getTime object me -- in ticks since start\n"
  "scheduleCallback object me, any callbackInfo\n"
  "pollCallbacks object me -- \n"
  "removeEvents object me, * -- \n"
  "-- Events\n"
  "noteon object me, int chan, int key, *\n"
  "noteoff object me, int chan, int key, *\n"
  "note object me, int chan, int key, * -- dur is in ticks\n"
  "allNotesOff object me, int channel, * -- sends a noteoff to all currently playing notes\n"
  "allSoundsOff object me, int channel, * -- instantly stops sound on the channel\n"
  "programChange object me, int chan, any program, * -- program = 0..127 or [#number:0..127, #bank:0..16383]\n"
  "getProgram object me, int chan -- returns [#name:name, #number:0..127, #bank:0..16383]\n"
  "controlChange object me, int chan, any ctrlParams, * -- ctrlParams = plist of ctrlSymbol/value. Optional seq.\n"
  "getControl object me, int channel, any ctrlSymbol -- returns the current value\n"
  "getControls object me, int channel -- returns the current values of all controls\n"
};
const static char msgTable3[] = { 
    "setGenerator object me, int channel, int generator, *\n"
    "getGenerator object me, int channel, int generator\n"
  "-- Recording\n"
  "startRecord object me, string filepath -- starts recording to filePath\n"
  "stopRecord object me -- stops recording\n"
  "isRecording object me -- returns true if recording\n"
  "-- Utils\n"
  "getError object me -- returns a human-readable error string\n"
  "+getError object me -- called on the Xtra object. Returns a human-readable error string\n"
  "debug object me, * -- print debugging information to file\n"
  "getCPULoad object me -- returns the CPU load (%)\n" 
  "downloadFolderSetPath object me, string localAbsolutePath -- \n" 
  "downloadFolderContainsLocal object me, string localName -- \n"
  "downloadFolderGetLocalProps object me, string localName -- \n"
  "downloadFolderDeleteLocal object me, string localName -- \n" 
  "downloadFolderStartDownload object me, string url, string localName -- \n"
  "downloadFolderAbortDownload object me -- \n"
  "downloadFolderGetDownloadRatio object me -- 1 is downloaded, 0 is downloading, < 0 is error \n" 
};

STDMETHODIMP 
MoaCreate_FLUIDReg(FLUIDReg FAR * This)
{
  MoaError err = kMoaErr_NoErr;
  return(err);
}

STDMETHODIMP_(void) 
MoaDestroy_FLUIDReg(FLUIDReg FAR * This)
{
  return;
}

FLUIDReg_IMoaRegister::FLUIDReg_IMoaRegister(MoaError FAR * pErr)
{ 
  *pErr = (kMoaErr_NoErr); 
}

FLUIDReg_IMoaRegister::~FLUIDReg_IMoaRegister() 
{
}

STDMETHODIMP_(MoaError) 
FLUIDReg_IMoaRegister::Register(PIMoaCache pCache, PIMoaDict pXtraDict)
{
  /* variable declaration */
  MoaError err = kMoaErr_NoErr;
  PIMoaDict pRegDict;
	PMoaVoid		pMemStr = NULL;

	/* Register the notification class */
	err = pCache->AddRegistryEntry(pXtraDict, &CLSID(FLUIDNotif), &IID_IMoaNotificationClient, &pRegDict);

  /* Register the scripting xtra */
  err = pCache->AddRegistryEntry(pXtraDict, &CLSID_FLUIDXtra, &IID_IMoaMmXScript, &pRegDict);
			
  {

	/* Mark this Xtra "Safe for Shockwave" */
	MoaBool bItsSafe = TRUE;
	err = pRegDict->Put(kMoaMmDictType_SafeForShockwave,
	  &bItsSafe, 	sizeof( bItsSafe ), kMoaMmDictKey_SafeForShockwave);
  }


	/* Register the method table */
	/* msgTable is too long, have to concatenate it */
	if (!(pMemStr = pObj->pCalloc->NRAlloc(sizeof(msgTable1) + sizeof(msgTable2) + sizeof(msgTable3)))) 
	{
		MOA_DEBUGSTR("calloc failed!\n");
		err = kMoaErr_OutOfMem;
		goto exit_gracefully;
	}

	strcpy((char *)pMemStr, msgTable1);
	strcat((char *)pMemStr, msgTable2);
	strcat((char *)pMemStr, msgTable3);
	
	err = pRegDict->Put(kMoaDrDictType_MessageTable, pMemStr, 0, kMoaDrDictKey_MessageTable);

exit_gracefully:

	if(pMemStr)
	{
		pObj->pCalloc->NRFree(pMemStr);
		pMemStr = NULL;
	}

	return(err);
}

