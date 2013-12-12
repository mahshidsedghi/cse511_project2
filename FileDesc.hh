#ifndef __FDESC_HH__
#define __FDESC_HH__

#include "data_types.hh"
#include <iostream>
#include <sstream>
#include <map>
#include <pthread.h>

using namespace std; 

#define MAX_NUM_FILES 256

typedef map<Interval,char, mycomparison> TOKEN_MAP ; 
class OpenFile_Entry{
public:
	int desc; 
	fileRecipe * file_recipe; 
	bool open; 
	
	string name; 
	string mode; 

	TOKEN_MAP tokens; 

	pthread_mutex_t entry_mutex;	
	
	OpenFile_Entry(){
		entry_mutex = PTHREAD_MUTEX_INITIALIZER; 
	}

}; 

class FileDescriptor {
private: 
	static int next_file_descriptor;  
	static OpenFile_Entry OFDT[MAX_NUM_FILES];
public:
	
	static void ofdt_print_all(); 
	static int ofdt_open_file(fileRecipe * file_recipe, string file_name, string file_mode);
	static int ofdt_close_file(int fdesc);
	static fileRecipe * ofdt_fetch_recipe(int fdesc);
	static string ofdt_fetch_name(int fdesc);
	static string ofdt_fetch_mode(int fdesc);
	static void addPermission(int fdesc, int start, int end, char mode);
	static bool checkPermission(int fdesc, int block, char mode);
	static void printTokens	(int fdesc);
	static string revokePermission(string file_name, int start, int end, char mode); 
	static void removePermission(int fdesc, int start, int end, char mode); 

	static pthread_mutex_t file_desc_mutex; 

};

 
#endif 
