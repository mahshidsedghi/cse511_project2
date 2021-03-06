#ifndef ClientCache_H_
#define ClientCache_H_

#include <iostream>
#include <string>
#include <list>
#include "data_types.hh"
#include "PracticalSocket.hh"
#include "FileDesc.hh"
#include <pthread.h>
#include <sstream>
#include <tr1/unordered_map>
#include <utility>
#include <unistd.h>
#include <assert.h>
#include <list>
#include "config.hh"
#include <cmath>
#include <string.h>
#include <stdlib.h>
#define RCVBUFSIZE 32
#include "net_addresses.hh"
#include <queue>

class ClientCache{
public:
	
	int revoker_port; 
	static size_t clientID;
	ClientCache();
	static size_t LAST_ACCESS;
	static const size_t FREE_LIST_MIN_T = 2035; //FIXME
	static const size_t FREE_LIST_MAX_T = 2042; //FIXME
	static const int BUFFER_CACHE_CAPACITY = 2*1024/PFS_BLOCK_SIZE; //max number of blocks in buffer cache

	
	static pthread_mutex_t cache_mutex;

	void *harvestingFunc();
	static void *callHarvestingFunc(void* arg) {
		//pthread_detach(pthread_self()); 
		return ((ClientCache*)arg) -> harvestingFunc();
	}

	void flush(string msg);
	void *flushingFunc(void);
	static void *callFlushingFunc(void* arg) {
		//pthread_detach(pthread_self()); 
		return ((ClientCache*)arg) -> flushingFunc();
	}

	void *revokingFunc(void);
	static void *callRevokingFunc(void* arg){
		return ((ClientCache*)arg) -> revokingFunc(); 
	}

	void HandleRevoker(TCPSocket *sock); 
	
	void insertSingleBlockIntoCache(blockT); //cache a new block
	void insertMultipleBlocksIntoCache(blockT*,size_t);
	bool lookupBlockInCache(string, size_t);
	blockT* getBlockFromCache(string, size_t);
	void putBlockIntoCache(blockT); //overwrite an existing block
	void showUsedSpace();
	void showCacheStatus();
	blockT* readFromFileServer(string file_name, size_t block_offset, std::string IP, int port_number);
	int writeToFileServer(blockT b);
	
	~ClientCache();
	
private:
	std::list<blockT> freeSpace;
	std::tr1::unordered_map<LBA,blockT> usedSpace; //only for clean and dirty blocks
	pthread_t harvester;
	pthread_t flusher;
	pthread_t revoker; 
};

#endif

