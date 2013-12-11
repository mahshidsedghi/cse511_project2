#ifndef __NET_ADDRESSES_H__ 
#define __NET_ADDRESSES_H__

#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in

#include "config.hh"
#include <iostream>
using namespace std; 

#define PUMA_ADDR 		"130.203.40.19"
#define GANGA_ADDR 		"130.203.59.130"
#define MAMBERAMO_ADDR 		"130.203.40.73"
#define TITAN_ADDR 		"130.203.59.10"


//#define SERVER0_ADDR 	MAMBERAMO_ADDR
#define SERVER0_ADDR 	TITAN_ADDR
#define SERVER0_PORT 	1111
#define SERVER1_ADDR 	MAMBERAMO_ADDR
#define SERVER1_PORT 	2222
#define SERVER2_ADDR 	MAMBERAMO_ADDR
#define SERVER2_PORT 	3333
#define SERVER3_ADDR 	MAMBERAMO_ADDR
#define SERVER3_PORT 	4444
#define SERVER4_ADDR 	MAMBERAMO_ADDR
#define SERVER4_PORT 	5555

//#define METADATA_ADDR 	MAMBERAMO_ADDR
#define METADATA_ADDR 	TITAN_ADDR
#define METADATA_PORT 	2345

static int revoker_ports[5] = {1110, 2220, 3330, 4440, 5550};  

static bool port_is_open_2(int port){
	try {
		TCPSocket sock ("127.0.0.1", port); 
		sock.~Socket();
		return false;  
	}catch(SocketException &e){
		return true; 
	}
}
static bool port_is_open(int port){
	
	struct sockaddr_in client; 
	
	client.sin_family = AF_INET; 
	client.sin_port = htons(port); 
  	client.sin_addr.s_addr = htonl(INADDR_ANY);

	int sock = (int) socket(AF_INET, SOCK_STREAM, 0);
	 
	int result = connect(sock, (struct sockaddr *)&client, sizeof(client)); 
	if (result == 0){ 
		close(sock); 
		return true; 
	}
	return false; 

}



inline void corresponding_server(size_t block_offset, int strip_width, string &server_address, int &server_port, size_t &offset_within){
	
	int server_number	 = ( block_offset / STRIP_SIZE) % strip_width; 
	offset_within = ((block_offset / STRIP_SIZE) / strip_width) * STRIP_SIZE + (block_offset % STRIP_SIZE); 

 
	// FIXME: no idea how to make this general  
	switch(server_number){
		case 0: 
			server_address = SERVER0_ADDR; 
			server_port    = SERVER0_PORT; 
			break; 
		case 1: 
			server_address = SERVER1_ADDR; 
			server_port    = SERVER1_PORT; 
			break; 
		case 2: 
			server_address = SERVER2_ADDR; 
			server_port    = SERVER2_PORT; 
			break; 
		case 3: 
			server_address = SERVER3_ADDR; 
			server_port    = SERVER3_PORT; 
			break; 
		case 4: 
			server_address = SERVER4_ADDR; 
			server_port    = SERVER4_PORT; 
			break; 

	}
	
	cout << "block#" << block_offset << " server#" << server_number << " offset within server: " << offset_within;  
	cout << " " << server_address << " " << server_port << endl; 
 
}


#endif 
