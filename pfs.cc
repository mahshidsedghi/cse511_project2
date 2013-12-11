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


#define ONEKB 1024


#define RCVBUFSIZE 32    // Size of receive buffer


ClientCache disk_cache;

string sendToServer(string input_str, string IP, int port){
	string response; 	
	try{
		TCPSocket sock(IP, port);
 		sock.send(input_str.c_str(), input_str.length()); 
	
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
	return response; 
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
			cerr << "unable to create "; 
			exit(1); 
		}
 
		echoBuffer[recvMsgSize]='\0'; 
		response = echoBuffer; 
	}catch(SocketException &e) {
    		cerr << "create: " << e.what() << endl;
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
			cerr << "unable to open "; 
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
			size_t within_offset; 
			corresponding_server(i, fr->stripeWidth, server_address, server_port, within_offset); // call by reference  of server_address, server_port, within_offset 
			 
			bt = disk_cache.readFromFileServer(file_name, within_offset, server_address, server_port); 
			
			disk_cache.insertSingleBlockIntoCache(*bt); 
		}
		response += bt->data; 
	}
	 
	int off_first = offset % (PFS_BLOCK_SIZE * ONEKB);

	strcpy((char *)buf, response.substr(off_first, nbyte).c_str());  
		
	return strlen((char *)buf); // FIXME: if nbytes read is less than available bytes  
}
size_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){
	
	fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes); 
	string file_mode = ofdt_fetch_mode   (filedes); 
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); 
	int end_block_offset = (offset + nbyte - 1) / (PFS_BLOCK_SIZE * ONEKB); 
	int n_blocks = end_block_offset - block_offset + 1 ;  


	int off_start = 0;
	int off_end = 0; 
	
 
	for (int i = block_offset; i <= end_block_offset; i++) {
		if (i == end_block_offset) 
			off_end = (offset + nbyte - 1) % (PFS_BLOCK_SIZE * ONEKB);
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
			string server_address; 
			int server_port; 
			size_t within_offset;
			 
			corresponding_server(i, fr->stripeWidth, server_address, server_port, within_offset); // call by reference  of server_address, server_port, within_offset 
			bt = disk_cache.readFromFileServer(file_name, within_offset, server_address, server_port); 
			
		}
		if (bt != NULL){
			bt->status = 'D';
			for (int j = off_start; j <= off_end; j++) {
				bt->data[j] = ((char*)buf)[i - block_offset + j];
			}
			if (hit == false){
				disk_cache.insertSingleBlockIntoCache(*bt); 
			}
			else 				///////////////
				disk_cache.putBlockIntoCache(*bt); //CONFIRM: put the modified block back into the cache. OK?
		}
		else 
			return 0;
				
	}	
	return strlen((char *)buf); 
}

int pfs_close(int filedes) {
// FIXME do we need to send something to metadata manager for close? I don't think so. 
/*
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
*/

	// FIXME: call flusher for all block which has write token || blocks are in cache and dirty 
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

	string response = sendToServer(command, servAddress, servPort); 

	if (toLower(response) == "success") 
		return 1; 
	return 0; 
}
int pfs_fstat(int filedes, struct pfs_stat * buf){
	string servAddress = METADATA_ADDR; 
	unsigned short servPort = METADATA_PORT; 
	
	string file_name = ofdt_fetch_name (filedes); 

	string command ("fstat "); 
	command += file_name;  
	command += "\0"; 

	// cout << command << endl; 
	int commandLen = command.length(); 

	string response = sendToServer(command, servAddress, servPort); 

	if (toLower(response) == "nack") return 0; // failed 

	size_t fsize = atoi((trim(nextToken(response)).c_str())); 	
	time_t mtime = atoi((trim(nextToken(response)).c_str())); 
	time_t ctime = atoi((trim(nextToken(response)).c_str())); 
	
	buf->pst_ctime = ctime; 
	buf->pst_mtime = mtime; 
	buf->pst_size = fsize; 
	return 1; // successful
}