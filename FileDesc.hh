
#include "data_types.hh"
#include <iostream>
#include <map>

using namespace std; 

#define MAX_NUM_FILES 256

typedef map<Interval,char, mycomparison> TOKEN_MAP ; 

int next_file_descriptor = 0; 

struct OpenFile_Entry{
	int desc; 
	fileRecipe * file_recipe; 
	bool open; 
	
	string name; 
	string mode; 

	TOKEN_MAP tokens; 

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

bool checkPermission(int fdesc, int block, char mode){
	TOKEN_MAP tok_map = OFDT[fdesc].tokens; 	
	
	Interval bl_int (block, block);
	if (tok_map.find(bl_int) == tok_map.end()) // don't have block token
	{
		return false; 
	}
	TOKEN_MAP::iterator it = tok_map.find(bl_int); 

	switch (mode){
		case 'r':
			return true; 
		case 'w': 
			if (it->second == 'r') 
				return false; 
			else 
				return true; 
		default: 
			return false; 
	}
}

void addPermission(int fdesc, int start, int end, char mode){
	TOKEN_MAP tok_map = OFDT[fdesc].tokens; 
	Interval bl_int (start, end); 
	
	while (tok_map.find(bl_int) != tok_map.end()){ // when it has overlap 
		TOKEN_MAP::iterator it = tok_map.find(bl_int); 
		if (it->first.m_start > bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){ 
				// new  ----------  				 
				// old    ----     
				if (mode == 'w' || it->second == 'r'){
					tok_map.erase(it); 
					// continue
				}else if (mode == 'r' && it->second == 'w'){
					Interval in (bl_int.m_start, it->first.m_start - 1); 
					tok_map.insert(make_pair( in, mode)); 
					bl_int.m_start = it->first.m_end + 1; 
					// continue
				}

			}else if (it->first.m_end == bl_int.m_end){
				// new -------------
				// old    ----------
				if (mode == 'w' || it->second == 'r'){
					tok_map.erase(it); 
					// continue
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_end = it->first.m_start - 1; 
					// continue
				}

			}else if (it->first.m_end > bl_int.m_end){
				// new -------------
				// old     -------------
				if (mode == 'w' && it->second == 'w'){
					bl_int.m_end = it->first.m_end; 
					tok_map.erase(it); 
					// continue
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int(bl_int.m_end + 1, it->first.m_end); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 

					// continue	
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_end = it->first.m_start - 1; 
					// continue
				}else if (mode == 'r' && it->second == 'r'){
					bl_int.m_end = it->first.m_end; 
					tok_map.erase (it); 
					// continue
				}
			}
		
		}else if (it->first.m_start == bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){
				// new ------------------
				// old -------
				if (mode == 'w' && it->second == 'w'){
					tok_map.erase(it); 
					// continue
				}else if (mode == 'w' && it->second == 'r'){
					tok_map.erase(it); 
					// continue	
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_start = it->first.m_end + 1; 
					// continue
				}else if (mode == 'r' && it->second == 'r'){
					tok_map.erase (it); 
					// continue
				}
	
			}else if (it->first.m_end == bl_int.m_end){
				// new ------------------
				// old ------------------
				if (mode == 'w' || it->second == 'r') {
					tok_map.erase(it); 
					// break;FIXME if you want to change it to continue, some changes needed
				}else if (mode == 'r' && it->second == 'w') {
					// break; 	FIXME if you want to change it to continue, some changes needed
				}
					
			}else if (it->first.m_end > bl_int.m_end){
				// new ------------
				// old ------------------
				if (mode == 'w' && it->second == 'w'){
					// break  FIXME if you want to continue here, some changes are needed
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int (bl_int.m_end + 1, it->first.m_end); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 
					// continue	
				}else if (mode == 'r' && it->second == 'w'){
					// break FIXME if you want to continue here, some changes are needed 
				}else if (mode == 'r' && it->second == 'r'){
					// break FIXME if you want to continue here, some changes are needed
				}

			}
		}else if  (it->first.m_start < bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){
				// new -------------------
				// old        ----- 
				if (mode == 'w' && it->second == 'w'){
					tok_map.erase(it); 
					// continue; 
				}else if (mode == 'w' && it->second == 'r'){
					tok_map.erase(it); 
					// continue	
				}else if (mode == 'r' && it->second == 'w'){
					Interval in (bl_int.m_start, it->first.m_start - 1);
					tok_map.insert(make_pair(in, mode)); 
					bl_int.m_start = it->first.m_end + 1; 
					// continue
				}else if (mode == 'r' && it->second == 'r'){
					tok_map.erase(it); 
					// continue; 
				}
			}else if (it->first.m_end == bl_int.m_end){
				// new -------------------
				// old     ---------------
				if (mode == 'w' || it->second == 'r') {
					tok_map.erase(it); 
					// continue
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_end = it->first.m_start - 1; 
					// continue
				}
			}else if (it->first.m_end > bl_int.m_end){
				// new ------------------
				// old       ---------------	
				if (mode == 'w' && it->second == 'w'){
					bl_int.m_end = it->first.m_end; 
					tok_map.erase(it); 
					// continue; 
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int(bl_int.m_end + 1, it->first.m_end); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 
					// continue	
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_end = it->first.m_start - 1;  
					// continue
				}else if (mode == 'r' && it->second == 'r'){
					bl_int.m_end = it->first.m_end; 
					tok_map.erase(it); 
					// continue; 
				}
			}
		
		}
	
	}
	if (tok_map.find(bl_int) == tok_map.end()) // don't have block token
	{

		Interval *right = NULL; 
		Interval *left  = NULL;
 
		if (end   != ULONG_MAX) 
	 		right  = new Interval(end+1, end+1); 
		if (start != 0        ) 
			left   = new Interval(start - 1, start -1); 	

		if (right != NULL && tok_map.find(*right) != tok_map.end()){
			if (tok_map.find(*right)->second == mode){
				TOKEN_MAP::iterator it_right = tok_map.find(*right); 
				bl_int.m_end = it_right->first.m_end; 	
				tok_map.erase(*right); 
			}
		}
		if (left != NULL && tok_map.find(*left) != tok_map.end()){
			if (tok_map.find(*left)->second == mode){
				TOKEN_MAP::iterator it_left = tok_map.find(*left); 
				bl_int.m_start = it_left->first.m_start; 
				tok_map.erase(*left); 
			}
		}
		
		tok_map.insert(make_pair(bl_int, mode)); 
	}else {
		// error 
		cout << "it's not supposed to find overlap any more " << endl; 
	}

}

		
	



