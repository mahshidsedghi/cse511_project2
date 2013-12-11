
#include "FileDesc.hh"


int FileDescriptor::next_file_descriptor = 0;  
OpenFile_Entry FileDescriptor::OFDT[MAX_NUM_FILES];


void FileDescriptor::ofdt_print_all() {
	for (int i = 0; i < MAX_NUM_FILES; i++){
		if (FileDescriptor::OFDT[i].open == true){
			cout << "---------------------------------" << endl; 
			cout << "[" << OFDT[i].desc<< "] " << OFDT[i].name << "\t(" << OFDT[i].mode<< ")" << endl; 
			cout << "---------------------------------" << endl; 
		}	
	}
}

int FileDescriptor::ofdt_open_file(fileRecipe * file_recipe, string file_name, string file_mode){ 

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
int FileDescriptor::ofdt_close_file(int fdesc){ 
	if (OFDT[fdesc].open == false)
		return 0; 
	FileDescriptor::OFDT[fdesc].open = false; 
	FileDescriptor::OFDT[fdesc].desc = -1;    
//	delete FileDescriptor::OFDT[fdesc].file_recipe; 

	return 1; 
}

// fetch a file information from the ofdt 
fileRecipe * FileDescriptor::ofdt_fetch_recipe(int fdesc){
	if (FileDescriptor::OFDT[fdesc].open == false) {
		cerr << "This file is not open! " << endl;
		return NULL; 
	}
	return FileDescriptor::OFDT[fdesc].file_recipe; 
}

string FileDescriptor::ofdt_fetch_name(int fdesc){
	if (FileDescriptor::OFDT[fdesc].open == false) {
		cerr << "This file is not open! " << endl;
		return ""; 
	}
	return FileDescriptor::OFDT[fdesc].name; 

}
string FileDescriptor::ofdt_fetch_mode(int fdesc){
	if (FileDescriptor::OFDT[fdesc].open == false) {
		cerr << "This file is not open! " << endl;
		return ""; 
	}
	return FileDescriptor::OFDT[fdesc].mode; 
}

void FileDescriptor::addPermission(int fdesc, int start, int end, char mode){
	TOKEN_MAP &tok_map = FileDescriptor::OFDT[fdesc].tokens; 
	
	Interval bl_int (start, end); 
	bool dont_add = false; 
	
	while (tok_map.find(bl_int) != tok_map.end()){ // when it has overlap 
		TOKEN_MAP::iterator it = tok_map.find(bl_int);
		if (it->first.m_start > bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){ 
				// new  ----------  				 
				// old    ----     
				if (mode == 'w' || it->second == 'r'){
					tok_map.erase(it); 
					continue;
				}else if (mode == 'r' && it->second == 'w'){
					Interval in (bl_int.m_start, it->first.m_start - 1); 
					tok_map.insert(make_pair( in, mode)); 
					bl_int.m_start = it->first.m_end + 1; 
					continue;
				}

			}else if (it->first.m_end == bl_int.m_end){
				// new -------------
				// old    ----------
				if (mode == 'w' || it->second == 'r'){
					tok_map.erase(it); 
					continue;
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_end = it->first.m_start - 1; 
					continue;
				}

			}else if (it->first.m_end > bl_int.m_end){
				// new -------------
				// old     -------------
				if (mode == 'w' && it->second == 'w'){
					bl_int.m_end = it->first.m_end; 
					tok_map.erase(it); 
					continue;
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int(bl_int.m_end + 1, it->first.m_end); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 

					continue;
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_end = it->first.m_start - 1; 
					continue;
				}else if (mode == 'r' && it->second == 'r'){
					bl_int.m_end = it->first.m_end; 
					tok_map.erase (it); 
					continue;
				}
			}
		
		}else if (it->first.m_start == bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){
				// new ------------------
				// old -------
				if (mode == 'w' && it->second == 'w'){
					tok_map.erase(it); 
					continue;
				}else if (mode == 'w' && it->second == 'r'){
					tok_map.erase(it); 
					continue;	
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_start = it->first.m_end + 1; 
					continue;
				}else if (mode == 'r' && it->second == 'r'){
					tok_map.erase (it); 
					continue;
				}
	
			}else if (it->first.m_end == bl_int.m_end){
				// new ------------------
				// old ------------------
				if (mode == 'w' || it->second == 'r') {
					tok_map.erase(it); 
					break; 
				}else if (mode == 'r' && it->second == 'w') {
					dont_add = true; 
					break;
				}
					
			}else if (it->first.m_end > bl_int.m_end){
				// new ------------
				// old ------------------
				if (mode == 'w' && it->second == 'w'){
					dont_add = true; 
					break; 
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int (bl_int.m_end + 1, it->first.m_end); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 
					continue;
				}else if (mode == 'r' && it->second == 'w'){
					dont_add = true; 
					break;  
				}else if (mode == 'r' && it->second == 'r'){
					dont_add = true; 
					break; 
				}

			}
		}else if  (it->first.m_start < bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){
				// new      ----------
				// old --------- 
				if (mode == 'w' && it->second == 'w'){
					bl_int.m_start = it->first.m_start; 
					tok_map.erase(it); 
					continue; 
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int (it->first.m_start, bl_int.m_start - 1); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 
					continue;	
				}else if (mode == 'r' && it->second == 'w'){
					bl_int.m_start = it->first.m_end + 1; 
					continue;
				}else if (mode == 'r' && it->second == 'r'){
					bl_int.m_start = it->first.m_start; 
					tok_map.erase(it); 
					continue; 
				}
			}else if (it->first.m_end == bl_int.m_end){
				// new        ------------
				// old     ---------------
				if (mode == 'w' && it->second == 'w'){
					dont_add = true; 
					continue; 
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int (it->first.m_start, bl_int.m_start - 1); 
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int, 'r')); 
					continue;	
				}else if (mode == 'r' && it->second == 'w'){
					dont_add = true; 
					continue;
				}else if (mode == 'r' && it->second == 'r'){
					dont_add = true; 
					continue; 
				}
			}else if (it->first.m_end > bl_int.m_end){
				// new 	  ----------
				// old -------------------	
				if (mode == 'w' && it->second == 'w'){
					dont_add = true; 
					continue; 
				}else if (mode == 'w' && it->second == 'r'){
					Interval temp_int1(it->first.m_start, bl_int.m_start - 1); 
					Interval temp_int2(bl_int.m_end + 1, it->first.m_end); 
					
					tok_map.erase(it); 
					tok_map.insert(make_pair(temp_int1, 'r'));
					tok_map.insert(make_pair(temp_int2, 'r'));
				 
					continue;
				}else if (mode == 'r' && it->second == 'w'){
					dont_add = true; 
					continue;
				}else if (mode == 'r' && it->second == 'r'){
					dont_add = true; 
					continue; 
				}
			}
		
		}
	
	}
	if (!dont_add && tok_map.find(bl_int) == tok_map.end()) // don't have block token
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
	}else if (!dont_add){
		// error 
		cout << "it's not supposed to find overlap any more " << endl; 
	}

}

