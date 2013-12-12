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
#include <tr1/tuple>
#include <limits.h>
//#include <utility>
#define MAX_INT UINT_MAX

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
//  	else if ( command == "close"  )
//		execFunc_close  (recvCommand); 
//  	else if ( command == "read"   )
//		execFunc_read   (recvCommand); 
//  	else if ( command == "write"  ) 
//		execFunc_write  (recvCommand); 
  	else if ( command == "delete" )
		response = execFunc_delete (recvCommand); 
  	else if ( command == "fstat"  )
		response = execFunc_fstat  (recvCommand); 
  	else if ( command == "request_token") //FIXME
		response = execFunc_request_token  (recvCommand + " " + sock->getForeignAddress()); //FIXME
	cout << "response from MDM :" << response << endl;

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
string sendToServer(string message, string IP, int port){
	string response; 
	try {
		TCPSocket sock(IP, port);
	 	sock.send(message.c_str(), message.length());
			
		char echoBuffer[RCVBUFSIZE+1]; 
		int recvMsgSize = 0;
					
		if ((recvMsgSize = (sock.recv(echoBuffer,RCVBUFSIZE))) <=0 ){
			cerr << "unable to recv "; 
			response="nack"; 
		}
		else {
			echoBuffer[recvMsgSize]='\0';
			response = echoBuffer; 
		}
	}catch(SocketException &e) {
		cerr << e.what() << endl;
		response="exception"; 
  	}
	return response; 
}
string execFunc_create(string arguments){

	pthread_mutex_lock(&gft_lock); //get general file table lock

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
	if (toLower(response) == "nack"){
		pthread_mutex_unlock(&gft_lock); //unlock general file table lock
		return string("failed");
	}
	
	pthread_mutex_unlock(&gft_lock); //unlock general file table lock
	return string("success"); 
}

string execFunc_open(string arguments){
	pthread_mutex_lock(&gft_lock); //get general file table lock

	string filename = trim(nextToken(arguments)); 
	string mode     = trim(nextToken(arguments)); 

	fileRecipe f_recipe; 
	map<string, fileEntry>::iterator it; 
	it = general_file_table.find(filename); 
	if ( it != general_file_table.end()) {
		f_recipe = it->second.file_recipe; 
		pthread_mutex_unlock(&gft_lock); //unlock general file table lock
		return f_recipe.toString(); 
	}
	
	pthread_mutex_unlock(&gft_lock); //unlock general file table lock
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

	pthread_mutex_lock(&gft_lock); //get general file table lock
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

	if (toLower(response) == "nack"){
		pthread_mutex_unlock(&gft_lock); //unlock general file table lock
	
		return string("failed");
	}

	pthread_mutex_unlock(&gft_lock); //unlock general file table lock
	return string("success"); 
}
string execFunc_fstat(string arguments){
	pthread_mutex_lock(&gft_lock); //get general file table lock
	
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
		}else {
			pthread_mutex_unlock(&gft_lock); //unlock general file table lock
			return "nack"; 	
		}

	}

	string ret_str = static_cast<ostringstream*>( &(ostringstream() << total_size ))->str();
	ret_str += " ";
	ret_str += static_cast<ostringstream*>( &(ostringstream() << last_mtime ))->str(); 
	ret_str += " ";
	ret_str += static_cast<ostringstream*>( &(ostringstream() << last_ctime ))->str();

	pthread_mutex_unlock(&gft_lock); //unlock general file table lock
	return ret_str; 
}

