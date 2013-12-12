#include "pfs.hh"
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




// AUXILIAY FUNCTIONS 
// --------------------------------------------------------
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

string requestToken(string file_name, int start, int end, char mode ){
	
	string command = "request_token ";
	command += file_name;
	command += " ";   
	command += static_cast<ostringstream*>( &(ostringstream() << start ))->str(); 
	command += " "; 
	command += static_cast<ostringstream*>( &(ostringstream() << end ))->str(); 
	command += " ";
	command += mode; 
	command += " ";  
	command += static_cast<ostringstream*>( &(ostringstream() << disk_cache.revoker_port ))->str(); 


	string servAddress = METADATA_ADDR; 
	int    servPort    = METADATA_PORT;
	cout << "CLIENT_TOKEN (" << command << ")";  
	string response = sendToServer(command, servAddress, servPort );
	cout << "(" << response << ")" << endl; 
	return response;  
}

vector<Interval> required_token(int filedes, int block_start, int block_end){
	int start = -1; 
	int end = -1; 
	vector<Interval> int_list; 
	for (int i = block_start; i <= block_end; i++){
		if (FileDescriptor::checkPermission(filedes, i , 'r')) {
			if (start != -1){
				end = i-1; 
				int_list.push_back(Interval(start , end)); 
				start = -1; 
				end = -1; 
			}
			continue; 
		}else{
			if (start == -1)
				start = i; 
		}
	}
	if (start != -1 && end == -1) {
		end = block_end; 
		int_list.push_back(Interval(start, end)); 
	}
	for (vector<Interval>::iterator it = int_list.begin(); it != int_list.end(); ++it){
		cout << "NEED_TOKEN (" << it->m_start << "," << it->m_end << ")"; 
	}		
	cout << endl; 


	return int_list; 

}

// Library functions 
// --------------------------------------------------------------
int pfs_create(const char * file_name, int stripe_width){
	string servAddress = METADATA_ADDR;  
	unsigned short servPort = METADATA_PORT; 

	string command ("create "); 
	command += file_name;  
	command += " "; 
	command += static_cast<ostringstream*>( &(ostringstream() << stripe_width ))->str(); 
	command += "\0"; 	

	int commandLen = command.length(); 

	cout << "CLIENT_CREATE (" << command << ")"; 
	string response = sendToServer(command, servAddress, servPort);  	
	
	cout << "(" << response<< ")" << endl; 

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

	cout << "TOKEN_OPEN (" << command << ")";  
	int commandLen = command.length(); 

	string response = sendToServer(command, servAddress, servPort); 	
	cout << "(" << response << ")" << endl; 
	string file_rec_str = response; 

	int st_width   = atoi(nextToken(file_rec_str).c_str()); 	
	string st_mask = nextToken(file_rec_str); 

	bitset<NUM_FILE_SERVERS> mask(st_mask); 
	
	fileRecipe * fr = new fileRecipe(st_width, mask); 

	string fname(file_name);
	string fmode(1, mode);   

	pthread_mutex_lock(&FileDescriptor::file_desc_mutex);
	int fdes = FileDescriptor::ofdt_open_file(fr, fname, fmode); // return file descriptor  
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex);
	return fdes; 
}
ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int * cache_hit){ 

	 
	pthread_mutex_lock(&FileDescriptor::file_desc_mutex); 
	fileRecipe *fr   = FileDescriptor::ofdt_fetch_recipe(filedes); 
	string file_name = FileDescriptor::ofdt_fetch_name(filedes); 
	string file_mode = FileDescriptor::ofdt_fetch_mode(filedes); 
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 
	
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); 
	int end_block_offset = (offset + nbyte - 1) / (PFS_BLOCK_SIZE * ONEKB); 
	int n_blocks = end_block_offset - block_offset + 1 ;  

	bool already_have_token[n_blocks];
	for (int i = 0; i < n_blocks; i++){
		already_have_token[i] = true;
	}
 
	do {
		// file desc lock 
		pthread_mutex_lock(&FileDescriptor::file_desc_mutex); 

		vector<Interval> int_list = required_token(filedes, block_offset, end_block_offset); 
		
		for (vector<Interval>::iterator it = int_list.begin(); it < int_list.end(); ++it){
			for (int i = it->m_start; i <= it->m_end; i++){
				already_have_token[i - block_offset] = false; 
			}
		}

		if (int_list.size() == 0) {
			break; 
		}else {
			
			// file desc unlock  
			pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 
				
			string token = requestToken(file_name, int_list[0].m_start, int_list[0].m_end, 'r');
			
			if(toLower(token) != "locking_failed" && toLower(token) != "failed"){
				int tok_start = atoi(trim(nextToken(token)).c_str());
				int tok_end   = atoi(trim(nextToken(token)).c_str());

				// file desck lock 
				pthread_mutex_lock(&FileDescriptor::file_desc_mutex); 

				FileDescriptor::addPermission(filedes, tok_start, tok_end, 'r'); 
				FileDescriptor::printTokens(filedes); 
				
				pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 
			}

		}
	}while (true);  

	// Now, I should have file_desc_mutex locked 

	string response(""); 
	for (int i = block_offset; i <= end_block_offset; i++){
		
		bool hit = false; 
		blockT *bt;  
		if (already_have_token[i - block_offset]){
			//cache lock
			pthread_mutex_lock(&ClientCache::cache_mutex); 

			hit = disk_cache.lookupBlockInCache(file_name, i); 
			if (hit){
				bt = disk_cache.getBlockFromCache(file_name, i); 
				response += bt->data; 
			}
			//cache unlock
			pthread_mutex_unlock(&ClientCache::cache_mutex); 			
		}
		if (!already_have_token[i-block_offset] || !hit){
			string server_address; 
			int server_port; 
			size_t within_offset; 
			corresponding_server(i, fr->stripeWidth, server_address, server_port, within_offset); // call by reference  of server_address, server_port, within_offset 
			 
			bt = disk_cache.readFromFileServer(file_name, within_offset, server_address, server_port); 	
			response += bt->data; 
						
			// cache lock
			pthread_mutex_lock(&ClientCache::cache_mutex); 

			disk_cache.insertSingleBlockIntoCache(*bt); 

			// cache unlock
			pthread_mutex_unlock(&ClientCache::cache_mutex); 
		}				
		
	}
	 
	// file desc unlock 
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 

	int off_first = offset % (PFS_BLOCK_SIZE * ONEKB);
	strcpy((char *)buf, response.substr(off_first, nbyte).c_str());  
		
	return strlen((char *)buf);
}

