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
	strcpy(buf, "ntahoeustnhaosentuhastnoheustnaoheusntahouesnthaonethuanhu"); 


	cout << pfs_write(fdes, (void *)buf, 5*1024, 2*1024, 0 ) << endl; 
	FileDescriptor::printTokens(fdes); 
	

	usleep(20000000); 

	return 0; 
}
