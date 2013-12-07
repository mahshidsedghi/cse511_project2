/*++ sockets on Unix and Windows
 *   Copyright (C) 2002
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "StringFunctions.hh"
#include "PracticalSocket.hh"  // For Socket, ServerSocket, and SocketException
#include "MetadataManager.hh"
#include "net_addresses.hh" // FIXME 
#include "mt_data_types.hh"

#include <iostream>           // For cout, cerr
#include <cstdlib>            // For atoi()  
#include <pthread.h>          // For POSIX threads  

const int RCVBUFSIZE = 64;

void HandleTCPClient(TCPSocket *sock);     // TCP client handling function
void *ThreadMain(void *arg);               // Main program of a thread  

int main(int argc, char *argv[]) {
		  
	FileServerList[0].first  = SERVER0_ADDR;
	FileServerList[0].second = SERVER0_PORT;
	FileServerList[1].first  = SERVER1_ADDR;
	FileServerList[1].second = SERVER1_PORT;
	FileServerList[2].first  = SERVER2_ADDR;
	FileServerList[2].second = SERVER2_PORT;
	FileServerList[3].first  = SERVER3_ADDR;
	FileServerList[3].second = SERVER3_PORT;
	FileServerList[4].first  = SERVER4_ADDR;
	FileServerList[4].second = SERVER4_PORT;
	 
	unsigned short servPort =  METADATA_PORT;  
 
	cout << "metadata is waiting on port " << servPort << endl;  
    try {
     TCPServerSocket servSock(servPort);   // Socket descriptor for server  
  
    for (;;) {      // Run forever  
      // Create separate memory for client argument  
      TCPSocket *clntSock = servSock.accept();
  
      // Create client thread  
      pthread_t threadID;              // Thread ID from pthread_create()  
      if (pthread_create(&threadID, NULL, ThreadMain, 
              (void *) clntSock) != 0) {
        cerr << "Unable to create thread" << endl;
        exit(1);
      }
     }
  } catch (SocketException &e) {
    cerr << e.what() << endl;
    exit(1);
  }
  // NOT REACHED

  return 0;
}

// TCP client handling function
void HandleTCPClient(TCPSocket *sock) {
  cout << "Handling client ";
  try {
    cout << sock->getForeignAddress() << ":";
  } catch (SocketException &e) {
    cerr << "Unable to get foreign address" << endl;
  }
  try {
    cout << sock->getForeignPort();
  } catch (SocketException &e) {
    cerr << "Unable to get foreign port" << endl;
  }
  cout << " with thread " << pthread_self() << endl;

  // Send received string and receive again until the end of transmission
  char echoBuffer[RCVBUFSIZE]; 
 
   
  int recvMsgSize;
  string recvCommand(""); 
  if ((recvMsgSize = sock->recv(echoBuffer, RCVBUFSIZE)) > 0) { // Zero means end of transmission
	echoBuffer[recvMsgSize] = '\0'; 
	recvCommand += echoBuffer; 

  	string command = nextToken(recvCommand); 
  	command = toLower(command); 
  	string response; 

	cout << "command: " << command << endl; 

  	if      ( command == "create" )
		response = execFunc_create (recvCommand); 
  	else if ( command == "open"   )
		response = execFunc_open   (recvCommand); 
  	else if ( command == "close"  )
		execFunc_close  (recvCommand); 
  	else if ( command == "read"   )
		execFunc_read   (recvCommand); 
  	else if ( command == "write"  ) 
		execFunc_write  (recvCommand); 
  	else if ( command == "delete" )
		response = execFunc_delete (recvCommand); 
  	else if ( command == "fstat"  )
		execFunc_fstat  (recvCommand); 

  	sock->send(response.c_str(), response.length());
  	
  }
  // Destructor closes socket
}

void *ThreadMain(void *clntSock) {
  // Guarantees that thread resources are deallocated upon return  
  pthread_detach(pthread_self()); 

  // Extract socket file descriptor from argument  
  HandleTCPClient((TCPSocket *) clntSock);

  delete (TCPSocket *) clntSock;
  return NULL;
}

string execFunc_create(string arguments){

	string filename     = trim(nextToken(arguments)); 
	string stripe_w_str = nextToken(arguments); 

	fileRecipe file_recipe; 
	file_recipe.stripeWidth = atoi(stripe_w_str.c_str()); 


	string response; 
	for (int i = 0; i < atoi(stripe_w_str.c_str()); i++){
		file_recipe.stripeMask.set(i); //FIXME decide in which servers we should stripe the file  
		
		// create a file in each server 
		string command("create "+ filename);
		cout << i<<"(" <<filename<<")" << endl;  
		try{
			TCPSocket sock(FileServerList[i].first, FileServerList[i].second);
 			sock.send(command.c_str(), command.length()); 
	
			char echoBuffer[RCVBUFSIZE+1]; 
			int recvMsgSize = 0;
		
			// should receive a lot of data from metadata manager 
			if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
				cerr << "unable to recv "; 
				response="nack"; 
			}else {
				echoBuffer[recvMsgSize]='\0'; 
				response = echoBuffer; 
			}
		}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		response="nack"; 
  		}
		

	}

	if (toLower(response) == "ack"){
		// put file_recipe in a table 
		fileEntry fentry(filename, file_recipe);
		general_file_table.insert(pair<string, fileEntry>(filename, fentry)); 
	}
	if (toLower(response) == "nack")
		return string("failed");

	return string("success"); 
}
string execFunc_open(string arguments){
	string filename = trim(nextToken(arguments)); 
	string mode     = trim(nextToken(arguments)); 

	fileRecipe f_recipe; 
	map<string, fileEntry>::iterator it; 
	it = general_file_table.find(filename); 
	if ( it != general_file_table.end()) {
		f_recipe = it->second.file_recipe; 
		return f_recipe.toString(); 
	}
	return ""; 
}

void execFunc_close(string arguments){
// Do we really need this? 
/*
	string filename = nextToken(arguments); 
	map<string, fileEntry>::iterator it; 
	it = general_file_table.find(filename); 
	if (it != general_file_table.end()){
				
	}

	
	//cout << filename << endl; 
*/
}
void execFunc_read(string arguments){
	// I'm not sure if we need this or not 
}
void execFunc_write(string arguments){
	// I'm not sure if we need this or not 
}
string execFunc_delete(string arguments){
	string filename     = nextToken(arguments); 
	map<string, fileEntry>::iterator it; 
	it = general_file_table.find(filename); 
	if ( it == general_file_table.end()) return "nack"; 

	bitset<NUM_FILE_SERVERS> stripmask = it->second.file_recipe.stripeMask;  
	
	string response; 
	for (int i = 0; i < NUM_FILE_SERVERS; i++){
		
		if (!stripmask.test(i)) continue; 

		string command("delete "+ filename); 
		try{
			TCPSocket sock(FileServerList[i].first, FileServerList[i].second);
 			sock.send(command.c_str(), command.length()); 
	
			char echoBuffer[RCVBUFSIZE+1]; 
			int recvMsgSize = 0;
		
			if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
				cerr << "unable to recv "; 
				response="nack"; 
			}else {
				echoBuffer[recvMsgSize]='\0'; 
				response = echoBuffer; 
			}
		}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		response="nack"; 
  		}
		

	}

	
	general_file_table.erase(filename); 

	if (toLower(response) == "nack")
		return string("failed");

	return string("success"); 
}
string execFunc_fstat(string arguments){
	string filename = nextToken(arguments); 
	
	map<string, fileEntry>::iterator it; 
	it = general_file_table.find(filename); 
	if ( it == general_file_table.end()) return "nack"; 

	fileEntry fe = it->second; 
	
	bitset<NUM_FILE_SERVERS> stripmask = fe.file_recipe.stripeMask;  
	

	size_t total_size = 0; 
	time_t last_mtime = 0; 
	time_t last_ctime = 0; 

	string response; 
	for (int i = 0; i < NUM_FILE_SERVERS; i++){
		
		if (!stripmask.test(i)) continue; 

		string command("fstat "+ filename); 
		try{
			TCPSocket sock(FileServerList[i].first, FileServerList[i].second);
 			sock.send(command.c_str(), command.length()); 
	
			char echoBuffer[RCVBUFSIZE+1]; 
			int recvMsgSize = 0;
		
			if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
				cerr << "unable to recv "; 
				response="nack"; 
			}else {
				echoBuffer[recvMsgSize]='\0'; 
				response = echoBuffer; 
			}
		}catch(SocketException &e) {
    		cerr << e.what() << endl;
    		response="nack"; 
  		}
		
		if (toLower(response) != "nack"){
			
			size_t file_size = atoi(trim(nextToken(response)).c_str());
			time_t modification_time = atoi(trim(nextToken(response)).c_str());
			time_t creation_time = atoi(trim(nextToken(response)).c_str()); 
	
			cout << "modification time in server " << i << ": " << modification_time << endl; 
			cout << "file size in server  " << i << ": " << file_size << endl;  			
			cout << "creation time in server " << i << ": " << creation_time << endl; 

			if (creation_time > last_ctime) last_ctime = creation_time; 
			if (modification_time > last_mtime) last_mtime = modification_time; 
			total_size += file_size; 
		}

	}

	string ret_str = static_cast<ostringstream*>( &(ostringstream() << total_size ))->str();
	ret_str += " ";
	ret_str += last_mtime; 
	ret_str += " ";
	ret_str += last_ctime;  

	return ret_str; 
}





