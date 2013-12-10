#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include <string>
#include <bitset>
#include <map>
#include "config.hh"


struct fileRecipe{
	int stripeWidth; 	
	std::bitset<NUM_FILE_SERVERS> stripeMask; 

	fileRecipe(int sw, std::bitset<NUM_FILE_SERVERS> sm){
		stripeWidth = sw; 
		stripeMask = sm; 
	}
	fileRecipe operator=(fileRecipe fr){
		stripeWidth = fr.stripeWidth; 
		stripeMask = fr.stripeMask; 
		return fr; 
 	}
	fileRecipe(){
		stripeWidth = 0;  
	}
	
	string toString(){
		string frs = static_cast<ostringstream*>( &(ostringstream() << stripeWidth ))->str();
		frs += " "; 
		frs += string(stripeMask.to_string()); 
		return frs; 
	}

}; 

struct fileEntry{
	string file_name;
	fileRecipe file_recipe;
	
	fileEntry(string fn, fileRecipe fr){
		file_name = fn; 
		file_recipe.stripeWidth = fr.stripeWidth; 
		file_recipe.stripeMask = fr.stripeMask; 
	}
}; 

typedef map<string, fileEntry> FILETABLE;  
FILETABLE general_file_table; 


#endif
