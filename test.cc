#include <iostream>
#include <sstream>
using namespace std; 

#include <iostream>
#include <tr1/functional>
#include <string>


int main(){

  char nts1[] = "Test";
  char nts2[] = "Test";
  std::string str1 (nts1);
  std::string str2 (nts2);

  std::tr1::hash<char*> ptr_hash;
  std::tr1::hash<std::string> str_hash;

  std::cout << "same hashes:\n" << std::boolalpha;
  std::cout << "nts1 and nts2: " << (ptr_hash(nts1)==ptr_hash(nts2)) << '\n';
  std::cout << "str1 and str2: " << (str_hash(str1)==str_hash(str2)) << '\n';
/*
	int a = 10; 
	ostringstream convert; 
	convert << a; 
	string str = convert.str();  
	cout << str << endl; 
*/

}

