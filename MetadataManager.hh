#include "config.hh"

string execFunc_create ( string arguments ); 
string execFunc_open   ( string arguments ); 
void execFunc_read   ( string arguments ); 
void execFunc_write  ( string arguments ); 
void execFunc_close  ( string arguments ); 
string execFunc_delete ( string arguments ); 
string execFunc_fstat  ( string arguments ); 
string execFunc_request_token( string arguments );


pair<string, int> FileServerList[NUM_FILE_SERVERS]; 



