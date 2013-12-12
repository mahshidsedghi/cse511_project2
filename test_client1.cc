#include <iostream>

#include "pfs.hh"

using namespace std; 

int main(){

	// TEST CREATE 
	string file_name = "test_file.txt"; 
	
	// TEST OPEN 
	int fdes = pfs_open(file_name.c_str(), 'r');  
	cout << "open file: " << file_name << " with file descriptor: " << fdes << endl << endl ; 

	// TEST READ 
	
	char * buf =  (char *)malloc(5*1024);
	pfs_read(fdes, (void *)buf, 5*1024, 0, 0); 

	FileDescriptor::printTokens(fdes); 
	
	//cout << "(" << buf  << ")"<< endl; 
	//cout << "---------------------------------------------------------" << endl; 
	


	usleep(30000000); 

	return 0; 
}
