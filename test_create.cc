#include <iostream>

#include "pfs.hh"

using namespace std; 

int main(){

	// TEST CREATE 
	string file_name = "test_file.txt"; 
	if (pfs_create(file_name.c_str(), 1) > 0)  cout << "successful creation of " << file_name << "!" << endl << endl; 
	else 	
		return 1; 
	cout << "---------------------------------------------------------" << endl; 


	return 0; 
}
