#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include <string>
#include <bitset>
#include "config.hh"

typedef size_t LBA; //Logical Block Address (LBA) type
//typedef std::pair<std::string,size_t> LBA; //Logical Block Address (LBA) type
struct fileRecipe{
	int stripeWidth; 	
	std::bitset<NUM_FILE_SERVERS> stripeMask; 

	fileRecipe(int sw, std::bitset<NUM_FILE_SERVERS> sm){
		stripeWidth = sw; 
		stripeMask = sm; 
	}
	fileRecipe(){
		stripeWidth = 1;  
	}
		
}; 

struct blockT {
	LBA blockAdr; //what about file server ID?
	char data[PFS_BLOCK_SIZE*1024/sizeof(char)];
	char status; //dirty, clean or free
	std::string file_name;
	size_t block_offset;
	size_t access_time; //FIXME: we need to update this everytime a block was accessed

	fileRecipe file_recipe;
};

class mycomparison
{
public: 
	bool operator() (const blockT& lhs, const blockT& rhs) const {
		return lhs.access_time < rhs.access_time;
	}
};
struct Interval
{
	Interval(int start, int length)
    	: m_start(start),
          m_length(length)
    	{}
        int m_start;
        int m_length;
};
#endif
