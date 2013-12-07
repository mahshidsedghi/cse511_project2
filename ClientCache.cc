#include "ClientCache.hh"
#include <stdexcept>
#include <signal.h>

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

//	rc = pthread_create(&harvester, NULL, &ClientCache::harvestingFunc, (void *)clientID);
	rc = pthread_create(&harvester, NULL, &ClientCache::callHarvestingFunc, this);
	if(rc)
	{
		errMsg << "Could not create harvester thread in client cache ID:" << clientID;
		throw runtime_error(errMsg.str());
	}

//	rc = pthread_create(&flusher, NULL, &ClientCache::flushingFunc, (void *)clientID);
	rc = pthread_create(&flusher, NULL, &ClientCache::callFlushingFunc, this);
	if(rc)
	{
		errMsg << "Could not create flusher thread in client cache ID:" << clientID;
		throw runtime_error(errMsg.str());
	}

}


void* ClientCache::harvestingFunc(){
//	size_t ID;
//	ID = size_t(id);
	cout << "harvester thread created successfully for client ID:" << endl;//<< ID << endl;
	while (true) {
		if(freeSpace.size() < FREE_LIST_MIN_T) {
			//do harvesting till size reaches FREE_LIST_MAX_T
			//this should be done based on LRU! but how?
		}
	}
	return 0;
}

//void* ClientCache::flushingFunc(void *id){
void* ClientCache::flushingFunc(){
//	size_t ID;
//	ID = size_t(id);
	cout << "flusher thread created successfully for client ID:" << endl;//ID << endl;
	std::tr1::unordered_map<LBA,blockT>::iterator it;
	while (true) {
		usleep(30000000);
		for (it = usedSpace.begin(); it != usedSpace.end(); ++it)
			if (it->second.status == 'D') {
//				writeToFileServer(it->second); //FIXME need to add fileserveraddress and fileserverport as the arguments
				it->second.status = 'C';
			}
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

bool ClientCache::lookupBlockInCache(string file_name, size_t block_offset) {
	tr1::hash<string> str_hash;
	size_t file_ID = str_hash(file_name);
	file_ID = file_ID << 22;
	size_t temp_offset = (block_offset & int(pow(2.0,22.0)-1));
	LBA block_ID = file_ID | temp_offset;

	if(usedSpace.find(block_ID) != usedSpace.end()) //will it search the whole bucket?
		return true;
	else
		return false;
}

blockT* ClientCache::getBlockFromCache(string file_name, size_t block_offset) {

	tr1::hash<string> str_hash;
	size_t file_ID = str_hash(file_name);
	file_ID = file_ID << 22;
	size_t temp_offset = (block_offset & int(pow(2.0,22.0)-1));
	LBA block_ID = file_ID | temp_offset;

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

blockT * ClientCache::readFromFileServer(string file_name, size_t block_offset, std::string IP, int port_number) {
	// create logical block ID + server 
	tr1::hash<string> str_hash;
	size_t file_ID = str_hash(file_name);
	file_ID = file_ID << 22;
	size_t temp_offset = (block_offset & int(pow(2.0,22.0)-1));
	LBA block_ID = file_ID | temp_offset;

	bool hit = lookupBlockInCache(file_name, block_ID); 
	
	string response;

	blockT * bt = new blockT(); 

	if (hit == true){
		cout << "the block hit in the cache " << endl; 
		bt = getBlockFromCache(file_name, block_ID); // FIXME get multiple blocks 
		response =  bt->data;
	}
	else {
		cout << "the block miss in the cache" << endl; 
		// FIXME read addresses and ports from tables   
		string servAddress = IP;  
		unsigned short servPort = port_number; 
		
		// read file_name offset nbyte 
		string command = string("read ") + file_name + string(" ") + static_cast<ostringstream*>( &(ostringstream() << block_offset ))->str(); 
		command += " 1"; 
		//command +=  static_cast<ostringstream*>( &(ostringstream() << 1 ))->str();  
 
		string response;
		try{
			TCPSocket sock(servAddress, servPort); 
			sock.send(command.c_str(), command.length()); 
	
			char echoBuffer[RCVBUFSIZE+1];
			int recvMsgSize = 0; 
		
			// should receive a lot of data from metadata manager 
			while ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) > 0 ){
				echoBuffer[recvMsgSize]='\0'; 
				response += echoBuffer; 
			}

			if (response.size() != 0)
				strcpy(bt->data,response.substr(0,strlen(bt->data)).c_str()); 
			bt->blockAdr = block_ID;
			bt->status = 'C'; 
			bt->file_name = file_name; 
			bt->block_offset = block_offset;
			return bt;
		}
		catch(SocketException &e){
			cerr << e.what() << endl; 
			exit(1);
		}
	}
	return &blockT();
}

//int ClientCache::writeToFileServer(char* file_name, LBA block_ID,std::string IP, size_t port_number) {
int ClientCache::writeToFileServer(blockT b, std::string IP, size_t port_number) {
        
		cout << "salam " << endl; 
		string servAddress = IP;
        unsigned short servPort = port_number;

        // read file_name offset nbyte
	// int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); //FIXME by that magic formula
	int block_offset = 0; //FIXME should be removed. it is only for test
        string command(string("write ") + b.file_name + string(" ") + static_cast<ostringstream*>( &(ostringstream() << block_offset ))->str() + string(" 1 "));
        command += string(b.data);
        command += "\0";
		cout << "command:(" << command << ")" << endl;
        string response;
        try {
                TCPSocket sock(servAddress, servPort);
                sock.send(command.c_str(), command.length());

                // FIXME
                //char echoBuffer[RCVBUFSIZE+1];
                //int recvMsgSize = 0;
                // should receive ack
                //while ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) > 0 ){
                //      echoBuffer[recvMsgSize]='\0';
                //      response += echoBuffer;
                //      cout << "res: " << echoBuffer << endl;
                //}
        }
	catch(SocketException &e) {
                cerr << e.what() << endl;
                exit(1);
        }
        cout <<"response received: " <<  response << endl;  // should be ack
	return 0;
}

ClientCache::~ClientCache(){
	pthread_kill(flusher, SIGINT); 
	pthread_kill(harvester, SIGINT); 
}

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