string FileDescriptor::revokePermission(string file_name, int start, int end, char mode){
	int file_desc = -1; 
	for (int i = 0; i < MAX_NUM_FILES; i++){
		if (OFDT[i].name == file_name) 
			file_desc = i; 
	} 	
	if (file_desc == -1 || OFDT[file_desc].open == false){
		string response = "0 "; 
		response += static_cast<ostringstream*>( &(ostringstream() << MAX_BLOCK_NUMBER ))->str(); 
		return response;
	}
	
	removePermission(file_desc, start, end, mode); 

	string response = static_cast<ostringstream*>( &(ostringstream() << start ))->str(); 
	response += " "; 
	response += static_cast<ostringstream*>( &(ostringstream() << end ))->str(); 
	return response;  
}

bool FileDescriptor::checkPermission(int fdesc, int block, char mode){	

	TOKEN_MAP& tok_map = FileDescriptor::OFDT[fdesc].tokens; 	

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


void FileDescriptor::printTokens	(int fdesc){

	TOKEN_MAP& tok_map_ = FileDescriptor::OFDT[fdesc].tokens; 

	for (TOKEN_MAP::iterator it=tok_map_.begin(); it!=tok_map_.end(); ++it)
	    cout << "(" << it->first.m_start <<","<< it->first.m_end <<")" << " => " << it->second << '\n';
	
	cout << " ======= " << endl; 	
}
void FileDescriptor::removePermission(int fdesc, int start, int end, char mode){

	TOKEN_MAP &tok_map = FileDescriptor::OFDT[fdesc].tokens; 
	
	Interval bl_int (start, end); 
	bool dont_add = false; 
	
	while (tok_map.find(bl_int) != tok_map.end()){ // when it has overlap 
		TOKEN_MAP::iterator it = tok_map.find(bl_int);
		if (it->first.m_start > bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){ 
				// new  ----------  				 
				// old    ----     
				tok_map.erase(it); 
				continue;

			}else if (it->first.m_end == bl_int.m_end){
				// new -------------
				// old    ----------
				tok_map.erase(it); 
				continue;

			}else if (it->first.m_end > bl_int.m_end){
				// new -------------
				// old     -------------
				Interval temp_int(bl_int.m_end + 1, it->first.m_end); 
				char it_mode = it->second; 
				tok_map.erase(it); 
				tok_map.insert(make_pair(temp_int, it_mode)); 

				continue;
			}
		
		}else if (it->first.m_start == bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){
				// new ------------------
				// old -------
				tok_map.erase(it); 
				continue;
	
			}else if (it->first.m_end == bl_int.m_end){
				// new ------------------
				// old ------------------
				tok_map.erase(it); 
				continue;  
					
			}else if (it->first.m_end > bl_int.m_end){
				// new ------------
				// old ------------------
				Interval temp_int (bl_int.m_end + 1, it->first.m_end); 
				char it_mode = it->second; 
				tok_map.erase(it); 
				tok_map.insert(make_pair(temp_int, it_mode)); 
				continue;
			}
		}else if  (it->first.m_start < bl_int.m_start){
			if (it->first.m_end < bl_int.m_end){
				// new      ----------
				// old --------- 
				Interval temp_int (it->first.m_start, bl_int.m_start - 1); 
				char it_mode = it->second; 
				tok_map.erase(it); 
				tok_map.insert(make_pair(temp_int, it_mode)); 
				continue;	
			}else if (it->first.m_end == bl_int.m_end){
				// new        ------------
				// old     ---------------
				Interval temp_int (it->first.m_start, bl_int.m_start - 1); 
				char it_mode = it->second; 
				tok_map.erase(it); 
				tok_map.insert(make_pair(temp_int, it_mode)); 
				continue; 	
			}else if (it->first.m_end > bl_int.m_end){
				// new 	  ----------
				// old -------------------	
				Interval temp_int1(it->first.m_start, bl_int.m_start - 1); 
				Interval temp_int2(bl_int.m_end + 1, it->first.m_end); 
				
				int it_mode = it->second; 	
				tok_map.erase(it); 
				tok_map.insert(make_pair(temp_int1, it_mode));
				tok_map.insert(make_pair(temp_int2, it_mode));
				 
				continue;
			}
		
		}
	
	}

}


