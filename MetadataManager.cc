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

#include <iostream>           // For cout, cerr
#include <cstdlib>            // For atoi()  
#include <pthread.h>          // For POSIX threads  

#include "data_types.hh"

#define servAddress 127.0.0.1
#define servPort 1234

const int RCVBUFSIZE = 64;

void HandleTCPClient(TCPSocket *sock);     // TCP client handling function
void *ThreadMain(void *arg);               // Main program of a thread  

int main(int argc, char *argv[]) {
  if (argc != 2) {                 // Test for correct number of arguments  
    cerr << "Usage: " << argv[0] << " <Server Port> " << endl;
    exit(1);
  }

  unsigned short echoServPort = atoi(argv[1]);    // First arg:  local port  

  try {
    TCPServerSocket servSock(echoServPort);   // Socket descriptor for server  
  
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
		execFunc_delete (recvCommand); 
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

	bool success = true; 

	string filename     = nextToken(arguments); 
	string stripe_w_str = nextToken(arguments); 

	fileRecipe file_recipe; 
	file_recipe.stripeWidth = atoi(stripe_w_str.c_str()); 

	for (int i = 0; i < atoi(stripe_w_str.c_str()); i++){
		file_recipe.stripeMask.set(i); //FIXME 
		
		// create a file in each server 
		string command("create "+ filename); 
		string response;
		try{
			TCPSocket sock(servAddress, servPort);
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
		
		if (response == "nack") 
			success = false; 

	}

	// put file_recipe in a table 
	FileEntry fentry(file_name, file_recipe);
	
	file_table.insert(file_name, fentry); 

	if (!success)
		return string("failed");

	return string("success"); 
}
string execFunc_open(string arguments){
	string filename = nextToken(arguments); 
	string mode     = nextToken(arguments); 

	// open the file 

	fileRecipe file_recipe; 
	file_recipe.stripeWidth = 3; // FIXME 
	file_recipe.stripeMask.set(0); // FIXME
	file_recipe.stripeMask.set(1);  // FIXME 
	file_recipe.stripeMask.set(2); // FIXME 
	
	string ret_str = static_cast<ostringstream*>( &(ostringstream() << file_recipe.stripeWidth ))->str(); 
	ret_str += " "; 
	ret_str += "11100"; //file_recipe.stripeMask;  // FIXME 

	return ret_str; 
}

void execFunc_close(string arguments){
	string filename = nextToken(arguments); 
	
	//cout << filename << endl; 

}
void execFunc_read(string arguments){
	// I'm not sure if we need this or not 
}
void execFunc_write(string arguments){
	// I'm not sure if we need this or not 
}
void execFunc_delete(string arguments){
	string filename = nextToken(arguments); 
	// delete file 	
	//cout << filename << endl; 
}
void execFunc_fstat(string argumnets){
	// not idea right now. 
}





