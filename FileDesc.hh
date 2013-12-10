
#include "data_types.hh"
#include <iostream>


using namespace std; 

#define MAX_NUM_FILES 256

int next_file_descriptor = 0; 

struct OpenFile_Entry{
	int desc; 
	fileRecipe * file_recipe; 
	bool open; 
	
	string name; 
	string mode; 

	map<Interval,char> tokens; 

}; 

OpenFile_Entry OFDT[MAX_NUM_FILES];

void ofdt_print_all(){
	for (int i = 0; i < MAX_NUM_FILES; i++){
		if (OFDT[i].open == true){
			cout << "---------------------------------" << endl; 
			cout << "[" << OFDT[i].desc<< "] " << OFDT[i].name << "\t(" << OFDT[i].mode<< ")" << endl; 
			cout << "---------------------------------" << endl; 
		}	
	}
}

int ofdt_open_file(fileRecipe * file_recipe, string file_name, string file_mode){ 
	int fdesc = -1;
	for (int i = 0; i < MAX_NUM_FILES; i++){ 
		if(OFDT[next_file_descriptor].open == false){
			fdesc = next_file_descriptor; 
			next_file_descriptor = (next_file_descriptor + 1) % MAX_NUM_FILES; 
			break; 
		}
		next_file_descriptor = (next_file_descriptor + 1) % MAX_NUM_FILES; 
	} 
	if (fdesc == -1) {
		cerr << "Cannot open another file, the maximum number of open files is " << MAX_NUM_FILES << endl; 
	}
	
	OFDT[fdesc].desc = fdesc;
	OFDT[fdesc].file_recipe = file_recipe;  
	OFDT[fdesc].open = true; 
	OFDT[fdesc].name = file_name; 
	OFDT[fdesc].mode = file_mode; 

	return fdesc; 
} 

// close a file, remove from open file descriptor table, return 1 if success, 0 if fail
int ofdt_close_file(int fdesc){ 
	if (OFDT[fdesc].open == false)
		return 0; 
	OFDT[fdesc].open = false; 
	OFDT[fdesc].desc = -1;    
//	delete OFDT[fdesc].file_recipe; 

	return 1; 
}

// fetch a file information from the ofdt 
fileRecipe * ofdt_fetch_recipe(int fdesc){
	if (OFDT[fdesc].open == false) {
		cerr << "This file is not open! " << endl;
		return NULL; 
	}
	return OFDT[fdesc].file_recipe; 
}

string ofdt_fetch_name(int fdesc){
	if (OFDT[fdesc].open == false) {
		cerr << "This file is not open! " << endl;
		return ""; 
	}
	return OFDT[fdesc].name; 

}
string ofdt_fetch_mode(int fdesc){
	if (OFDT[fdesc].open == false) {
		cerr << "This file is not open! " << endl;
		return ""; 
	}
	return OFDT[fdesc].mode; 
}

