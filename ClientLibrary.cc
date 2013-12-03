#include "StringFunctions.hh"
#include "PracticalSocket.hh"  // For Socket and SocketException
#include "ClientCache.hh"
#include "FileDesc.hh"
#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()
#include <string.h>

#include <tr1/functional>
#include <cmath>
#include <string>
 
using namespace std;

#define  metadataAddress "127.0.0.1" 
#define  metadataPort 1234
#define  fileserverAddress "130.203.40.19"
#define  fileserverPort 1234

#define ONEKB 1024


const int RCVBUFSIZE = 32;    // Size of receive buffer


ClientCache disk_cache;

int pfs_create(const char * file_name, int stripe_width){
	string servAddress = metadataAddress; 
	unsigned short servPort = metadataPort; 

	string command ("create "); 
	command += file_name;  
	command += " "; 
	command += static_cast<ostringstream*>( &(ostringstream() << stripe_width ))->str(); 
	command += "\0"; 	

	// cout << command << endl; 
	int commandLen = command.length(); 

	string response; 	
	try{
		TCPSocket sock(servAddress, servPort);
 		sock.send(command.c_str(), commandLen); 
	
		char echoBuffer[RCVBUFSIZE+1]; 
		int recvMsgSize = 0;
		
		if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
			cerr << "unable to read "; 
			exit(1); 
		}
 
		echoBuffer[recvMsgSize]='\0'; 
		response = echoBuffer; 
	}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		exit(1);
  	}

	if (toLower(response) == "success") 
		return 1; 
	return 0; 
}

