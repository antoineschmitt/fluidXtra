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

#ifndef _FLUIDXTRA_H
#define _FLUIDXTRA_H

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

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

#include "fluidsynth.h"
#include "bytesfifo.h"
#include "sndfile.h"

DEFINE_GUID(CLSID(FLUIDXtra), 0x1184022AL, 0x5EE8, 0x11D7, 0xA4, 0x5E, 0x00, 0x50, 0xE4, 0xCE, 0xC9, 0x7C);

/* for the srcs of sequencer events */
typedef struct _fluid_xtra_srcname fluid_xtra_srcname;
struct _fluid_xtra_srcname {
	fluid_xtra_srcname* next;
	short id;
	char * name;
};

/* for the sound font stack */
typedef struct _fluid_xtra_stack_item fluid_xtra_stack_item;
struct _fluid_xtra_stack_item {
	fluid_xtra_stack_item* next;
	int sfID;
	int type;
};

/* Sound font types */
enum {
	fluid_xtra_soundfont_type_file = 0,
	fluid_xtra_soundfont_type_ram
};

/* SoundID */
typedef struct _fluid_xtra_sample_t fluid_xtra_sample_t;
struct _fluid_xtra_sample_t {
	unsigned int sampleID;
	fluid_sample_t *sample;
	fluid_sample_t *sample2;
	fluid_ramsfont_t *sfont;
	unsigned int bank;
	unsigned int num;
	int isLooped;
	float loopstart;
	float loopend;
	float delay;
	float attack;
	float hold;
	float decay;
	float sustainLevel;
	float release;
	int keylo;
	int keyhi;
	
	fluid_xtra_sample_t * next;
};

EXTERN_BEGIN_DEFINE_CLASS_INSTANCE_VARS(FLUIDXtra)
     PIMoaMmValue pMmValue;
     PIMoaMmUtils2 pMmUtils2;
     PIMoaMmList pMmList;
	 PIMoaDrPlayer pDrPlayer;
	 PIMoaNotification pNotification;
	 PIMoaNotificationClient pNotificationClient;
	 PIMoaDrMovieContext pIDrMovieContext;
	 bool inited;
	 fluid_synth_t* synth;
	 fluid_audio_driver_t* adriver;
	 fluid_sequencer_t* sequencer;
	 int reverbOn;
	 int chorusOn;
	 short synthSeqID;
	 short xtraSeqID;
	 int debug;
	 char	*debugBuf;
	 int debugSize;
	 char *debugPtr;
	 char logFile[1024];
	 MoaMmValue	errorString;
	 fluid_xtra_stack_item* soundFontStack; // was soundFontIDs
	 fluid_xtra_srcname* srcNames;
	 fluid_xtra_sample_t* ramsamples;
	 unsigned int curSampleID;
	 MoaMmValue curCallbacks;
	 MoaLong curCallbackID;
	 MoaMmSymbol	*dones;
	 int	maxDones;
	 int	curDones;
	 bool	doneBusy;
	PIMoaStream2				pStream2;
	PIMoaHandle					pHandle;
	char pDownloadPath[1024];
	char pDownloadLocalname[1024];
	int pDownloadError;
	float pDownloadPercent;
	bool	isRecordingP;
	bytesfifo *			pFifo;
	SNDFILE *			pRecFile;
	int					pRecError;
	GThread *			pWritingThread;
#ifndef _WIN32
	short *				pOutBufShort;
#endif
	short *				pWriteBufShort;
	int					pWriteBufFrameCount;
EXTERN_END_DEFINE_CLASS_INSTANCE_VARS


EXTERN_BEGIN_DEFINE_CLASS_INTERFACE(FLUIDXtra, IMoaMmXScript)
     EXTERN_DEFINE_METHOD(MoaError, Call, (PMoaMmCallInfo))
     EXTERN_DEFINE_METHOD(void, sequencerCallback, (unsigned int, fluid_event_t*, fluid_sequencer_t*))
	 EXTERN_DEFINE_METHOD(void, _recordWriteThread, ())
#ifdef _WIN32
	EXTERN_DEFINE_METHOD(void, _dsound_cb, (int, short*))
#else
	EXTERN_DEFINE_METHOD(int, _coreaudio_cb, (int , int , float** , int , float** ))
