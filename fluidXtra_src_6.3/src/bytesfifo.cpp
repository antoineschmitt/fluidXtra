/* bytesfifo.cpp
   Antoine Schmitt June 2004  
*/


#include "bytesfifo.h"

#ifndef _WIN32
// Macintosh
#include <Memory.h>
#include <OSUtils.h>
#endif
#include <stdlib.h>
#include <string.h>

#define errOK 0
#define errStdErr -1

bytesfifo::bytesfifo(int bytesCount, bool fixedSize) {

	mBufBytesCount = bytesCount;
	mFixedSize = fixedSize;
	mData = NULL;
	mWriteIndex = 0;
	mReadIndex = 0;
	g_static_mutex_init(&mMutex);

	mData = (char *)malloc(mBufBytesCount);
}

bytesfifo::~bytesfifo() {
	free(mData);
	mData = NULL;
	g_static_mutex_free(&mMutex);
}

void bytesfifo::_reallocDouble() {
	int newFrameCount = mBufBytesCount*2;
	char *	mDataNew = (char *)malloc(newFrameCount);
	int available = availableBytes();
	if (available > 0) {
		if (mReadIndex + available > mBufBytesCount) {
			int tmp = mBufBytesCount - mReadIndex;
			memcpy(mDataNew, mData + mReadIndex, tmp);
			memcpy(mDataNew + mReadIndex, mData, (available - tmp));
		} else {
			memcpy(mDataNew, mData + mReadIndex, available);
		}
	}

	// ok
	free(mData);
	mData = mDataNew;
	mBufBytesCount = newFrameCount;
	mReadIndex = 0;
	mWriteIndex = available;
}

int bytesfifo::queueBytesFrom(void *buffer, int bytesCount) {
	char *bBuf = (char *)buffer;
	int tmp;

	// just to be sure
	if (bytesCount > mBufBytesCount) bytesCount = mBufBytesCount;

	g_static_mutex_lock(&mMutex);

	// enough room ? if not, push read cursor and forget old data
	tmp = mReadIndex - mWriteIndex - 1;
	if (tmp < 0) tmp += mBufBytesCount;
	if (bytesCount > tmp) {
		if (mFixedSize) {
			// push read of bytesCount - tmp
			mReadIndex += 1; // bytesCount - tmp; // BIZARRE : why did I do this ?
			if (mReadIndex >= mBufBytesCount) mReadIndex -= mBufBytesCount;
		} else _reallocDouble();
	}

	// copy data
	if (mWriteIndex + bytesCount > mBufBytesCount) {
		int fc1 = (mBufBytesCount - mWriteIndex);
		memcpy(mData + mWriteIndex, bBuf, fc1);
		memcpy(mData, bBuf + fc1, (bytesCount - fc1));
	} else memcpy(mData + mWriteIndex, bBuf, bytesCount);

	// increment write cursor
	mWriteIndex += bytesCount;
	if (mWriteIndex >= mBufBytesCount) mWriteIndex -= mBufBytesCount;

	g_static_mutex_unlock(&mMutex);

	return errOK;
}

int bytesfifo::availableBytes() {
	int tmp;
	tmp = mWriteIndex - mReadIndex;
	if (tmp < 0) tmp += mBufBytesCount;
	return (tmp);
}

void bytesfifo::empty() {
	g_static_mutex_lock(&mMutex);
	mWriteIndex = mReadIndex = 0;
	g_static_mutex_unlock(&mMutex);
}

void bytesfifo::fill() {
	g_static_mutex_lock(&mMutex);
	mReadIndex = 0;
	mWriteIndex = mBufBytesCount - 1;
	g_static_mutex_unlock(&mMutex);
}

// fails if not enough data
int bytesfifo::peekBytesInto(void *buffer, int bytesCount) {
	// data available ?
	int avail = availableBytes();
	if (avail < bytesCount) return errStdErr;

	// there is indeed data
	// get it
	char *bBuf = (char *)buffer;
	if (mReadIndex + bytesCount > mBufBytesCount) {
		int fc1 = (mBufBytesCount - mReadIndex);
		memcpy(bBuf, mData + mReadIndex, fc1);
		memcpy(bBuf + fc1, mData, (bytesCount - fc1));
	} else memcpy(bBuf, mData + mReadIndex, bytesCount);

	return errOK;
}

int bytesfifo::unqueueBytesInto(void *buffer, int bytesCount) {

	// data available ?
	int avail = availableBytes();
	if (avail < bytesCount) {
		return errStdErr;
	}
	
	g_static_mutex_lock(&mMutex);

	// there is indeed data
	// get it
	char *bBuf = (char *)buffer;
	if (mReadIndex + bytesCount > mBufBytesCount) {
		int fc1 = (mBufBytesCount - mReadIndex);
		memcpy(bBuf, mData + mReadIndex, fc1);
		memcpy(bBuf + fc1, mData, (bytesCount - fc1));
	} else memcpy(bBuf, mData + mReadIndex, bytesCount);

	// increment write cursor
	mReadIndex += bytesCount;
	if (mReadIndex >= mBufBytesCount) mReadIndex -= mBufBytesCount;

	g_static_mutex_unlock(&mMutex);

	return errOK;
}

int bytesfifo::dirtyBytes(int bytesCount) {
	// increment write cursor
	mReadIndex += bytesCount;
	if (mReadIndex >= mBufBytesCount) mReadIndex -= mBufBytesCount;
	return errOK;
}
