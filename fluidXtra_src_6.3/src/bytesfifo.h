/* bytesfifo__H
   Antoine Schmitt June 2004
   
   Sept 2006 : added dirtyFrames
   Sept 2006 : ported to Mac : no mutex
   May 2012 : was asSoundStreamFifo, changed name, ported to fluid mutex
*/

#ifndef bytesfifo__H
#define bytesfifo__H 

#include <glib.h>

class bytesfifo {
public:
	bytesfifo(int bytesCount, bool fixedSize);
	~bytesfifo();

	int queueBytesFrom(void *buffer, int bytesCount);
	int unqueueBytesInto(void *buffer, int bytesCount);
	int peekBytesInto(void *buffer, int bytesCount);
	int dirtyBytes(int bytesCount);
	void empty();
	void fill();
	int availableBytes();

	int mWriteIndex;
	int mReadIndex;

private:
	void _reallocDouble();
	char *	mData;
	GStaticMutex	mMutex;
	int		mBufBytesCount;
	bool	mFixedSize;
};

#endif // bytesfifo__H
