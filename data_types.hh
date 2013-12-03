#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include <bitset>
#include "config.hh"

//typedef size_t LBA; //Logical Block Address (LBA) type
typedef std::pair<string,size_t> LBA; //Logical Block Address (LBA) type

struct blockT {
	LBA blockAdr; //what about file server ID?
	char data[PFS_BLOCK_SIZE*1024/sizeof(char)];
	char status; //dirty, clean or free
};

struct fileRecipe{
	int stripeWidth; 	
	std::bitset<NUM_FILE_SERVERS> stripeMask; 

	fileRecipe(int sw, std::bitset<NUM_FILE_SERVERS> sm){
		stripeWidth = sw; 
		stripeMask = sm; 
	}
	fileRecipe(){
		stripeWidth = 0;  
	}
}; 
#endif
