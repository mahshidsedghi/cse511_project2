#include "stringFunctions.hh"
string nextToken(string &str){
	string token; 
	istringstream iss(str); 
	iss >> token; 
	str = "";  
	while (iss){
		string temp; 
		iss >> temp; 
		str += temp + " "; 
	}
	return token; 
}
string toLower(string str){
	string lower (""); 
	for (unsigned int i=0; i < str.length(); ++i)
		lower += tolower(str[i]); 
	
	return lower; 
}


