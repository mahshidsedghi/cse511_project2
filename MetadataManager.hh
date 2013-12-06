#include "config.hh"

string execFunc_create ( string arguments ); 
string execFunc_open   ( string arguments ); 
void execFunc_read   ( string arguments ); 
void execFunc_write  ( string arguments ); 
void execFunc_close  ( string arguments ); 
void execFunc_delete ( string arguments ); 
void execFunc_fstat  ( string arguments ); 


pair<string, int> FileServerList[NUM_FILE_SERVERS]; 




