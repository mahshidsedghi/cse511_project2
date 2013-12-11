#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <sstream>
using namespace std; 

inline string nextToken(string &str){
	string token;
	size_t found = str.find(" "); 
	if (found != string::npos){ 
		token = str.substr(0, str.find(" ")); 
		str = str.substr(token.size() + 1); 	
	}else{ 
		token = str; 
		str = ""; 	
	}

	return token; 
} 
inline string toLower(string str){
	string lower (""); 
	for (unsigned int i=0; i < str.length(); ++i)
		lower += tolower(str[i]); 
	
	return lower; 
} 


inline string ltrim(string s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
inline string rtrim(string s) {
	s.erase(find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
inline string trim(string s) {
	return ltrim(rtrim(s));
}

