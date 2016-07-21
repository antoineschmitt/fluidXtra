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
 
/***************************************************************
 *
 *                           Xtra
 *
 ***************************************************************/

#include "fluidxtra.h"
#include "fluidnotif.h"
#include "fluidimportfile.h"

#include <Math.h>
#include <stdlib.h>

#include "supportFolder.h"

#ifdef _WINDOWS
	#include <dsound.h> 
#else // macintosh
	#include <Sound.h>
#endif

extern MoaFileRef gXtraFileRef;

#ifdef WIN32
#define log2(x) (log(x)/log(2.0))
#endif

void _myFree(FLUIDXtra FAR * pObj);
void _stopRecord(FLUIDXtra FAR * pObj, bool fromWritingThread);

#ifdef _WIN32
extern "C" int fluid_dsound_cb(void* data, int len, short* out);
#endif
int fluid_coreaudio_cb(void* data, int len,
				 int nin, float** in,
				 int nout, float** out);


/**********************************************/
/************** FLUIDXtra  *****************/
/**********************************************/
BEGIN_DEFINE_CLASS_INTERFACE(FLUIDXtra, IMoaMmXScript)
END_DEFINE_CLASS_INTERFACE


/**************************************/
/************** Log   *****************/
/**************************************/

/* For error management in logfunctions */
extern "C"
void
fluid_xtra_log(int level, char* message, void* userData)
{
	if (userData == NULL) return;
	FLUIDXtra * pObj = (FLUIDXtra *)userData;

	if (level == FLUID_ERR)
		_fluid_xtra_setErrorString(pObj, message);
	
	if (!pObj->debug)
		return;

	if (pObj->debugBuf == NULL) {		
		/* Log to file */
	  FILE* log = fopen(pObj->logFile, "a");
	  if (log == NULL) return;
	  fprintf(log, "%s\n", message);
	  fflush(log);
	  fclose(log);
	  /* end log to file */
	  return;
	}
	
	// log to buffer
	int len = strlen(message);
	if (pObj->debugPtr + len > pObj->debugBuf + pObj->debugSize) {
		// buffer full : dump buffer
		if (pObj->debugPtr > pObj->debugBuf) {
				
			
			/* Log to file */
			/***
			pObj->debugPtr[0] = 0;
		  FILE* log = fopen(pObj->logFile, "a");
		  if (log == NULL) return;
		  fprintf(log, "%s", pObj->debugBuf);
		  fflush(log);
		  fclose(log);
		  ****/
		  /* end log to file */
		  
		  pObj->debugPtr = pObj->debugBuf;
		} // buffer dumped
		
		if (len >= pObj->debugSize) {
			// still too large : dump message now and return
			/* Log to file */
		  FILE* log = fopen(pObj->logFile, "a");
		  if (log == NULL) return;
		  fprintf(log, "%s\n", message);
		  fflush(log);
		  fclose(log);
		  /* end log to file */
			return;
		}
	}
	
	// write to buffer
	strcpy(pObj->debugPtr, message);
	pObj->debugPtr += len;
	pObj->debugPtr[0] = '\n';
	pObj->debugPtr++;
}


// ****************************************************
// UNICODE
// ****************************************************
int getDirectorVersion(FLUIDXtra FAR * pObj);
void _ansiStringToValue(FLUIDXtra FAR * pObj, const char *instr, PMoaMmValue pVal);
void _stringToValue(FLUIDXtra FAR * pObj, const char *instr, PMoaMmValue pVal);
void _valueToAnsiString(FLUIDXtra FAR * pObj, PMoaMmValue pVal, char *outstr, int buflen);
void _valueToString(FLUIDXtra FAR * pObj, PMoaMmValue pVal, char *outstr, int buflen);

static int gDirectorVersion = 0;
int getDirectorVersion(FLUIDXtra FAR * pObj) {
	// cached ?
	if (gDirectorVersion != 0) return gDirectorVersion;

	gDirectorVersion = -1;

	PIMoaAppInfo	pAppInfo;
	MoaChar			buf[64];
	int				err;

	if (err = pObj->pCallback->QueryInterface(&IID_IMoaAppInfo, (PPMoaVoid)&pAppInfo))
		goto exit_gracefully;

	if (err = pAppInfo->GetInfo(kMoaAppInfo_ProductVersion, buf, 64))
		goto exit_gracefully;

	if (buf[1] == '.') {
		buf[1] = 0;
		gDirectorVersion = atoi(buf);
	} else if (buf[2] == '.') {
		buf[2] = 0;
		gDirectorVersion = atoi(buf);
	}

exit_gracefully:

	if (pAppInfo) {
		pAppInfo->Release();
		pAppInfo = NULL;
	}

	return gDirectorVersion;
}

void _stringToValue(FLUIDXtra FAR * pObj, const char *instr, PMoaMmValue pVal) {
	pObj->pMmValue->StringToValue(instr, pVal);
}

void
_ansiStringToValue(FLUIDXtra FAR * pObj, const char *instr, PMoaMmValue pVal) {
	if (getDirectorVersion(pObj) >= 11) {
		// convert ansi to UTF8
		int inlen = strlen(instr);
		int utf8bytescount = inlen*2+1;
		char *utf8str = (char *)malloc(utf8bytescount);
		#ifdef _WINDOWS
			LPWSTR lpszW = new WCHAR[inlen + 1]; //new widechar string
			MultiByteToWideChar(CP_ACP, 0, instr, -1, lpszW, inlen); // ansi to W
			lpszW[inlen] = 0;
			WideCharToMultiByte(CP_UTF8, 0, lpszW, -1, utf8str, utf8bytescount, NULL, NULL); // W to UTF8
			pObj->pMmValue->StringToValue(utf8str, pVal);
			delete[] lpszW;
		#else
			CFStringRef str = CFStringCreateWithCString(NULL, instr, CFStringGetSystemEncoding());
			CFStringGetCString(str, utf8str, utf8bytescount, kCFStringEncodingUTF8);
			pObj->pMmValue->StringToValue(utf8str, pVal);
			CFRelease(str);
		#endif
		free(utf8str);
	} else {
		// pre D11
		pObj->pMmValue->StringToValue(instr, pVal);
	}
}

// Since fopen accepts UTF8, we use do not need to convert to ANSI
void _valueToString(FLUIDXtra FAR * pObj, PMoaMmValue pVal, char *outstr, int buflen) {
	pObj->pMmValue->ValueToString(pVal, outstr, buflen);
}

void _valueToAnsiString(FLUIDXtra FAR * pObj, PMoaMmValue pVal, char *outstr, int buflen) {
	if (getDirectorVersion(pObj) >= 11) {
		// convert UTF8 to ansi 
		MoaLong len;
		pObj->pMmValue->ValueStringLength(pVal, &len);
		int utf8strlen = len*2+1;
		char *utf8str = (char *)malloc(utf8strlen);
		pObj->pMmValue->ValueToString(pVal, utf8str, utf8strlen);

		#ifdef _WINDOWS
			// utf8 to W to ansi
			LPWSTR lpszW = new WCHAR[len + 1]; //new widechar string
			MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, lpszW, len);
			lpszW[len] = 0;
			WideCharToMultiByte(CP_ACP, 0, lpszW, -1, outstr, buflen, NULL, NULL); // W to ansi
			delete[] lpszW;
		#else
			CFStringRef str = CFStringCreateWithCString(NULL, utf8str, kCFStringEncodingUTF8);
			CFStringGetCString(str, outstr, buflen, CFStringGetSystemEncoding());
			CFRelease(str);
		#endif

		free(utf8str);
	} else {
		// pre D11
		pObj->pMmValue->ValueToString(pVal, outstr, buflen);
	}
}

/**************************************/
/************** Errors & Debug ********/
/**************************************/

char * _MoaNetErrorDesc(int errNb) {
	switch(errNb) {
		case kMoaFileErr_IoError : return (char *)"kMoaFileErr_IoError";
		case kMoaFileErr_BufferTooSmall : return (char *)"kMoaFileErr_BufferTooSmall";
		case kMoaFileErr_DuplicateSpec : return (char *)"kMoaFileErr_DuplicateSpec";
		case kMoaFileErr_DiskFull : return (char *)"kMoaFileErr_DiskFull";
		case kMoaFileErr_FileBusy : return (char *)"kMoaFileErr_FileBusy";
		case kMoaFileErr_BadFileSpec : return (char *)"kMoaFileErr_BadFileSpec";

		case kMoaErr_ShMemLockError : return (char *)"Sharedm memory: locked but shouldn't be; unlocked but should be";
		case kMoaErr_CacheIncompatible : return (char *)"Running at same time as an app with incompatible version of our cache";
		case kMoaErr_CacheDownloadStopped : return (char *)"Download: Transfer halted";
		case kMoaErr_CacheWrongState : return (char *)"Client made a cache call at a bad time";
		
		case kMoaStreamErr_DataNotAvail : return (char *)"Non-blocking read: data hasn't yet arrived";
		case kMoaStreamErr_OpNotDone : return (char *)"Op hasn't finished, so results not avail";
		case kMoaStreamErr_StreamNotOpen : return (char *)"Stream not open";
		case kMoaStreamErr_IoError : return (char *)"catch-all r/w/pos error";
		case kMoaStreamErr_StreamAlreadyOpen : return (char *)"kMoaStreamErr_StreamAlreadyOpen";
		case kMoaStreamErr_BadParameter : return (char *)"kMoaStreamErr_BadParameter";
		case kMoaStreamErr_ReadPastEnd : return (char *)"kMoaStreamErr_ReadPastEnd";
		case kMoaStreamErr_BadAccessMode : return (char *)"kMoaStreamErr_BadAccessMode";
		case kMoaStreamErr_BadSetPositionMode : return (char *)"kMoaStreamErr_BadSetPositionMode";
		case kMoaStreamErr_ReadAheadTooFar : return (char *)"kMoaStreamErr_ReadAheadTooFar";
		case kMoaStreamErr_WrotePastEnd : return (char *)"kMoaStreamErr_WrotePastEnd";
	}
	return (char *)"Unknown error";
}


void
_fluid_xtra_setErrorString(FLUIDXtra FAR * pObj, const char * aString) {
	if (!pObj->inited) return; // errorString no ready
	
	pObj->pMmValue->ValueRelease(&pObj->errorString);
	_stringToValue(pObj, aString, &pObj->errorString);	
}

void
_fluid_xtra_getErrorString(FLUIDXtra FAR * pObj, PMoaMmCallInfo callPtr) {
	pObj->pMmValue->ValueAddRef(&pObj->errorString);	
	callPtr->resultValue = pObj->errorString;
}

MoaError 
FLUIDXtra_IMoaMmXScript::debug(PMoaDrCallInfo callPtr)
{	
  MoaError myErr = kMoaErr_NoErr;
  MoaMmValue value;

  /* get the flags value */
  GetArgByIndex(2, &value);
  if (_checkValueType(&value, kMoaMmValueType_Void) || _checkValueType(&value, kMoaMmValueType_String)) {
  
  	// flush
  	if (pObj->debug) {  		
		  fluid_log(FLUID_DBG, "fluidsynth: debugging off");
	  
	  	if (pObj->debugBuf && (pObj->debugPtr > pObj->debugBuf)) {				
				pObj->debugPtr[0] = 0;
	  		/* Log to file */
			  FILE* log = fopen(pObj->logFile, "a");
			  if (log != NULL) {		  	
				  fprintf(log, "%s", pObj->debugBuf);
				  fflush(log);
				  fclose(log);
			  }
			  /* end log to file */
	  	}
			pObj->debug = 0;
			pObj->logFile[0] = 0;
  	}
  	
		if (_checkValueType(&value, kMoaMmValueType_String)) {

			_valueToString(pObj, &value, pObj->logFile, 1024);
			pathHFS2POSIX(pObj->logFile, 1024);

			/* open file */
			FILE* log;
			log = fopen(pObj->logFile, "w");
			if (log == NULL) {
				// fails to open
				pObj->debug = 0;
				// dont call fluid_log, it is not working yet...
				_fluid_xtra_setErrorString(pObj, (char *)"Cannot open file for writing");
				myErr = FLUIDXTRAERR_OPENWRITEFILE;

			} else {
				fprintf(log, "%s\n", "fluidsynth: debugging on");
				fflush(log);
				fclose(log);
				pObj->debug = 1;
			}
		}

	} else {
		// bad argument
  	fluid_log(FLUID_ERR, "logFile should be a string");
  	myErr = FLUIDXTRAERR_BADARGUMENT;
	}
	
	pObj->pMmValue->IntegerToValue(myErr, &callPtr->resultValue);
  return(kMoaErr_NoErr);
}

/**************************************/
/******* Utils ************************/
/**************************************/

bool FLUIDXtra_IMoaMmXScript::_isInShockwave(PMoaDrCallInfo callPtr) {
	MoaError err;
	
	bool inShock = true;
	PIMoaAppInfo				piAppInfo = NULL;
	pObj->pCallback->QueryInterface(&IID_IMoaAppInfo, (PPMoaVoid)&piAppInfo);

	char runmodeBuf[256];
	err = piAppInfo->GetInfo(kMoaAppInfo_RunMode, runmodeBuf, 255); 
	if (err == kMoaErr_NoErr)
		inShock = !strcmp(runmodeBuf, "Plugin");
	
	if (piAppInfo) piAppInfo->Release();
	piAppInfo = NULL;

	return inShock;
}

