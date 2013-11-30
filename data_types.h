#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include "config.h"

typedef size_t LBA; //Logical Block Address (LBA) type

struct blockT {
	LBA blockAdr; //what about file server ID?
	int data[PFS_BLOCK_SIZE*1024/sizeof(int)];
	char status; //dirty, clean or free
};

#endif
