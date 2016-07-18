#ifndef _SUPPORTFOLDER_H
#define _SUPPORTFOLDER_H

// all functions accept HFS paths on Mac (separator = ':' and path starts with DiskName)
bool sfGetSchockwaveSupportFolderPath(char *buf, int buflen);
// posix
void pathHFS2POSIX(char *path, int buflen);

bool sfDirectoryExists(char *pathName, int buflen);
bool sfFileExists(char * pathName, int buflen);
int sfFileSize(char * pathName, int buflen);
bool sfFileDate(char * pathName, int buflen, int *yearPtr, int *monthPtr, int * dayPtr);
bool sfCreateDirectory(char *path, int buflen);
bool sfDeleteFile(char *filePath, int buflen);
void sfStripLastSeparator(char *filePath);
void sfBuildPathFromFolderAndFile(char *pathbuf, int buflen, char * folder, char * file);
bool sfPathContainsSeparators(char *filePath);
bool sfExtractLastComponentFromPath(char *compbuf, int buflen, char * path);
void getRootPath(char * buf, int buflen); // "" on Windows, the disk name on Mac

#endif