void find_buf_offset(int i, off_t offset, int nbyte, unsigned int block_offset, unsigned int end_block_offset, int &buf_start, int &buf_end){
	if (i == end_block_offset) 
		buf_end = (offset + nbyte - 1) % (PFS_BLOCK_SIZE * ONEKB);
	else
		 buf_end = PFS_BLOCK_SIZE * ONEKB - 1; 

	if (i == block_offset)
		 buf_start = offset % (PFS_BLOCK_SIZE * ONEKB); 
	else
		 buf_start = 0; 


}
ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit){
	 
	pthread_mutex_lock(&FileDescriptor::file_desc_mutex); 
	fileRecipe *fr   = FileDescriptor::ofdt_fetch_recipe(filedes); 
	string file_name = FileDescriptor::ofdt_fetch_name(filedes); 
	string file_mode = FileDescriptor::ofdt_fetch_mode(filedes); 
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 
	
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB); 
	int end_block_offset = (offset + nbyte - 1) / (PFS_BLOCK_SIZE * ONEKB); 
	int n_blocks = end_block_offset - block_offset + 1 ;  

	bool already_have_token[n_blocks];
	for (int i = 0; i < n_blocks; i++){
		already_have_token[i] = true;
	}
 
	do {
		// file desc lock 
		pthread_mutex_lock(&FileDescriptor::file_desc_mutex); 

		vector<Interval> int_list = required_token(filedes, block_offset, end_block_offset); 
		
		for (vector<Interval>::iterator it = int_list.begin(); it < int_list.end(); ++it){
			for (int i = it->m_start; i <= it->m_end; i++){
				already_have_token[i - block_offset] = false; 
			}
		}

		if (int_list.size() == 0) {
			break; 
		}else {
			
			// file desc unlock  
			pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 
				
			string token = requestToken(file_name, int_list[0].m_start, int_list[0].m_end, 'w');
			
			if(toLower(token) != "locking_failed" && toLower(token) != "failed"){
				int tok_start = atoi(trim(nextToken(token)).c_str());
				int tok_end   = atoi(trim(nextToken(token)).c_str());

				// file desck lock 
				pthread_mutex_lock(&FileDescriptor::file_desc_mutex); 

				FileDescriptor::addPermission(filedes, tok_start, tok_end, 'w'); 
				FileDescriptor::printTokens(filedes); 
				
				pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 
			}

		}
	}while (true);  

	// Now, I should have file_desc_mutex locked 

	int buf_start = -1; 
	int buf_end = -1; 
	
	string response((char *)buf);
	for (int i = block_offset; i <= end_block_offset; i++){
		
		find_buf_offset(i, offset, nbyte, block_offset, end_block_offset, buf_start, buf_end);

		bool hit = false; 
		blockT *bt;  
		if (already_have_token[i - block_offset]){
			//cache lock
			pthread_mutex_lock(&ClientCache::cache_mutex); 

			hit = disk_cache.lookupBlockInCache(file_name, i); 
			if (hit){
				bt = disk_cache.getBlockFromCache(file_name, i); 
		
				for (int j = buf_start; j <= buf_end; j++) {
					bt->data[j] = ((char*)buf)[i - block_offset + j];
				}

				bt->status = 'D';
			}
			//cache unlock
			pthread_mutex_unlock(&ClientCache::cache_mutex); 			
		}
		if (!already_have_token[i-block_offset] || !hit){
			string server_address; 
			int server_port; 
			size_t within_offset; 
			corresponding_server(i, fr->stripeWidth, server_address, server_port, within_offset); // call by reference  of server_address, server_port, within_offset 
			 
			bt = disk_cache.readFromFileServer(file_name, within_offset, server_address, server_port); 	
						
			for (int j = buf_start; j <= buf_end; j++) {
				bt->data[j] = ((char*)buf)[i - block_offset + j];
			}

			bt->status = 'D';
			// cache lock
			pthread_mutex_lock(&ClientCache::cache_mutex); 

			disk_cache.insertSingleBlockIntoCache(*bt); 

			// cache unlock
			pthread_mutex_unlock(&ClientCache::cache_mutex); 
		}				
		
	}
	 
	// file desc unlock 
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex); 

	return strlen((char *)buf);

}

