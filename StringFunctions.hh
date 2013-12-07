#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <sstream>
using namespace std; 

string nextToken(string &str){
	string token; 
	token = str.substr(0, str.find(" ")); 
	str = str.substr(token.size() + 1); 	

	return token; 
} 
string toLower(string str){
	string lower (""); 
	for (unsigned int i=0; i < str.length(); ++i)
		lower += tolower(str[i]); 
	
	return lower; 
} 


string ltrim(string s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
string &rtrim(string s) {
	s.erase(find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
string trim(string s) {
	return ltrim(rtrim(s));
}

