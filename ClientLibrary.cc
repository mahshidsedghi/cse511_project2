#include "StringFunctions.hh"
#include "PracticalSocket.hh"  // For Socket and SocketException
#include "FileDesc.hh"
#include <iostream>           // For cerr and cout
#include <cstdlib>            // For atoi()

using namespace std;

#define ONEKB 1024

int next_file_desc = 0;

struct fileDesc {
	int des; 
	int file_recipe; // we need to define this 
};  

const int RCVBUFSIZE = 32;    // Size of receive buffer

int pfs_create(const char * file_name, int stripe_width){
	string servAddress = "127.0.0.1"; 
	unsigned short servPort = 1234; 

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
	string servAddress = "127.0.0.1"; 
	unsigned short servPort = 1234; 
	
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
	// FIXME check file_mode if it can read from file 

	// create logical block ID + server 

	// FIXME read addresses and ports from tables   
	string servAddress = "75.102.87.54"; 
	unsigned short servPort = 1234; 
		
	int block_offset = offset / (PFS_BLOCK_SIZE * ONEKB);  
	// read file_name offset nbyte 
	string command = string("read ") + file_name + string(" ") + static_cast<ostringstream*>( &(ostringstream() << block_offset ))->str() + string("  10"); 
	string response; 
	try{
		TCPSocket sock(servAddress, servPort); 
		sock.send(command.c_str(), command.length()); 
		
		char echoBuffer[RCVBUFSIZE+1];
		int recvMsgSize = 0; 
	
		// should receive a lot of data from metadata manager 
		while ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
			echoBuffer[recvMsgSize]='\0'; 
			response += echoBuffer; 
		}
		
	}catch(SocketException &e){
		cerr << e.what() << endl; 
		exit(1); 
	}

	cout << response; 
	// send to mahshid for search 
	// get result and put in the buf 

	return 0; 
} 
int pfs_close(int filedes){
	fileRecipe *fr   = ofdt_fetch_recipe (filedes); 
	string file_name = ofdt_fetch_name   (filedes);

	string servAddress = "127.0.0.1"; 
	unsigned short servPort = 1234; 
	
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
	string servAddress = "127.0.0.1"; 
	unsigned short servPort = 1234; 
	
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
  	cout << "open file: " << ifdes; 
	
	char * buf =  (char *)malloc(5*ONEKB);
  	size_t nread = pfs_read(ifdes, (void *)buf, 4*ONEKB,0, 0);
	cout << nread; 
	return 0;
}