int pfs_open(const char * file_name, const char mode){
	string servAddress = metadataAddress; 
	unsigned short servPort = metadataPort; 
	
	string command ("open "); 
	command += file_name;  
	command += " "; 
	command += mode; 
	command += "\0"; 	

	// cout << command << endl; 
	int commandLen = command.length(); 

	string response; 	
	try{
		TCPSocket sock(servAddress, servPort);
 		sock.send(command.c_str(), commandLen); 
	
		char echoBuffer[RCVBUFSIZE+1]; 
		int recvMsgSize = 0;
		

		// should receive a lot of data from metadata manager 
		if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
			cerr << "unable to read "; 
			exit(1); 
		}
 
		echoBuffer[recvMsgSize]='\0'; 
		response = echoBuffer; 
	}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		exit(1);
  	}
	
	string file_rec_str = response; 

	int st_width   = atoi(nextToken(file_rec_str).c_str()); 	
	string st_mask = nextToken(file_rec_str); 

	bitset<NUM_FILE_SERVERS> mask(st_mask); 
	
	fileRecipe * fr = new fileRecipe(st_width, mask); 

	string fname(file_name);
	string fmode(1, mode);   
	return ofdt_open_file(fr, fname, fmode); // return file descriptor  

}
ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int * cache_hit){ 

	//fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes); 
	string file_mode = ofdt_fetch_mode   (filedes); 
	// FIXME check file_mode if it can read from file 

	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); 
	int end_block_offset = (offset + nbyte - 1) / (PFS_BLOCK_SIZE * ONEKB); 
	int n_blocks = end_block_offset - block_offset + 1 ;  
	
	// create logical block ID + server 
	tr1::hash<string> str_hash;

	size_t file_ID = str_hash(file_name);
	file_ID = file_ID << 22;
	size_t temp_offset = (block_offset & int(pow(2,22)-1));
	LBA block_ID = file_ID | temp_offset;
	
	bool hit = disk_cache.lookupBlockInCache(block_ID); 
	
	string response; 

	if (hit == true){
		cout << "hit " << endl; 
		blockT * bt = disk_cache.getBlockFromCache(block_ID);
		strcpy(buf, bt->data); // FIXME  
	}
	else {
		cout << "miss" << endl; 
		// FIXME read addresses and ports from tables   
		string servAddress = fileserverAddress;  
		unsigned short servPort = fileserverPort; 
		
		// read file_name offset nbyte 
		string command = string("read ") + file_name + string(" ") + static_cast<ostringstream*>( &(ostringstream() << block_offset ))->str(); 
		command += " "; 
		command +=  static_cast<ostringstream*>( &(ostringstream() << n_blocks ))->str();  
 
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
		
		}catch(SocketException &e){
			cerr << e.what() << endl; 
			exit(1); 
		}

		// send to mahshid for search 
		// get result and put in the buf 
	
		blockT bt;
		strcpy(bt.data, response.c_str());  
		bt.blockAdr = block_ID; 
		bt.status = 'C'; 
	
		disk_cache.writeSingleBlockToCache(bt); 

		strcpy(buf, bt.data); 
	} 
	return 0; 
}
size_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){
	//fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes); 
	string file_mode = ofdt_fetch_mode   (filedes); 
	// FIXME check file_mode if it can read from file 

	// create logical block ID + server 

	// FIXME read addresses and ports from tables   
	string servAddress = fileserverAddress; 
	unsigned short servPort = fileserverPort;  
		
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB);  
	// read file_name offset nbyte 
	string command = string("write ") + file_name + string(" ") + static_cast<ostringstream*>( &(ostringstream() << block_offset ))->str() + string(" 1 ");
	command += string((char*)buf); 
	command += "\0"; 
 
	string response; 
	try{
		TCPSocket sock(servAddress, servPort); 
		sock.send(command.c_str(), command.length()); 
		

		// FIXME	
		//char echoBuffer[RCVBUFSIZE+1];
		//int recvMsgSize = 0; 
		// should receive ack  
		//while ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) > 0 ){	
		//	echoBuffer[recvMsgSize]='\0'; 
		//	response += echoBuffer;
		//	cout << "res: " << echoBuffer << endl;  
		//}
		
	}catch(SocketException &e){
		cerr << e.what() << endl; 
		exit(1); 
	}

	cout <<"response received: " <<  response << endl;  // should be ack 

	return 0; 
} 
int pfs_close(int filedes){
	//fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes);

	string servAddress = metadataAddress;  
	unsigned short servPort = metadataPort; 
	
	string command ("close "); 
	command += file_name;  
	command += "\0"; 	

	// cout << command << endl; 
	int commandLen = command.length(); 

	string response; 	
	try{
		TCPSocket sock(servAddress, servPort);
 		sock.send(command.c_str(), commandLen); 
	
		char echoBuffer[RCVBUFSIZE+1]; 
		int recvMsgSize = 0;
		
		// should receive a lot of data from metadata manager 
		if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
			cerr << "unable to recv "; 
			exit(1); 
		}
 
		echoBuffer[recvMsgSize]='\0'; 
		response = echoBuffer; 
	}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		exit(1);
  	}

	return ofdt_close_file(filedes); 	
} 
int pfs_delete(const char * file_name){ 
	string servAddress = metadataAddress; 
	unsigned short servPort = metadataPort; 
	
	string command ("delete "); 
	command += file_name;  
	command += "\0"; 

	// cout << command << endl; 
	int commandLen = command.length(); 

	string response; 	
	try{
		TCPSocket sock(servAddress, servPort);
 		sock.send(command.c_str(), commandLen); 
	
		char echoBuffer[RCVBUFSIZE+1]; 
		int recvMsgSize = 0;
		
		if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
			cerr << "unable to recv"; 
			exit(1); 
		}
 
		echoBuffer[recvMsgSize]='\0'; 
		response = echoBuffer; 
	}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		exit(1);
  	}

	if (toLower(response) == "success") 
		return 1; 
	return 0; 
}
int pfs_fstat(int filedes, struct pfs_stat * buf){
	string name = ofdt_fetch_name(filedes);
	// send name to metadata manager 
	// receive fstat 
	// put in buffer 
	return 0; // what? 

}

int main(int argc, char *argv[]) {
		//	if (pfs_create("khoshgel", 4) > 0)  cout << "successful creation of khoshgel! " << endl; 
		//  	cout << "open file: " << pfs_open("khoshgel", 'r') << endl; 
		//	if (pfs_create("nanaz", 3) > 0) cout << "successful creation of nanaz! " << endl; 
		//	cout << "open file: " << pfs_open("nanaz", 'r') << endl; 

		//	ofdt_print_all(); 


		int ifdes; 
		ifdes = pfs_open("golabi.txt", 'r');  
		cout << "open file: " << ifdes << endl;  

		char * buf =  (char *)malloc(1*ONEKB);

		//	strcpy(buf, "soft kitty, warm kitty little ball of fur happy kitty sleepy kitty purr purr purr"); 	
		//	pfs_write(ifdes, (void *)buf, 1*ONEKB, 0, 0); 


		ssize_t nread = pfs_read(ifdes, (void *)buf, 1*ONEKB , 0, 0);

		cout << buf << endl; 
			 	nread = pfs_read(ifdes, (void *)buf, 1*ONEKB , 0, 0);
		cout << buf << endl; 
	
	return 0;
}