int
FLUIDXtra_IMoaMmXScript::_checkValueType(PMoaMmValue ptmpValue, MoaMmValueType checkType)
{
	  MoaMmValueType valueType;
		pObj->pMmValue->ValueType(ptmpValue, &valueType);
		return (valueType == checkType);
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getIntArg(PMoaDrCallInfo callPtr, int nbarg, const char * argName, MoaLong *presult, int voidIsError)
{
  MoaMmValue value;
  if (callPtr->nargs < nbarg) {
  	if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s is VOID", argName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	}
  	return -2;
  }
  
  GetArgByIndex(nbarg, &value);
  
	if (!_checkValueType(&value, kMoaMmValueType_Integer) && !_checkValueType(&value, kMoaMmValueType_Float)) {
  	fluid_log(FLUID_ERR, "%s should be a int", argName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return -1;
  }
  
  if (_checkValueType(&value, kMoaMmValueType_Float)) {
  	// convert to int
  	MoaDouble presultF;
  	pObj->pMmValue->ValueToFloat(&value, &presultF);
  	*presult = (MoaLong)presultF;
  } else {
	  pObj->pMmValue->ValueToInteger(&value, presult);
  }
  
  return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getStringArg(PMoaDrCallInfo callPtr, int nbarg, const char * argName, char* buffer, int buflen, int voidIsError)
{
  MoaMmValue value;
  if (callPtr->nargs < nbarg) {
  	if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s is VOID", argName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	}
  	return -2;
  }
  
  GetArgByIndex(nbarg, &value);
  
	if (!_checkValueType(&value, kMoaMmValueType_String)) {
  	fluid_log(FLUID_ERR, "%s should be a string", argName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return -1;
  }
  
	pObj->pMmValue->ValueToString(&value, buffer, buflen);
  
  return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getAnsiStringArg(PMoaDrCallInfo callPtr, int nbarg, const char * argName, char* buffer, int buflen, int voidIsError)
{
  MoaMmValue value;
  if (callPtr->nargs < nbarg) {
  	if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s is VOID", argName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	}
  	return -2;
  }
  
  GetArgByIndex(nbarg, &value);
  
	if (!_checkValueType(&value, kMoaMmValueType_String)) {
  	fluid_log(FLUID_ERR, "%s should be a string", argName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return -1;
  }
  
	_valueToString(pObj, &value, buffer, buflen);
  
  return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getFloatArg(PMoaDrCallInfo callPtr, int nbarg, const char * argName, MoaDouble *presult, int voidIsError)
{
  MoaMmValue value;
  if (callPtr->nargs < nbarg) {
  	if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s is VOID", argName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	}
  	return -2;
  }
  
  GetArgByIndex(nbarg, &value);
  
	if (!_checkValueType(&value, kMoaMmValueType_Float) && !_checkValueType(&value, kMoaMmValueType_Integer)) {
  	fluid_log(FLUID_ERR, "%s should be a float", argName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return -1;
  }
  
  if (_checkValueType(&value, kMoaMmValueType_Integer)) {
  	// convert to float
  	MoaLong presultI;
  	pObj->pMmValue->ValueToInteger(&value, &presultI);
  	*presult = presultI;
  } else {
	  pObj->pMmValue->ValueToFloat(&value, presult);
  }
  
  return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getSymbolArg(PMoaDrCallInfo callPtr, int nbarg, const char * argName, MoaMmSymbol *presult, int voidIsError)
{
  MoaMmValue value;
  if (callPtr->nargs < nbarg) {
  	if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s is VOID", argName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	}
  	return -2;
  }
  
  GetArgByIndex(nbarg, &value);
  
	if (!_checkValueType(&value, kMoaMmValueType_Symbol)) {
  	fluid_log(FLUID_ERR, "%s should be a symbol", argName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return -1;
  }
  pObj->pMmValue->ValueToSymbol(&value, presult);
  
  return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getPlistArg(PMoaDrCallInfo callPtr, int nbarg, const char * argName, PMoaMmValue presult, int voidIsError)
{
  if (callPtr->nargs < nbarg) {
  	if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s is VOID", argName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	}
  	return -2;
  }
  
  GetArgByIndex(nbarg, presult);
  
	if (!_checkValueType(presult, kMoaMmValueType_PropList)) {
  	fluid_log(FLUID_ERR, "%s should be a plist", argName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return -1;
  }
  
  return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getIntProp(PMoaDrCallInfo callPtr, MoaMmValue value, const char * propName, const char * publicName, MoaLong *presult, int voidIsError)
{
	MoaMmSymbol symbol;
	MoaMmValue symbolValue, intValue;
	MoaError err;

	pObj->pMmValue->StringToSymbol(propName, &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	err = pObj->pMmList->GetValueByProperty(&value, &symbolValue, &intValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	if (err != kMoaErr_NoErr) {
		if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s missing", publicName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	    return kMoaErr_NoErr;
		}
		return -2;
	}

	if (!_checkValueType(&intValue, kMoaMmValueType_Integer) && !_checkValueType(&intValue, kMoaMmValueType_Float)) {
	  pObj->pMmValue->ValueRelease(&intValue);
		fluid_log(FLUID_ERR, "%s should be a int", publicName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	  return -1;
	}

	if (_checkValueType(&intValue, kMoaMmValueType_Float)) {
		// convert to int
		MoaDouble presultF;
		pObj->pMmValue->ValueToFloat(&intValue, &presultF);
		*presult = (MoaLong)presultF;
	} else {
		pObj->pMmValue->ValueToInteger(&intValue, presult);
	}

	pObj->pMmValue->ValueRelease(&intValue);

	return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getFloatProp(PMoaDrCallInfo callPtr, MoaMmValue value, const char * propName, const char * publicName, MoaDouble *presult, int voidIsError)
{
	MoaMmSymbol symbol;
	MoaMmValue symbolValue, intValue;
	MoaError err;

	pObj->pMmValue->StringToSymbol(propName, &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	err = pObj->pMmList->GetValueByProperty(&value, &symbolValue, &intValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	if (err != kMoaErr_NoErr) {
		if (voidIsError) {
	  	fluid_log(FLUID_ERR, "%s missing", publicName);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	    return kMoaErr_NoErr;
		}
		return -2;
	}

	if (!_checkValueType(&intValue, kMoaMmValueType_Integer) && !_checkValueType(&intValue, kMoaMmValueType_Float)) {
	  pObj->pMmValue->ValueRelease(&intValue);
		fluid_log(FLUID_ERR, "%s should be a float", publicName);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	  return -1;
	}

	if (_checkValueType(&intValue, kMoaMmValueType_Integer)) {
		// convert to float
		MoaLong presultI;
		pObj->pMmValue->ValueToInteger(&intValue, &presultI);
		*presult = presultI;
	} else {
		pObj->pMmValue->ValueToFloat(&intValue, presult);
	}

	pObj->pMmValue->ValueRelease(&intValue);

	return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getStringProp(PMoaDrCallInfo callPtr, MoaMmValue value, const char * propName, const char * publicName, char* buffer, int buflen, int voidIsError)
{
		MoaMmSymbol symbol;
		MoaMmValue symbolValue, stringValue;
		MoaError err;
		
	  pObj->pMmValue->StringToSymbol(propName, &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	  err = pObj->pMmList->GetValueByProperty(&value, &symbolValue, &stringValue);
	  pObj->pMmValue->ValueRelease(&symbolValue);

	  if (err != kMoaErr_NoErr) {
	  	if (voidIsError) {
	    	if (callPtr) {
			  	fluid_log(FLUID_ERR, "%s missing", publicName);
					pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
				}
		    return -1;
	  	}
	  	return -2;
	  }
	  
	  if (!_checkValueType(&stringValue, kMoaMmValueType_String)) {
		  pObj->pMmValue->ValueRelease(&stringValue);
	    if (callPtr) {
		  	fluid_log(FLUID_ERR, "%s should be a string", publicName);
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
			}
	    return -1;
		}
		pObj->pMmValue->ValueToString(&stringValue, buffer, buflen);
		pObj->pMmValue->ValueRelease(&stringValue);
	
	return 0;
}

/* returns -1 if error, - 2 if VOID, 0 if ok */
int
FLUIDXtra_IMoaMmXScript::_getAnsiStringProp(PMoaDrCallInfo callPtr, MoaMmValue value, const char * propName, const char * publicName, char* buffer, int buflen, int voidIsError)
{
		MoaMmSymbol symbol;
		MoaMmValue symbolValue, stringValue;
		MoaError err;
		
	  pObj->pMmValue->StringToSymbol(propName, &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	  err = pObj->pMmList->GetValueByProperty(&value, &symbolValue, &stringValue);
	  pObj->pMmValue->ValueRelease(&symbolValue);

	  if (err != kMoaErr_NoErr) {
	  	if (voidIsError) {
	    	if (callPtr) {
			  	fluid_log(FLUID_ERR, "%s missing", publicName);
					pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
				}
		    return -1;
	  	}
	  	return -2;
	  }
	  
	  if (!_checkValueType(&stringValue, kMoaMmValueType_String)) {
		  pObj->pMmValue->ValueRelease(&stringValue);
	    if (callPtr) {
		  	fluid_log(FLUID_ERR, "%s should be a string", publicName);
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
			}
	    return -1;
		}
		_valueToString(pObj, &stringValue, buffer, buflen);
		pObj->pMmValue->ValueRelease(&stringValue);
	
	return 0;
}

int
FLUIDXtra_IMoaMmXScript::_getPropAt(PMoaDrCallInfo callPtr, PMoaMmValue ppListValue, int i, PMoaMmValue pRes)
{
	MoaMmValue indexV;
	pObj->pMmValue->IntegerToValue(i, &indexV);

	MoaMmSymbol getPropAtS = 0;
	pObj->pMmValue->StringToSymbol("GetPropAt", &getPropAtS);

	MoaMmValue args[2];
	args[0] = *ppListValue;
	args[1] = indexV;

	pObj->pDrPlayer->CallHandler(getPropAtS, 2, &args[0], pRes);

	pObj->pMmValue->ValueRelease(&indexV);
	return 0;
}


/*********************************************/
/***************** Alert *********************/
/*********************************************/

void
FLUIDXtra_IMoaMmXScript::_Alert(char * txt) {
	MoaMmSymbol		sym; 
	MoaMmValue		msgStr;
	pObj->pMmValue->StringToSymbol("alert", &sym);
	_stringToValue(pObj, txt, &msgStr);
	pObj->pDrPlayer->CallHandler(sym, 1, &msgStr, NULL);
	pObj->pMmValue->ValueRelease(&msgStr);
}


/* sequencer callback */
extern "C"
void fluid_xtra_seq_callback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* seq, void* data)
{
	FLUIDXtra_IMoaMmXScript* me = (FLUIDXtra_IMoaMmXScript *)data;
	me->sequencerCallback(time, event, seq);
}


/**************************************/
/******* Instance management **********/
/**************************************/

/* interface creation/destruction */
FLUIDXtra_IMoaMmXScript::FLUIDXtra_IMoaMmXScript(MoaError FAR * pErr)
{ *pErr = kMoaErr_NoErr; }
FLUIDXtra_IMoaMmXScript::~FLUIDXtra_IMoaMmXScript() {}


STDMETHODIMP MoaCreate_FLUIDXtra(FLUIDXtra FAR * pObj) {

	/* Called at each new(Xtra...) call */
	/* plus a first one for global/parent calls */
			
#ifdef _WINDOWS
  if (fluid_get_hinstance() == NULL) {
    /* Set the HINSTANCE of the Xtra. This is needed to create the
       DirectX sound driver. */
    fluid_set_hinstance((void*) gXtraFileRef);
  }
#endif

  /* Set the log functions fluid_xtra_log */
  fluid_set_log_function(FLUID_PANIC, fluid_xtra_log, pObj);
  fluid_set_log_function(FLUID_ERR, fluid_xtra_log, pObj);
  fluid_set_log_function(FLUID_WARN, fluid_xtra_log, pObj);
  fluid_set_log_function(FLUID_DBG, fluid_xtra_log, pObj);
  fluid_set_log_function(FLUID_INFO, fluid_xtra_log, pObj);

  /* initialize all the instance variables */
  pObj->pMmValue = NULL;
  pObj->pMmUtils2 = NULL;
  pObj->pMmList = NULL;
  pObj->pDrPlayer = NULL;
	pObj->pStream2 = NULL;
	pObj->pHandle = NULL;
	strcpy(pObj->pDownloadPath, "");
	strcpy(pObj->pDownloadLocalname, "");
	pObj->pDownloadError = kMoaErr_NoErr;
	pObj->pDownloadPercent = 0.0;

  pObj->synth = NULL;
  pObj->sequencer = NULL;
  pObj->debug = 0;
  pObj->debugSize = 102400;
  pObj->debugBuf = NULL; // (char *)malloc(pObj->debugSize);
  pObj->debugPtr = pObj->debugBuf;
	pObj->srcNames = NULL;
	pObj->ramsamples = NULL;
	pObj->curSampleID = 0;

  /* acquire all the needed director interfaces */
  pObj->pCallback->QueryInterface(&IID_IMoaMmValue, (PPMoaVoid) &pObj->pMmValue);
  pObj->pCallback->QueryInterface(&IID_IMoaMmUtils2, (PPMoaVoid) &pObj->pMmUtils2);
  pObj->pCallback->QueryInterface(&IID_IMoaMmList, (PPMoaVoid) &pObj->pMmList);
  pObj->pCallback->QueryInterface(&IID_IMoaDrPlayer, (PPMoaVoid)&pObj->pDrPlayer);
  pObj->pCallback->QueryInterface(&IID_IMoaHandle, (PPMoaVoid)&pObj->pHandle);

	/* setup notification */
	pObj->pNotification = NULL;
	pObj->pCallback->QueryInterface( &IID_IMoaNotification, (PPMoaVoid) &pObj->pNotification);
	if (pObj->pNotification)
		pObj->pCallback->MoaCreateInstance(&CLSID(FLUIDNotif), &IID(IMoaNotificationClient), (PPMoaVoid)&pObj->pNotificationClient);
	if (pObj->pNotification && pObj->pNotificationClient) {
		MoaLong ms = 1;
		pObj->pNotification->RegisterNotificationClient(pObj->pNotificationClient, &NID_DrNIdle, &ms, pObj);
	}
	
	// errorString
	_stringToValue(pObj, "", &pObj->errorString);	

	// soundFontStack
	pObj->soundFontStack = NULL;

	// curCallbacks
	pObj->pMmList->NewPropListValue(&pObj->curCallbacks);
 	{
		//	Sort it for fast finds
		MoaMmValue res;
		MoaMmSymbol sortPropS = 0;
		pObj->pMmValue->StringToSymbol("sort", &sortPropS);
		pObj->pDrPlayer->CallHandler(sortPropS, 1, &pObj->curCallbacks, &res);
	}
	pObj->curCallbackID = 0;	
	
	pObj->maxDones = 100;
	pObj->dones = (MoaMmSymbol *)pObj->pCalloc->NRAlloc(pObj->maxDones * sizeof(MoaMmSymbol));
	pObj->curDones = 0;
	pObj->doneBusy = 0;

	pObj->inited = true;
	
  fluid_log(FLUID_DBG, "** MoaCreate_FLUIDXtra[%d]", (long) pObj);

  return(kMoaErr_NoErr);
}

STDMETHODIMP_(void) MoaDestroy_FLUIDXtra(FLUIDXtra FAR * pObj) {
	
		/* Called at each instance release */
  fluid_log(FLUID_DBG, "** MoaDestroy_FLUIDXtra[%d]", (long) pObj);

	_myFree(pObj);

	// Release curCallbacks
	fluid_log(FLUID_DBG, ".. Release curCallbacks");
	if (pObj->inited)
		pObj->pMmValue->ValueRelease(&pObj->curCallbacks);

	// Release error string
	fluid_log(FLUID_DBG, ".. Release error string");
	if (pObj->inited)
		pObj->pMmValue->ValueRelease(&pObj->errorString);
	
  if (pObj->pMmList != NULL) {
    pObj->pMmList->Release();
    pObj->pMmList = NULL;
  }

  if (pObj->pDrPlayer != NULL) {
    pObj->pDrPlayer->Release();
    pObj->pDrPlayer = NULL;
  }

  if (pObj->pMmValue != NULL) {
    pObj->pMmValue->Release();
    pObj->pMmValue = NULL;
  }

  if (pObj->pMmUtils2 != NULL) {
    pObj->pMmUtils2->Release();
    pObj->pMmUtils2 = NULL;
  }

 	if (pObj->pHandle) pObj->pHandle->Release();
	pObj->pHandle = NULL;
	
  return;
}

void _myFree(FLUIDXtra FAR * pObj) {
		fluid_log(FLUID_DBG, "-> _myFree");

	/* deregister from notification */
	if (pObj->pNotification) {
		if (pObj->pNotificationClient) {
			pObj->pNotification->UnregisterNotificationClient(pObj->pNotificationClient, &NID_DrNIdle, NULL);
	    pObj->pNotificationClient->Release();
	    pObj->pNotificationClient = NULL;
		}
    pObj->pNotification->Release();
    pObj->pNotification = NULL;
	}

	// release ramsamples
	if (pObj->ramsamples) {
		fluid_xtra_sample_t* tmp = pObj->ramsamples;
		fluid_xtra_sample_t* next;
		while (tmp) {
			next = tmp->next;
			pObj->pCalloc->NRFree(tmp);
			tmp = next;
		}
		pObj->ramsamples = NULL;
	}
	
	// AS 6nov07 : MUST stop the audio driver before all, since it calls synth and thus seq
	if (pObj->adriver != NULL) {
		fluid_log(FLUID_DBG, "** destroying the audio driver");
		delete_fluid_audio_driver(pObj->adriver);
		pObj->adriver = NULL;
	}

	if (pObj->synth != NULL) {
		fluid_log(FLUID_DBG, "** destroying the synthesizer");
		delete_fluid_synth(pObj->synth);
		pObj->synth = NULL;
	}

	if (pObj->sequencer != NULL) {
		fluid_log(FLUID_DBG, "** destroying the sequencer");
		delete_fluid_sequencer(pObj->sequencer);
		pObj->sequencer = NULL;
	}

	// release srcNames
	if (pObj->srcNames) {
		fluid_log(FLUID_DBG, ".. release srcNames");
		fluid_xtra_srcname* tmp = pObj->srcNames;
		fluid_xtra_srcname* next;
		while (tmp) {
			next = tmp->next;
			pObj->pCalloc->NRFree(tmp->name);
			pObj->pCalloc->NRFree(tmp);
			tmp = next;
		}
		pObj->srcNames = NULL;
	}
	
	// Release soundFontStack
	if (pObj->soundFontStack) {
		fluid_log(FLUID_DBG, ".. Release soundFontStack");
		fluid_xtra_stack_item* tmp = pObj->soundFontStack;
		fluid_xtra_stack_item* next;
		while (tmp) {
			next = tmp->next;
			pObj->pCalloc->NRFree(tmp);
			tmp = next;
		}
		pObj->soundFontStack = NULL;
	}
	
	// abort download
	if (pObj->pStream2) {
		fluid_log(FLUID_DBG, ".. Release pObj->pStream2");
		pObj->pStream2->Release();
		pObj->pStream2 = NULL;
	}

	if (pObj->dones) {
		fluid_log(FLUID_DBG, ".. Release dones");
		pObj->pCalloc->NRFree(pObj->dones);
		pObj->dones = NULL;
	}

  fluid_log(FLUID_DBG, ".. release fluid_set_log_function");
  fluid_set_log_function(FLUID_PANIC, fluid_xtra_log, NULL);
  fluid_set_log_function(FLUID_ERR, fluid_xtra_log, NULL);
  fluid_set_log_function(FLUID_WARN, fluid_xtra_log, NULL);
  fluid_set_log_function(FLUID_DBG, fluid_xtra_log, NULL);
  fluid_set_log_function(FLUID_INFO, fluid_xtra_log, NULL);
  fluid_log(FLUID_DBG, ".. release fluid_set_log_function done");

	if (pObj->debugBuf) {
		fluid_log(FLUID_DBG, ".. release pObj->debugBuf");
		free(pObj->debugBuf);
		pObj->debugBuf = NULL;
	}

	fluid_log(FLUID_DBG, ".. _myfree done");
	
	pObj->inited = false;
}

MoaError 
FLUIDXtra_IMoaMmXScript::free(PMoaDrCallInfo callPtr) {

	_myFree(pObj);

	pObj->pMmValue->IntegerToValue(1, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::mynew(PMoaDrCallInfo callPtr)
{  
	/*	In this function, the Xtra instance is already in the resultValue.
			So don't touch the result value if all goes well 
			This may lead to a leak if we dont release the resultValue
			in case of error, but this seems dangerous
	*/
	
  fluid_log(FLUID_DBG, "** mynew[%d]", (long) pObj);
  
  /* Initalize synth */	
  fluid_settings_t* settings = new_fluid_settings();

	// default settings
  char * yes = (char *)"yes";
  char * no = (char *)"no";

  // Set my default Reverb
  pObj->reverbOn = true;
  fluid_settings_setstr(settings, "synth.reverb.active" , pObj->reverbOn ? yes : no);

  // Set my default Chorus
  pObj->chorusOn = false;
  fluid_settings_setstr(settings, "synth.chorus.active" , pObj->chorusOn ? yes : no);
	
	// set my default gain
  fluid_settings_setnum(settings, "synth.gain" , 0.2);
	
  // get arguments for settings
  if (callPtr->nargs > 1) {
		/*
		[#channels: nbChannels, "settingprop":"settingvalue"]
		*/
		MoaMmValue tmpPlistValue;
		
		/* get plist arg and check type */
		GetArgByIndex(2, &tmpPlistValue);	/* Dont release tmpPlistValue */
		if (!_checkValueType(&tmpPlistValue, kMoaMmValueType_PropList)) {
			fluid_log(FLUID_ERR, "Argument should be a plist");
			
		} else {
		
			/* Compatibility : get channels */
			MoaLong nbChannels = -1;
			if (_getIntProp(callPtr, tmpPlistValue, (char *)"channels", (char *)"channels", &nbChannels, false) == 0)
				fluid_settings_setint(settings, "synth.midi-channels", nbChannels);

			//  parse all settings
			int i, count = pObj->pMmList->CountElements(&tmpPlistValue);
			for (i = 1; i <= count; i++) {
				MoaMmValue propV, valV;

				_getPropAt(callPtr, &tmpPlistValue, i, &propV);
				pObj->pMmList->GetValueByIndex(&tmpPlistValue, i, &valV);
					
				if (!_checkValueType(&propV, kMoaMmValueType_String))
					continue;
				
				// string settings prop
				char propStr[1024];
 				_valueToString(pObj, &propV, propStr, 1024);
				int settingsType = fluid_settings_get_type(settings, propStr);
				
				if (settingsType == FLUID_NUM_TYPE) {
				  if (!_checkValueType(&valV, kMoaMmValueType_Float)) {
						fluid_log(FLUID_ERR, "Settings : Failed to set '%s' - value should be an int or a float\n", propStr);
					} else {
						
						// get value
						MoaDouble valFloat;
						MoaLong valInt;
						if (_checkValueType(&valV, kMoaMmValueType_Float)) {
							pObj->pMmValue->ValueToFloat(&valV, &valFloat);
						} else {
						  pObj->pMmValue->ValueToInteger(&valV, &valInt);
							valFloat = valInt;
						}
						
						if (!fluid_settings_setnum(settings, propStr, valFloat))
							fluid_log(FLUID_ERR, "Settings 1: Failed to set '%s' - most likely the setting does not exist.\n", propStr);
					}		
				} else if (settingsType == FLUID_INT_TYPE) {
				  if (!_checkValueType(&valV, kMoaMmValueType_Integer)) {
						fluid_log(FLUID_ERR, "Settings : Failed to set '%s' - value should be an int or a float\n", propStr);
					} else {
						
						// get value
						MoaDouble valFloat;
						MoaLong valInt;
						if (_checkValueType(&valV, kMoaMmValueType_Float)) {
							pObj->pMmValue->ValueToFloat(&valV, &valFloat);
							valInt = (MoaLong)valFloat;
						} else {
						  pObj->pMmValue->ValueToInteger(&valV, &valInt);
						}
						
						if (!fluid_settings_setint(settings, propStr, valInt))
							fluid_log(FLUID_ERR, "Settings 2: Failed to set '%s' - most likely the setting does not exist.\n", propStr);
					}		
				} else if (settingsType == FLUID_STR_TYPE) {
			  	if (!_checkValueType(&valV, kMoaMmValueType_String)) {
						fluid_log(FLUID_ERR, "Settings 3: Failed to set '%s' - value should be a string\n", propStr);
					} else {
						char valStr[1024];
	 					_valueToString(pObj, &valV, valStr, 1024);
				    if (!fluid_settings_setstr(settings, propStr, valStr))
							fluid_log(FLUID_ERR, "Settings 4: Failed to set '%s' - most likely the setting does not exist.\n", propStr);
			    }
				} else {
					fluid_log(FLUID_ERR, "Settings 5: Failed to find '%s'\n", propStr);
				}
			}
		}
  }
  

  fluid_log(FLUID_DBG, "** creating the synthesizer");

  pObj->synth = NULL;
 
  pObj->synth = new_fluid_synth(settings);
  if (pObj->synth == NULL) {
	  pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
	  return kMoaErr_NoErr;
  }

#ifdef _WIN32
  char *tmpStr;
  if (!fluid_settings_getstr(settings, "audio.driver", &tmpStr))
    tmpStr = fluid_settings_getstr_default(settings, "audio.driver");
  pObj->dsoundDriver = (!strcmp(tmpStr, "dsound"));
  if (pObj->dsoundDriver)
	pObj->adriver = new_fluid_audio_driver(settings, pObj->synth);
  else // portaudio
	pObj->adriver = new_fluid_audio_driver2(settings, fluid_coreaudio_cb, (void *)this);
#else
  pObj->adriver = new_fluid_audio_driver2(settings, fluid_coreaudio_cb, (void *)this);
#endif
	
  if (pObj->adriver == NULL) {
	  pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
	  return kMoaErr_NoErr;
  }

  // Set my default Reverb props
 	fluid_synth_set_reverb(pObj->synth, 0.5, 0.5, 1.0, 1.0);

  // Set my default Chorus props
	fluid_synth_set_chorus(pObj->synth, 3, 1.0, 3.0, 10.0, FLUID_CHORUS_MOD_SINE);
	  
  // create sequencer
  pObj->sequencer = new_fluid_sequencer();
  if (pObj->sequencer == NULL) {
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
    delete_fluid_synth(pObj->synth);
    pObj->synth = NULL;
    delete_fluid_audio_driver(pObj->adriver);
	pObj->adriver = NULL;
  	return kMoaErr_NoErr;
  }

	// register synth as first destination
	pObj->synthSeqID = fluid_sequencer_register_fluidsynth(pObj->sequencer, pObj->synth);
	
	// register myself as second destination
	pObj->xtraSeqID = fluid_sequencer_register_client(pObj->sequencer, "fluidXtra", 
				     fluid_xtra_seq_callback, (void *)this);

	// set channel 9 as melodic (not drum, as fallback mechanism is not adapted to RAM soundfonts, which are not the main usage of fluidXtra)
	fluid_synth_set_channel_type(pObj->synth, 9, CHANNEL_TYPE_MELODIC);
	
  // success
  return kMoaErr_NoErr;
}


MoaError
FLUIDXtra_IMoaMmXScript::getSettingsOptions(PMoaDrCallInfo callPtr)
{
	char propStr[1024];
	if (_getAnsiStringArg(callPtr, 2, (char *)"setting", propStr, 1024, true) < 0)
		return kMoaErr_NoErr;
    
	fluid_settings_t* settings = new_fluid_settings(); // fluid_synth_get_settings(pObj->synth);
	int settingType = fluid_settings_get_type(settings, propStr);
	if (settingType == FLUID_STR_TYPE) {
    char * optionsStr = fluid_settings_option_concat(settings, propStr, ",");
    _stringToValue(pObj, optionsStr, &callPtr->resultValue);
    delete_fluid_settings(settings);
    return kMoaErr_NoErr;
  }
  
  // default to 0
  pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  
  delete_fluid_settings(settings);
  return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::getSetting(PMoaDrCallInfo callPtr)
{
	char propStr[1024];
	if (_getAnsiStringArg(callPtr, 2, (char *)"setting", propStr, 1024, true) < 0)
		return kMoaErr_NoErr;
  
  // a synth, use synth settings, otherwise, use default
	fluid_settings_t* settings = (pObj->synth == NULL) ? new_fluid_settings() : fluid_synth_get_settings(pObj->synth);
	int settingType = fluid_settings_get_type(settings, propStr);
	
	if (settingType == FLUID_INT_TYPE) {
		int val;
		if (!fluid_settings_getint(settings, propStr, &val))
			val = fluid_settings_getint_default(settings, propStr);
		pObj->pMmValue->IntegerToValue(val, &callPtr->resultValue);
    
	} else if (settingType == FLUID_NUM_TYPE) {
		double val;
		if (!fluid_settings_getnum(settings, propStr, &val))
			val = fluid_settings_getnum_default(settings, propStr);
		pObj->pMmValue->FloatToValue(val, &callPtr->resultValue);
    
	} else if (settingType == FLUID_STR_TYPE) {
		char *tmpStr;
		if (!fluid_settings_getstr(settings, propStr, &tmpStr))
			tmpStr = fluid_settings_getstr_default(settings, propStr);
		_stringToValue(pObj, tmpStr, &callPtr->resultValue);
	} else {
    
    // failure
    fluid_log(FLUID_ERR, "Settings : Failed to get '%s' - most likely the setting does not exist.\n", propStr);
    pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  }
  
  if (pObj->synth == NULL) delete_fluid_settings(settings); // delete default

	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getChannelsCount(PMoaDrCallInfo callPtr)
{
	int nbChannels = -1;
	fluid_settings_t* settings = fluid_synth_get_settings(pObj->synth);
	fluid_settings_getint(settings, "synth.midi-channels", &nbChannels);
	pObj->pMmValue->IntegerToValue(nbChannels, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::getMasterGain(PMoaDrCallInfo callPtr)
{
	MoaDouble gain = fluid_synth_get_gain(pObj->synth);
	pObj->pMmValue->FloatToValue(gain, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::getCPULoad(PMoaDrCallInfo callPtr)
{
	MoaDouble cpuload = fluid_synth_get_cpu_load(pObj->synth);
	if (cpuload > 100.0) cpuload = 100.0;
	if (cpuload < 0.0) cpuload = 0.0;
	
	pObj->pMmValue->FloatToValue(cpuload, &callPtr->resultValue);
  return kMoaErr_NoErr;
}


MoaError
FLUIDXtra_IMoaMmXScript::setMasterGain(PMoaDrCallInfo callPtr)
{
  MoaDouble gain;
 	if (_getFloatArg(callPtr, 2, (char *)"gain", &gain, true) < 0)
    return kMoaErr_NoErr;
  
  if (gain < 0.0) gain = 0.0;
  if (gain > 2.0) gain = 2.0;
  
	fluid_synth_set_gain(pObj->synth, (float)gain);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

/**************************************/
/********* Recording ******************/
/**************************************/


// ---------------------------------------------
// these callbacks receive from sound thread and push to FIFO

// for mac and portaudio/Win
int fluid_coreaudio_cb(void* data, int len, int nin, float** in, int nout, float** out) {
	FLUIDXtra_IMoaMmXScript *This = (FLUIDXtra_IMoaMmXScript *)data;
	return This->_coreaudio_cb(len, nin, in, nout, out);
}
int FLUIDXtra_IMoaMmXScript::_coreaudio_cb(int len, int nin, float** in, int nout, float** out) {
	// standard stuff
	int res = fluid_synth_write_float(pObj->synth, len, out[0], 0, 1, out[1], 0, 1);

	if (pObj->isRecordingP && pObj->pFifo && pObj->pOutBufShort) {
		// push to fifo fast
		// Convert float to short, and interlace
		int starti = 0, nbFrames;
		nbFrames = len - starti;
		if (nbFrames > pObj->pWriteBufFrameCount) nbFrames = pObj->pWriteBufFrameCount;
		while (nbFrames > 0) {
			float * left = out[0];
			float * right = out[1];
			short * outb = pObj->pOutBufShort;
			float s;
			for (int i = 0; i < nbFrames; i++) {
				
				s = left[starti+i];
				if (s > 1.0) s = 1.0; if (s < -1.0) s = -1.0;
				outb[2*i] = (short)(s*32767.0);
				
				s = right[starti+i];
				if (s > 1.0) s = 1.0; if (s < -1.0) s = -1.0;
				outb[2*i+1] = (short)(s*32767.0);
			}
			pObj->pFifo->queueBytesFrom(pObj->pOutBufShort, nbFrames*2*2);
			starti += nbFrames;
			nbFrames = len - starti;
			if (nbFrames > pObj->pWriteBufFrameCount) nbFrames = pObj->pWriteBufFrameCount;
		}
	}
	return res;
}

#ifdef _WIN32 // Windows/dsound
extern "C" int
fluid_dsound_cb(void* arg, int len, short* data) {
	FLUIDXtra_IMoaMmXScript *This = (FLUIDXtra_IMoaMmXScript *)arg;
	This->_dsound_cb(len, data);
	return 0;
}

void
FLUIDXtra_IMoaMmXScript::_dsound_cb(int nbFrames, short* data) {
	
	if (!pObj->isRecordingP || !pObj->pRecFile || !data)
		return;
	
	// queue
	pObj->pFifo->queueBytesFrom(data, nbFrames*2*2);
}

#endif

// ---------------------------------------------
// This thread pumps from FIFO and writes to file
void writeThread(void* data) {
	FLUIDXtra_IMoaMmXScript *This = (FLUIDXtra_IMoaMmXScript *)data;
	This->_recordWriteThread();
}

void FLUIDXtra_IMoaMmXScript::_recordWriteThread() {
	bool errorHere = false;
	while (pObj->isRecordingP) {
		
		if (pObj->pFifo->availableBytes() > 0) {
			// pump what is possible
			unsigned int nbFrames = pObj->pWriteBufFrameCount;
			
			while (pObj->pFifo->unqueueBytesInto(pObj->pWriteBufShort, nbFrames*4) == 0) {
				if (sf_write_short (pObj->pRecFile, (short *)pObj->pWriteBufShort, nbFrames*2) != nbFrames*2) {
					errorHere = true;
					break;
				}
			}
			
		}
		
		if (errorHere)
			break;
		
#if defined(WIN32)
		Sleep(1);
#else
		usleep(1000);
#endif
	}
	
	if (errorHere) {
		_stopRecord(pObj, true);
		pObj->pRecError = FLUIDXTRAERR_WRITEFILE;			
	} else {
		// stopped because stopped from outside
		// write all pFifo to file now and leave
		unsigned int bufBytesCount;
		unsigned int bytesCount;
		bufBytesCount = pObj->pWriteBufFrameCount*4;
		while ((bytesCount = pObj->pFifo->availableBytes()) > 0) {
			if (bytesCount > bufBytesCount)
				bytesCount = bufBytesCount;
			if (pObj->pFifo->unqueueBytesInto(pObj->pWriteBufShort, bytesCount) == 0)
				// write to file now
				sf_write_short (pObj->pRecFile, (short *)pObj->pWriteBufShort, bytesCount/2);
		}
	}
}


MoaError
FLUIDXtra_IMoaMmXScript::startRecord(PMoaDrCallInfo callPtr)
{
	if (pObj->isRecordingP) {
  		fluid_log(FLUID_ERR, "Already Recording");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_ALREADYRECORDING, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	char filePath[1024];
	if (_getStringArg(callPtr, 2, (char *)"file", filePath, 1024, true) < 0)
		return kMoaErr_NoErr;
	pathHFS2POSIX(filePath, 1024);
	
	// Open file
	SF_INFO	sfinfo ;
	memset (&sfinfo, 0, sizeof (sfinfo)) ; 	 
	double synthsamplerate = 44100;
	fluid_settings_t*settings = fluid_synth_get_settings(pObj->synth);
	fluid_settings_getnum(settings, "synth.sample-rate", &synthsamplerate);
	sfinfo.samplerate  = synthsamplerate + 0.5 ; 
	sfinfo.channels	   = 2 ; 
	sfinfo.pcmbitwidth = 16 ; 
	sfinfo.format      = (SF_FORMAT_WAV | SF_FORMAT_PCM) ; 
	
	fluid_log(FLUID_DBG, "opening record file : %s", filePath);
	if (! (pObj->pRecFile = sf_open_write (filePath, &sfinfo))) {
  		fluid_log(FLUID_ERR, "Error opening record file: %s", filePath);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_OPENWRITEFILE, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	// start the record
	pObj->pRecError = 0;
	pObj->isRecordingP = 1;
	
	// start FIFO and write thread
	// create the FIFO
	int periods, period_size;
	fluid_settings_getint(settings, "audio.periods", &periods);
	fluid_settings_getint(settings, "audio.period-size", &period_size);
	pObj->pWriteBufFrameCount = period_size*periods;
	int nbOutBuffers = 64;
	pObj->pFifo = new bytesfifo(pObj->pWriteBufFrameCount*2*sizeof(short)*nbOutBuffers, false);
	pObj->pWriteBufShort = (short *)malloc(pObj->pWriteBufFrameCount*2*sizeof(short));
	pObj->pOutBufShort = NULL;
#ifndef _WIN32
	pObj->pOutBufShort = (short *)malloc(pObj->pWriteBufFrameCount*2*sizeof(short));
#endif
#ifdef _WIN32
	if (!pObj->dsoundDriver)
		pObj->pOutBufShort = (short *)malloc(pObj->pWriteBufFrameCount*2*sizeof(short));
#endif

	GError *err = NULL;
	pObj->pWritingThread = g_thread_create((GThreadFunc)(&writeThread), (void *)this, true, &err);
	
#ifdef _WIN32
	if (pObj->dsoundDriver)
		fluid_dsound_audio_driver_setcallback(pObj->adriver, fluid_dsound_cb, (void *)this);
#endif
	
	fluid_log(FLUID_DBG, "startRecord started");
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	return(kMoaErr_NoErr);	
}

MoaError
FLUIDXtra_IMoaMmXScript::isRecording(PMoaDrCallInfo callPtr)
{		
	pObj->pMmValue->IntegerToValue(pObj->isRecordingP, &callPtr->resultValue);
	return(kMoaErr_NoErr);	
}

MoaError
FLUIDXtra_IMoaMmXScript::stopRecord(PMoaDrCallInfo callPtr)
{
	if (!pObj->isRecordingP) {
		fluid_log(FLUID_ERR, "Not recording");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NOTRECORDING, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	_stopRecord(pObj, false);
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	return(kMoaErr_NoErr);	
}

void
_stopRecord(FLUIDXtra FAR * pObj, bool fromWritingThread) {
	pObj->pRecError = 0;
	fluid_log(FLUID_DBG, "_stopRecord ENTER");
	
	if (pObj->isRecordingP) {
		// stops recording
		pObj->isRecordingP = false;
#ifdef _WIN32
		if (pObj->adriver && pObj->dsoundDriver) {
			fluid_dsound_audio_driver_setcallback(pObj->adriver, NULL, NULL);
		}
#endif
	}
	
	// wait for writing thread to end
	if (pObj->pWritingThread) {
		if (!fromWritingThread)
			g_thread_join(pObj->pWritingThread);
		pObj->pWritingThread = NULL;
	}
	
	if (pObj->pWriteBufShort != NULL) {
		free(pObj->pWriteBufShort);
		pObj->pWriteBufShort = NULL;
	}

	if (pObj->pOutBufShort != NULL) {
		free(pObj->pOutBufShort);
		pObj->pOutBufShort = NULL;
	}

	if (pObj->pRecFile != NULL) {
		// close file
		sf_close (pObj->pRecFile) ;
		pObj->pRecFile = NULL;
	}
	
	if (pObj->pFifo != NULL)
		delete pObj->pFifo;
	pObj->pFifo = NULL;
}



/**************************************/
/******  SoundFontFile Download *******/
/**************************************/



MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderDeleteLocal(PMoaDrCallInfo callPtr)
{
	char localName[1024];
	memset(localName, 0, 1024);
	if (_getAnsiStringArg(callPtr, 2, "localName", localName, 1024, true) < 0)
		return kMoaErr_NoErr;

	// download path
	if ((pObj->pDownloadPath[0] == 0) && _isInShockwave(callPtr))
		sfGetSchockwaveSupportFolderPath(pObj->pDownloadPath, 1024);
	
	if (pObj->pDownloadPath[0] == 0) {
		fluid_log(FLUID_ERR, "downloadFolder does not exist\n");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	char localPath[2048];
	sfBuildPathFromFolderAndFile(localPath, 2048, pObj->pDownloadPath, localName);
	int done = sfDeleteFile(localPath, 2048);

	pObj->pMmValue->IntegerToValue(done, &callPtr->resultValue);
  return kMoaErr_NoErr;
}


MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderSetPath(PMoaDrCallInfo callPtr)
{
	int err = _getAnsiStringArg(callPtr, 2, "localAbsolutePath", pObj->pDownloadPath, 1024, false);
	if (err == -1) return kMoaErr_NoErr;
	if (err == -2 || pObj->pDownloadPath[0] == 0) {
		// void or empty string : reset pObj->pDownloadPath
			pObj->pDownloadPath[0] = 0;
			pObj->pMmValue->IntegerToValue(1, &callPtr->resultValue);
			return kMoaErr_NoErr;	
	}
	
	sfStripLastSeparator(pObj->pDownloadPath);

#ifdef MACINTOSH
	// prepend the rootPath on Mac, if not already
	char rootPath[2048];
	getRootPath(rootPath, 1024);
	if (strlen(rootPath) > 0) {
		if (strncmp(pObj->pDownloadPath, rootPath, strlen(rootPath))) {
			// does not start by rootPath : prepend
			strcpy(rootPath + strlen(rootPath), pObj->pDownloadPath);
			strcpy(pObj->pDownloadPath, rootPath);
		}
	}
#endif	

	if (_isInShockwave(callPtr)) {

		// ensure that dswmedia is in path
		char *folder = (char *)"dswmedia";
#ifdef MACINTOSH
		char *sep = (char *)":";
#else
		char *sep = "\\";
#endif	
		char fullfol[1024];
		sprintf(fullfol, "%s%s%s", sep, folder, sep);
		char lastfol[1024];
		sprintf(lastfol, "%s%s", sep, folder);
		int inPath = (strstr(pObj->pDownloadPath, fullfol) != NULL);
		int lastOne = !strncmp(pObj->pDownloadPath+strlen(pObj->pDownloadPath)-strlen(lastfol), lastfol, strlen(lastfol));
		if (!inPath && !lastOne) { 
			// no...
			pObj->pDownloadPath[0] = 0;
			fluid_log(FLUID_ERR, "downloadFolderSetPath : no '%s' in path : Forbidden in schockwave\n", folder);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FORBIDDENINSHOCKWAVE, &callPtr->resultValue);
			return kMoaErr_NoErr;		
		}
	}

	if (!sfDirectoryExists(pObj->pDownloadPath, 1024)) {
		fluid_log(FLUID_ERR, "Folder does not exist\n");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND, &callPtr->resultValue);
		pObj->pDownloadPath[0] = 0;
		return kMoaErr_NoErr;
	}
	
	pObj->pMmValue->IntegerToValue(1, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderContainsLocal(PMoaDrCallInfo callPtr)
{
	char localName[1024];
	memset(localName, 0, 1024);
	if (_getAnsiStringArg(callPtr, 2, "localName", localName, 1024, true) < 0)
	  return kMoaErr_NoErr;
	
	// download path
	if ((pObj->pDownloadPath[0] == 0) && _isInShockwave(callPtr))
		sfGetSchockwaveSupportFolderPath(pObj->pDownloadPath, 1024);
	
	if (pObj->pDownloadPath[0] == 0) {
		fluid_log(FLUID_ERR, "downloadFolder does not exist\n");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}

	char localPath[2048];
	sfBuildPathFromFolderAndFile(localPath, 2048, pObj->pDownloadPath, localName);
	int exists = sfFileExists(localPath, 2048);
	fluid_log(FLUID_DBG, "contains %s = %d", localPath, exists);
	pObj->pMmValue->IntegerToValue(exists, &callPtr->resultValue);
	
  return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderGetLocalProps(PMoaDrCallInfo callPtr)
{
	char localName[1024];
	memset(localName, 0, 1024);
	if (_getAnsiStringArg(callPtr, 2, "localName", localName, 1024, true) < 0)
		return kMoaErr_NoErr;
	
	// download path
	if ((pObj->pDownloadPath[0] == 0) && _isInShockwave(callPtr))
		sfGetSchockwaveSupportFolderPath(pObj->pDownloadPath, 1024);
	
	if (pObj->pDownloadPath[0] == 0) {
		fluid_log(FLUID_ERR, "downloadFolder does not exist\n");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	char localPath[2048];
	sfBuildPathFromFolderAndFile(localPath, 2048, pObj->pDownloadPath, localName);
	
	int siz = sfFileSize(localPath, 2048);
	if (siz > 0) {
		fluid_log(FLUID_DBG, "size %s = %d", localPath, siz);
		int iyear, imonth, iday;
		bool res = sfFileDate(localPath, 2048, &iyear, &imonth, &iday);
		if (res) {
			fluid_log(FLUID_DBG, "date %s = %d/%d/%d", localPath, iyear, imonth, iday);
			// build proplist
			MoaMmValue element;
			MoaMmSymbol symbol = 0;
			MoaMmValue symbolValue;
			pObj->pMmList->NewPropListValue(&callPtr->resultValue);
			
			// Get size
			pObj->pMmValue->StringToSymbol("size", &symbol);
			pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
			pObj->pMmValue->IntegerToValue(siz, &element);
			pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
			pObj->pMmValue->ValueRelease(&element);
			pObj->pMmValue->ValueRelease(&symbolValue);
			
			// Get iyear
			pObj->pMmValue->StringToSymbol("year", &symbol);
			pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
			pObj->pMmValue->IntegerToValue(iyear, &element);
			pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
			pObj->pMmValue->ValueRelease(&element);
			pObj->pMmValue->ValueRelease(&symbolValue);
			
			// Get month
			pObj->pMmValue->StringToSymbol("month", &symbol);
			pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
			pObj->pMmValue->IntegerToValue(imonth, &element);
			pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
			pObj->pMmValue->ValueRelease(&element);
			pObj->pMmValue->ValueRelease(&symbolValue);

			// Get day
			pObj->pMmValue->StringToSymbol("day", &symbol);
			pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
			pObj->pMmValue->IntegerToValue(iday, &element);
			pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
			pObj->pMmValue->ValueRelease(&element);
			pObj->pMmValue->ValueRelease(&symbolValue);
			
			return kMoaErr_NoErr;
		}
	}
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	
	return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderAbortDownload(PMoaDrCallInfo callPtr)
{
	if (pObj->pStream2) {
		pObj->pStream2->Release();
		pObj->pStream2 = NULL;
	}
	pObj->pDownloadError = kMoaErr_NoErr;
	strcpy(pObj->pDownloadLocalname, "");
	pObj->pMmValue->IntegerToValue(1, &callPtr->resultValue);

	return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderStartDownload(PMoaDrCallInfo callPtr)
{
	fluid_log(FLUID_DBG, "downloadFolderStartDownload start");

	MoaError	err = kMoaErr_NoErr;
	char pUrlString[MOA_MAX_PATHNAME];
	err = _getAnsiStringArg(callPtr, 2, "pathorUrl", pUrlString, 1024, true);
	if (err == -1) return kMoaErr_NoErr;
	if (err == -2) return kMoaErr_NoErr;
	err = _getAnsiStringArg(callPtr, 3, "localName", pObj->pDownloadLocalname, 1024, true);
	if (err == -1) return kMoaErr_NoErr;
	if (err == -2) return kMoaErr_NoErr;

	// check that download path exists
	if ((pObj->pDownloadPath[0] == 0) && _isInShockwave(callPtr))
		sfGetSchockwaveSupportFolderPath(pObj->pDownloadPath, 1024);

	if (pObj->pDownloadPath[0] == 0) {
		fluid_log(FLUID_ERR, "downloadFolder does not exist\n");
		pObj->pDownloadError = FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND;
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}

	// ok lets go
	PIMoaPathName pPathName = NULL;
	PIMoaFile2 pMoaFile = NULL;

	// Need these interfaces
	pObj->pCallback->MoaCreateInstance(&CLSID_CMoaPath, &IID_IMoaPathName, ( void ** )&pPathName );
	pObj->pCallback->MoaCreateInstance(&CLSID_CNetFile, &IID_IMoaFile2, ( void ** )&pMoaFile );

	// IMoaPathName interface converts the URL C-string into a
	// structure we understand.
	pPathName->InitFromString(pUrlString, kMoaPathDialect_URL_STYLE, TRUE, FALSE ); //  kMoaPathDialect_URL_STYLE ?

	// IMoaFile2 interface knows it's dealing with a file, and can
	// give us a "stream" for the file.
	err = pMoaFile->SetPathName( pPathName );
	if (err) {	
		fluid_log(FLUID_ERR, "downloadFolderStartDownload : %s\n", _MoaNetErrorDesc(err));
		pObj->pDownloadError = err;
		pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
		pMoaFile->Release();
		pPathName->Release();
		return kMoaErr_NoErr;	
	}

	// just in case there was a download going on.. stop it
	if (pObj->pStream2) {
		pObj->pStream2->Release();
		pObj->pStream2 = NULL;
	}

	err = pMoaFile->GetStream( 0, &( pObj->pStream2 ) );
	if (err) {	
		fluid_log(FLUID_ERR, "downloadFolderStartDownload : %s\n", _MoaNetErrorDesc(err));
		pObj->pDownloadError = err;
		pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
		pMoaFile->Release();
		pPathName->Release();
		return kMoaErr_NoErr;	
	}

	// Now we can open the stream.  This initiates HTTP traffic
	// with the server.
	err = pObj->pStream2->Open(kMoaStreamOpenAccess_ReadOnly, kMoaStreamSetPositionType_None );
	if (err) {	
		fluid_log(FLUID_ERR, "downloadFolderStartDownload : %s\n", _MoaNetErrorDesc(err));
		pObj->pDownloadError = err;
		pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
		pMoaFile->Release();
		pPathName->Release();
		return kMoaErr_NoErr;	
	}

	// It may be a long time (many seconds) before anything comes
	// back from the server.  Return from this code and check
	// IsStreamDone() periodically, e.g. on idle or enterFrame messages.

	pObj->pDownloadError = kMoaErr_NoErr;
	
	// release interfaces when done
	pMoaFile->Release();
	pPathName->Release();

	fluid_log(FLUID_DBG, "downloadFolderStartDownload started");

	// success
	pObj->pMmValue->IntegerToValue(1, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

void checkSFDownload(FLUIDXtra FAR * pObj) {

	if (pObj->pStream2 == NULL) return;

	fluid_log(FLUID_DBG, "checkSFDownload...");
	
	MoaError	err = kMoaErr_NoErr;
	MoaStreamPosition endPos, length;
	int done = 0;
	
	// find the end position (or length) of the download
	err = pObj->pStream2->GetEnd(&endPos);
	if (err == kMoaErr_NoErr) {
		// find out how much we have downloaded
		err = pObj->pStream2->GetCurrentLength(&length);
		// if no error and we have downloaded the entire file, we are done
		if ( ( err == kMoaErr_NoErr )&&( length >= endPos ) ) {
			// done ok
			done = 1;
		}
	}

	// if there is an error and the error is not data not available, we are done
	if (err && err != kMoaStreamErr_DataNotAvail) {
		// error
		done = 1;
	}
	
	if (endPos == 0) 
		pObj->pDownloadPercent = 0;
	else
		pObj->pDownloadPercent = (float)length/(float)endPos;
	
	if (done) {
		if ( endPos == 0 ) {
			fluid_log(FLUID_ERR, "No data in download (bad url ?)\n");
			pObj->pDownloadError = FLUIDXTRAERR_BADARGUMENT;
			return;
		}

		// **********
		// write to file
		
		// download path
		if (pObj->pDownloadPath[0] == 0) {
			fluid_log(FLUID_ERR, "downloadFolder does not exist\n");
			pObj->pDownloadError = FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND;
			return;
		}
	
		char localPath[2048];
		sfBuildPathFromFolderAndFile(localPath, 2048, pObj->pDownloadPath, pObj->pDownloadLocalname);
		pathHFS2POSIX(localPath, 2048);
		
		// open file for writing
	  FILE* outFile = fopen(localPath, "wb");
	  if (outFile == NULL) {
			fluid_log(FLUID_ERR, "unable to open file for writing '%s'\n", pObj->pDownloadLocalname);
			pObj->pDownloadError = FLUIDXTRAERR_OPENWRITEFILE;
			return;
	  }
		
		// get strem dbytes in RAM...
		MoaHandle  myHandle = pObj->pHandle->ZeroAlloc( endPos+1 );
		MoaChar   *ptr2 = (MoaChar *) pObj->pHandle->Lock( myHandle );

		unsigned long pNumActuallyRead;
		pObj->pStream2->Read((void *)ptr2, endPos, &pNumActuallyRead);
		
		// write to file
		fwrite(ptr2, 1, pNumActuallyRead, outFile);

		fflush(outFile);
		fclose(outFile);
		
		pObj->pHandle->Unlock( myHandle );
			
		// forget this
		strcpy(pObj->pDownloadLocalname, "");

		// finished
		if (pObj->pStream2) {
			pObj->pStream2->Release();
			pObj->pStream2 = NULL;
		}

		pObj->pDownloadError = 1;
	}
	
}

MoaError
FLUIDXtra_IMoaMmXScript::downloadFolderGetDownloadRatio(PMoaDrCallInfo callPtr)
{
	checkSFDownload(pObj);
	
	if (pObj->pDownloadError == 0) {
		pObj->pMmValue->FloatToValue(pObj->pDownloadPercent, &callPtr->resultValue);
	} else {
		pObj->pMmValue->FloatToValue((float)pObj->pDownloadError, &callPtr->resultValue);
	}
  return kMoaErr_NoErr;
}

/**************************************/
/****** Reverb/Chorus management ******/
/**************************************/
MoaError 
FLUIDXtra_IMoaMmXScript::getReverb(PMoaDrCallInfo callPtr)
{
	pObj->pMmValue->IntegerToValue(pObj->reverbOn, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setReverb(PMoaDrCallInfo callPtr)
{
	MoaLong onOff;
	int res;
	res = _getIntArg(callPtr, 2, "onOff", &onOff, false);
 	if (res == -1)
    return kMoaErr_NoErr;
  if (res == -2)
  	onOff = 1;
  
  if (onOff != 0) onOff = 1;

  pObj->reverbOn = onOff;
  fluid_synth_set_reverb_on(pObj->synth, onOff);

	pObj->pMmValue->IntegerToValue(kMoaErr_NoErr, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getReverbProp(PMoaDrCallInfo callPtr)
{
	MoaMmSymbol prop, refSymbol;
 	if (_getSymbolArg(callPtr, 2, "reverbProp", &prop, true) < 0)
    return kMoaErr_NoErr;
    
  pObj->pMmValue->StringToSymbol("level", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_reverb_level(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
  pObj->pMmValue->StringToSymbol("roomsize", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_reverb_roomsize(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
  pObj->pMmValue->StringToSymbol("width", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_reverb_width(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
  pObj->pMmValue->StringToSymbol("damping", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_reverb_damp(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
	char symbolstring[1024];
	pObj->pMmValue->SymbolToString(prop, symbolstring, 1024);
	fluid_log(FLUID_ERR, "unknown reverb property : %s", prop);
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setReverbProp(PMoaDrCallInfo callPtr)
{
	MoaMmSymbol prop, refSymbol;
	MoaDouble val;
  double roomsize, level, width, damp;
  
 	if (_getSymbolArg(callPtr, 2, "reverbProp", &prop, true) < 0)
    return kMoaErr_NoErr;
 	if (_getFloatArg(callPtr, 3, "reverbValue", &val, true) < 0)
    return kMoaErr_NoErr;
  
  if (val > 1.0) val = 1.0;
  if (val < 0.0) val = 0.0;
  
  level = fluid_synth_get_reverb_level(pObj->synth);
  roomsize = fluid_synth_get_reverb_roomsize(pObj->synth);
  width = fluid_synth_get_reverb_width(pObj->synth);
  damp = fluid_synth_get_reverb_damp(pObj->synth);
  
  pObj->pMmValue->StringToSymbol("level", &refSymbol);
	if (refSymbol == prop) {
		fluid_synth_set_reverb(pObj->synth, roomsize, damp, width, val);
		goto ok;
	}
  pObj->pMmValue->StringToSymbol("roomsize", &refSymbol);
	if (refSymbol == prop) {
		fluid_synth_set_reverb(pObj->synth, val, damp, width, level);
		goto ok;
	}
  pObj->pMmValue->StringToSymbol("width", &refSymbol);
	if (refSymbol == prop) {
		fluid_synth_set_reverb(pObj->synth, roomsize, damp, val, level);
		goto ok;
	}
  pObj->pMmValue->StringToSymbol("damping", &refSymbol);
	if (refSymbol == prop) {
		fluid_synth_set_reverb(pObj->synth, roomsize, val, width, level);
		goto ok;
	}
		
	char symbolstring[1024];
	pObj->pMmValue->SymbolToString(prop, symbolstring, 1024);
	fluid_log(FLUID_ERR, "unknown reverb property : %s", prop);
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  return kMoaErr_NoErr;

ok:
		pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	  return kMoaErr_NoErr;

}

/* Chorus */
MoaError 
FLUIDXtra_IMoaMmXScript::getChorus(PMoaDrCallInfo callPtr)
{
	pObj->pMmValue->IntegerToValue(pObj->chorusOn, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setChorus(PMoaDrCallInfo callPtr)
{
	MoaLong onOff;
	int res;
	res = _getIntArg(callPtr, 2, "onOff", &onOff, false);
 	if (res == -1)
    return kMoaErr_NoErr;
  if (res == -2)
  	onOff = 1;
  
  if (onOff != 0) onOff = 1;

	pObj->chorusOn = onOff;
	fluid_synth_set_chorus_on(pObj->synth, onOff);

	pObj->pMmValue->IntegerToValue(kMoaErr_NoErr, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getChorusProp(PMoaDrCallInfo callPtr)
{
	MoaMmSymbol prop, refSymbol;
 	if (_getSymbolArg(callPtr, 2, "chorusProp", &prop, true) < 0)
    return kMoaErr_NoErr;
    
  pObj->pMmValue->StringToSymbol("level", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_chorus_level(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
  pObj->pMmValue->StringToSymbol("number", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->IntegerToValue(fluid_synth_get_chorus_nr(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
  pObj->pMmValue->StringToSymbol("speed", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_chorus_speed_Hz(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
  pObj->pMmValue->StringToSymbol("depth", &refSymbol);
	if (refSymbol == prop) {
		pObj->pMmValue->FloatToValue(fluid_synth_get_chorus_depth_ms(pObj->synth), &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
	char symbolstring[1024];
	pObj->pMmValue->SymbolToString(prop, symbolstring, 1024);
	fluid_log(FLUID_ERR, "unknown chorus property : %s", prop);
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setChorusProp(PMoaDrCallInfo callPtr)
{
	MoaMmSymbol prop, refSymbol;
	MoaDouble val;
	MoaLong valInt;
	double level, speed, depth;
	int chorusNumber;
	
 	if (_getSymbolArg(callPtr, 2, "chorusProp", &prop, true) < 0)
    return kMoaErr_NoErr;
  
  level = fluid_synth_get_chorus_level(pObj->synth);
  speed = fluid_synth_get_chorus_speed_Hz(pObj->synth);
  depth = fluid_synth_get_chorus_depth_ms(pObj->synth);
  chorusNumber = fluid_synth_get_chorus_nr(pObj->synth);
  
  pObj->pMmValue->StringToSymbol("level", &refSymbol);
	if (refSymbol == prop) {
	 	if (_getFloatArg(callPtr, 3, "level", &val, true) < 0)
	    return kMoaErr_NoErr;
	  fluid_synth_set_chorus(pObj->synth, chorusNumber, val, speed, depth, FLUID_CHORUS_MOD_SINE);
		goto ok;
	}
  pObj->pMmValue->StringToSymbol("number", &refSymbol);
	if (refSymbol == prop) {
	 	if (_getIntArg(callPtr, 3, "number", &valInt, true) < 0)
	    return kMoaErr_NoErr;
	  fluid_synth_set_chorus(pObj->synth, valInt, level, speed, depth, FLUID_CHORUS_MOD_SINE);
		goto ok;
	}
  pObj->pMmValue->StringToSymbol("speed", &refSymbol);
	if (refSymbol == prop) {
	 	if (_getFloatArg(callPtr, 3, "speed", &val, true) < 0)
	    return kMoaErr_NoErr;
	  fluid_synth_set_chorus(pObj->synth, chorusNumber, level, val, depth, FLUID_CHORUS_MOD_SINE);
		goto ok;
	}
  pObj->pMmValue->StringToSymbol("depth", &refSymbol);
	if (refSymbol == prop) {
	 	if (_getFloatArg(callPtr, 3, "depth", &val, true) < 0)
	    return kMoaErr_NoErr;
	  fluid_synth_set_chorus(pObj->synth, chorusNumber, level, speed, val, FLUID_CHORUS_MOD_SINE);
		goto ok;
	}
		
	char symbolstring[1024];
	pObj->pMmValue->SymbolToString(prop, symbolstring, 1024);
	fluid_log(FLUID_ERR, "unknown chorus property : %s", prop);
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  return kMoaErr_NoErr;

ok:
		pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	  return kMoaErr_NoErr;

}


/**************************************/
/****** SoundFont management **********/
/**************************************/


MoaError 
FLUIDXtra_IMoaMmXScript::loadSoundFont(PMoaDrCallInfo callPtr)
{
  MoaMmValue value;
  MoaChar filename[1024];
  
  GetArgByIndex(2, &value);
	if (!_checkValueType(&value, kMoaMmValueType_String)) {
  	fluid_log(FLUID_ERR, "filepathOrName should be a string");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	return kMoaErr_NoErr;
  }

	_valueToString(pObj, &value, filename, 1024);

	// absolute or relative
	if (!sfPathContainsSeparators(filename)) {
		// from downloadFolder
		if ((pObj->pDownloadPath[0] == 0) && _isInShockwave(callPtr))
			sfGetSchockwaveSupportFolderPath(pObj->pDownloadPath, 1024);

		if (pObj->pDownloadPath[0] == 0) {
			fluid_log(FLUID_ERR, "downloadFolder does not exist\n");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_SUPPORTFOLDERNOTFOUND, &callPtr->resultValue);
			return kMoaErr_NoErr;
		}

		char localPath[2048];
		sfBuildPathFromFolderAndFile(localPath, 2048, pObj->pDownloadPath, filename);
		int exists = sfFileExists(localPath, 2048);
		if (exists) {
			// use it
			strncpy(filename, localPath, 1024);
		}
	}

  fluid_log(FLUID_DBG, "fluidsynth: fluid_synth_sfload '%s'", filename);

  pathHFS2POSIX(filename, 1024);
  int sfID = fluid_synth_sfload(pObj->synth, (const char*) filename, true);
  if (sfID < 0) {
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
  }
		
	// store type
	fluid_xtra_stack_item *new_item = (fluid_xtra_stack_item *)pObj->pCalloc->NRAlloc(sizeof(fluid_xtra_stack_item));
	new_item->sfID = sfID;
	new_item->type = fluid_xtra_soundfont_type_file;
	
	// prepend
	new_item->next = pObj->soundFontStack;
	pObj->soundFontStack = new_item;
	
	pObj->pMmValue->IntegerToValue(sfID, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

int 
FLUIDXtra_IMoaMmXScript::createSoundFont(PMoaDrCallInfo callPtr, bool fromLingo)
{
	char name[21];
	name[0] = 0;
	int sfID;

	fluid_sfont_t* sfont = fluid_ramsfont_create_sfont();
	if (sfont == NULL) {
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return FLUIDXTRAERR_FLUIDERR;
	}
	
	// find name in callPtr 
  if (fromLingo && callPtr->nargs > 1) {
  	MoaMmValue value;
	  GetArgByIndex(2, &value);
		if (!_checkValueType(&value, kMoaMmValueType_String)) {
	  	fluid_log(FLUID_ERR, "name should be a string");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	  	return FLUIDXTRAERR_BADARGUMENT;
	  }
  	_valueToString(pObj, &value, name, 21);

		// set name
		fluid_ramsfont_t* ramsf =(fluid_ramsfont_t *)sfont->data;
		fluid_ramsfont_set_name(ramsf, name);
	}
	
  sfID = fluid_synth_add_sfont(pObj->synth, sfont);
	if (sfID < 0) {
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return FLUIDXTRAERR_FLUIDERR;
	}
		
	// store type
	fluid_xtra_stack_item *new_item = (fluid_xtra_stack_item *)pObj->pCalloc->NRAlloc(sizeof(fluid_xtra_stack_item));
	new_item->sfID = sfID;
	new_item->type = fluid_xtra_soundfont_type_ram;
	
	// prepend
	new_item->next = pObj->soundFontStack;
	pObj->soundFontStack = new_item;
	
	pObj->pMmValue->IntegerToValue(sfID, &callPtr->resultValue);
	return sfID;
}

fluid_xtra_sample_t*
FLUIDXtra_IMoaMmXScript::_getRamSample(unsigned int sampleID) {
	fluid_xtra_sample_t *tmp = pObj->ramsamples;
	while (tmp) {
		if (tmp->sampleID == sampleID)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

void
FLUIDXtra_IMoaMmXScript::_deleteRamSample(unsigned int sampleID) {
	fluid_xtra_sample_t *prev = NULL, *tmp = pObj->ramsamples;
	bool done = false;
	while (tmp && !done) {
		if (tmp->sampleID == sampleID) {
			if (prev) {
				prev->next = tmp->next;
			} else {
				tmp = pObj->ramsamples;
				pObj->ramsamples = tmp->next;
			}
			pObj->pCalloc->NRFree(tmp);
			done = true;
		} else {
			prev = tmp;
			tmp = tmp->next;
		}
	}
}

void
FLUIDXtra_IMoaMmXScript::_deleteRamSamplesForRamsfont(fluid_ramsfont_t *sfont) {
	fluid_xtra_sample_t *prev = NULL, *tmp = pObj->ramsamples, *next = NULL;
	while (tmp) {
		if (tmp->sfont == sfont) {
			if (prev) {
				prev->next = tmp->next;
			} else {
				tmp = pObj->ramsamples;
				pObj->ramsamples = tmp->next;
			}
			next = tmp->next;
			pObj->pCalloc->NRFree(tmp);
			tmp = next;
		} else {
			prev = tmp;
			tmp = tmp->next;
		}
	}
}

MoaError 
FLUIDXtra_IMoaMmXScript::deleteSample(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	// remove the izone, which stops the voices still running
	fluid_ramsfont_remove_izone(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample);
	if (ramSample->sample2) fluid_ramsfont_remove_izone(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2);
	
	// We can delete the samples not, as the voices are stopped
	if (ramSample->sample) delete_fluid_ramsample(ramSample->sample);
	if (ramSample->sample2) delete_fluid_ramsample(ramSample->sample2);
	
	// remove from our datastruct
	_deleteRamSample(sampleID);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getKeyRange(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	pObj->pMmList->NewListValue(&callPtr->resultValue);
	
	MoaMmValue element;
	pObj->pMmValue->IntegerToValue(ramSample->keylo, &element);
	pObj->pMmList->AppendValueToList(&callPtr->resultValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	
	pObj->pMmValue->IntegerToValue(ramSample->keyhi, &element);
	pObj->pMmList->AppendValueToList(&callPtr->resultValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	
	return kMoaErr_NoErr;
}


MoaError 
FLUIDXtra_IMoaMmXScript::getSampleName(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	fluid_sample_t *sample = ramSample->sample;
	char *name = sample->name;
	
	_stringToValue(pObj, name, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getRootKey(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	fluid_sample_t *sample = ramSample->sample;
	unsigned int rootKey = sample->origpitch;
	
	pObj->pMmValue->IntegerToValue(rootKey, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getFrameCount(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	fluid_sample_t *sample = ramSample->sample;
	unsigned int frameCount = sample->end - sample->start;
	
	pObj->pMmValue->IntegerToValue(frameCount, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getFrameRate(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
		fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	fluid_sample_t *sample = ramSample->sample;
	unsigned int samplerate = sample->samplerate;
	
	pObj->pMmValue->IntegerToValue(samplerate, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setLoop(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	MoaLong loopLong;
  // get the loopLong
 	if (_getIntArg(callPtr, 3, "isLooped", &loopLong, true) < 0)
    return kMoaErr_NoErr;
	if (loopLong != 0) loopLong = 1;
	
	ramSample->isLooped = loopLong;
	fluid_ramsfont_izone_set_loop(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample, loopLong, ramSample->loopstart, ramSample->loopend);
	if (ramSample->sample2)
		fluid_ramsfont_izone_set_loop(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2, loopLong, ramSample->loopstart, ramSample->loopend);
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setLoopPoints(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	MoaDouble loopStart, loopEnd;
  // get the loopStart
 	if (_getFloatArg(callPtr, 3, "loopStart", &loopStart, true) < 0)
    return kMoaErr_NoErr;
  // get the loopEnd
 	if (_getFloatArg(callPtr, 4, "loopEnd", &loopEnd, true) < 0)
    return kMoaErr_NoErr;
	
	// checks that :
	// - loopstart is >= 0 and loopend is <= 0
	// - we have at least 32 frames between the looppoints
	// Note that this may fail if the sample if too short => unknown results
	fluid_sample_t *sample = ramSample->sample;
	if (loopStart < 0) loopStart = 0.;
	if (loopEnd > 0) loopEnd = 0.;
	int MINLOOP = 64; // just to be sure
	if (sample->loopstart + loopStart > sample->loopend + loopEnd - MINLOOP) {
			loopEnd =  sample->loopstart + loopStart - sample->loopend + MINLOOP;
			// be sure that we are in the sample after the shift
			if (loopEnd > 0) {
				loopStart -= loopEnd;
				loopEnd = 0.;
			}
	}
		
	// set and remember
	ramSample->loopstart = (float)loopStart;
	ramSample->loopend = (float)loopEnd;
	if (ramSample->isLooped) {
		fluid_ramsfont_izone_set_loop(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample, true, ramSample->loopstart, ramSample->loopend);
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_loop(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2, true, ramSample->loopstart, ramSample->loopend);
	}
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	return kMoaErr_NoErr;
}


MoaError 
FLUIDXtra_IMoaMmXScript::getLoop(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
		
	pObj->pMmValue->IntegerToValue(ramSample->isLooped, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getLoopPoints(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
		
	pObj->pMmList->NewListValue(&callPtr->resultValue);
	
	MoaMmValue element;
	pObj->pMmValue->FloatToValue(ramSample->loopstart, &element);
	pObj->pMmList->AppendValueToList(&callPtr->resultValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	
	pObj->pMmValue->FloatToValue(ramSample->loopend, &element);
	pObj->pMmList->AppendValueToList(&callPtr->resultValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::setEnvelope(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
  MoaMmValue 				tempArg;
	GetArgByIndex(3, &tempArg);
	if (!_checkValueType(&tempArg, kMoaMmValueType_PropList)) {
  	fluid_log(FLUID_ERR, "envelopeInfo should be a plist");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	MoaDouble delay, attack, hold, sustainLevel, decay, release;

	if (_getFloatProp(callPtr, tempArg, "delay", "delay", &delay, false) == -1) {
	  	return kMoaErr_NoErr;
	} else {
		if (delay < 0.0) delay = 0.0;
		ramSample->delay = (float)delay;
		fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample,
			GEN_VOLENVDELAY, (float)(1200.0*log2(delay/1000.0)));
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2,
				GEN_VOLENVDELAY, (float)(1200.0*log2(delay/1000.0)));
	}
	
	if (_getFloatProp(callPtr, tempArg, "attack", "attack", &attack, false) == -1) {
	  	return kMoaErr_NoErr;
	} else {
		if (attack < 0.0) attack = 0.0;
		ramSample->attack = (float)attack;
		fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample,
			GEN_VOLENVATTACK, (float)(1200.0*log2(attack/1000.0)));
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2,
				GEN_VOLENVATTACK, (float)(1200.0*log2(attack/1000.0)));
	}

	if (_getFloatProp(callPtr, tempArg, "hold", "hold", &hold, false) == -1) {
	  	return kMoaErr_NoErr;
	} else {
		if (hold < 0.0) hold = 0.0;
		ramSample->hold = (float)hold;
		fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample,
			GEN_VOLENVHOLD, (float)(1200.0*log2(hold/1000.0)));
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2,
				GEN_VOLENVHOLD, (float)(1200.0*log2(hold/1000.0)));
	}
	
	if (_getFloatProp(callPtr, tempArg, "decay", "decay", &decay, false) == -1) {
	  	return kMoaErr_NoErr;
	} else {
		if (decay < 0.0) decay = 0.0;
		ramSample->decay = (float)decay;
		fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample,
			GEN_VOLENVDECAY, (float)(1200.0*log2(decay/1000.0)));
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2,
				GEN_VOLENVDECAY, (float)(1200.0*log2(decay/1000.0)));
	}
	
	if (_getFloatProp(callPtr, tempArg, "sustainLevel", "sustainLevel", &sustainLevel, false) == -1) {
	  	return kMoaErr_NoErr;
	} else {
		if (sustainLevel < 0.0) sustainLevel = 0.0;
		ramSample->sustainLevel = (float)sustainLevel;
		int sustainInt = (int)sustainLevel;
		fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample,
			GEN_VOLENVSUSTAIN, (float)(sustainInt*10));
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2,
				GEN_VOLENVSUSTAIN, (float)(sustainInt*10));
	}

	if (_getFloatProp(callPtr, tempArg, "release", "release", &release, false) == -1) {
	  	return kMoaErr_NoErr;
	} else {
		if (release < 0.0) release = 0.0;
		ramSample->release = (float)release;
		fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample,
			GEN_VOLENVRELEASE, (float)(1200.0*log2(release/1000.0)));
		if (ramSample->sample2)
			fluid_ramsfont_izone_set_gen(ramSample->sfont, ramSample->bank, ramSample->num, ramSample->sample2,
				GEN_VOLENVRELEASE, (float)(1200.0*log2(release/1000.0)));
	}
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getEnvelope(PMoaDrCallInfo callPtr) {
	MoaLong 	sampleIDlong = -1;
	unsigned int sampleID;
	
  // get the sampleIDlong
 	if (_getIntArg(callPtr, 2, "sampleID", &sampleIDlong, true) < 0)
    return kMoaErr_NoErr;
	sampleID = (unsigned int)sampleIDlong;
	
	fluid_xtra_sample_t* ramSample = _getRamSample(sampleID);
	if (!ramSample) {
  	fluid_log(FLUID_ERR, "no such sampleID %ld", sampleID);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	// ok good sample ID
	
	MoaMmValue element;
	MoaMmSymbol symbol = 0;
  MoaMmValue symbolValue;
	pObj->pMmList->NewPropListValue(&callPtr->resultValue);

	// Get delay
	pObj->pMmValue->StringToSymbol("delay", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->FloatToValue(ramSample->delay, &element);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// Get attack
	pObj->pMmValue->StringToSymbol("attack", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->FloatToValue(ramSample->attack, &element);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// Get hold
	pObj->pMmValue->StringToSymbol("hold", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->FloatToValue(ramSample->hold, &element);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// Get decay
	pObj->pMmValue->StringToSymbol("decay", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->FloatToValue(ramSample->decay, &element);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// Get sustainLevel
	pObj->pMmValue->StringToSymbol("sustainLevel", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->FloatToValue(ramSample->sustainLevel, &element);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// Get release
	pObj->pMmValue->StringToSymbol("release", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->FloatToValue(ramSample->release, &element);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &element);
	pObj->pMmValue->ValueRelease(&element);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	return kMoaErr_NoErr;
}

// If needed, and if provided, left and right buffers are used
// If data, left and right are owned by the function : used or freed
MoaError 
FLUIDXtra_IMoaMmXScript::_addSample(PMoaDrCallInfo callPtr, int sfontArgNb, short* data, short* left, short* right, long nbframes, int framerate, int nbChannels)
{
	/* parse arguments:
		presetNum or
		[	#number:presetNum
			<,#bank:bank>
			<,#soundFont:stackID>
			<,#name:name>
			<,#presetname:presetname>
			<,#rootKey:rootKey>
			<,#keyRangeStart:keyRangeStart>
			<,#keyRangeEnd:keyRangeEnd>
			<,#loop:loop>
			<,#loopstart:loopstart>
			<,#loopend:loopend>
			<,#attack:attack>
			<,#decay:decay>
			<,#sustainLevel:sustainLevel>
			<,#release:release>
		]
	*/
  MoaMmValue 				tempArg;
  MoaError 					err;
	MoaLong 					rootkey = 60;
	MoaLong 					bank = 0;
	MoaLong 					presetNum = 1;
	MoaLong 					lokey = 0, hikey = 127;
	char 							sampleName[21];
	char							presetName[21];
	MoaLong						loop = false;
	MoaDouble 				delay = 0.0;
	MoaDouble 				attack = 0.0;
	MoaDouble 				hold = 0.0;
	MoaDouble 				decay = 0.0;
	MoaDouble 				sustainLevel = 0.0;
	MoaDouble 				release = 0.0;
	MoaDouble 				loopstart = 0.0;
	MoaDouble 				loopend = 0.0;
	MoaLong						sfID = -1;
	
	sampleName[0] = 0;
	presetName[0] = 0;
	
  // get the sfID
 	if (_getIntArg(callPtr, sfontArgNb++, "soundFontID", &sfID, true) < 0)
    return kMoaErr_NoErr;
	
	GetArgByIndex(sfontArgNb++, &tempArg);
  if (_checkValueType(&tempArg, kMoaMmValueType_Integer)) {
		pObj->pMmValue->ValueToInteger(&tempArg, &presetNum);
  	
	} else if (_checkValueType(&tempArg, kMoaMmValueType_PropList)) {
		/* plist : find all properties */
		
		if (_getIntProp(callPtr, tempArg, "number", "number", &presetNum, true) != kMoaErr_NoErr)
		  return kMoaErr_NoErr;
		if (presetNum < 0) presetNum = 0;
		if (presetNum > 127) presetNum = 127;

		if (_getIntProp(callPtr, tempArg, "bank", "bank", &bank, false) == -1)
		  	return kMoaErr_NoErr;
		if (bank < 0) bank = 0;
		if (bank > 16383) bank = 16383;

		if (_getIntProp(callPtr, tempArg, "rootKey", "rootKey", &rootkey, false) == -1)
		  	return kMoaErr_NoErr;
		if (rootkey < 0) rootkey = 0;
		if (rootkey > 127) rootkey = 127;

		if (_getIntProp(callPtr, tempArg, "keyRangeStart", "keyRangeStart", &lokey, false) == -1)
		  	return kMoaErr_NoErr;
		if (lokey < 0) lokey = 0;
		if (lokey > 127) lokey = 127;

		if (_getIntProp(callPtr, tempArg, "keyRangeEnd", "keyRangeEnd", &hikey, false) == -1)
		  	return kMoaErr_NoErr;
		if (hikey < 0) hikey = 0;
		if (hikey > 127) hikey = 127;

		if (_getIntProp(callPtr, tempArg, "loop", "loop", &loop, false) == -1)
		  	return kMoaErr_NoErr;
		if (loop < 0) loop = 0;
		if (loop > 1) loop = 1;

		if (_getFloatProp(callPtr, tempArg, "loopstart", "loopstart", &loopstart, false) == -1)
		  	return kMoaErr_NoErr;
		
		if (_getFloatProp(callPtr, tempArg, "loopend", "loopend", &loopend, false) == -1)
		  	return kMoaErr_NoErr;
		
		if (_getFloatProp(callPtr, tempArg, "delay", "delay", &delay, false) == -1)
		  	return kMoaErr_NoErr;
		if (delay < 0.0) delay = 0.0;
		
		if (_getFloatProp(callPtr, tempArg, "attack", "attack", &attack, false) == -1)
		  	return kMoaErr_NoErr;
		if (attack < 0.0) attack = 0.0;
		
		if (_getFloatProp(callPtr, tempArg, "hold", "hold", &hold, false) == -1)
		  	return kMoaErr_NoErr;
		if (hold < 0.0) hold = 0.0;
		
		if (_getFloatProp(callPtr, tempArg, "decay", "decay", &decay, false) == -1)
		  	return kMoaErr_NoErr;
		if (decay < 0.0) decay = 0.0;

		if (_getFloatProp(callPtr, tempArg, "sustainLevel", "sustainLevel", &sustainLevel, false) == -1)
		  	return kMoaErr_NoErr;
		if (sustainLevel < 0.0) sustainLevel = 0.0;

		if (_getFloatProp(callPtr, tempArg, "release", "release", &release, false) == -1)
		  	return kMoaErr_NoErr;
		if (release < 0.0) release = 0.0;

		if (_getAnsiStringProp(callPtr, tempArg, "name", "name", sampleName, 20, false) == -1)
		  	return kMoaErr_NoErr;
		  	
		if (_getAnsiStringProp(callPtr, tempArg, "presetname", "presetname", presetName, 20, false) == -1)
		  	return kMoaErr_NoErr;
		
	} else {
  	fluid_log(FLUID_ERR, "presetInfo should be a plist or an int");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}
	
	// check sfID
	fluid_xtra_stack_item *tmp = pObj->soundFontStack;
	bool done = false;
	while (tmp && !done) {
		if (tmp->sfID == sfID) {
			if (tmp->type != fluid_xtra_soundfont_type_ram) {
		  	fluid_log(FLUID_ERR, "soundFontID should refer to a memory soundfont");
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
				return kMoaErr_NoErr;
			}
			done = true;
		}
		tmp = tmp->next;
	}
	
	/* all is ok : add the sample */
	unsigned int 				sampleID = -1;
	fluid_sample_t* 		sample = NULL;
	fluid_sample_t* 		sample2 = NULL;
	fluid_sfont_t* 			sfont = NULL;
	fluid_ramsfont_t* 	ramsf = NULL;
	short *tmp1 = NULL, *tmp2 = NULL;
	{
		// add sample
		
		// get sfont
		sfont = fluid_synth_get_sfont_by_id(pObj->synth, sfID);
		if (sfont == NULL) {
  		err = FLUIDXTRAERR_FLUIDERR;
			goto error_cleanup;
		}
		/* get the ramsf */
		ramsf = (fluid_ramsfont_t *)sfont->data;

		/* create 1 or 2 samples */
		sample = new_fluid_ramsample();
		if (sample == NULL) {
  		err = FLUIDXTRAERR_FLUIDERR;
			goto error_cleanup;
		}
		if (nbChannels == 2) {
			sample2 = new_fluid_ramsample();
			if (sample == NULL) {
	  		err = FLUIDXTRAERR_FLUIDERR;
				goto error_cleanup;
			}
		}
		
		/* set name */
		fluid_sample_set_name(sample, sampleName);
		if (sample2) fluid_sample_set_name(sample2, sampleName);

		if (nbChannels == 2) {
			// stereo
			// must de-interleave the samples...
            if (left != NULL) {
                tmp1 = left;
            } else {
                tmp1 = fluidxtra_malloc_buffer(sizeof(short)*nbframes);
                if (!tmp1) {
                    err = FLUIDXTRAERR_NOMEMORY;
                    goto error_cleanup;
                }
            }
            if (right != NULL) {
                tmp2 = right;
            } else {
                tmp2 = fluidxtra_malloc_buffer(sizeof(short)*nbframes);
                if (!tmp2) {
                    err = FLUIDXTRAERR_NOMEMORY;
                    goto error_cleanup;
                }
            }
			
			// de-interleave
            long i;
			short *tmp = data, *tmp11 = tmp1, *tmp22 = tmp2;
			for (i = 0 ; i < nbframes ; i++) {
				*tmp11++ = *tmp++;
				*tmp22++ = *tmp++;
			}
			
            // We can free data here.
            fluidxtra_free_buffer(data);
            data = NULL;
            
			// store (do not copy..)
			err = fluid_sample_set_sound_data(sample, tmp1, nbframes, false, rootkey);
			if (err != 0) {
                err = FLUIDXTRAERR_FLUIDERR;
				goto error_cleanup;
			}
			tmp1 = NULL;
			
			err = fluid_sample_set_sound_data(sample2, tmp2, nbframes, false, rootkey);
			if (err != 0) {
                err = FLUIDXTRAERR_FLUIDERR;
				goto error_cleanup;
			}
			tmp2 = NULL;
									
		} else {
			// mono : one sample
			/* fill sample with data */
			err = fluid_sample_set_sound_data(sample, data, nbframes, false, rootkey);
			if (err != 0) {
                err = FLUIDXTRAERR_FLUIDERR;
				goto error_cleanup;
			}
		}

		/* set framerate */
		/* We do this after fluid_sample_set_sound_data as fluid_sample_set_sound_data sets the framerate to 44100, which is a bug in fluidSynth BTW */
		sample->samplerate = framerate;
		if (sample2) sample2->samplerate = framerate;
		
		/* assign a sampleID and store description */
		sampleID = pObj->curSampleID;
		{
			fluid_xtra_sample_t *newOne = (fluid_xtra_sample_t *)pObj->pCalloc->NRAlloc(sizeof(fluid_xtra_sample_t));
			if (!newOne) {
                err = FLUIDXTRAERR_NOMEMORY;
				goto error_cleanup;
			}
			
			newOne->sampleID = sampleID;
			newOne->sample = sample;
			newOne->sample2 = sample2;
			newOne->sfont = ramsf;
			newOne->bank = bank;
			newOne->num = presetNum;
			newOne->isLooped = loop;
			newOne->loopstart = (float)loopstart;
			newOne->loopend = (float)loopend;
			newOne->delay = (float)delay;
			newOne->attack = (float)attack;
			newOne->hold = (float)hold;
			newOne->decay = (float)decay;
			newOne->sustainLevel = (float)sustainLevel;
			newOne->release = (float)release;
			newOne->keylo = lokey;
			newOne->keyhi = hikey;

			newOne->next = NULL;
			
			// prepend
			newOne->next = pObj->ramsamples;
			pObj->ramsamples = newOne;
		
			// next nb
			pObj->curSampleID++;
		}
				
		/* add it to soundfont */
		/* we use a trick here to assign the name to the preset, if needed: we change temporarily the name of the sample
		This is because we dont have any API to set the name. Change this when there is one. */
		fluid_sample_set_name(sample, presetName);
		if (sample2) fluid_sample_set_name(sample2, presetName);
		
		err = fluid_ramsfont_add_izone(ramsf, bank, presetNum, sample, lokey, hikey);
		if (err != 0) {
  		err = FLUIDXTRAERR_FLUIDERR;
			goto error_cleanup;
		}
		if (sample2) {
			err = fluid_ramsfont_add_izone(ramsf, bank, presetNum, sample2, lokey, hikey);
			if (err != 0) {
	  		err = FLUIDXTRAERR_FLUIDERR;
				goto error_cleanup;
			}
		}
		/* put it back */
		fluid_sample_set_name(sample, sampleName);
		if (sample2) fluid_sample_set_name(sample2, sampleName);
		
		if (sample2) {
			// stereo : set the pans
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_PAN, -500.);
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_PAN, 500.);
		}
		
		/* set generators for envelope */
		// loop, attack, decay, sustainLevel, release
		if (delay != 0.0) {
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_VOLENVDELAY, (float)(1200.0*log2(delay/1000.0)));
			if (sample2) fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_VOLENVDELAY, (float)(1200.0*log2(delay/1000.0)));
		}
		
		if (attack != 0.0) {
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_VOLENVATTACK, (float)(1200.0*log2(attack/1000.0)));
			if (sample2) fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_VOLENVATTACK, (float)(1200.0*log2(attack/1000.0)));
		}
		
		if (hold != 0.0) {
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_VOLENVHOLD, (float)(1200.0*log2(hold/1000.0)));
			if (sample2) fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_VOLENVHOLD, (float)(1200.0*log2(hold/1000.0)));
		}
		
		if (decay != 0.0) {
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_VOLENVDECAY, (float)(1200.0*log2(decay/1000.0)));
			if (sample2) fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_VOLENVDECAY, (float)(1200.0*log2(decay/1000.0)));
		}
		
		if (sustainLevel != 0.0) {
			int sustainInt = (int)sustainLevel;
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_VOLENVSUSTAIN, (float)(sustainInt*10));
			if (sample2) fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_VOLENVSUSTAIN, (float)(sustainInt*10));
		}
		
		if (release != 0.0) {
			fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample, GEN_VOLENVRELEASE, (float)(1200.0*log2(release/1000.0)));
			if (sample2) fluid_ramsfont_izone_set_gen(ramsf, bank, presetNum, sample2, GEN_VOLENVRELEASE, (float)(1200.0*log2(release/1000.0)));
		}
		
		if (loop) {
			fluid_ramsfont_izone_set_loop(ramsf, bank, presetNum, sample, true, (float)loopstart, (float)loopend);
			if (sample2) fluid_ramsfont_izone_set_loop(ramsf, bank, presetNum, sample2, true, (float)loopstart, (float)loopend);
		}
	}
	
	pObj->pMmValue->IntegerToValue(sampleID, &callPtr->resultValue);
	return kMoaErr_NoErr;

error_cleanup:
	if (sampleID != -1) _deleteRamSample(sampleID);
	
	if (sample) {
		if (ramsf) fluid_ramsfont_remove_izone(ramsf, bank, presetNum, sample);
		delete_fluid_ramsample(sample);
	}
	if (sample2) {
		if (ramsf) fluid_ramsfont_remove_izone(ramsf, bank, presetNum, sample2);
		delete_fluid_ramsample(sample2);
	}
	
	pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
	
	return err;
}

#ifdef _DIRECTOR_XCODE				
#ifdef __APPLE__
// mac

void Swap2Bytes( short * pData ) {
		(*pData) = (( (*pData) >> 8 ) & 0x00FF ) + (( (*pData) << 8 ) & 0xFF00 );
}

void Swap4Bytes( long * pData ) { 	(*pData) = (( (*pData) >> 24 ) & 0x000000FFL ) + (( (*pData) >> 8 ) &
			0x0000FF00L ) + (( (*pData) << 8 ) & 0xFF0000L ) +
			(( (*pData) << 24 ) & 0xFF000000L );
}

/*
	This code parses a 'snd ' resource header.  While Sound Manager 3.2 will
	do this for you, this is for education (and entertainment).  Just in case
	the Sound Manager routine is better, try it first, if it's available.
*/

typedef union {
	SoundHeaderPtr			standardHeaderPtr;
	ExtSoundHeaderPtr		extendedHeaderPtr;
	CmpSoundHeaderPtr		compressedHeaderPtr;
} headerTemplate, *headerTemplatePtr;

#define kBufferCmd        0x8051
#define kSampledSound      5
#define kChannelsMask      0xDF
#define kMono          1
#define kStereo          2
#define k8BitSample        8

/*-----------------------------------------------------------------------*/
		OSErr	MyParseSndHeader		(SndListHandle theSoundHeader,
										SoundComponentData *sndInfo,
										unsigned long *numFrames,
										unsigned long *dataOffset)
/*-----------------------------------------------------------------------*/
{
	NumVersion				SndManagerVer;
	headerTemplate			theHeader;
	long					headerOffset	= 0;
	OSErr					theErr			= noErr;
	//short					numChannels		= 0;
	short					numCommands		= 0;
	short					i				= 0;
	Boolean					parsed			= false;
	//unsigned char			headerFormat	= 0;
//	long					swapLong = 0;
//	short					swapShort = 0;
	Snd2ListPtr				snd2ptr;

	SndManagerVer = SndSoundManagerVersion ();

#ifdef __i386__
		//Yes, we have to byteswap the structure directly, as it is deprecated
		//see parsesoundheader.rtf for more information on the meaning of bytes
		Swap2Bytes(&(*theSoundHeader)->format);
#endif

	/* If it's not available, or it failed, let's try it ourselves */
	if (theErr != noErr || parsed == false) {
		theErr = noErr;
		switch ((*theSoundHeader)->format) {
			case firstSoundFormat:					/* Normal 'snd ' resource */
			//fluid_log(FLUID_ERR, "sound format is snd");
			#ifdef __i386__
					Swap2Bytes((short *) &(*theSoundHeader)->modifierPart->modNumber);
			#endif
				if ((*theSoundHeader)->modifierPart->modNumber != kSampledSound) {
					theErr = badFormat;				/* can only deal with sampled-sound data */
				}
				else {
					#ifdef __i386__
						Swap2Bytes(&(*theSoundHeader)->numCommands);
					#endif
					numCommands = (*theSoundHeader)->numCommands;
					if (numCommands != 1) {
						theErr = badFormat;			/* can only deal with one sound per resource, for now */
					}
					else {
						for (i = 0; i < numCommands; i++) {
							#ifdef __i386__
								Swap2Bytes((short *) &(*theSoundHeader)->commandPart->cmd);
							#endif
							if ((*theSoundHeader)->commandPart->cmd == kBufferCmd) {
								#ifdef __i386__
									Swap4Bytes(&(*theSoundHeader)->commandPart->param2);
								#endif
								headerOffset = (*theSoundHeader)->commandPart->param2;
							}
							else {
								theErr = badFormat;	/* can only deal with sampled-sound data */
							}
						}
					}
				}
				break;
			case secondSoundFormat:					/* Hypercard 'snd ' resource */
				//structure is different
				//fluid_log(FLUID_ERR, "sound format is Hypercard snd");
				snd2ptr = (Snd2ListPtr) *theSoundHeader;
				numCommands = snd2ptr->numCommands;
				#ifdef __i386__
					Swap2Bytes(&numCommands);
				#endif
				if (numCommands != 1) {
					theErr = badFormat;				/* can only deal with one sound per resource, for now */
				}
				else {
					for (i = 0; i < numCommands; i++) {
						#ifdef __i386__
							Swap2Bytes((short *) &snd2ptr->commandPart->cmd);
						#endif
						if (snd2ptr->commandPart->cmd == kBufferCmd) {
							#ifdef __i386__
									Swap4Bytes(&snd2ptr->commandPart->param2);
							#endif
							headerOffset = snd2ptr->commandPart->param2;
						}
						else {
							theErr = badFormat;		/* can only deal with sampled-sound data */
						}
					}
				}
				break;
			default:
				theErr = badFormat;					/* unknown resource format */
		}
		if (theErr == noErr) {
			theHeader.standardHeaderPtr = (SoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);
			// [AS May 12] Fix a bug in D11.x, where the macSnd format is broken. The encode is wrong. It is 0xFD where it should be 0xFF (standard stdSH)
			if (theHeader.standardHeaderPtr->encode == 0xFD)
					theHeader.standardHeaderPtr->encode = 0xFF;
			// error here with sound members...
			//fluid_log(FLUID_ERR, "encode=%d stdSH=%d extSH=%d cmpSH=%d", theHeader.standardHeaderPtr->encode, stdSH, extSH, cmpSH);
			switch (theHeader.standardHeaderPtr->encode) {
				case stdSH:
					theHeader.standardHeaderPtr = (SoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);
					sndInfo->format			= kOffsetBinary;	/* Can only be raw sounds */
					#ifdef __i386__
						Swap4Bytes(&(*theSoundHeader)->modifierPart->modInit);
					#endif
					switch ((*theSoundHeader)->modifierPart->modInit & kChannelsMask) {
						case initMono:
							sndInfo->numChannels	= kMono;
							break;
						case initStereo:
							sndInfo->numChannels	= kStereo;
							break;
						default:
//							theErr = badFormat;					/* unknown number of channels */
							sndInfo->numChannels	= kMono;
							break;
					}
					sndInfo->sampleSize		= k8BitSample;		/* Can only be 8 bit sounds */
					#ifdef __i386__
						Swap4Bytes((long *) &theHeader.standardHeaderPtr->sampleRate);
						Swap4Bytes((long *) &theHeader.standardHeaderPtr->length);
					#endif
					sndInfo->sampleRate		= theHeader.standardHeaderPtr->sampleRate;
					sndInfo->sampleCount	= theHeader.standardHeaderPtr->length;
					*dataOffset				= headerOffset + sizeof (SoundHeader) - sizeof (short);
					break;
				case extSH:
					theHeader.extendedHeaderPtr = (ExtSoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);
					#ifdef __i386__
						Swap4Bytes((long *) &theHeader.extendedHeaderPtr->numChannels);
						Swap2Bytes((short *) &theHeader.extendedHeaderPtr->sampleSize);
						Swap4Bytes((long *) &theHeader.extendedHeaderPtr->sampleRate);
						Swap4Bytes((long *) &theHeader.extendedHeaderPtr->numFrames);
					#endif
					sndInfo->format			= kOffsetBinary;	/* Can only be raw sounds */
					// [as april 2008] No :
					if (theHeader.extendedHeaderPtr->sampleSize == 16)
						sndInfo->format = k16BitBigEndianFormat; // k16BitLittleEndianFormat for Intel ?
					sndInfo->numChannels	= theHeader.extendedHeaderPtr->numChannels;
					sndInfo->sampleSize		= theHeader.extendedHeaderPtr->sampleSize;
					sndInfo->sampleRate		= theHeader.extendedHeaderPtr->sampleRate;
					sndInfo->sampleCount	= theHeader.extendedHeaderPtr->numFrames;
					*dataOffset				= headerOffset + sizeof (ExtSoundHeader) - sizeof (short);
					break;
				case cmpSH:
					theHeader.compressedHeaderPtr = (CmpSoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);
					#ifdef __i386__
						//defined as OSType, 4 bytes : swap of not ??
						Swap4Bytes((long *) &theHeader.compressedHeaderPtr->format);
					#endif
					sndInfo->format			= theHeader.compressedHeaderPtr->format;
					if (sndInfo->format == 0) {
						#ifdef __i386__
							Swap2Bytes(&theHeader.compressedHeaderPtr->compressionID);
						#endif
						switch (theHeader.compressedHeaderPtr->compressionID) {
							case twoToOne:
								sndInfo->format 	= kULawCompression;
								break;
							case eightToThree:					/* I don't know what compressor this is */
								theErr = badFormat;
								break;
							case threeToOne:
								sndInfo->format 	= kMACE3Compression;
								break;
							case sixToOne:
								sndInfo->format 	= kMACE6Compression;
								break;
							default:
								//DebugPrint ("\pUnknown sound format");
								theErr = badFormat;
						}
					}
					#ifdef __i386__
						Swap4Bytes((long *)&theHeader.compressedHeaderPtr->numChannels);
						Swap2Bytes((short *)&theHeader.compressedHeaderPtr->sampleSize);
						Swap4Bytes((long *) &theHeader.compressedHeaderPtr->sampleRate);
						Swap4Bytes((long *)&theHeader.compressedHeaderPtr->numFrames);
					#endif
					sndInfo->numChannels	= theHeader.compressedHeaderPtr->numChannels;
					sndInfo->sampleSize		= theHeader.compressedHeaderPtr->sampleSize;
					sndInfo->sampleRate		= theHeader.compressedHeaderPtr->sampleRate;
					sndInfo->sampleCount	= theHeader.compressedHeaderPtr->numFrames;
					*dataOffset				= headerOffset + sizeof (CmpSoundHeader) - sizeof (short);
					break;
				default:
					theErr = badFormat;		/* A header format we don't know about */
			}
			*numFrames				= (unsigned long)sndInfo->sampleCount;
			sndInfo->flags			= 0;	/* ?? */
			sndInfo->buffer			= 0;	/* should always be 0, data follows header */
			sndInfo->reserved		= 0;	/* just set it to 0 */
		}

	}

	return theErr;
}

#endif
#endif


MoaError 
FLUIDXtra_IMoaMmXScript::loadSampleMember(PMoaDrCallInfo callPtr)
{
	MoaError			err = kMoaErr_NoErr;
  MoaMmValue 		tempArg;

	/********************************/
	/* Get first argument as member */
	/********************************/
	PIMoaDrCastMem 			pDrCastMem = NULL;
	PIMoaDrMovie 				pIMoaDrMovie = NULL;

	pObj->pDrPlayer->GetActiveMovie(&pIMoaDrMovie);
	
	GetArgByIndex(2, &tempArg);
	
	/* is it a CMRef */	
	PIMoaDrValue				pDrValue = NULL;
	MoaDrCMRef					cmRef;
	pObj->pCallback->QueryInterface(&IID_IMoaDrValue, (PPMoaVoid)&pDrValue);
	err = pDrValue->ValueToCMRef(&tempArg, &cmRef);
	pDrValue->Release();
	if (err == kMoaErr_NoErr) {
		/* it was directly a CMref */
		pIMoaDrMovie->GetCastMemFromCMRef(&cmRef, &pDrCastMem); 		
	} else {
		/* an int or a string ? */
		MoaDrMemberIndex	castMemIndex = 0;
		MoaDrCastIndex		castIndex = 0;
		MoaLong 					memberAndCastIndex = 0;
		PIMoaDrCast				pDrCast = NULL;
		
		if ((err = pObj->pMmValue->ValueToInteger(&tempArg, &memberAndCastIndex)) == kMoaErr_NoErr)	{
			/* int */
			/* we are returned a value = to (65536 * castIndex) + member Index*/
			castIndex = memberAndCastIndex / 65536;
			castMemIndex = memberAndCastIndex % 65536;
			
			/* Get cast */
			pIMoaDrMovie->GetNthCast(castIndex, &pDrCast);
		} else {
			/* if it's not an integer, assume it's a string */
			char	memName[1024];
			
			if ((err = pObj->pMmValue->ValueToString( &tempArg, memName, 1024)) == kMoaErr_NoErr) {
				MoaDrCMRef		CMRef = {0};
				err = pIMoaDrMovie->GetCMRefFromMemberName(memName, &CMRef);
				if (err == kMoaErr_NoErr) {
					castMemIndex = CMRef.memberIndex;
					pIMoaDrMovie->GetNthCast(CMRef.movieCastIndex, &pDrCast);
				}
			}
		}
		
		/* Get the cast member */
		if(pDrCast) err = pDrCast->GetCastMem(castMemIndex, &pDrCastMem);
		if(pDrCast) pDrCast->Release();
	}
	
	if (pIMoaDrMovie) pIMoaDrMovie->Release();
	if (pDrCastMem == NULL) {
		fluid_log(FLUID_ERR, "cannot find cast member");
		if (err != kMoaErr_NoErr) {
			pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
		} else {
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		}
		return kMoaErr_NoErr;
	}
	
	/*********************/
	/* cast member found */
	/*********************/
	
	PIMoaDrUtils			pDrUtils = NULL;
	MoaDrMediaInfo 		mediaInfo = {0};
		
	pObj->pCallback->QueryInterface(&IID_IMoaDrUtils, (PPMoaVoid)&pDrUtils);

	/* Get its media as Sound */
	pObj->pMmValue->StringToSymbol("Sound", &mediaInfo.labelSymbol);
	
#ifdef _WINDOWS
	// Windows RIFF WAVE GlobalHandle; RIFF sound format 
	pObj->pMmValue->StringToSymbol("winWAVE", &mediaInfo.formatSymbol);
#else
	// Macintosh sndHdl in Macintosh sound resource format
	pObj->pMmValue->StringToSymbol("macSnd", &mediaInfo.formatSymbol);
#endif

	/* Get access to the media */
	(void) pDrUtils->NewMediaInfo(mediaInfo.labelSymbol, mediaInfo.formatSymbol, NULL, kMoaDrMediaOpts_None, NULL, &mediaInfo);
	err = pDrCastMem->GetMedia(&mediaInfo);

	if (err == kMoaErr_NoErr) {
						
#ifdef _WINDOWS

		char * pmarker;
		
		char* buf = (char *)GlobalLock(mediaInfo.mediaData);
		pmarker = buf;
		if (buf != NULL) {
			DWORD *riffSize, *chunkSize;
			unsigned int *factSamples;
			WAVEFORMATEX *fmt;
			unsigned long			myNumFrames;
			bool more = true;
			bool dataFound = false;
			unsigned int sampleRate = 44100;
			int nbChannels = 1;
			
			pmarker = buf;

			while (more) {

				if (!strncmp(pmarker, "RIFF", 4)) {
					/* RIFF marker */
					pmarker += 4;

					/* get riffSize */
					riffSize = (DWORD*)pmarker;
					pmarker += sizeof(DWORD);

				} else if (!strncmp(pmarker, "WAVE", 4)) {
					/* WAVE marker */
					pmarker += 4;

				} else if (!strncmp(pmarker, "fact", 4)) {
					/* fact marker */
					pmarker += 4;

					/* get chunkSize */
					chunkSize = (DWORD *)pmarker;
					pmarker += sizeof(DWORD);
					
					/* get factSamples */
					factSamples = (unsigned int *)pmarker;
					pmarker += *chunkSize;

				} else if (!strncmp(pmarker, "fmt ", 4)) {
					/* "fmt " marker */
					pmarker += 4;

					/* get formatSize */
					chunkSize = (DWORD *)pmarker;
					pmarker += sizeof(DWORD);

					/* get WAVEFORMATEX */
					fmt = (WAVEFORMATEX*)pmarker;

					/* PCM, 16bits ? */
					if ((fmt->wFormatTag != WAVE_FORMAT_PCM) ||
							(fmt->wBitsPerSample != 16)) {
						
						fluid_log(FLUID_ERR, "Sound format not supported (16)");
						err = FLUIDXTRAERR_BADARGUMENT;
						pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
						goto exit_gracefully;
					}

					sampleRate = fmt->nSamplesPerSec;
					nbChannels = fmt->nChannels;
					
					pmarker += *chunkSize;
				} else if (!strncmp(pmarker, "data", 4)) {
					/* "data" marker */
					pmarker += 4;

					/* get chunkSize */
					chunkSize = (DWORD*)pmarker;
					pmarker += sizeof(DWORD);

					/* do it */
					myNumFrames = (*chunkSize)*8/(fmt->wBitsPerSample*nbChannels);
                    
                    short *newdata = fluidxtra_malloc_buffer(myNumFrames*nbChannels*sizeof(short));
                    if (newdata == NULL) {
 						fluid_log(FLUID_ERR, "NOT ENOUGH MEMORY");
						err = FLUIDXTRAERR_NOMEMORY;
						pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
						goto exit_gracefully;
                    }
                    memcpy(newdata, pmarker, myNumFrames*nbChannels*sizeof(short));
					_addSample(callPtr, 3, (short *)newdata, NULL, NULL, myNumFrames, sampleRate, nbChannels);

					pmarker += *chunkSize;
					dataFound = true;
					more = false;
				} else {
					more = false;
				}
			}	// while more

			if (!dataFound) {
				fluid_log(FLUID_ERR, "Bad Sound format, data not found");
				err = FLUIDXTRAERR_BADARGUMENT;
				pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
			}
			
exit_gracefully:
			GlobalUnlock(mediaInfo.mediaData);
			
		}

#else // macintosh

		OSErr					myErr = noErr;
		SoundComponentData		myResourceInfo;
		unsigned long			myNumFrames;
		unsigned long			myOffset;
		Handle					sndHdl = (Handle)mediaInfo.mediaData;

#ifdef _DIRECTOR_XCODE
		// On MacIntel, the ParseSndHeader function fails., so we wrote our own function (thanks to Mauricio Piacentini).	
		myErr = MyParseSndHeader((SndListHandle)sndHdl, &myResourceInfo, &myNumFrames, &myOffset); // MyParseSndHeader
#else
		myErr = ParseSndHeader((SndListHandle)sndHdl, &myResourceInfo, &myNumFrames, &myOffset);
#endif

		if (myErr == noErr) {
			
			if (myResourceInfo.format != k16BitBigEndianFormat) {
				fluid_log(FLUID_ERR, "Sound format not supported (16 bits only)");
				err = FLUIDXTRAERR_BADARGUMENT;
				pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
			} else {
				unsigned int samplerate = (myResourceInfo.sampleRate == rate44khz) ? 44100 : 
											((myResourceInfo.sampleRate == rate48khz) ? 48000 :
											((myResourceInfo.sampleRate == rate22050hz) ? 22050 :
											((myResourceInfo.sampleRate == rate22khz) ? 22254 :
											((myResourceInfo.sampleRate == rate11khz) ? 11127 :
											((myResourceInfo.sampleRate == rate11025hz) ? 11025 :
											 myResourceInfo.sampleRate
											)))));
				int nbChannels = myResourceInfo.numChannels;
								
				HLock(sndHdl);
				
				short * samples = (short *)(*sndHdl + myOffset);
									
#ifdef _DIRECTOR_XCODE
				// The shorts are big endian : swap them now
				#ifdef __i386__
					for (int kk = 0; kk < myNumFrames*nbChannels; kk ++)
						Swap2Bytes(&samples[kk]);
				#endif
#endif				
                short *newdata = fluidxtra_malloc_buffer(myNumFrames*nbChannels*sizeof(short));
                if (newdata == NULL) {
                    fluid_log(FLUID_ERR, "NOT ENOUGH MEMORY");
                    err = FLUIDXTRAERR_NOMEMORY;
                    pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
                } else {
                    memcpy(newdata, samples, myNumFrames*nbChannels*sizeof(short));
                    _addSample(callPtr, 3, samples, NULL, NULL, myNumFrames, samplerate, nbChannels);
                }

				HUnlock(sndHdl);
			}
		} else {
			fluid_log(FLUID_ERR, "Bad Sound format : ParseSndHeader err = %ld", myErr);
			err = FLUIDXTRAERR_BADARGUMENT;
			pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
		}

#endif // macintosh

		pDrUtils->MediaRelease(&mediaInfo); // will do equivalent of pObj->pHandle->Free((MoaHandle *)mediaInfo.mediaData);
		
	} else {
		/* media not found */
		fluid_log(FLUID_ERR, "cannot find Sound media in cast member");
		err = FLUIDXTRAERR_BADARGUMENT;
		pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
	}
	
	if (pDrUtils) pDrUtils->Release();
	if(pDrCastMem) pDrCastMem->Release();

	return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::getSampleFileSamples(PMoaDrCallInfo callPtr) {
    MoaError err;

    /* Load sample file */
    MoaMmValue value;
    MoaChar filename[1024];
    
    GetArgByIndex(2, &value);
	if (!_checkValueType(&value, kMoaMmValueType_String)) {
        fluid_log(FLUID_ERR, "filepath should be a string");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
        return kMoaErr_NoErr;
    }
    
    _valueToString(pObj, &value, filename, 1024);
    pathHFS2POSIX(filename, 1024);
    fluid_log(FLUID_DBG, "fluidsynth: loading sample file '%s'", filename);
    
	// read data
    long nbFileFrames;
	int nbChannels;
    err = fluid_sample_import_compute_file_samples(filename, &nbFileFrames, &nbChannels);
    if (err < 0) {
        if (err == FLUIDXTRAERR_OPENREADFILE) _fluid_xtra_setErrorString(pObj, (char *)"Cannot open file for reading");
        if (err == FLUIDXTRAERR_BADFILEFORMAT) _fluid_xtra_setErrorString(pObj, (char *)"Bad file format, should be 16bits");
		pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
        return kMoaErr_NoErr;
    }
    
    pObj->pMmValue->IntegerToValue(nbFileFrames, &callPtr->resultValue);
    
    return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::loadSampleFile(PMoaDrCallInfo callPtr) {
    return _loadSampleFile(callPtr, 3, 0, -1);
}

MoaError
FLUIDXtra_IMoaMmXScript::loadSampleFileExtract(PMoaDrCallInfo callPtr) {
    long startFrame;
    long nbFrames;
	int res;
    
	res = _getIntArg(callPtr, 3, "startFrame", &startFrame, true);
 	if (res == -1) return kMoaErr_NoErr;
    if (startFrame < 0) startFrame = 0;
    
	res = _getIntArg(callPtr, 4, "nbFrames", &nbFrames, true);
 	if (res == -1) return kMoaErr_NoErr;

    return _loadSampleFile(callPtr, 5, startFrame, nbFrames);
}

MoaError
FLUIDXtra_IMoaMmXScript::_loadSampleFile(PMoaDrCallInfo callPtr, int sfontArgNb, long startFrame, long nbFramesToRead)
{
  MoaError err;
	short *data = NULL;
	long nbFramesLoaded = 0;
    int framerate;
	int nbChannels;
	
  /* Load sample file */
  MoaMmValue value;
  MoaChar filename[1024];
  
  GetArgByIndex(2, &value);
	if (!_checkValueType(&value, kMoaMmValueType_String)) {
  	fluid_log(FLUID_ERR, "filepath should be a string");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
  	return kMoaErr_NoErr;
  }

  _valueToString(pObj, &value, filename, 1024);
  pathHFS2POSIX(filename, 1024);
  fluid_log(FLUID_DBG, "fluidsynth: loading sample file '%s'", filename);

	// read data
	data = NULL;
    long bytesNeeded, nbFileFrames;
    err = fluid_sample_import_compute_file_samples(filename, &nbFileFrames, &nbChannels);
    if (err < 0) {
        if (err == FLUIDXTRAERR_OPENREADFILE) _fluid_xtra_setErrorString(pObj, (char *)"Cannot open file for reading");
        if (err == FLUIDXTRAERR_BADFILEFORMAT) _fluid_xtra_setErrorString(pObj, (char *)"Bad file format, should be 16bits");
		pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
        return kMoaErr_NoErr;
    }
    
    // bytesNeeded
    if (startFrame < 0) startFrame = 0;
    long realFramesToRead = nbFileFrames - startFrame;
    if (nbFramesToRead >= 0) realFramesToRead = nbFramesToRead;
    if (realFramesToRead > nbFileFrames - startFrame) realFramesToRead = nbFileFrames - startFrame;
    bytesNeeded = realFramesToRead*nbChannels*2;
    
    data = fluidxtra_malloc_buffer(bytesNeeded);
    if (data == NULL) {
        _fluid_xtra_setErrorString(pObj, (char *)"Not enough memory");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NOMEMORY, &callPtr->resultValue);
        return kMoaErr_NoErr;
    }
    
    // premalloc the two channel buffers for stereo samples
    short * left = NULL, *right = NULL;
    if (nbChannels == 2) {
        left = fluidxtra_malloc_buffer(bytesNeeded/2);
        if (left == NULL) {
            _fluid_xtra_setErrorString(pObj, (char *)"Not enough memory");
            pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NOMEMORY, &callPtr->resultValue);
            
            fluidxtra_free_buffer(data);
            
            return kMoaErr_NoErr;
        }
        
        right = fluidxtra_malloc_buffer(bytesNeeded/2);
        if (right == NULL) {
            _fluid_xtra_setErrorString(pObj, (char *)"Not enough memory");
            pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NOMEMORY, &callPtr->resultValue);
 
            fluidxtra_free_buffer(left);
            fluidxtra_free_buffer(data);

            return kMoaErr_NoErr;
        }
    }
    
	err = fluid_sample_import_file(filename, data, startFrame, realFramesToRead, &nbFramesLoaded, &framerate, &nbChannels);
	if (err != kMoaErr_NoErr) {
        if (err == FLUIDXTRAERR_OPENREADFILE) _fluid_xtra_setErrorString(pObj, (char *)"Cannot open file for reading");
        if (err == FLUIDXTRAERR_BADFILEFORMAT) _fluid_xtra_setErrorString(pObj, (char *)"Bad file format, should be 16bits");
        if (err == FLUIDXTRAERR_SEEKFILE) _fluid_xtra_setErrorString(pObj, (char *)"Error seeking file, bad position");
		pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
        return kMoaErr_NoErr;
	}
	
	_addSample(callPtr, sfontArgNb, data, left, right, nbFramesLoaded, framerate, nbChannels);
	
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::unloadSoundFont(PMoaDrCallInfo callPtr)
{
  MoaLong sfID = -1;
  
  if (callPtr->nargs > 1) {
	  MoaMmValue value;
  	
	  GetArgByIndex(2, &value);
		if (!_checkValueType(&value, kMoaMmValueType_Integer)) {
	  	fluid_log(FLUID_ERR, "SoundFontID should be an int");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
			return kMoaErr_NoErr;
	  }

		pObj->pMmValue->ValueToInteger(&value, &sfID);
		
	} else {
		// get the id of the last soundfont (index = 0)
		if (fluid_synth_sfcount(pObj->synth) == 0) {
	  	fluid_log(FLUID_ERR, "No SoundFont in stack");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADSTACKID, &callPtr->resultValue);
			return kMoaErr_NoErr;
		}
		fluid_sfont_t * sfont = fluid_synth_get_sfont(pObj->synth, 0);
		sfID = sfont->id;
	}

  fluid_log(FLUID_DBG, "fluidsynth: unloading soundFont id '%d'", sfID);
  
  fluid_log(FLUID_DBG, "...removing 'type' struct and ramsamples");

	// remove the 'type' struct, and remove the samples attached to it, if it is a ramsfont
	fluid_xtra_stack_item *prev = NULL, *tmp = pObj->soundFontStack;
	bool done = false;
	while (tmp && !done) {
		if (tmp->sfID == sfID) {
			if (tmp->type == fluid_xtra_soundfont_type_ram) {
				fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(pObj->synth, sfID);
				fluid_ramsfont_t* ramsf =(fluid_ramsfont_t *)sfont->data;
				_deleteRamSamplesForRamsfont(ramsf);
			}
			// remove the type
			if (prev) {
				prev->next = tmp->next;
			} else {
				tmp = pObj->soundFontStack;
				pObj->soundFontStack = tmp->next;
			}
			pObj->pCalloc->NRFree(tmp);
			done = true;
		} else {
			prev = tmp;
			tmp = tmp->next;
		}
	}
		
  fluid_log(FLUID_ERR, "...fluid_synth_sfunload");
  int res = fluid_synth_sfunload(pObj->synth, sfID, true);
	if (res != 0) {
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
		return kMoaErr_NoErr;
	}	

	pObj->pMmValue->IntegerToValue(kMoaErr_NoErr, &callPtr->resultValue);
  return(kMoaErr_NoErr);
}

MoaError 
FLUIDXtra_IMoaMmXScript::reloadSoundFont(PMoaDrCallInfo callPtr)
{
  MoaLong sfID = -1;
  
  if (callPtr->nargs > 1) {
	  MoaMmValue value;
  	
	  GetArgByIndex(2, &value);
		if (!_checkValueType(&value, kMoaMmValueType_Integer)) {
	  	fluid_log(FLUID_ERR, "SoundFontID should be an int");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		  return(kMoaErr_NoErr);
	  }
	  
		pObj->pMmValue->ValueToInteger(&value, &sfID);

	} else {
		// get the id of the last soundfont (index = 0)
		if (fluid_synth_sfcount(pObj->synth) == 0) {
	  	fluid_log(FLUID_ERR, "No SoundFont in stack");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADSTACKID, &callPtr->resultValue);
			return kMoaErr_NoErr;
		}
		fluid_sfont_t * sfont = fluid_synth_get_sfont(pObj->synth, 0);
		sfID = sfont->id;
	}
	
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NYI, &callPtr->resultValue);
	return kMoaErr_NoErr;
	
  fluid_log(FLUID_DBG, "fluidsynth: reloading soundFont id '%d'", sfID);

  int res = fluid_synth_sfreload(pObj->synth, sfID);
	if (res != 0) {
    return FLUIDXTRAERR_FLUIDERR;
	}

  return(kMoaErr_NoErr);
}

MoaError 
FLUIDXtra_IMoaMmXScript::getSoundFontsStack(PMoaDrCallInfo callPtr)
{
	MoaMmValue stackIDValue;
	
	pObj->pMmList->NewListValue(&callPtr->resultValue);
	
	fluid_xtra_stack_item *tmp = pObj->soundFontStack;
	while (tmp) {
		MoaLong sfID = tmp->sfID;
		
		pObj->pMmValue->IntegerToValue(sfID, &stackIDValue);
		pObj->pMmList->AppendValueToList(&callPtr->resultValue, &stackIDValue);
		pObj->pMmValue->ValueRelease(&stackIDValue);
			
		tmp = tmp->next;
	}
	
  return(kMoaErr_NoErr);
}

MoaError 
FLUIDXtra_IMoaMmXScript::getSoundFontInfo(PMoaDrCallInfo callPtr)
{
	MoaMmValue value;
	MoaLong sfID = 0;
	fluid_sfont_t* sfont = NULL;

	if (callPtr->nargs > 1) {  	
		GetArgByIndex(2, &value);
		if (!_checkValueType(&value, kMoaMmValueType_Integer)) {
	  		fluid_log(FLUID_ERR, "SoundFontID should be an int");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
			return kMoaErr_NoErr;
		}

		pObj->pMmValue->ValueToInteger(&value, &sfID);

		sfont = fluid_synth_get_sfont_by_id(pObj->synth, sfID);
		if (!sfont) {
 			fluid_log(FLUID_ERR, "No SoundFont with id = %d\n", sfID);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADSTACKID, &callPtr->resultValue);
			return kMoaErr_NoErr;
		}
	} else {
		// by default the last one
		sfont = fluid_synth_get_sfont(pObj->synth, 0);
		if (!sfont) {
 			fluid_log(FLUID_ERR, "No SoundFont loaded");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADSTACKID, &callPtr->resultValue);
			return kMoaErr_NoErr;
		}
		sfID = sfont->id;
	}
	
	// build plist
	MoaMmSymbol symbol = 0, symbol2 = 0;
  MoaMmValue symbolValue;

	pObj->pMmList->NewPropListValue(&callPtr->resultValue);

	// Get name, store name
	char * sfontName = (*(sfont)->get_name)(sfont);	
	pObj->pMmValue->StringToSymbol("name", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	_stringToValue(pObj, sfontName, &value);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &value);
	pObj->pMmValue->ValueRelease(&value);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// Get type, store type
	int type = -1;
	fluid_xtra_stack_item *tmp = pObj->soundFontStack;
	while (tmp && (type == -1)) {
		if (tmp->sfID == sfID) type = tmp->type;
		tmp = tmp->next;
	}
	pObj->pMmValue->StringToSymbol("type", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->StringToSymbol((type == fluid_xtra_soundfont_type_file) ? "file" : "ram", &symbol2);
	pObj->pMmValue->SymbolToValue(symbol2, &value);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &value);
	pObj->pMmValue->ValueRelease(&value);
	pObj->pMmValue->ValueRelease(&symbolValue);
	
	// presets
 	MoaMmValue presetsListValue;
	pObj->pMmList->NewListValue(&presetsListValue);
	fluid_preset_t preset;
	(*(sfont)->iteration_start)(sfont);
  // fluid_sfont_iteration_start(sfont);
  while ((*(sfont)->iteration_next)(sfont, &preset)) {
  	int bankNum, presetNum;
  	char *presetName;
  	MoaMmValue presetPlistValue;
		pObj->pMmList->NewPropListValue(&presetPlistValue);

  	bankNum = (*(&preset)->get_banknum)(&preset);
  	presetNum = (*(&preset)->get_num)(&preset);
  	presetName = (*(&preset)->get_name)(&preset);
  	
  	// Store name
		pObj->pMmValue->StringToSymbol("name", &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
		_stringToValue(pObj, presetName, &value);
		pObj->pMmList->AppendValueToPropList(&presetPlistValue, &symbolValue, &value);
		pObj->pMmValue->ValueRelease(&value);
		pObj->pMmValue->ValueRelease(&symbolValue);
  	
  	// Store bank
		pObj->pMmValue->StringToSymbol("bank", &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
		pObj->pMmValue->IntegerToValue(bankNum, &value);
		pObj->pMmList->AppendValueToPropList(&presetPlistValue, &symbolValue, &value);
		pObj->pMmValue->ValueRelease(&value);
		pObj->pMmValue->ValueRelease(&symbolValue);
  	
  	// Store number
		pObj->pMmValue->StringToSymbol("number", &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
		pObj->pMmValue->IntegerToValue(presetNum, &value);
		pObj->pMmList->AppendValueToPropList(&presetPlistValue, &symbolValue, &value);
		pObj->pMmValue->ValueRelease(&value);
		pObj->pMmValue->ValueRelease(&symbolValue);
  	
  	// if ram soundfont, add a list containing all the sampleIDs of this preset
  	if (type == fluid_xtra_soundfont_type_ram) {
  		fluid_ramsfont_t* ramsf =(fluid_ramsfont_t *)sfont->data;
 	 		MoaMmValue sampleIDsList, sampleElem;
 	 		
 			pObj->pMmList->NewListValue(&sampleIDsList);
			// iterate ramsamples 			
			fluid_xtra_sample_t *tmp = pObj->ramsamples;
			while (tmp) {
				if ((tmp->sfont == ramsf) && (tmp->bank == bankNum) && (tmp->num == presetNum)) {
					//
					pObj->pMmValue->IntegerToValue(tmp->sampleID, &sampleElem);
					pObj->pMmList->AppendValueToList(&sampleIDsList, &sampleElem);
					pObj->pMmValue->ValueRelease(&sampleElem);
					//
				}
				tmp = tmp->next;
			}
 			
	  	// Store sampleIDs
			pObj->pMmValue->StringToSymbol("sampleIDs", &symbol);
			pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
			pObj->pMmList->AppendValueToPropList(&presetPlistValue, &symbolValue, &sampleIDsList);
			pObj->pMmValue->ValueRelease(&symbolValue);
	
			pObj->pMmValue->ValueRelease(&sampleIDsList);
  	}
  	
		pObj->pMmList->AppendValueToList(&presetsListValue, &presetPlistValue);
		pObj->pMmValue->ValueRelease(&presetPlistValue);
  }
	pObj->pMmValue->StringToSymbol("presets", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &presetsListValue);
	pObj->pMmValue->ValueRelease(&presetsListValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

  return(kMoaErr_NoErr);
}

/**************************************/
/********* Events management **********/
/**************************************/


// Return -2 if source is unknown and not created
short
FLUIDXtra_IMoaMmXScript::_getSourceIdForName(char *srcName, bool createIfNone) {
  short src = -2;
	// get sourceID for string
	fluid_xtra_srcname* tmp = pObj->srcNames;
	int found = 0;
	while (tmp && !found) {
		if (!strcmp(srcName, tmp->name)) {
			found = 1;
			src = tmp->id;
		}
		tmp = tmp->next;
	}
	
	// if none, create one
	if (!found && createIfNone) {
		tmp = (fluid_xtra_srcname *)pObj->pCalloc->NRAlloc(sizeof(fluid_xtra_srcname));
		tmp->name = (char *)pObj->pCalloc->NRAlloc(strlen(srcName)+1);
		strcpy(tmp->name, srcName);
		tmp->name[strlen(srcName)] = 0;
		src = tmp->id = fluid_sequencer_register_client(pObj->sequencer, tmp->name, NULL, NULL);
		
		// prepend
		tmp->next = pObj->srcNames;

		pObj->srcNames = tmp;
	}
	return src;
}

int 
FLUIDXtra_IMoaMmXScript::_parseSeqPlist(PMoaDrCallInfo callPtr, MoaMmValue value, unsigned int* pdate, int* pabsolute, short* pdest, short*psrc, int createSrcIfUnknown)
{
	int res;
	
	// gets date/delay + src + dest
		
	// date/delay
	MoaLong longDate, longAbsolute;
	// by default : now
	longDate = 0;
	longAbsolute = 0;
	// date
	res = _getIntProp(callPtr, value, "date", "date", &longDate, false);
	if (res == -1)
	    return kMoaErr_NoErr;
	    
	if (res == 0) {
		longAbsolute = 1;

	} else {
		// delay
		res = _getIntProp(callPtr, value, "delay", "delay", &longDate, false);
		if (res == -1)
		    return kMoaErr_NoErr;
		if (res == 0) {
			longAbsolute = 0;
		} 
	}
	*pdate = longDate;
	*pabsolute = longAbsolute;
	
	// destination
	char destname[1024];
	res = _getAnsiStringProp(callPtr, value, "dest", "dest", destname, 1024, false);
	if (res == -1)
	    return kMoaErr_NoErr;
	if (res == 0) {
		// there is a dest
		// check validity
		int i, count, found;
		found = 0;
		count = fluid_sequencer_count_clients(pObj->sequencer);
		for (i = 0; i < count && !found; i++) {
			short id = fluid_sequencer_get_client_id(pObj->sequencer, i);
			if (fluid_sequencer_client_is_dest(pObj->sequencer, id)) {
				char *name = fluid_sequencer_get_client_name(pObj->sequencer, id);
				if (!strcmp(name, destname)) {
					*pdest = id;
					found = 1;
				} 
			}
		}
		if (!found) {
	  	fluid_log(FLUID_ERR, "unknown destination : %s", destname);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_UNKNOWNDESTINATION, &callPtr->resultValue);
			return -1;
		}
	}

	// src
	char sourceBuf[1024];
	res = _getAnsiStringProp(callPtr, value, "source", "source", sourceBuf, 1024, false);
	if (res == -1)
	  return(kMoaErr_NoErr);

	if (res == 0) {
		// get sourceID for string
		*psrc = _getSourceIdForName(sourceBuf, createSrcIfUnknown);
 	}

	return (0);
}

MoaError 
FLUIDXtra_IMoaMmXScript::setTimeUnit(PMoaDrCallInfo callPtr)
{
	MoaDouble dblTimeUnit;

  // get the ticksPerSecond
 	if (_getFloatArg(callPtr, 2, "ticksPerSecond", &dblTimeUnit, true) < 0)
    return kMoaErr_NoErr;
	
	if (dblTimeUnit <= 0) {
  	fluid_log(FLUID_ERR, "ticksPerSecond should be > 0 %f", dblTimeUnit);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	  return kMoaErr_NoErr;
	}
	
	fluid_sequencer_set_time_scale(pObj->sequencer, dblTimeUnit);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getTimeUnit(PMoaDrCallInfo callPtr)
{
	double timeNow;
	MoaDouble longNow;
	
	timeNow = fluid_sequencer_get_time_scale(pObj->sequencer);
	
	longNow = timeNow;
	
	pObj->pMmValue->FloatToValue(longNow, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getTime(PMoaDrCallInfo callPtr)
{
	unsigned int timeNow;
	MoaLong longNow;
	
	timeNow = fluid_sequencer_get_tick(pObj->sequencer);
	
	longNow = timeNow;
	
	pObj->pMmValue->IntegerToValue(longNow, &callPtr->resultValue);
  return kMoaErr_NoErr;
}


MoaError 
FLUIDXtra_IMoaMmXScript::getDestinations(PMoaDrCallInfo callPtr)
{
	int i, count;
  MoaMmValue value;
	
	pObj->pMmList->NewListValue(&callPtr->resultValue);
	
	count = fluid_sequencer_count_clients(pObj->sequencer);
	for (i = 0; i < count ; i++) {
		short id = fluid_sequencer_get_client_id(pObj->sequencer, i);
		if (fluid_sequencer_client_is_dest(pObj->sequencer, id)) {
			char *name = fluid_sequencer_get_client_name(pObj->sequencer, id);
			_stringToValue(pObj, name, &value);
			pObj->pMmList->AppendValueToList(&callPtr->resultValue, &value);	
			pObj->pMmValue->ValueRelease(&value);
		}
	}
  return(kMoaErr_NoErr);
}

MoaError 
FLUIDXtra_IMoaMmXScript::noteon(PMoaDrCallInfo callPtr)
{
  MoaLong chan, key, velInt;
	MoaDouble vel;
	
  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;
  
  // get the key
 	if (_getIntArg(callPtr, 3, "key", &key, true) < 0)
    return kMoaErr_NoErr;

  // get the velocity
 	if (_getFloatArg(callPtr, 4, "vel", &vel, true) < 0)
    return kMoaErr_NoErr;
  velInt = (MoaLong)(127.0*vel);
	if (velInt > 127) velInt = 127;
	if (velInt < 0) velInt = 0;
	
	// seq plist
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	MoaMmValue value;	
	int res = _getPlistArg(callPtr, 5, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }
	
  fluid_log(FLUID_DBG, "fluidsynth: posting noteon [chan=%d,key=%d,vel=%d,date=%d,abd=%d]", chan, key, velInt, date, absolute);

	int fluid_res;
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, dest);
	
	fluid_event_noteon(evt, chan, (short)key, (short)velInt);
	fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
	if (fluid_res != 0) {
		delete_fluid_event(evt);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	delete_fluid_event(evt);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::noteoff(PMoaDrCallInfo callPtr)
{
  MoaLong chan, key;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

  // get the key
 	if (_getIntArg(callPtr, 3, "key", &key, true) < 0)
    return kMoaErr_NoErr;

	// seq plist
	MoaMmValue value;
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	int res = _getPlistArg(callPtr, 4, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }
	
  fluid_log(FLUID_DBG, "fluidsynth: posting noteoff [chan=%d,key=%d,date=%d,abs=%d]", chan, key, date, absolute);

	int fluid_res;
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, dest);
	
	fluid_event_noteoff(evt, chan, (short)key);
	fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
	if (fluid_res != 0) {
		delete_fluid_event(evt);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	delete_fluid_event(evt);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::note(PMoaDrCallInfo callPtr)
{
  MoaLong chan, key, dur, velInt;
	MoaDouble vel;
	
  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;
  
  // get the key
 	if (_getIntArg(callPtr, 3, "key", &key, true) < 0)
    return kMoaErr_NoErr;

  // get the velocity
 	if (_getFloatArg(callPtr, 4, "vel", &vel, true) < 0)
    return kMoaErr_NoErr;
  velInt = (MoaLong)(127.0*vel);
	if (velInt > 127) velInt = 127;
	if (velInt < 0) velInt = 0;
	
  // get the dur
 	if (_getIntArg(callPtr, 5, "dur", &dur, true) < 0)
    return kMoaErr_NoErr;

	// seq plist
	MoaMmValue value;	
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	int res = _getPlistArg(callPtr, 6, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }
	
  fluid_log(FLUID_DBG, "fluidsynth: posting note [chan=%d,key=%d,vel=%d,dur=%d,date=%d,abd=%d]", chan, key, velInt, dur, date, absolute);

	int fluid_res;
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, dest);
	
	fluid_event_note(evt, chan, (short)key, (short)velInt, dur);
	fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
	if (fluid_res != 0) {
		delete_fluid_event(evt);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	
	delete_fluid_event(evt);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

/**************************************/
/********* Program management *********/
/**************************************/


MoaError 
FLUIDXtra_IMoaMmXScript::programChange(PMoaDrCallInfo callPtr)
{
//  MoaError err = kMoaErr_NoErr;
  MoaMmValue value;
  MoaLong chan, prog, bank = 0, sfID = -1;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

  // get the program
  // can be an int, or a plist bank+number
  GetArgByIndex(3, &value);
	if (_checkValueType(&value, kMoaMmValueType_Integer)) {
	
	  pObj->pMmValue->ValueToInteger(&value, &prog);
	  
	} else if (_checkValueType(&value, kMoaMmValueType_PropList)) {
	
		// number
		if (_getIntProp(callPtr, value, "number", "preset number", &prog, true) < 0)
	    return kMoaErr_NoErr;			

		// bank
		int res = _getIntProp(callPtr, value, "bank", "bank number", &bank, false);
		if (res == -1)
	    return kMoaErr_NoErr;
		if (res == -2)	// VOID
			bank = 0;
					
		// soundFont
		res = _getIntProp(callPtr, value, "soundFont", "soundFontID", &sfID, false);
		if (res == -1)
	    return kMoaErr_NoErr;
		if (res == -2)	// VOID
			sfID = -1;

	} else { 
  	fluid_log(FLUID_ERR, "presetInfo should be a int or a plist");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return kMoaErr_NoErr;
  }

	// seq plist
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	int res = _getPlistArg(callPtr, 4, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }

	if (sfID == -1) {
		// No SoundFont : use stack
	  fluid_log(FLUID_DBG, "fluidsynth: sending program change [chan=%d,bank=%d,prog=%d,date=%d,abs=%d]", chan, bank, prog, date, absolute);
	
		int fluid_res;
		fluid_event_t *evt = new_fluid_event();
		fluid_event_set_source(evt, src);
		fluid_event_set_dest(evt, dest);
		
		fluid_event_bank_select(evt, chan, (short)bank);
		fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
		if (fluid_res != 0) {
			delete_fluid_event(evt);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
	  	return kMoaErr_NoErr;
		}
		
		fluid_event_program_change(evt, chan, (short)prog);
		fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
		if (fluid_res != 0) {
			delete_fluid_event(evt);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
	  	return kMoaErr_NoErr;
		}
		delete_fluid_event(evt);

	} else {
		// soundFont specified : use program_select
		int fluid_res;
		fluid_event_t *evt = new_fluid_event();
		fluid_event_set_source(evt, src);
		fluid_event_set_dest(evt, dest);
		
		fluid_event_program_select(evt, chan, sfID, (short)bank, (short)prog);

		fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
		if (fluid_res != 0) {
			delete_fluid_event(evt);
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
	  	return kMoaErr_NoErr;
		}
		delete_fluid_event(evt);
	}
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getProgram(PMoaDrCallInfo callPtr)
{
//  MoaError err = kMoaErr_NoErr;
  MoaLong chan;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, false) < 0)
    return kMoaErr_NoErr;
	
	fluid_preset_t *preset;
	preset = fluid_synth_get_channel_preset(pObj->synth, chan);
	if (preset == NULL) {
  		fluid_log(FLUID_ERR, "no preset for channel %d", chan);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NOPRESET, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	
	int prognum;
	unsigned int bankNum;
	char * presetName;
  MoaMmValue value;
  bankNum = (*(preset)->get_banknum)(preset);
	prognum = (*(preset)->get_num)(preset);
	presetName = (*(preset)->get_name)(preset);

	pObj->pMmList->NewPropListValue(&callPtr->resultValue);
	MoaMmSymbol symbol = 0;
  MoaMmValue symbolValue;
  
	// Store name
	pObj->pMmValue->StringToSymbol("name", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	_stringToValue(pObj, presetName, &value);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &value);
	pObj->pMmValue->ValueRelease(&value);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// number
	pObj->pMmValue->StringToSymbol("number", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->IntegerToValue((MoaLong)prognum, &value);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &value);
	pObj->pMmValue->ValueRelease(&value);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// bank
	pObj->pMmValue->StringToSymbol("bank", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	pObj->pMmValue->IntegerToValue((MoaLong)bankNum, &value);
	pObj->pMmList->AppendValueToPropList(&callPtr->resultValue, &symbolValue, &value);
	pObj->pMmValue->ValueRelease(&value);
	pObj->pMmValue->ValueRelease(&symbolValue);

  return kMoaErr_NoErr;
}

/**************************************/
/********* Control Change *************/
/**************************************/

MoaError 
FLUIDXtra_IMoaMmXScript::controlChange(PMoaDrCallInfo callPtr)
{
  MoaMmValue value, ctrlPlistValue;
  MoaLong chan;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

  // get the controlParams
	if (_getPlistArg(callPtr, 3, "ctrlParams", &ctrlPlistValue, true) < 0)
    return kMoaErr_NoErr;

	// seq plist
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	int res = _getPlistArg(callPtr, 4, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }
  
	MoaLong i, count = pObj->pMmList->CountElements(&ctrlPlistValue);
	MoaMmValue propValue, valValue;
	MoaMmSymbol symbol, refSymbol;
	
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, dest);

	for (i = 1 ; i <= count ; i++) {
		int found;
		MoaLong valNum;
		MoaDouble valFloat;

		// get the value
		pObj->pMmList->GetValueByIndex(&ctrlPlistValue, i, &valValue);
		if (!_checkValueType(&valValue, kMoaMmValueType_Integer) && !_checkValueType(&valValue, kMoaMmValueType_Float)) {
			pObj->pMmValue->ValueRelease(&valValue);
	  	fluid_log(FLUID_ERR, "value should be a int or a float");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	    return kMoaErr_NoErr;
	  }
	  
		if (_checkValueType(&valValue, kMoaMmValueType_Float)) {
			pObj->pMmValue->ValueToFloat(&valValue, &valFloat);
			valNum = (MoaLong)valFloat;
		} else {
		  pObj->pMmValue->ValueToInteger(&valValue, &valNum);
			valFloat = valNum;
		}
		pObj->pMmValue->ValueRelease(&valValue);

		// get the prop
		pObj->pMmList->GetPropertyNameByIndex(&ctrlPlistValue, i, &propValue);
		pObj->pMmValue->ValueToSymbol(&propValue, &symbol);
		pObj->pMmValue->ValueRelease(&propValue);
		
		found = 0;
		
		// pan
		pObj->pMmValue->StringToSymbol("pan", &refSymbol);
		if (refSymbol == symbol) {
			short val = (short)(64.0 + 64.0*valFloat);
			if (val < 0) val = 0;
			if (val > 127) val = 127;
			fluid_log(FLUID_DBG, "fluidsynth: sending pan [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
			fluid_event_pan(evt, chan, val);		
			found = 1;
		}
		
		if (!found) {
			// volume
			pObj->pMmValue->StringToSymbol("volume", &refSymbol);
			if (refSymbol == symbol) {
				short val = (short)(valFloat*127.0);
				if (val < 0) val = 0;
				if (val > 127) val = 127;
			  fluid_log(FLUID_DBG, "fluidsynth: sending volume [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
				fluid_event_volume(evt, chan, val);		
				found = 1;
			}
		}

		if (!found) {
			// sustain
			pObj->pMmValue->StringToSymbol("sustain", &refSymbol);
			if (refSymbol == symbol) {
				// lingo 1 => fluid 127, else 0
				if (valNum == 1) valNum = 127;
				else valNum = 0;
			  fluid_log(FLUID_DBG, "fluidsynth: sending sustain [chan=%d,val=%d,date=%d,abs=%d]", chan, valNum, date, absolute);
				fluid_event_sustain(evt, chan, (short)valNum);		
				found = 1;
			}
		}

		if (!found) {
			// reverbsend
			pObj->pMmValue->StringToSymbol("reverbsend", &refSymbol);
			if (refSymbol == symbol) {
				short val = (short)(valFloat*127.0);
				if (val < 0) val = 0;
				if (val > 127) val = 127;
			  fluid_log(FLUID_DBG, "fluidsynth: sending reverbsend [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
				fluid_event_reverb_send(evt, chan, val);		
				found = 1;
			}
		}

		if (!found) {
			// chorussend
			pObj->pMmValue->StringToSymbol("chorussend", &refSymbol);
			if (refSymbol == symbol) {
				short val = (short)(valFloat*127.0);
				if (val < 0) val = 0;
				if (val > 127) val = 127;
			  fluid_log(FLUID_DBG, "fluidsynth: sending chorussend [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
				fluid_event_chorus_send(evt, chan, val);		
				found = 1;
			}
		}

		if (!found) {
			// modulation
			pObj->pMmValue->StringToSymbol("modulation", &refSymbol);
			if (refSymbol == symbol) {
				short val = (short)(valFloat*127.0);
				if (val < 0) val = 0;
				if (val > 127) val = 127;
			  fluid_log(FLUID_DBG, "fluidsynth: sending modulation [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
				fluid_event_modulation(evt, chan, val);		
				found = 1;
			}
		}

		if (!found) {
			// pitchbend
			pObj->pMmValue->StringToSymbol("pitchbend", &refSymbol);
			if (refSymbol == symbol) {
				short val = (short)(8192.0*(1.0 + valFloat));
				if (val < 0) val = 0;
				if (val > 16383) val = 16383;
			  fluid_log(FLUID_DBG, "fluidsynth: sending pitch bend [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
				fluid_event_pitch_bend(evt, chan, val);		
				found = 1;
			}
		}
		
		if (!found) {
			// pitchsensitivity
			pObj->pMmValue->StringToSymbol("pitchsensitivity", &refSymbol);
			if (refSymbol == symbol) {
				short val = (short)valNum;
				if (val < 0) val = 0;
				if (val > 127) val = 127;
			  fluid_log(FLUID_DBG, "fluidsynth: sending pitch bend [chan=%d,val=%d,date=%d,abs=%d]", chan, val, date, absolute);
				fluid_event_pitch_wheelsens(evt, chan, val);		
				found = 1;
			}
		}

		if (found) {
			/* event ready to send to seq */
			int fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
			if (fluid_res != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				goto bail;
			}

		} else {
				// unknown control symbol
				char symbolstring[1024];
				pObj->pMmList->GetPropertyNameByIndex(&ctrlPlistValue, i, &propValue);
				pObj->pMmValue->ValueToString(&propValue, symbolstring, 1024);
				pObj->pMmValue->ValueRelease(&propValue);
  			fluid_log(FLUID_ERR, "unknown control property : %s", symbolstring);
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_UNKNOWNCONTROL, &callPtr->resultValue);
				goto bail;
		}
		
	}

	// success
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);

bail:
	delete_fluid_event(evt);
  return kMoaErr_NoErr;
}


MoaError 
FLUIDXtra_IMoaMmXScript::allSoundsOff(PMoaDrCallInfo callPtr)
{
  MoaMmValue value;
  MoaLong chan;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

	// seq plist
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	int res = _getPlistArg(callPtr, 3, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }

	fluid_log(FLUID_DBG, "fluidsynth: sending allsoundsoff [chan=%d]", chan);
	int fluid_res;
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, dest);

	fluid_event_all_sounds_off(evt, chan);
	
	fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
	if (fluid_res != 0) {
		delete_fluid_event(evt);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	delete_fluid_event(evt);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::reset(PMoaDrCallInfo callPtr)
{
	int fluid_res;

	fluid_log(FLUID_DBG, "fluidsynth: doing reset");
	
	fluid_res = fluid_synth_system_reset(pObj->synth);
	if (fluid_res != 0) {
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::allNotesOff(PMoaDrCallInfo callPtr)
{
  MoaMmValue value;
  MoaLong chan;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

	// seq plist
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest = pObj->synthSeqID;
	int res = _getPlistArg(callPtr, 3, "seq", &value, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// get date/delay/src/dest from seqplist		
		if (_parseSeqPlist(callPtr, value, &date, &absolute, &dest, &src, 1) != 0)
			return kMoaErr_NoErr;
  }

	fluid_log(FLUID_DBG, "fluidsynth: sending allnotesoff [chan=%d]", chan);
	int fluid_res;
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, dest);

	fluid_event_all_notes_off(evt, chan);

	fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
	if (fluid_res != 0) {
		delete_fluid_event(evt);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	delete_fluid_event(evt);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

MoaError
FLUIDXtra_IMoaMmXScript::getGenerator(PMoaDrCallInfo callPtr)
{
  MoaLong chan, generator;
	float val;
	
  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;
  
  // get the generator
 	if (_getIntArg(callPtr, 3, "generator", &generator, true) < 0)
    return kMoaErr_NoErr;

	val = fluid_synth_get_gen(pObj->synth, chan, generator);
	pObj->pMmValue->FloatToValue(val, &callPtr->resultValue);
	
	return kMoaErr_NoErr;
}
MoaError
FLUIDXtra_IMoaMmXScript::setGenerator(PMoaDrCallInfo callPtr)
{
  MoaLong chan, generator;
  MoaDouble value;
	
  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;
  
  // get the generator
 	if (_getIntArg(callPtr, 3, "generator", &generator, true) < 0)
    return kMoaErr_NoErr;

  // get the generator
 	if (_getFloatArg(callPtr, 4, "value", &value, true) < 0)
    return kMoaErr_NoErr;

	MoaError err = fluid_synth_set_gen(pObj->synth, chan, generator, (float)value);
	
	if (err != 0)
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
	else
		pObj->pMmValue->IntegerToValue(err, &callPtr->resultValue);
  return kMoaErr_NoErr;
}

int
FLUIDXtra_IMoaMmXScript::_getControlValue(PMoaDrCallInfo callPtr, PMoaMmValue pcontrolSymbolValue, int chan, PMoaMmValue pvalue)
{
	/* fills the pvalue with the value of the control pcontrolSymbolValue */
	/* if error, fills the return value with the right error number */
	MoaLong ctrlNum;
	MoaMmSymbol symbol, refSymbol;
//	int isRealControl = 0;
	int valNum;
	MoaDouble valFloat;

	pObj->pMmValue->ValueToSymbol(pcontrolSymbolValue, &symbol);
	
	// pan
	pObj->pMmValue->StringToSymbol("pan", &refSymbol);
	if (refSymbol == symbol) {
		ctrlNum = (MoaLong)0x0A;
		if (fluid_synth_get_cc(pObj->synth, chan, ctrlNum, &valNum) != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				return 1;
		}

		valFloat = valNum/64.0 - 1.0;
		pObj->pMmValue->FloatToValue(valFloat, pvalue);
		return 0;
	}
	// volume
	pObj->pMmValue->StringToSymbol("volume", &refSymbol);
	if (refSymbol == symbol) {
		ctrlNum = (MoaLong)0x07;
		if (fluid_synth_get_cc(pObj->synth, chan, ctrlNum, &valNum) != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				return 1;
		}
		valFloat = valNum/127.0;
		pObj->pMmValue->FloatToValue(valFloat, pvalue);
		return 0;
	}
	// sustain
	pObj->pMmValue->StringToSymbol("sustain", &refSymbol);
	if (refSymbol == symbol) {
		ctrlNum = (MoaLong)0x40;
		if (fluid_synth_get_cc(pObj->synth, chan, ctrlNum, &valNum) != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				return 1;
		}
		pObj->pMmValue->IntegerToValue((valNum > 63), pvalue);
		return 0;
	}
	// reverbsend
	pObj->pMmValue->StringToSymbol("reverbsend", &refSymbol);
	if (refSymbol == symbol) {
		ctrlNum = (MoaLong)0x5B;	// EFFECTS_DEPTH1
		if (fluid_synth_get_cc(pObj->synth, chan, ctrlNum, &valNum) != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				return 1;
		}
		valFloat = valNum/127.0;
		pObj->pMmValue->FloatToValue(valFloat, pvalue);
		return 0;
	}
	// chorussend
	pObj->pMmValue->StringToSymbol("chorussend", &refSymbol);
	if (refSymbol == symbol) {
		ctrlNum = (MoaLong)0x5D;	// EFFECTS_DEPTH3
		if (fluid_synth_get_cc(pObj->synth, chan, ctrlNum, &valNum) != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				return 1;
		}
		valFloat = valNum/127.0;
		pObj->pMmValue->FloatToValue(valFloat, pvalue);
		return 0;
	}
	// modulation
	pObj->pMmValue->StringToSymbol("modulation", &refSymbol);
	if (refSymbol == symbol) {
		ctrlNum = (MoaLong)0x01;	// MODULATION_MSB
		if (fluid_synth_get_cc(pObj->synth, chan, ctrlNum, &valNum) != 0) {
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
				return 1;
		}
		valFloat = valNum/127.0;
		pObj->pMmValue->FloatToValue(valFloat, pvalue);
		return 0;
	}
	// pitchbend
	pObj->pMmValue->StringToSymbol("pitchbend", &refSymbol);
	if (refSymbol == symbol) {
		// get pitchbend
		int res = fluid_synth_get_pitch_bend(pObj->synth, chan, &valNum);
		if (res != 0) {
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
			return 1;
		}
		valFloat = valNum/8192.0 - 1.0;
		pObj->pMmValue->FloatToValue(valFloat, pvalue);
		return 0;
	}
	// pitchsensitivity
	pObj->pMmValue->StringToSymbol("pitchsensitivity", &refSymbol);
	if (refSymbol == symbol) {
		// get pitchsensitivity
		int res = fluid_synth_get_pitch_wheel_sens(pObj->synth, chan, &valNum);
		if (res != 0) {
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
			return 1;
		}
		pObj->pMmValue->IntegerToValue(valNum, pvalue);
		return 0;
	}
		
	// unknown property
	char symbolstring[1024];
	pObj->pMmValue->ValueToString(pcontrolSymbolValue, symbolstring, 1024);
	fluid_log(FLUID_ERR, "unknown control property : %s", symbolstring);
	pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_UNKNOWNCONTROL, &callPtr->resultValue);
	return -1;
}


MoaError 
FLUIDXtra_IMoaMmXScript::getControl(PMoaDrCallInfo callPtr)
{
  MoaMmValue value;
  MoaLong chan;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

  // get the control symbol
  GetArgByIndex(3, &value);
	if (!_checkValueType(&value, kMoaMmValueType_Symbol)) {
  	fluid_log(FLUID_ERR, "ctrlSymbol should be a symbol");
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
    return kMoaErr_NoErr;
  }
  _getControlValue(callPtr, &value, chan, &callPtr->resultValue);
	return kMoaErr_NoErr;
}

MoaError 
FLUIDXtra_IMoaMmXScript::getControls(PMoaDrCallInfo callPtr)
{
  MoaLong chan;

  // get the channel
 	if (_getIntArg(callPtr, 2, "chan", &chan, true) < 0)
    return kMoaErr_NoErr;

	MoaMmSymbol symbol = 0;
  MoaMmValue symbolValue, resultValue;
  int res;
  MoaMmValue resValue;
  
	pObj->pMmList->NewPropListValue(&resultValue);

	// Store pan
	pObj->pMmValue->StringToSymbol("pan", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);
  
	// Store volume
	pObj->pMmValue->StringToSymbol("volume", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// Store sustain
	pObj->pMmValue->StringToSymbol("sustain", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);
  
	// Store pitchbend
	pObj->pMmValue->StringToSymbol("pitchbend", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// Store chorussend
	pObj->pMmValue->StringToSymbol("chorussend", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// Store reverbsend
	pObj->pMmValue->StringToSymbol("reverbsend", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// Store modulation
	pObj->pMmValue->StringToSymbol("modulation", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

	// Store pitchsensitivity
	pObj->pMmValue->StringToSymbol("pitchsensitivity", &symbol);
	pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	res = _getControlValue(callPtr, &symbolValue, chan, &resValue);
	if (res != 0) {
		pObj->pMmValue->ValueRelease(&resultValue);
		pObj->pMmValue->ValueRelease(&symbolValue);
		return kMoaErr_NoErr;
	}
	pObj->pMmList->AppendValueToPropList(&resultValue, &symbolValue, &resValue);
	pObj->pMmValue->ValueRelease(&resValue);
	pObj->pMmValue->ValueRelease(&symbolValue);

  callPtr->resultValue = resultValue;
	return kMoaErr_NoErr;
}

/******************************/
/*     callbacks              */
/******************************/

void
FLUIDXtra_IMoaMmXScript::_ensureEnoughDones()
{
	// makes sure that the dones array is large enough for all pending callbacks
	int nbCallbacks = pObj->pMmList->CountElements(&pObj->curCallbacks);
	if (pObj->maxDones < nbCallbacks) {
		// we have to reallocate done
		MoaMmSymbol *tmp;
		int newSiz = pObj->maxDones + 100; // by 100
		tmp = (MoaMmSymbol *)pObj->pCalloc->NRAlloc(newSiz * sizeof(MoaMmSymbol));
		memset(tmp, 0, newSiz * sizeof(MoaMmSymbol));
		
		// wait for donebusy over
		while (pObj->doneBusy);
		
		memcpy(tmp, pObj->dones, pObj->maxDones * sizeof(MoaMmSymbol));
		pObj->pCalloc->NRFree(pObj->dones);
		pObj->dones = tmp;
		
		pObj->maxDones = newSiz;
	}	
}

/* fills the callbackID, as a value */
void
FLUIDXtra_IMoaMmXScript::_storeCallback(PMoaDrCallInfo callPtr, PMoaMmValue pcallbackInfo, PMoaMmSymbol callbackID)
{	
	// SetAProp pcallbackInfo to curCallbacks, with the curCallbackID as prop
	pObj->curCallbackID++;
	char symbolString[1024];
  MoaMmValue idSymbolValue;
	sprintf(symbolString, "c%ld", pObj->curCallbackID);
	pObj->pMmValue->StringToSymbol(symbolString, callbackID);
	pObj->pMmValue->SymbolToValue(*callbackID, &idSymbolValue);
	pObj->pMmList->AppendValueToPropList(&pObj->curCallbacks, &idSymbolValue, pcallbackInfo);
	pObj->pMmValue->ValueRelease(&idSymbolValue);
	
	_ensureEnoughDones();
}

void
FLUIDXtra_IMoaMmXScript::_forgetCallback(PMoaDrCallInfo callPtr, MoaMmSymbol callbackID)
{
  MoaMmValue idSymbolValue;
	pObj->pMmValue->SymbolToValue(callbackID, &idSymbolValue);
	
		//	pObj->pMmList->DeleteValueByProperty(&pObj->curCallbacks, &idSymbolValue);
		MoaMmValue res;
		MoaMmSymbol deletePropS = 0;
		MoaMmValue args[2];
		pObj->pMmValue->StringToSymbol("DeleteProp", &deletePropS);
		args[0] = pObj->curCallbacks;
		args[1] = idSymbolValue;
		pObj->pDrPlayer->CallHandler(deletePropS, 2, &args[0], &res);

	pObj->pMmValue->ValueRelease(&idSymbolValue);
}

void
FLUIDXtra_IMoaMmXScript::_removeCallbacks(short templSrc)
{
  if (templSrc == -1) {
    // remove all : empty curCallbacks
  	pObj->pMmValue->ValueRelease(&pObj->curCallbacks);
	  pObj->pMmList->NewPropListValue(&pObj->curCallbacks);
		{
					//	Sort it for fast finds
					MoaMmValue res;
					MoaMmSymbol sortPropS = 0;
					pObj->pMmValue->StringToSymbol("sort", &sortPropS);
					pObj->pDrPlayer->CallHandler(sortPropS, 1, &pObj->curCallbacks, &res);
		}
    return;
  }
  
  // find entries with the same src
  // ?? This is too slow
	MoaMmValue toRemove;
	pObj->pMmList->NewListValue(&toRemove);

	int i, count = pObj->pMmList->CountElements(&pObj->curCallbacks);
	for (i = 0 ; i < count ; i++) {
		MoaMmValue callbackInfo;
		MoaLong src = -1;
		pObj->pMmList->GetValueByIndex(&pObj->curCallbacks, i+1, &callbackInfo);
		
		{
			// find src
			char sourceBuf[1024];
			int res = _getAnsiStringProp(NULL, callbackInfo, "source", "source", sourceBuf, 1024, false);
			if (res == 0) {
			  src = _getSourceIdForName(sourceBuf, false);
			}
		}
		
		if (templSrc == src) {
			MoaMmValue prop;
			pObj->pMmList->GetPropertyNameByIndex(&pObj->curCallbacks, i+1, &prop);
			pObj->pMmList->AppendValueToList(&toRemove, &prop);
			pObj->pMmValue->ValueRelease(&prop);
		}
		
		pObj->pMmValue->ValueRelease(&callbackInfo);
	}
	
	// remove toRemove
	count = pObj->pMmList->CountElements(&toRemove);
	for (i = 0 ; i < count ; i++) {
		MoaMmValue prop;
		pObj->pMmList->GetValueByIndex(&toRemove, i+1, &prop);
		
			//	pObj->pMmList->DeleteValueByProperty(&pObj->curCallbacks, &prop);
			MoaMmValue res;
			MoaMmSymbol deletePropS = 0;
			MoaMmValue args[2];
			pObj->pMmValue->StringToSymbol("DeleteProp", &deletePropS);
			args[0] = pObj->curCallbacks;
			args[1] = prop;
			pObj->pDrPlayer->CallHandler(deletePropS, 2, &args[0], &res);
		
		pObj->pMmValue->ValueRelease(&prop);
	}
	
	pObj->pMmValue->ValueRelease(&toRemove);
}

void
FLUIDXtra_IMoaMmXScript::_setCallbackDone(MoaMmSymbol callbackID)
{
	if (pObj->dones == NULL) return;
	
	// called from thread or interrupt = no memory allocation, and lock.
	pObj->doneBusy = 1;
	pObj->dones[pObj->curDones++] = callbackID;
	pObj->doneBusy = 0;
}

/* callback called from fluid */
void
FLUIDXtra_IMoaMmXScript::sequencerCallback(unsigned int time, fluid_event_t* event, fluid_sequencer_t* seq)
{
	MoaMmSymbol callbackID;
	callbackID = (MoaMmSymbol)fluid_event_get_data(event);
	_setCallbackDone(callbackID);
}

MoaError
FLUIDXtra_IMoaMmXScript::removeEvents(PMoaDrCallInfo callPtr)
{
	short src = -1, dest = -1;
	int type = -1;
	unsigned int date;
	int absolute;
	
	// filter ?
	MoaMmValue filterValue;	
	int res = _getPlistArg(callPtr, 2, "filter", &filterValue, false);
	if (res == -1)
			return kMoaErr_NoErr;
	if (res == 0) {
		// source, dest, type
		if (_parseSeqPlist(callPtr, filterValue, &date, &absolute, &dest, &src, 0) != 0)
			return kMoaErr_NoErr;
		
		// If unknown src, no need to go further
		if (src == -2) {
			pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
			return(kMoaErr_NoErr);
		}

		// type
		MoaMmSymbol symbol;
		MoaMmValue symbolValue, typeValue;
		MoaError err;
		
	  pObj->pMmValue->StringToSymbol("type", &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	  err = pObj->pMmList->GetValueByProperty(&filterValue, &symbolValue, &typeValue);
	  pObj->pMmValue->ValueRelease(&symbolValue);
	  
	  if (err == kMoaErr_NoErr) {
		  if (!_checkValueType(&typeValue, kMoaMmValueType_Symbol)) {
			  pObj->pMmValue->ValueRelease(&typeValue);
		  	fluid_log(FLUID_ERR, "type should be a symbol");
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		    return kMoaErr_NoErr;
		  }
	  	MoaMmSymbol typeSymbol;
	  	pObj->pMmValue->ValueToSymbol(&typeValue, &typeSymbol);
	  	
	  	// which one ?
	  	MoaMmSymbol tmpSymbol;
	  	int found = 0;
	  	pObj->pMmValue->StringToSymbol("callback", &tmpSymbol);
	  	if (typeSymbol == tmpSymbol) {
	  		type = FLUID_SEQ_TIMER;
	  		found = 1;
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("note", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_NOTE;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("noteon", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_NOTEON;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("noteoff", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_NOTEOFF;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("allsoundsoff", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_ALLSOUNDSOFF;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("allnotesoff", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_ALLNOTESOFF;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("programchange", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_PROGRAMCHANGE;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("pitchbend", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_PITCHBEND;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("modulation", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_MODULATION;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("sustain", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_SUSTAIN;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("pan", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_PAN;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("volume", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_VOLUME;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("reverbsend", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_REVERBSEND;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("chorussend", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_CHORUSSEND;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
		  	pObj->pMmValue->StringToSymbol("controlchange", &tmpSymbol);
		  	if (typeSymbol == tmpSymbol) {
		  		type = FLUID_SEQ_ANYCONTROLCHANGE;
		  		found = 1;
		  	}
	  	}
	  	if (!found) {
				// unknown type
				char symbolstring[1024];
				pObj->pMmValue->ValueToString(&typeValue, symbolstring, 1024);
				pObj->pMmValue->ValueRelease(&typeValue);
 				fluid_log(FLUID_ERR, "unknown type : %s", symbolstring);
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_UNKNOWNCONTROL, &callPtr->resultValue);
				return(kMoaErr_NoErr);
			}
		  pObj->pMmValue->ValueRelease(&typeValue);
	  }
	}
	
	if (type == FLUID_SEQ_TIMER) {
	
		// cleanup our datastructure
		_removeCallbacks(src);

		// remove only our timers
		fluid_sequencer_remove_events(pObj->sequencer, src, pObj->xtraSeqID, FLUID_SEQ_TIMER);
		
	} else {
		// call sequencer in all cases
		fluid_sequencer_remove_events(pObj->sequencer, src, dest, type);
	}
	
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return(kMoaErr_NoErr);
}

void poll(FLUIDXtra FAR * pObj) {

	MoaMmSymbol *dones = NULL;
	int curDones, iDone;
	
	// just in case we are already freed
	if (pObj->dones == NULL) return;

	if (pObj->pStream2)
		checkSFDownload(pObj);

	if (pObj->curDones == 0) return;

	// get a copy of the dones
	dones = (MoaMmSymbol *)pObj->pCalloc->NRAlloc(pObj->maxDones * sizeof(MoaMmSymbol));
	// wait for doneBusy
	while (pObj->doneBusy);
	// Do it
	memcpy(dones, pObj->dones, pObj->maxDones * sizeof(MoaMmSymbol));
	curDones = pObj->curDones;
	pObj->curDones = 0;
	
	/* Prepare the pushContext, in order to call safely into Director */
	PIMoaDrMovieContext pIDrMovieContext = NULL;
	PIMoaDrMovie pIMoaDrMovie = NULL;
	DrContextState drContextState;
	if (pObj->pDrPlayer->GetActiveMovie(&pIMoaDrMovie) == kMoaErr_NoErr) {
		pIMoaDrMovie->QueryInterface(&IID_IMoaDrMovieContext, (PPMoaVoid)&pIDrMovieContext);
		pIMoaDrMovie->Release();
	}

	if (pIDrMovieContext && (pIDrMovieContext->PushXtraContext(&drContextState) == kMoaErr_NoErr)) {
		// call into Director OK

		for (iDone = 0; iDone < curDones ; iDone++) {
			MoaMmSymbol propSym;
			MoaMmValue prop;
			MoaMmValue callbackInfo;
			MoaError err;
						
			propSym = dones[iDone];
			pObj->pMmValue->SymbolToValue(propSym, &prop);
			err = pObj->pMmList->GetValueByProperty(&pObj->curCallbacks, &prop, &callbackInfo);
			if (err == kMoaErr_NoErr) {
				
				// do the callback
				MoaMmValue theArgsValue;
				MoaError argsPresent;
				char theHandlerString[1024];
				int iarg, nbArgs;
				{
					// handler into theHandlerString
					MoaMmSymbol symbol;
					MoaMmValue symbolValue, theHandlerValue;
				  pObj->pMmValue->StringToSymbol("handler", &symbol);
					pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
				  pObj->pMmList->GetValueByProperty(&callbackInfo, &symbolValue, &theHandlerValue);
				  pObj->pMmValue->ValueRelease(&symbolValue);
				  pObj->pMmValue->ValueToString(&theHandlerValue, theHandlerString, 1024);
				  pObj->pMmValue->ValueRelease(&theHandlerValue);
				}
				
				{
					// args
					MoaMmSymbol symbol;
					MoaMmValue symbolValue;
					
				  pObj->pMmValue->StringToSymbol("args", &symbol);
					pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
				  argsPresent = pObj->pMmList->GetValueByProperty(&callbackInfo, &symbolValue, &theArgsValue);
				  pObj->pMmValue->ValueRelease(&symbolValue);
				  nbArgs = 0;
				  if (argsPresent == kMoaErr_NoErr)
					  nbArgs = pObj->pMmList->CountElements(&theArgsValue);
				}
				
				MoaMmValue res;
				MoaMmSymbol handlerSymbol = 0;
				PMoaMmValue args;

				pObj->pMmValue->StringToSymbol(theHandlerString, &handlerSymbol);
				
				// fill args
				if (nbArgs > 0) {
					args = (PMoaMmValue)pObj->pCalloc->NRAlloc(nbArgs*sizeof(MoaMmValue));
					for (iarg = 0 ; iarg < nbArgs ; iarg++)
						pObj->pMmList->GetValueByIndex(&theArgsValue, iarg+1, &args[iarg]);
				}
				
				pObj->pDrPlayer->CallHandler(handlerSymbol, nbArgs, &args[0], &res);

				if (nbArgs > 0) {
					for (iarg = 0 ; iarg < nbArgs ; iarg++)
						pObj->pMmValue->ValueRelease(&args[iarg]);
					pObj->pCalloc->NRFree(args);
				}
				
			  if (argsPresent == kMoaErr_NoErr)
					pObj->pMmValue->ValueRelease(&theArgsValue);
					
				pObj->pMmValue->ValueRelease(&callbackInfo);
				
				//	pObj->pMmList->DeleteValueByProperty(&pObj->curCallbacks, &prop);
				{
					MoaMmValue res;
					MoaMmSymbol deletePropS = 0;
					MoaMmValue args[2];
					pObj->pMmValue->StringToSymbol("DeleteProp", &deletePropS);
					args[0] = pObj->curCallbacks;
					args[1] = prop;
					pObj->pDrPlayer->CallHandler(deletePropS, 2, &args[0], &res);
				}
				
				// callbackInfo present
			}
			
			pObj->pMmValue->ValueRelease(&prop);
		}

		pIDrMovieContext->PopXtraContext(&drContextState);
		pIDrMovieContext->Release();
	}
	if (dones) pObj->pCalloc->NRFree(dones);
}

MoaError 
FLUIDXtra_IMoaMmXScript::pollCallbacks(PMoaDrCallInfo callPtr)
{

	poll(pObj);
	
	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return(kMoaErr_NoErr);
}

MoaError 
FLUIDXtra_IMoaMmXScript::scheduleCallback(PMoaDrCallInfo callPtr)
{
	int res;
	MoaMmValue callbackInfo;
	
	// get callbackInfo arg
	res = _getPlistArg(callPtr, 2, "callbackInfo", &callbackInfo, true);
	if (res != 0)
	  return(kMoaErr_NoErr);
	  
	// parse callBackInfo for std props
	unsigned int date = 0;
	int absolute = 0;
	short src = -1, dest;
	if (_parseSeqPlist(callPtr, callbackInfo, &date, &absolute, &dest, &src, 1) != 0)
		return kMoaErr_NoErr;
	
	// check handler presence
	{
		MoaMmSymbol symbol;
		MoaMmValue symbolValue, stringValue;
		MoaError err;
		
	  pObj->pMmValue->StringToSymbol("handler", &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	  err = pObj->pMmList->GetValueByProperty(&callbackInfo, &symbolValue, &stringValue);
	  pObj->pMmValue->ValueRelease(&symbolValue);
		if (err != kMoaErr_NoErr) {
	  	fluid_log(FLUID_ERR, "handler is missing");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	    return kMoaErr_NoErr;
		}
	  if (!_checkValueType(&stringValue, kMoaMmValueType_String)) {
		  pObj->pMmValue->ValueRelease(&stringValue);
	  	fluid_log(FLUID_ERR, "handler should be a string");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
	    return kMoaErr_NoErr;
		}
		pObj->pMmValue->ValueRelease(&stringValue);
	}
	
	// check args type
	{
		// args
		MoaMmSymbol symbol;
		MoaMmValue symbolValue, argsValue;
		MoaError err;
		
	  pObj->pMmValue->StringToSymbol("args", &symbol);
		pObj->pMmValue->SymbolToValue(symbol, &symbolValue);
	  err = pObj->pMmList->GetValueByProperty(&callbackInfo, &symbolValue, &argsValue);
	  pObj->pMmValue->ValueRelease(&symbolValue);
	  if (err == kMoaErr_NoErr) {
		  if (!_checkValueType(&argsValue, kMoaMmValueType_List)) {
			  pObj->pMmValue->ValueRelease(&argsValue);
		  	fluid_log(FLUID_ERR, "args should be a list");
				pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_BADARGUMENT, &callPtr->resultValue);
		    return kMoaErr_NoErr;
		  }
		  pObj->pMmValue->ValueRelease(&argsValue);
	  }
	}

	// store our callback
	MoaMmSymbol callbackID;
	_storeCallback(callPtr, &callbackInfo, &callbackID);
	
  fluid_log(FLUID_DBG, "fluidsynth: posting callback [date=%d,abd=%d]", date, absolute);

	// create event
	int fluid_res;
	fluid_event_t *evt = new_fluid_event();
	fluid_event_set_source(evt, src);
	fluid_event_set_dest(evt, pObj->xtraSeqID);
	
	fluid_event_timer(evt, (void *)(callbackID));
	fluid_res = fluid_sequencer_send_at(pObj->sequencer, evt, date, absolute);
	if (fluid_res != 0) {
		_forgetCallback(callPtr, callbackID);
		delete_fluid_event(evt);
		pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_FLUIDERR, &callPtr->resultValue);
  	return kMoaErr_NoErr;
	}
	delete_fluid_event(evt);

	pObj->pMmValue->IntegerToValue(0, &callPtr->resultValue);
  return(kMoaErr_NoErr);
}



/**************************************/
/********* The Main Switch ************/
/**************************************/

STDMETHODIMP 
FLUIDXtra_IMoaMmXScript::Call(PMoaMmCallInfo callPtr)
{
	
	if (callPtr->methodSelector == m_getError) {
		_fluid_xtra_getErrorString(pObj, callPtr);
		return kMoaErr_NoErr;
	}
	
	_fluid_xtra_setErrorString(pObj, "");
	
	if (callPtr->methodSelector == m_new) {
		mynew(callPtr);
		
	} else if (callPtr->methodSelector == m_getSettingsOptions) {
		getSettingsOptions(callPtr);
    
	} else if (callPtr->methodSelector == m_getSettingDefaultValue) {
		getSetting(callPtr);
		
	} else if (callPtr->methodSelector == m_free) {
	  free(callPtr);
	  
	} else if (callPtr->methodSelector == m_debug) {
	  debug(callPtr);
	  
	} else {
	 if (pObj->synth == NULL || pObj->sequencer == NULL) {
	    fluid_log(FLUID_ERR, "fluidsynth: no synthesizer");
			pObj->pMmValue->IntegerToValue(FLUIDXTRAERR_NOSYNTH, &callPtr->resultValue);
	    
	  } else {

			switch ( callPtr->methodSelector ) {

			case m_getCPULoad:
			  getCPULoad(callPtr);
			  break;

			case m_downloadFolderSetPath: downloadFolderSetPath(callPtr); break;
			
				case m_downloadFolderContainsLocal:
					downloadFolderContainsLocal(callPtr);
					break;
				case m_downloadFolderGetLocalProps:
					downloadFolderGetLocalProps(callPtr);
					break;
					
			case m_downloadFolderStartDownload:
			  downloadFolderStartDownload(callPtr);
			  break;
			case m_downloadFolderGetDownloadRatio:
			  downloadFolderGetDownloadRatio(callPtr);
			  break;
			case m_downloadFolderDeleteLocal:
			  downloadFolderDeleteLocal(callPtr);
			  break;
			case m_downloadFolderAbortDownload:
			  downloadFolderAbortDownload(callPtr);
			  break;
		
			case m_getChannelsCount:
			  getChannelsCount(callPtr);
			  break;

			case m_loadSoundFont:
			  loadSoundFont(callPtr);
			  break;

			case m_createSoundFont:
			  createSoundFont(callPtr, true);
			  break;

        case m_unloadSoundFont:
          unloadSoundFont(callPtr);
          break;
          
        case m_getSetting:
          getSetting(callPtr);
          break;

			case m_reloadSoundFont:
			  reloadSoundFont(callPtr);
			  break;
			  
			case m_getSoundFontsStack:
			  getSoundFontsStack(callPtr);
			  break;

			case m_getSoundFontInfo:
			  getSoundFontInfo(callPtr);
			  break;
			  
                case m_loadSampleFile:
                    loadSampleFile(callPtr);
                    break;
                    
                case m_loadSampleFileExtract:
                    loadSampleFileExtract(callPtr);
                    break;
                    
                case m_getSampleFileSamples:
                    getSampleFileSamples(callPtr);
                    break;
	
			case m_loadSampleMember:
			  loadSampleMember(callPtr);
			  break;
			  
			case m_deleteSample:
			  deleteSample(callPtr);
			  break;
			  
			case m_getSampleName:
			  getSampleName(callPtr);
			  break;

			case m_getRootKey:
			  getRootKey(callPtr);
			  break;

			case m_getKeyRange:
			  getKeyRange(callPtr);
			  break;

			case m_getFrameCount:
			  getFrameCount(callPtr);
			  break;

			case m_getFrameRate:
			  getFrameRate(callPtr);
			  break;

			case m_setLoop:
			  setLoop(callPtr);
			  break;

			case m_getLoop:
			  getLoop(callPtr);
			  break;

			case m_setLoopPoints:
			  setLoopPoints(callPtr);
			  break;

			case m_getLoopPoints:
			  getLoopPoints(callPtr);
			  break;

			case m_setEnvelope:
			  setEnvelope(callPtr);
			  break;

			case m_getEnvelope:
			  getEnvelope(callPtr);
			  break;

			case m_setReverb:
			  setReverb(callPtr);
			  break;
			case m_getReverb:
			  getReverb(callPtr);
			  break;
			case m_setReverbProp:
			  setReverbProp(callPtr);
			  break;
			case m_getReverbProp:
			  getReverbProp(callPtr);
			  break;
			    
			case m_setChorus:
			  setChorus(callPtr);
			  break;
			case m_getChorus:
			  getChorus(callPtr);
			  break;
			case m_setChorusProp:
			  setChorusProp(callPtr);
			  break;
			case m_getChorusProp:
			  getChorusProp(callPtr);
			  break;
			    
			case m_getTimeUnit:
			  getTimeUnit(callPtr);
			  break;
			  
			case m_setTimeUnit:
			  setTimeUnit(callPtr);
			  break;

			case m_getDestinations:
			  getDestinations(callPtr);
			  break;

			case m_getTime:
			  getTime(callPtr);
			  break;

			case m_scheduleCallback:
			  scheduleCallback(callPtr);
			  break;

			case m_pollCallbacks:
			  pollCallbacks(callPtr);
			  break;
			  						
			case m_removeEvents:
			  removeEvents(callPtr);
			  break;
			
			case m_noteOn:
			  noteon(callPtr);
			  break;
			  
			case m_noteOff:
			  noteoff(callPtr);
			  break;
			  
			case m_note:
			  note(callPtr);
			  break;
			  		  
			case m_allNotesOff:
			  allNotesOff(callPtr);
			  break;
			
			case m_reset:
			  reset(callPtr);
			  break;
			
			case m_allSoundsOff:
			  allSoundsOff(callPtr);
			  break;
			
			case m_programChange:
			  programChange(callPtr);
			  break;
			  
			case m_getProgram:
			  getProgram(callPtr);
			  break;
			   
			case m_controlChange:
			  controlChange(callPtr);
			  break;

			case m_getControl:
			  getControl(callPtr);
			  break;
			
			case m_getControls:
			  getControls(callPtr);
			  break;
			
			case m_setGenerator:
			  setGenerator(callPtr);
			  break;
			
			case m_getGenerator:
			  getGenerator(callPtr);
			  break;
						
			case m_startRecord:
				startRecord(callPtr);
				break;
				
			case m_stopRecord:
				stopRecord(callPtr);
				break;
				
			case m_isRecording:
				isRecording(callPtr);
				break;
					
				case m_getMasterGain:
			  getMasterGain(callPtr);
			  break;
			
			case m_setMasterGain:
			  setMasterGain(callPtr);
			  break;
			
			}
		}
	}
	
  return(kMoaErr_NoErr);
}

