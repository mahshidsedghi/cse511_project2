#include "StringFunctions.hh"
#include "PracticalSocket.hh"  // For Socket and SocketException
#include "ClientCache.hh"
#include "FileDesc.hh"
#include "net_addresses.hh"
#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()
#include <string.h>

#include <tr1/functional>
#include <cmath>
#include <string>
 
using namespace std;


#define ONEKB 1024


#define RCVBUFSIZE 32    // Size of receive buffer


ClientCache disk_cache;



void corresponding_server(size_t block_offset, string &server_address, int &server_port){
	
}

// Library functions 

int pfs_create(const char * file_name, int stripe_width){
//	string servAddress = metadataAddress; 
//	unsigned short servPort = metadataPort;

	string command ("create "); 
	command += file_name;  
	command += " "; 
	command += static_cast<ostringstream*>( &(ostringstream() << stripe_width ))->str(); 
	command += "\0"; 	

	// cout << command << endl; 
	int commandLen = command.length(); 

	string response; 	
	try{
		string servAddress = METADATA_ADDR; 
		unsigned short servPort = METADATA_PORT;
		TCPSocket sock(servAddress, servPort);
 		sock.send(command.c_str(), commandLen); 
	
		char echoBuffer[RCVBUFSIZE+1]; 
		int recvMsgSize = 0;
		
		if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){ //do we expect to receive an ack?
			cerr << "unable to read "; 
			exit(1); 
		}
 
		echoBuffer[recvMsgSize]='\0'; 
		response = echoBuffer; 
	}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		exit(1);
  	}

	cout << response << endl; 

	if (toLower(response) == "success") 
		return 1; 
	return 0; 
}

int pfs_open(const char * file_name, const char mode){
	string servAddress = METADATA_ADDR;  
	unsigned short servPort = METADATA_PORT; 
	
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

	fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes); 
	string file_mode = ofdt_fetch_mode   (filedes); 
	
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); 
	int end_block_offset = (offset + nbyte - 1) / (PFS_BLOCK_SIZE * ONEKB); 
	
	string response(""); 

	for (int i = block_offset; i <= end_block_offset; i++){
		bool hit = disk_cache.lookupBlockInCache(file_name, i);
		blockT * bt;  
		if (hit == true) {
			bt = disk_cache.getBlockFromCache(file_name, i);
		}
		else {
			string server_address; 
			int server_port; 
			corresponding_server(i, server_address, server_port); // call by reference 
			 
			*bt = disk_cache.readFromFileServer(file_name, i, server_address, server_port); 
			
			disk_cache.insertSingleBlockIntoCache(*bt); 
		}
		response += bt->data; 
	}
	 
	int off_first = offset % (PFS_BLOCK_SIZE * ONEKB);

	strcpy((char *)buf, response.substr(off_first, nbyte).c_str());  
		
	return nbyte; // FIXME: if nbytes read is less than available bytes  
}
size_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){
	
	//fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes); 
	string file_mode = ofdt_fetch_mode   (filedes); 
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); 
	int end_block_offset = (offset + nbyte - 1) / (PFS_BLOCK_SIZE * ONEKB); 
	int n_blocks = end_block_offset - block_offset + 1 ;  


	int off_start;
	int off_end; 
	 
	for (int i = block_offset; i <= end_block_offset; i++) {
		if (i == end_block_offset) 
			off_end = (offset + nbyte) % (PFS_BLOCK_SIZE * ONEKB);
 		else
			 off_end = PFS_BLOCK_SIZE * ONEKB - 1; 

		if (i == block_offset)
			 off_start = offset % (PFS_BLOCK_SIZE * ONEKB); 
		else
			 off_start = 0; 
	
		bool hit = disk_cache.lookupBlockInCache(file_name, i);
		blockT * bt;  
		if (hit == true) {
			bt = disk_cache.getBlockFromCache(file_name, i);
		}
		else {
			*bt = disk_cache.readFromFileServer(file_name, i, string("130.203.40.19"), 1234); 
			
			disk_cache.insertSingleBlockIntoCache(*bt); 
		}

		bt->status = 'D'; 
		for (int j = off_start; j <= off_end; j++) {
			bt->data[j] = ((char*)buf)[i - block_offset + j];
		}
		 
		
	}	
	return nbyte; 
}

int pfs_close(int filedes) {
	//fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes);

	string servAddress = METADATA_ADDR;  
	unsigned short servPort = METADATA_PORT; 
	
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
int pfs_delete(const char * file_name) { 
	string servAddress = METADATA_ADDR; 
	unsigned short servPort = METADATA_PORT; 
	
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
	//if (pfs_create("khoshgel", 4) > 0)  cout << "successful creation of khoshgel! " << endl; 
	// cout << "open file: " << pfs_open("khoshgel", 'r') << endl; 
	//if (pfs_create("nanaz", 3) > 0) cout << "successful creation of nanaz! " << endl; 
	//cout << "open file: " << pfs_open("nanaz", 'r') << endl; 
	//ofdt_print_all(); 
	int ifdes; 
	//ifdes = pfs_open("golabi.txt", 'r');  
	//cout << "open file: " << ifdes << endl;  
	char * buf =  (char *)malloc(1*ONEKB);
	//strcpy(buf, "soft kitty, warm kitty little ball of fur happy kitty sleepy kitty purr purr purr"); 	
	//pfs_write(ifdes, (void *)buf, 1*ONEKB, 0, 0); 
	//ssize_t nread = pfs_read(ifdes, (void *)buf, 1*ONEKB , 0, 0);
	//cout << buf << endl; 
 	//nread = pfs_read(ifdes, (void *)buf, 1*ONEKB , 0, 0);
	//cout << buf << endl; 
 	int success = pfs_create("googol.txt",1);
	cout <<"successful creation: " << success << endl; 

	blockT b1;
	b1.file_name = "baghali.txt";
	strcpy(b1.data , "this line was written by client on the server using writeToFileServerFunction");
//	size_t nwrite = disk_cache.writeToFileServer(b1,(string)fileserverAddress,(size_t)fileserverPort); //gives seg fault
//	cout <<"nwrite:" << nwrite << endl; 

	return 0;
}

