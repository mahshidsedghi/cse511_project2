#ifndef ClientCache_H_
#define ClientCache_H_

#include <iostream>
#include <string>
#include <list>
#include "data_types.hh"
#include <pthread.h>
#include <sstream>
#include <tr1/unordered_map>
#include <utility>
#include <unistd.h>
#include <assert.h>
#include <list>
#include "config.hh"

class ClientCache{
public:
	static size_t clientID;
	ClientCache();
	static const int FREE_LIST_MIN_T = 10;
	static const int FREE_LIST_MAX_T = 20;
	static const int BUFFER_CACHE_CAPACITY = 2*1024/PFS_BLOCK_SIZE; //max number of blocks in buffer cache
	static void *harvestingFunc(void*);
	static void *flushingFunc(void*);
	void insertSingleBlockIntoCache(blockT); //cache a new block
	void insertMultipleBlocksIntoCache(blockT*,size_t);
	bool lookupBlockInCache(LBA);
	blockT* getBlockFromCache(LBA);
	void putBlockIntoCache(blockT); //overwrite an existing block
	void showUsedSpace();
	void showCacheStatus();
	blockT readFromFileServer(char* file_name, LBA block_ID,std::string IP, int port_number);
	int writeToFileServer(char* file_name, LBA block_ID,std::string IP, size_t port_number);
	
	~ClientCache();
	
private:
	std::list<blockT> freeSpace;
	std::tr1::unordered_map<LBA,blockT> usedSpace; //only for clean and dirty blocks
	pthread_t harvester;
	pthread_t flusher;
};

#endif