int pfs_close(int filedes) {
	// FIXME: call flusher for all block which has write token || blocks are in cache and dirty 
	pthread_mutex_lock(&FileDescriptor::file_desc_mutex);
	return FileDescriptor::ofdt_close_file(filedes); 	
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex);
} 
int pfs_delete(const char * file_name) { 
	string servAddress = METADATA_ADDR; 
	unsigned short servPort = METADATA_PORT; 
	
	string command ("delete "); 
	command += file_name;  
	command += "\0"; 

	int commandLen = command.length(); 

	string response = sendToServer(command, servAddress, servPort); 

	if (toLower(response) == "success") 
		return 1; 
	return 0; 
}
int pfs_fstat(int filedes, struct pfs_stat * buf){
	string servAddress = METADATA_ADDR; 
	unsigned short servPort = METADATA_PORT; 
	
	pthread_mutex_lock(&FileDescriptor::file_desc_mutex);
	string file_name = FileDescriptor::ofdt_fetch_name (filedes); 
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex);

	string command ("fstat "); 
	command += file_name;  
	command += "\0"; 

	int commandLen = command.length(); 

	string response = sendToServer(command, servAddress, servPort); 

	if (toLower(response) == "nack") return 0; // failed 

	pthread_mutex_lock(&FileDescriptor::file_desc_mutex);
	size_t fsize = atoi((trim(nextToken(response)).c_str())); 	
	time_t mtime = atoi((trim(nextToken(response)).c_str())); 
	time_t ctime = atoi((trim(nextToken(response)).c_str())); 
	pthread_mutex_unlock(&FileDescriptor::file_desc_mutex);
	
	buf->pst_ctime = ctime; 
	buf->pst_mtime = mtime; 
	buf->pst_size = fsize; 
	return 1; // successful
}
