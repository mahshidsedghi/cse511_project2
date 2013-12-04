#include "ClientCache.hh"
#include <stdexcept>

using namespace std;

size_t ClientCache::clientID = 0;

ClientCache::ClientCache(){
	clientID++; 
	int rc;
	stringstream errMsg;
	blockT b1;
	b1.status = 'F';
	for(int i = 0; i < BUFFER_CACHE_CAPACITY; i++)
		freeSpace.push_front(b1);
	cout << "freespace size: " << freeSpace.size() <<endl;
	rc = pthread_create(&harvester, NULL, &ClientCache::harvestingFunc, (void *)clientID);
	if(rc)
	{
		errMsg << "Could not create harvester thread in client cache ID:" << clientID;
		throw runtime_error(errMsg.str());
	}
	rc = pthread_create(&flusher, NULL, &ClientCache::flushingFunc, (void *)clientID);
	if(rc)
	{
		errMsg << "Could not create flusher thread in client cache ID:" << clientID;
		throw runtime_error(errMsg.str());
	}
}

void* ClientCache::harvestingFunc(void *id){
	size_t ID;
	ID = size_t(id);
	cout << "harvester thread created successfully for client ID:" << ID << endl;
/*	while (true) {
		if(freeSpace.size() < FREE_LIST_MIN_T) {
			//do harvesting till size reaches FREE_LIST_MAX_T
			//this should be done based on LRU! but how?
		}
	}*/
	return 0;
}

void* ClientCache::flushingFunc(void *id){
	size_t ID;
	ID = size_t(id);
	cout << "flusher thread created successfully for client ID:" << ID << endl;
	while (true) {
		usleep(30000000);
		//do flushing
	}
	return 0;
}

void ClientCache::insertSingleBlockIntoCache(blockT b) {
//	assert(freeSpace.size() + usedSpace.size() <= BUFFER_CACHE_CAPACITY);
	if (!freeSpace.empty()) {
		std::pair<LBA,blockT> mypair = make_pair(b.blockAdr,b);
		freeSpace.pop_front();
		usedSpace.insert(mypair);
	}
	else {
		stringstream errMsg;
		errMsg << "Trying to write to the cache while no free space is available";
		throw runtime_error(errMsg.str());
	}
}

void ClientCache::insertMultipleBlocksIntoCache(blockT* blocks, size_t n) {
	if (freeSpace.size() < n) {
		stringstream errMsg;
		errMsg << "Trying to write to the cache while not enough free space is available";
		throw runtime_error(errMsg.str());
	}
	else {
		for (size_t i = 0; i < n; i++)
			insertSingleBlockIntoCache(blocks[i]);
	}
}

void ClientCache::showUsedSpace() {
	cout << "Used space size:" << usedSpace.size() << endl;
//	for (auto &pair : usedSpace)
//		cout << "LBA_" << (size_t)pair.first << "[0]: " << pair.second.data[0] << endl;
	for (std::tr1::unordered_map<LBA,blockT>::iterator it = usedSpace.begin(); it != usedSpace.end(); ++it)
		cout << "LBA_" << it->first << "[0]: " << it->second.data[0] << endl;
}

bool ClientCache::lookupBlockInCache(LBA block_ID) {
	if(usedSpace.find(block_ID) != usedSpace.end()) //will it search the whole bucket?
		return true;
	else
		return false;
}

blockT* ClientCache::getBlockFromCache(LBA block_ID) {
	std::tr1::unordered_map<LBA,blockT>::iterator it;
	it = usedSpace.find(block_ID);
	if(it != usedSpace.end()) //will it search the whole bucket?
		return &(it->second);
	else
		return NULL;
}

void ClientCache::putBlockIntoCache(blockT b) {
	std::pair<LBA,blockT> mypair = make_pair(b.blockAdr,b);
	usedSpace.insert(mypair);
}

void ClientCache::showCacheStatus()
{
	cout << "Client #"<< clientID << " Free Space Size: " << freeSpace.size() << endl;
	cout << "Client #"<< clientID << " Used Space Size: " << usedSpace.size() << endl;
}

bool readFromFileServer(char* file_name, LBA block_ID,std::string IP, size_t port_number) {
		
}

bool writeToFileServer(char* file_name, LBA block_ID,std::string IP, size_t port_number) {

}

ClientCache::~ClientCache(){}

/*int main() {
	ClientCache mycache;
	blockT b1;
	for (int i = 0; i < 10; i++)
	{
		b1.blockAdr = (size_t)i;
		b1.data[0] = i;
		mycache.writeSingleBlockToCache(b1);
	}
	b1.blockAdr = (size_t)20;
	mycache.showUsedSpace();
	cout << "lookup result:" << mycache.lookupBlockInCache(b1.blockAdr) << endl;
	mycache.showCacheStatus();

//	string str = "hello world";
//	std::hash<string> str_hash;
//	cout << "hash of str: " << sizeof(str_hash(str)) << endl;
	list<char> x;
	return 0;
} */

