#ifndef ClientCache_H_
#define ClientCache_H_

#include <iostream>
#include <string>
#include <list>
#include "data_types.h"
#include <pthread.h>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <unistd.h>
#include <assert.h>
#include <list>

class ClientCache{
public:
	size_t clientID;
	ClientCache(size_t);
	static const int FREE_LIST_MIN_T = 10;
	static const int FREE_LIST_MAX_T = 20;
	static const int BUFFER_CACHE_CAPACITY = 2*1024/PFS_BLOCK_SIZE; //max number of blocks in buffer cache
	static void *harvestingFunc(void*);
	static void *flushingFunc(void*);
	void writeSingleBlockToCache(blockT);
	void writeMultipleBlocksToCache(blockT*,size_t);
	bool lookupBlockInCache(blockT);
	void showUsedSpace();
	void showCacheStatus();
	~ClientCache();
	
private:
	std::list<blockT> freeSpace;
	std::unordered_map<LBA,blockT> usedSpace; //only for clean and dirty blocks
	pthread_t harvester;
	pthread_t flusher;
};

#endif

