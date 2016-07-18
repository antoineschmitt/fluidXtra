// supportFolder.cpp
// Antoine Schmitt 6nov2007
// access to shockwave support folder

#include "moaxtra.h"
#include "supportFolder.h"

#include <sys/stat.h> //stat
#include <fcntl.h> // fopen
#include <unistd.h> // close
#include <stdlib.h> // malloc
#include <time.h>

#ifdef _WINDOWS
#define snprintf _snprintf
#endif


void pathHFS2POSIX(char *path, int buflen) {
#ifdef _DIRECTOR_XCODE
	// must use POSIX file paths
	CFStringRef pathStr = CFStringCreateWithCString (NULL, path, kCFStringEncodingUTF8);
	CFURLRef pathUrl = CFURLCreateWithFileSystemPath (NULL, pathStr, kCFURLHFSPathStyle, false);
	CFStringRef newpathStr = CFURLCopyFileSystemPath (pathUrl, kCFURLPOSIXPathStyle);
	CFStringGetCString (newpathStr, path, buflen, kCFStringEncodingUTF8);
	CFRelease(newpathStr);
	CFRelease(pathUrl);
	CFRelease(pathStr);
#endif
}

bool sfDirectoryExists(char *pathName, int buflen) {
	char *tmpbuf = (char *)malloc(buflen);
	strncpy(tmpbuf, pathName, buflen);
#ifdef _DIRECTOR_XCODE
	// convert to POSIX first
	pathHFS2POSIX(tmpbuf, buflen);
#endif
	
	bool itIs = false;
	struct stat statinfo;
	int res = stat(tmpbuf, &statinfo);
	if (res != -1) {
		itIs = ((statinfo.st_mode & S_IFMT) == S_IFDIR);	
	}

	free(tmpbuf);
	return itIs;
}

bool sfCreateDirectory(char *pathName, int buflen) {

#ifdef _WINDOWS
	return (_mkdir(pathName) == 0);
#elif defined(_DIRECTOR_XCODE)
	char *tmpbuf = (char *)malloc(buflen);
	strncpy(tmpbuf, pathName, buflen);
	// convert to POSIX first
	pathHFS2POSIX(tmpbuf, buflen);
	bool res = (mkdir(tmpbuf, S_IRWXU|S_IRGRP|S_IROTH) == 0); // 744
	free(tmpbuf);
	return res;
#else
	return(mkdir(pathName) == 0);
#endif

}

bool sfFileExists(char * pathName, int buflen) {
	char *tmpbuf = (char *)malloc(buflen);
	strncpy(tmpbuf, pathName, buflen);
#ifdef _DIRECTOR_XCODE
	// convert to POSIX first
	pathHFS2POSIX(tmpbuf, buflen);
#endif
	
	struct stat statinfo;
	int res = stat(tmpbuf, &statinfo);
	free(tmpbuf);

	return (res != -1);
}

int sfFileSize(char * pathName, int buflen) {
	char *tmpbuf = (char *)malloc(buflen);
	strncpy(tmpbuf, pathName, buflen);
#ifdef _DIRECTOR_XCODE
	// convert to POSIX first
	pathHFS2POSIX(tmpbuf, buflen);
#endif
	
	struct stat statinfo;
	int res = stat(tmpbuf, &statinfo);
	free(tmpbuf);
	
	if (res == -1) return 0;
	return (statinfo.st_size);
}

bool sfFileDate(char * pathName, int buflen, int *yearPtr, int *monthPtr, int * dayPtr) {
	char *tmpbuf = (char *)malloc(buflen);
	strncpy(tmpbuf, pathName, buflen);
#ifdef _DIRECTOR_XCODE
	// convert to POSIX first
	pathHFS2POSIX(tmpbuf, buflen);
#endif
	
	struct stat statinfo;
	int res = stat(tmpbuf, &statinfo);
	free(tmpbuf);
	
	if (res == -1) return false;
	
	struct tm *ptm = gmtime(&(statinfo.st_mtime));
	*yearPtr = ptm->tm_year + 1900;
	*monthPtr = ptm->tm_mon + 1; // 1 to 12
	*dayPtr = ptm->tm_mday;
	
	return true;
}

bool sfDeleteFile(char *pathName, int buflen) {
	char *tmpbuf = (char *)malloc(buflen);
	strncpy(tmpbuf, pathName, buflen);
#ifdef _DIRECTOR_XCODE
	// convert to POSIX first
	pathHFS2POSIX(tmpbuf, buflen);
#endif
	
	unlink(tmpbuf);
	
	free(tmpbuf);
	return true;
}

void sfStripLastSeparator(char * folder) {
#ifdef _WINDOWS
	int follen = strlen(folder);
	if (follen > 0 && ((folder[follen-1] == '/') || (folder[follen-1] == '\\')))
		folder[follen-1] = 0;
#else
	char sep = ':';
	int follen = strlen(folder);
	if (follen > 0 && (folder[follen-1] == sep)) // already separator at the end
		folder[follen-1] = 0;
#endif
}


void sfBuildPathFromFolderAndFile(char *pathbuf, int buflen, char * folder, char * file) {
	char sep;
#ifdef _WINDOWS
	sep = '/';
#else
	sep = ':';
#endif

	snprintf(pathbuf, buflen, "%s%c%s", folder, sep, file);
}

bool sfPathContainsSeparators(char *filePath) {
	char *c = filePath;
#ifdef _WINDOWS
	while (*c++) if (*c == '/' || *c == '\\') return true;
#else
	// Mac
	while (*c++) if (*c == ':') return true;
#endif
	return false;
}

