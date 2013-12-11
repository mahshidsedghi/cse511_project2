
#include "pfs.hh"

int main(){
	//ofdt_print_all(); 

/*	blockT b1;
	b1.file_name = "baghali.txt";
	strcpy(b1.data , "this line was written by client on the server using writeToFileServer Function");
	size_t nwrite = disk_cache.writeToFileServer(b1);//,SERVER0_ADDR,SERVER0_PORT); //gives seg fault
	cout <<"nwrite:" << nwrite << endl; 
*/	
	

/*
	// TEST CREATE 
	string file_name = "test_file.txt"; 
	if (pfs_create(file_name.c_str(), 2) > 0)  cout << "successful creation of " << file_name << "!" << endl << endl; 

	cout << "---------------------------------------------------------" << endl; 
	// TEST OPEN 
	int fdes = pfs_open(file_name.c_str(), 'r');  
	cout << "open file: " << file_name << " with file descriptor: " << fdes << endl << endl ; 


	cout << "---------------------------------------------------------" << endl; 
	// TEST WRITE 
	char * buf =  (char *)malloc(1*ONEKB);
	strcpy(buf , "this line was written by client  on the server using writeToFileServerFunction");
	cout << "pfs write " << pfs_write(fdes, (void *)buf, 1*ONEKB, 0, 0) << endl;  
	
	cout << "---------------------------------------------------------" << endl; 
	
	// TEST READ 
	strcpy(buf , "something else"); 
	cout << pfs_read(fdes, (void *)buf, 1*ONEKB, 0, 0); 

	cout << "(" << buf  << ")"<< endl; 
	cout << "---------------------------------------------------------" << endl; 
	

	usleep(20000000); 

	// TEST DELETE 
	//if (pfs_delete(file_name.c_str()) > 0) cout << "successful delete of " << file_name << "!" << endl << endl;  

	pfs_stat st; 
	cout << "get fstat: (" << pfs_fstat(fdes, &st) << ")" << endl; 
	cout << "size " << st.pst_size << " ctime " << st.pst_ctime << " mtime " << st.pst_mtime << endl; 
	
*/

	//TEST HARVESTING FUNCTION
	







	return 0; 
}
