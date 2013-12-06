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

#include "PracticalSocket.hh"  // For Socket, ServerSocket, and SocketException
#include "StringFunctions.hh"
#include "FileServer.hh"

#include <iostream>           // For cout, cerr
#include <cstdlib>            // For atoi()  
#include <pthread.h>          // For POSIX threads  
#include <stdio.h>
#include "config.hh"
#include "string.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace std;

const int RCVBUFSIZE = 64;

void HandleTCPClient(TCPSocket *sock);     // TCP client handling function
void *ThreadMain(void *arg);               // Main program of a thread  

int main(int argc, char *argv[]) {

	
  unsigned short echoServPort = SERVER0_PORT;    // First arg:  local port  

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

  /*while ((recvMsgSize = sock->recv(echoBuffer, RCVBUFSIZE)) > 0) { // Zero means
                                                         // end of transmission
    // Echo message back to client
    sock->send(echoBuffer, recvMsgSize);
  }*/
  //serve the client request
  string message;
  string command;
  if ((recvMsgSize = sock->recv(echoBuffer, RCVBUFSIZE)) > 0) {
	  message = echoBuffer;
	  command = nextToken(message);
	  if (toLower(command) == "create") {
			execFunc_create(sock, message); 
	  }
	  else if (toLower(command) == "read") {
			execFunc_read(sock, message); 
	  }
	  else if (toLower(command) == "write") {
      		execFunc_write(sock, message); 
	  }
	  else if (toLower(command) == "delete"){
			execFunc_delete(sock, message); 
	  }
	  else {
			cout << "unknown command" << endl; 		
	  }
//	  fclose (pfs_file);
	cerr << "exisiting the server loop" ;
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

void execFunc_create ( TCPSocket * sock, string arguments ){
	string file_name = nextToken(arguments);
	struct stat st;
	if (stat(file_name.c_str(),&st) == 0 ){
		cout << "Trying to create file: " << file_name << " which already exists\n";
		sock->send("nack", 4); 
	}else {
		int fd = creat(file_name.c_str(), O_CREAT); //what about file access rights?
	  	if (fd >= 0) {
			cout << "File : " << file_name << " created on the server\n";
	 		close(fd);
			sock->send("ack", 3); 
		}
		else {
			cerr << "Failed creating the file on the server!\n";
			sock->send("nack", 4); 
		}
	}

} 
void execFunc_read   ( TCPSocket * sock, string arguments ){
 	string file_name = nextToken(arguments);
	FILE* pfs_file = fopen(file_name.c_str(), "r");
	if (pfs_file != NULL) {
		int block_offset = atoi(nextToken(arguments).c_str());
		int num_blocks = atoi(nextToken(arguments).c_str());
		long int byte_offset = block_offset * PFS_BLOCK_SIZE * 1024;
		cout << "message: " << "read" << ", filename: " <<file_name <<
				", block_offset: " << block_offset << ", num_blocks: "<< num_blocks << endl;
		if(fseek(pfs_file,byte_offset,SEEK_SET))
			cerr <<"fseek failed\n";
		for(int i=0; i<num_blocks; i++) {
			char* block_buffer = (char*) malloc (PFS_BLOCK_SIZE * 1024);

			int num_read = fread(block_buffer, PFS_BLOCK_SIZE * 1024, 1, pfs_file);
			cout << "num read:" << num_read <<endl;
			cout << "block buffer" << block_buffer << endl;
			sock->send(block_buffer, PFS_BLOCK_SIZE * 1024);
		}
		fclose (pfs_file);
	}
	else
		cerr << "Reading from file which does not exist!\n";
} 
void execFunc_write  ( TCPSocket * sock, string arguments ){
		  string file_name = nextToken(arguments);
  		  char echoBuffer[RCVBUFSIZE];
  		  int recvMsgSize;
		  FILE* pfs_file = fopen(file_name.c_str(), "w");
		  if (pfs_file != NULL) {
			  int block_offset = atoi(nextToken(arguments).c_str());
			  int num_blocks = atoi(nextToken(arguments).c_str());
			  long int byte_offset = block_offset * PFS_BLOCK_SIZE * 1024;
			  cout << "message: " << "write" << ", filename: " <<file_name <<
					  ", block_offset: " << block_offset << ", num_blocks: "<< num_blocks <<endl; 
			  if(fseek(pfs_file,byte_offset,SEEK_SET))
				  cerr <<"fseek failed\n";
	
			string message; 
			while ((recvMsgSize = sock->recv(echoBuffer, RCVBUFSIZE)) > 0) {
				message += string(echoBuffer);
//				sock->send("ack",3); //FIXME
			}
//			cerr <<"msg" << message;
			int num_write = fwrite(message.c_str(), message.size(), 1, pfs_file);
			cout << "num write:" << num_write << endl;
			cout << "message" << message << endl;
//		 	sock->send("ack", 3); //FIXME
			fclose (pfs_file);
		  }
		  else
			  cerr << "Reading from file which does not exist!\n";
	

} 
void execFunc_delete ( TCPSocket * sock, string arguments ){


}



