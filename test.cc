#include <iostream>
#include <sstream>
using namespace std; 


int main(){
	int a = 10; 
	ostringstream convert; 
	convert << a; 
	string str = convert.str();  
	cout << str << endl; 
}