#endif
  private:
     EXTERN_DEFINE_METHOD(MoaError, free, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, mynew, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getMasterGain, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setMasterGain, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, loadSoundFont, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(int, createSoundFont, (PMoaDrCallInfo, bool))
     EXTERN_DEFINE_METHOD(MoaError, unloadSoundFont, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, reloadSoundFont, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getSoundFontsStack, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getSoundFontInfo, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, loadSampleFile, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, loadSampleFileExtract, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getSampleFileSamples, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, loadSampleMember, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, deleteSample, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getSampleName, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getRootKey, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getKeyRange, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getTimeUnit, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setTimeUnit, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getTime, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, note, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, noteon, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, noteoff, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, reset, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, allNotesOff, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, allSoundsOff, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, programChange, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getProgram, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, controlChange, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getDestinations, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, scheduleCallback, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, pollCallbacks, (PMoaDrCallInfo))   
     EXTERN_DEFINE_METHOD(MoaError, removeEvents, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setReverb, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setReverbProp, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getReverb, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getReverbProp, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setChorus, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setChorusProp, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getChorus, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getChorusProp, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getChannelsCount, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getSetting, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getSettingsOptions, (PMoaDrCallInfo))
     
     EXTERN_DEFINE_METHOD(MoaError, getFrameCount, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getFrameRate, (PMoaDrCallInfo))   
     EXTERN_DEFINE_METHOD(MoaError, setLoop, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getLoop, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setLoopPoints, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getLoopPoints, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setEnvelope, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getEnvelope, (PMoaDrCallInfo))

	EXTERN_DEFINE_METHOD(MoaError, startRecord, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, stopRecord, (PMoaDrCallInfo))
	EXTERN_DEFINE_METHOD(MoaError, isRecording, (PMoaDrCallInfo))

     EXTERN_DEFINE_METHOD(void, _Alert, (char *))
     EXTERN_DEFINE_METHOD(void, _setCallbackDone, (MoaMmSymbol))
     EXTERN_DEFINE_METHOD(void, _ensureEnoughDones, ())
     EXTERN_DEFINE_METHOD(void, _removeCallbacks, (short))
     EXTERN_DEFINE_METHOD(void, _forgetCallback, (PMoaDrCallInfo, MoaMmSymbol))
     EXTERN_DEFINE_METHOD(void, _storeCallback, (PMoaDrCallInfo, PMoaMmValue, PMoaMmSymbol))
     EXTERN_DEFINE_METHOD(int, _getControlValue, (PMoaDrCallInfo, PMoaMmValue, int, PMoaMmValue))
     EXTERN_DEFINE_METHOD(int, _parseSeqPlist, (PMoaDrCallInfo, MoaMmValue, unsigned int *, int *, short*, short*, int))
     EXTERN_DEFINE_METHOD(bool, _isInShockwave, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(int, _getIntArg, (PMoaDrCallInfo, int, const char *, MoaLong *, int))
     EXTERN_DEFINE_METHOD(int, _getSymbolArg, (PMoaDrCallInfo, int, const char *, MoaMmSymbol *, int))
     EXTERN_DEFINE_METHOD(int, _getStringArg, (PMoaDrCallInfo, int, const char *, char *, int, int))
     EXTERN_DEFINE_METHOD(int, _getAnsiStringArg, (PMoaDrCallInfo, int, const char *, char *, int, int))
     EXTERN_DEFINE_METHOD(int, _getFloatArg, (PMoaDrCallInfo, int, const char *, MoaDouble *, int))
     EXTERN_DEFINE_METHOD(int, _getPlistArg, (PMoaDrCallInfo, int, const char *, PMoaMmValue, int))
     EXTERN_DEFINE_METHOD(int, _getIntProp, (PMoaDrCallInfo, MoaMmValue, const char *, const char *, MoaLong *, int))
     EXTERN_DEFINE_METHOD(int, _getFloatProp, (PMoaDrCallInfo, MoaMmValue, const char *, const char *, MoaDouble *, int))
     EXTERN_DEFINE_METHOD(int, _getStringProp, (PMoaDrCallInfo, MoaMmValue, const char *, const char *, char *, int, int))
     EXTERN_DEFINE_METHOD(int, _getAnsiStringProp, (PMoaDrCallInfo, MoaMmValue, const char *, const char *, char *, int, int))
     EXTERN_DEFINE_METHOD(int, _getPropAt, (PMoaDrCallInfo, PMoaMmValue, int, PMoaMmValue))
     EXTERN_DEFINE_METHOD(void, _deleteRamSample, (unsigned int))   
     EXTERN_DEFINE_METHOD(void, _deleteRamSamplesForRamsfont, (fluid_ramsfont_t*))   
     EXTERN_DEFINE_METHOD(fluid_xtra_sample_t*, _getRamSample, (unsigned int))   
     EXTERN_DEFINE_METHOD(short, _getSourceIdForName, (char *, bool))
     EXTERN_DEFINE_METHOD(MoaError, _addSample, (PMoaDrCallInfo, int, short*, short*, short*, long, int, int))
     EXTERN_DEFINE_METHOD(MoaError, _loadSampleFile, (PMoaDrCallInfo, int, long, long))
     EXTERN_DEFINE_METHOD(MoaError, getControl, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getControls, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, setGenerator, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getGenerator, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, debug, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, getCPULoad, (PMoaDrCallInfo))  
	 EXTERN_DEFINE_METHOD(MoaError, downloadFolderContainsLocal, (PMoaDrCallInfo))
	 EXTERN_DEFINE_METHOD(MoaError, downloadFolderGetLocalProps, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, downloadFolderSetPath, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, downloadFolderDeleteLocal, (PMoaDrCallInfo))  
     EXTERN_DEFINE_METHOD(MoaError, downloadFolderStartDownload, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, downloadFolderAbortDownload, (PMoaDrCallInfo))
     EXTERN_DEFINE_METHOD(MoaError, downloadFolderGetDownloadRatio, (PMoaDrCallInfo))  
      
     EXTERN_DEFINE_METHOD(int, _checkValueType, (PMoaMmValue, MoaMmValueType))
EXTERN_END_DEFINE_CLASS_INTERFACE

enum {
  m_new = 0,
  m_free,
  m_reset,
  m_getSetting,
  m_getSettingsOptions,
  m_getSettingDefaultValue,
  m_getChannelsCount,
  m_getMasterGain,
  m_setMasterGain,
  m_loadSoundFont,
  m_createSoundFont,
  m_unloadSoundFont,
  m_reloadSoundFont,
  m_getSoundFontsStack,
  m_getSoundFontInfo,
  m_loadSampleFile,
  m_loadSampleFileExtract,
  m_getSampleFileSamples,
  m_loadSampleMember,
  m_deleteSample,
  m_getSampleName,
  m_getRootKey,
  m_getKeyRange,
  m_getFrameCount,
  m_getFrameRate,
  m_setLoop,
  m_getLoop,
  m_setLoopPoints,
  m_getLoopPoints,
  m_setEnvelope,
  m_getEnvelope,
  m_setReverb,
  m_getReverb,
  m_setReverbProp,
  m_getReverbProp,
  m_setChorus,
  m_getChorus,
  m_setChorusProp,
  m_getChorusProp,
  m_getDestinations,
  m_setTimeUnit,
  m_getTimeUnit,
  m_getTime,
  m_scheduleCallback,
  m_pollCallbacks,
  m_removeEvents,
  m_noteOn,
  m_noteOff,
  m_note,
  m_allNotesOff,
  m_allSoundsOff,
  m_programChange,
  m_getProgram,
  m_controlChange,
  m_getControl,
  m_getControls,
  m_setGenerator,
  m_getGenerator,
  m_startRecord,
  m_stopRecord,
  m_isRecording,
  m_getError,
  m_debug,
  m_getCPULoad,
  m_downloadFolderSetPath,
  m_downloadFolderContainsLocal,
  m_downloadFolderGetLocalProps,
  m_downloadFolderDeleteLocal,
  m_downloadFolderStartDownload,
  m_downloadFolderAbortDownload,
  m_downloadFolderGetDownloadRatio
};

// Prototypes
void poll(FLUIDXtra FAR * pObj);
void checkSFDownload(FLUIDXtra FAR * pObj);
void _fluid_xtra_setErrorString(FLUIDXtra FAR * pObj, const char * aString);
void _fluid_xtra_getErrorString(FLUIDXtra FAR * pObj, PMoaMmCallInfo callPtr);

typedef enum {
    FLUIDXTRAERR_NOSYNTH = -13500,
    FLUIDXTRAERR_NOMEMORY,
    FLUIDXTRAERR_BADARGUMENT,
    FLUIDXTRAERR_FLUIDERR,
    FLUIDXTRAERR_OPENWRITEFILE,
    FLUIDXTRAERR_WRITEFILE,
    FLUIDXTRAERR_BADSTACKID,
    FLUIDXTRAERR_LOADINGFILE,
    FLUIDXTRAERR_NYI,
    FLUIDXTRAERR_NOPRESET,
    FLUIDXTRAERR_UNKNOWNCONTROL,
    FLUIDXTRAERR_UNKNOWNDESTINATION,
    FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND,
    FLUIDXTRAERR_FORBIDDENINSHOCKWAVE,
    FLUIDXTRAERR_NOTRECORDING,
    FLUIDXTRAERR_ALREADYRECORDING,
    FLUIDXTRAERR_OPENREADFILE,
    FLUIDXTRAERR_READFILE,
    FLUIDXTRAERR_BADFILEFORMAT,
    FLUIDXTRAERR_SEEKFILE,
    
    FLUIDXTRAERR_STDERROR
} fluid_xtra_errornum;


#endif /* _FLUIDXTRA_H */
