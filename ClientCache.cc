#include "ClientCache.hh"
#include <stdexcept>
#include <signal.h>

using namespace std;


size_t ClientCache::clientID = 0;

size_t ClientCache::LAST_ACCESS = 0;

ClientCache::ClientCache(){
	clientID++; 
	int rc;
	stringstream errMsg;
	blockT b1;
	b1.status = 'F';
	for(int i = 0; i < BUFFER_CACHE_CAPACITY; i++)
		freeSpace.push_front(b1);
	cout << "freespace size: " << freeSpace.size() <<endl;

	rc = pthread_create(&harvester, NULL, &ClientCache::callHarvestingFunc, this);
	if(rc)
	{
		errMsg << "Could not create harvester thread in client cache ID:" << clientID;
		throw runtime_error(errMsg.str());
	}

	rc = pthread_create(&flusher, NULL, &ClientCache::callFlushingFunc, this);
	if(rc)
	{
		errMsg << "Could not create flusher thread in client cache ID:" << clientID;
		throw runtime_error(errMsg.str());
	}

}

void* ClientCache::harvestingFunc(){
//	cout << "harvester thread created successfully for client ID:" << endl;
	priority_queue<blockT,std::vector<blockT>,mycomparison> harvest_queue;
	std::tr1::unordered_map<LBA,blockT>::iterator it;
	int k;
	while (true) {
		if (freeSpace.size() < FREE_LIST_MIN_T) {
			cout << freeSpace.size() << "\t" << FREE_LIST_MIN_T <<endl << endl;
			cout << "harvesting" << endl;
			k = FREE_LIST_MAX_T - freeSpace.size(); //max elements needed to harvest
			for ( it = usedSpace.begin(); it != usedSpace.end(); ++it ) {
				if (it->second.status == 'C') {
					if (harvest_queue.size() >= k )
					{
						if (it->second.access_time < harvest_queue.top().access_time) {
							harvest_queue.pop();
							harvest_queue.push(it->second);
						}	
					}
					else
						harvest_queue.push(it->second);
				}
			}
		}
		int j = harvest_queue.size();
		cout << "j in harvesting: "<< j << endl;
		blockT temp;
		for (int i = 0;i< j; i++){
			cout <<"harvesting block with access time:"<<harvest_queue.top().access_time << endl;
			usedSpace.erase(harvest_queue.top().blockAdr); //remove the element from usedSpace
			temp.status = 'F'; //make the block free
			freeSpace.push_front(temp); //add to free list
			harvest_queue.pop();
		}
	}
	return 0;
}

void* ClientCache::flushingFunc(){
//	cout << "flusher thread created successfully for client ID:" << endl;
	std::tr1::unordered_map<LBA,blockT>::iterator it;
	while (true) {
		usleep(10000000); //FIXME: 30S
		for (it = usedSpace.begin(); it != usedSpace.end(); ++it)
			if (it->second.status == 'D') {
				writeToFileServer(it->second);
				it->second.status = 'C';
			}
	}
	return 0; 
}

void ClientCache::insertSingleBlockIntoCache(blockT b) {
//	assert(freeSpace.size() + usedSpace.size() <= BUFFER_CACHE_CAPACITY);
	if (!freeSpace.empty()) {
		b.access_time = ClientCache::LAST_ACCESS++; //for LRU purposes
		std::pair<LBA,blockT> mypair = make_pair(b.blockAdr,b);
		freeSpace.pop_front();
		usedSpace.insert(mypair);
		cout << "inserted a new block into cache with access time:" << b.access_time << "and free space size:" << freeSpace.size() << endl;
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
	if(it != usedSpace.end()) {//will it search the whole bucket?
		it->second.access_time = ClientCache::LAST_ACCESS++; //for LRU purposes
		cout << "accessed a block in cache and updated its access_time as: " << it->second.access_time << endl;
		return &(it->second);
	}
	else
		return NULL;
}

void ClientCache::putBlockIntoCache(blockT b) {
	std::pair<LBA,blockT> mypair = make_pair(b.blockAdr,b);
	b.access_time = ClientCache::LAST_ACCESS++; //for LRU purposes
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
		string command = string("read ") + file_name + string(" ") + 
				 static_cast<ostringstream*>( &(ostringstream() << block_offset ))->str(); 
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

int ClientCache::writeToFileServer(blockT b) {
        
	string servAddress;
        int servPort;
	size_t offset_within;
	corresponding_server(b.block_offset,b.file_recipe.stripeWidth,servAddress,servPort,offset_within);
        string command(string("write ") + b.file_name + string(" ") + 
	static_cast<ostringstream*>( &(ostringstream() <<offset_within ))->str() + string(" 1 "));
        
	command += string(b.data);
        command += "\0";
	cout << "command:" << command << endl;
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