bool sfExtractSuperpathFromPath(char *uppathbuf, int buflen, char * path) {
	int l = strlen(path);
	if (l == 0 || l == 1) return false;
	
	char *startpc = path+l-1;
	char *pc;
	
	// last sep
#ifdef _WINDOWS
	if (*startpc == '/' || *startpc == '\\') startpc--;	// skip sep if last char
	pc = startpc;
	while (pc >= path && *pc != '/' && *pc != '\\') pc--;
#else // MAC
	if (*startpc == ':') startpc--;	// skip sep if last char
	pc = startpc;
	while (pc >= path && *pc != ':') pc--;
#endif

	// not found
	if (pc < path) return false;
	
	if (buflen > pc-path) buflen = pc-path;
	strncpy(uppathbuf, path, buflen);
	uppathbuf[buflen] = 0;

	return true;
}

bool sfExtractLastComponentFromPath(char *compbuf, int buflen, char * path) {
	int l = strlen(path);
	if (l == 0 || l == 1) return false;
	
	char *startpc = path+l-1;
	char *pc;
	
	// last sep
#ifdef _WINDOWS
	if (*startpc == '/' || *startpc == '\\') startpc--;	// skip sep if last char
	pc = startpc;
	while (pc >= path && *pc != '/' && *pc != '\\') pc--;
#else // MAC
	if (*startpc == ':') startpc--;	// skip sep if last char
	pc = startpc;
	while (pc >= path && *pc != ':') pc--;
#endif

	if (pc < path) return false;
	
	if (buflen > startpc-pc) buflen = startpc-pc;
	strncpy(compbuf, pc+1, buflen);
	compbuf[buflen] = 0;

	return true;
}



bool sfGetSchockwaveSupportFolderPath(char *buf, int buflen) {
	char dirPath[1024];
#ifdef _WINDOWS
	GetModuleFileName((HINSTANCE)gXtraFileRef, dirPath, 1024);
#else // Mac
	// I'm sure it could be simpler...
	FSRef fileRef;
	FSpMakeFSRef((FSSpec *)gXtraFileRef, &fileRef);
	CFURLRef tPathRef = CFURLCreateFromFSRef(NULL, &fileRef);
	CFStringRef dPathStr = CFURLCopyFileSystemPath (tPathRef, kCFURLHFSPathStyle);
	CFStringGetCString(dPathStr, dirPath, 1024, kCFStringEncodingUTF8);
	CFRelease(dPathStr);
	CFRelease(tPathRef);
#endif

	char tmpBuf[1024];

	// walk up to find the Xtras folder
	int maxUps = 7;
	bool found = false;

	while (maxUps-- > 0) {
		// up one level
		if (!sfExtractSuperpathFromPath(tmpBuf,  1024, dirPath)) break;
		strncpy(dirPath, tmpBuf, 1024);

		// extract last component
		if (!sfExtractLastComponentFromPath(tmpBuf, 1024, dirPath))
			break;

		// compare with "Xtras"
		if (!strcmp(tmpBuf, "Xtras") || !strcmp(tmpBuf, "xtras")) {
			// ok, fond Xtras folder
			if (!sfExtractSuperpathFromPath(tmpBuf, 1024, dirPath)) break;
			strncpy(dirPath, tmpBuf, 1024);

			// find DswMedia
			char dswPath[1024];
			char *possibilities[4] = {"DswMedia", "dswmedia", "Dswmedia", "dswMedia"};
			for (int i = 0; i < 4 ; i++) {
				sfBuildPathFromFolderAndFile(dswPath, 1024, dirPath, possibilities[i]);
				if (sfDirectoryExists(dswPath, 1024)) {
					// exists !

					// create subdir SoundFont
					sfBuildPathFromFolderAndFile(tmpBuf, 1024, dswPath, "SoundFonts");
					bool dirok = true;
					if (!sfDirectoryExists(tmpBuf, 1024))
						if (!sfCreateDirectory(tmpBuf, 1024))
							dirok = false;

					if (dirok) {
						// ok, return it
						strncpy(buf, tmpBuf, buflen);
						buf[buflen] = 0;
						found = true;
					}
					break; // for
				}
			}
			if (!found) {
				//  not found in Support folder...
				// create it
				sfBuildPathFromFolderAndFile(dswPath, 1024, dirPath, "dswmedia");
				if (sfCreateDirectory(dswPath, 1024)) {
					sfBuildPathFromFolderAndFile(tmpBuf, 1024, dswPath, "SoundFonts");
					if (sfCreateDirectory(tmpBuf, 1024)) {
						strncpy(buf, tmpBuf, buflen);
						buf[buflen] = 0;
						found = true;
					}
				}
			}
			break; // dont go higher in any case
		}
	}
	return found;
}

// "" on Windows, the disk name on Mac
void getRootPath(char * buf, int buflen) {
	
#ifdef _WINDOWS
	//strcpy(buf, "");
#else // Mac
	// get path to Xtra itself
	// I'm sure it could be simpler...
	FSRef fileRef;
	FSpMakeFSRef((FSSpec *)gXtraFileRef, &fileRef);
	CFURLRef tPathRef = CFURLCreateFromFSRef(NULL, &fileRef);
	CFStringRef dPathStr = CFURLCopyFileSystemPath (tPathRef, kCFURLHFSPathStyle);
	CFStringGetCString(dPathStr, buf, buflen, kCFStringEncodingUTF8);
	CFRelease(dPathStr);
	CFRelease(tPathRef);
#endif

	// find first ":" and replace by 0
	char *pp = buf;
	while (*pp != 0) {
		if (*pp == ':') {
			*pp = 0;
			break;
		}
		pp++;
	}

}

