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
#include <iostream>           // For cout, cerr
#include <cstdlib>            // For atoi()  
#include <pthread.h>          // For POSIX threads  
#include <stdio.h>
#include "config.hh"

const int RCVBUFSIZE = 32;

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

  /*while ((recvMsgSize = sock->recv(echoBuffer, RCVBUFSIZE)) > 0) { // Zero means
                                                         // end of transmission
    // Echo message back to client
    sock->send(echoBuffer, recvMsgSize);
  }*/
  //serve the client request
  string message;
  string command;
  string file_name;
  FILE* pfs_file;
  int block_offset;
  int num_blocks;
  char* block_buffer = (char*) malloc (PFS_BLOCK_SIZE * 1024);
  long int byte_offset;
  while ((recvMsgSize = sock->recv(echoBuffer, RCVBUFSIZE)) > 0) {
	  message = echoBuffer;
	  command = nextToken(message);
	  if (toLower(command) == "read") {
		  file_name = nextToken(message);
		  pfs_file = fopen(file_name.c_str(), "r");
		  if (pfs_file != NULL) {
			  block_offset = atoi(nextToken(message).c_str());
			  num_blocks = atoi(nextToken(message).c_str());
			  byte_offset = block_offset * PFS_BLOCK_SIZE * 1024;
			  cout << "message: " << command << ", filename: " <<file_name <<
					  ", block_offset: " << block_offset << ", num_blocks: "<< num_blocks << endl;
			  if(fseek(pfs_file,byte_offset,SEEK_SET))
				  cerr <<"fseek failed\n";
			  for(int i=0;i<num_blocks;i++){
				  fread(block_buffer, PFS_BLOCK_SIZE * 1024, 1, pfs_file);
				  sock->send(block_buffer, PFS_BLOCK_SIZE * 1024);
			  }
			  fclose (pfs_file);
		  }
		  else
			  cerr << "Reading from file which does not exist!\n";
	  }
	  else if (toLower(nextToken(message)) == "write") {}
	  else {}
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