string execFunc_request_token(string arguments){ //FIXME <request_token,file_name,block_offset,mode>
	cout << "TOKEN_MNG: request_token called with this arg: " << arguments << endl;
	
	string file_name = nextToken(arguments);
	
	int interval_start = atoi(trim(nextToken(arguments)).c_str());
	int interval_end = atoi(trim(nextToken(arguments)).c_str());
	char mode = trim(nextToken(arguments))[0];
	int client_port  = atoi(trim(nextToken(arguments)).c_str()); //revoker port
	string client_IP = trim(nextToken(arguments)); 
	
	cout << "TOKEN_MNG: " << file_name << " " << interval_start << " " << interval_end << " " << mode; 
	cout << " " << client_port << " " << client_IP << endl; 
	
	string message;
	string response;
	string writer_IP, reader_IP;
	int writer_port, reader_port;

	pthread_mutex_lock(&gft_lock); 
	map<string, fileEntry>::iterator it = general_file_table.find(file_name); //FIX: does it need lock?

	cout << "locked gft " << endl; 
	if (it == general_file_table.end()) { //first reader/writer of the file
		pthread_mutex_unlock(&gft_lock); 
		cout << "TOKEN_MNG: trying to get tokens for a non exisiting file!\n";
		return "nack";
	}
	
	fileEntry& fe = it->second;

	
	//try for file entry lock
	if (pthread_mutex_trylock(&fe.fe_lock) != 0 ){ //failed to get lock for this file entry
			
		pthread_mutex_unlock(&gft_lock);
		cout << "return locking failed " << endl; 
		return "locking_failed";
	}
	
	Interval interval(interval_start, interval_end);
	map<Interval,tr1::tuple<string,int> >::iterator mdwtokens_it;
	vector<tr1::tuple<Interval,string,int> >::iterator mdrtokens_it;

	if(mode == 'r') { //client wants to read
		mdwtokens_it = fe.MDWTokens.find(interval);
		while(mdwtokens_it != fe.MDWTokens.end()) { //FIXME: check if I am the not the writer
			
			cout << " there is a writer need to revoke  " << endl; 
			writer_IP = tr1::get<0>(mdwtokens_it->second);
			writer_port = tr1::get<1>(mdwtokens_it->second);

			message = "";
			message = "revoke " + file_name + " ";;
			message += static_cast<ostringstream*>( &(ostringstream() << interval.m_start ))->str();
			message += " ";
			message += static_cast<ostringstream*>( &(ostringstream() << interval.m_end ))->str();
			message += " "; 
			string s; s.push_back(mode); 
			message += s; 
				
			response = sendToServer(message, writer_IP, writer_port); 
			cout << "TOKEN_MNG: send (" << message << ") to " << writer_IP << "," << writer_port << "("<< response<<") "<< endl; 
			if (response != "nack"){
				tr1::tuple<string, int> temp = mdwtokens_it->second; 

				Interval interval_old (mdwtokens_it->first.m_start, mdwtokens_it->first.m_end);
				vector<Interval> sub_intervals = interval_old.subtract_interval(interval);
					
				fe.MDWTokens.erase(mdwtokens_it); //remove the writers token				

				for (int i = 0; i < sub_intervals.size(); i++){
					fe.MDWTokens.insert(make_pair(sub_intervals[i], temp)); //remove the writers token				
				}

			}
			mdwtokens_it = fe.MDWTokens.find(interval);
					
				//do merging of intervals and call find again
				//merge intervals
		} //while
			//now all writers are revoked
			//now what are correct interval start and end?
		/*
		int new_start = 0;
		int new_end = UINT_MAX; 
		for ( map<Interval,tr1::tuple<string,int> >::iterator it = fe.MDWTokens.begin(); it != fe.MDWTokens.end(); ++it){
			if (it->first.m_end < interval.m_start)
				new_start = it->first.m_end + 1; 
			if (it->first.m_start > interval.m_end ){
				new_end = it->first.m_start - 1;
				break; // the first one after interval  
			}
		} 
		
		interval.m_start = new_start; 
		interval.m_end = new_end; 
		*/
		fe.MDRTokens.push_back(tr1::make_tuple(interval,client_IP,client_port));
		response = "";
		response += static_cast<ostringstream*>( &(ostringstream() << interval.m_start ))->str();
		response += " ";
		response += static_cast<ostringstream*>( &(ostringstream() << interval.m_end ))->str();
		fe.print();

		
	} //read mode

	else if(mode == 'w') { //client wants to write
		//revoke other writers
		mdwtokens_it = fe.MDWTokens.find(interval);
		
		//handle the writers first
		while (mdwtokens_it != fe.MDWTokens.end()) { //no writer
			 //there are some writers, so you need to revoke the tokens from them
				writer_IP = tr1::get<0>(mdwtokens_it->second);
				writer_port = tr1::get<1>(mdwtokens_it->second);
				message = "revoke ";
				message += static_cast<ostringstream*>( &(ostringstream() << interval.m_start ))->str();
				message += " ";
				message += static_cast<ostringstream*>( &(ostringstream() << interval.m_end ))->str();
				message += " ";
				string s; s.push_back(mode);  
				message += s; 

				response = sendToServer(message, writer_IP, writer_port); 
				if (response != "nack"){
					tr1::tuple<string, int> temp = mdwtokens_it->second; 

					Interval interval_old (mdwtokens_it->first.m_start, mdwtokens_it->first.m_end);
					vector<Interval> sub_intervals = interval_old.subtract_interval(interval);
					
					fe.MDWTokens.erase(mdwtokens_it); //remove the writers token				

					for (int i = 0; i < sub_intervals.size(); i++){
						fe.MDWTokens.insert(make_pair(sub_intervals[i], temp)); //remove the writers token				
					}

				}else{
					cout << "TOKEN_MNG: receive nack from revoker " << endl; 
				}
				//do merging of intervals and call find again
				//merge intervals
				mdwtokens_it = fe.MDWTokens.find(interval);
		 } //while
		 //now all writers are revoked
		
		/*
		unsigned int new_start = 0;
		unsigned int new_end = UINT_MAX; 
		for ( map<Interval,tr1::tuple<string,int> >::iterator it = fe.MDWTokens.begin(); it != fe.MDWTokens.end(); ++it){
			if (it->first.m_end < interval.m_start)
				new_start = it->first.m_end + 1; 
			if (it->first.m_start > interval.m_end ){
				new_end = it->first.m_start - 1;
				break; // the first one after interval  
			}
		} 


		interval.m_start = new_start; 
		interval.m_end = new_end; 	
		*/
		//then handle the readers
		for(mdrtokens_it = fe.MDRTokens.begin(); mdrtokens_it != fe.MDRTokens.end(); ++mdrtokens_it) {
			Interval interval2 = tr1::get<0>(*mdrtokens_it);  
			if (interval.is_equal(interval2)) {//if there is overlap 
				reader_IP = tr1::get<1>(*mdrtokens_it);
				reader_port = tr1::get<2>(*mdrtokens_it);
			
				message = "revoke ";
				message += static_cast<ostringstream*>( &(ostringstream() << interval.m_start ))->str();
				message += " ";
				message += static_cast<ostringstream*>( &(ostringstream() << interval.m_end ))->str();
				string s;
				s.push_back(mode); 
				message += " ";
				message += s;  

				response = sendToServer(message, reader_IP, reader_port);
				cout<< "revoke request sent to :" << "reader IP:" << reader_IP<< ",reader_port:" << reader_port << endl;
				if (response != "nack"){
	
//					tr1::tuple<string, int> temp = mdrtokens_it->second; 
					
					Interval interval_old (tr1::get<0>(*mdrtokens_it).m_start,tr1::get<0>(*mdrtokens_it).m_end); 
					vector<Interval> sub_intervals = interval_old.subtract_interval(interval);
					cout << "sub_interval.size():" << sub_intervals.size() << endl;

					for(int j=0;j<sub_intervals.size();j++)
						cout <<"sub_interval[" << j <<"]: " << sub_intervals[j].toString() << endl;

					mdrtokens_it = fe.MDRTokens.erase(mdrtokens_it); //remove the writers token				
					// FIXME 

					for (int i = 0; i < sub_intervals.size(); i++){
						fe.MDRTokens.push_back(tr1::make_tuple(sub_intervals[i], reader_IP, reader_port)); //remove the writers token				
					}
					
				//	if (mdrtokens_it != fe.MDRTokens.begin()){
				//	mdrtokens_it--; 
				//	}

				}else {
					cout << "get nack from revoker " << endl; 
				}
				
			} //if
		} //for

		fe.MDWTokens.insert(make_pair(interval,tr1::make_tuple(client_IP,client_port))); //add it to writers

		response = "";
		response += static_cast<ostringstream*>( &(ostringstream() << interval.m_start ))->str();
		response += " ";
		response += static_cast<ostringstream*>( &(ostringstream() << interval.m_end ))->str();
		fe.print();
		
	} //write mode

	fe.print();

	pthread_mutex_unlock(&(fe.fe_lock));
	pthread_mutex_unlock(&gft_lock);

	return response;
}
