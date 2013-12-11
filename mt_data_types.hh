#ifndef DATA_TYPES_H_
#define DATA_TYPES_H_

#include <string>
#include <bitset>
#include <map>
#include "config.hh"
#include <vector>
#include <tr1/tuple>

struct fileRecipe{
	int stripeWidth; 	
	std::bitset<NUM_FILE_SERVERS> stripeMask; 

	fileRecipe(int sw, std::bitset<NUM_FILE_SERVERS> sm){
		stripeWidth = sw; 
		stripeMask = sm; 
	}
	fileRecipe operator=(fileRecipe fr){
		stripeWidth = fr.stripeWidth; 
		stripeMask = fr.stripeMask; 
		return fr; 
 	}
	fileRecipe(){
		stripeWidth = 0;  
	}
	
	string toString(){
		string frs = static_cast<ostringstream*>( &(ostringstream() << stripeWidth ))->str();
		frs += " "; 
		frs += string(stripeMask.to_string()); 
		return frs; 
	}

}; 

struct Interval
{
	Interval(unsigned int start, unsigned int end)
    	: m_start(start),
          m_end(end)
    	{}
        unsigned int m_start;
        unsigned int m_end;
	bool operator<(const Interval& in) const
	{
		if (m_start < in.m_start)
			return true; 
		if (m_start == in.m_start && m_end < in.m_end)
			return true; 
		return false; 
	}
	bool operator==(const Interval& in1) const 
	{
		if (!(m_end < in1.m_start) && !(in1.m_end < m_start))
			return true; 
		return false; 
	}
	bool is_equal(Interval in){
		if (!(m_end < in.m_start) && !(in.m_end < m_start))
			return true; 
		return false; 

	}
	vector<Interval> subtract_interval(Interval in) { 
		vector<Interval> ret_vector;
		if (!(!(m_end < in.m_start) && !(in.m_end < m_start))) {//no overlap, so return the original interval
			Interval interval(m_start,m_end);
			ret_vector.push_back(Interval(m_start,m_end));
		}

		//new	--------------	------------	 ------------
		//old	   ------	-----		 	-----
		else if ((in.m_start <= m_start) && (in.m_end >= m_end)) {
		}
		
		//new     ----	      	 ----			-----
		//old ------------ 	 ----------  	  ------------ 
		else if ((in.m_start >= m_start) && (in.m_end <= m_end)) {
			if (m_start != in.m_start)
				ret_vector.push_back(Interval(m_start,in.m_start-1));
			if (m_end != in.m_end)
				ret_vector.push_back(Interval(in.m_end+1,m_end));
		}

		//new  --------	           --------
		//old      ---------     	  -------
		else if ((in.m_start<m_start) && (in.m_end>=m_start) && (in.m_end<m_end) ) {
			ret_vector.push_back(Interval(in.m_end+1,m_end));
		}
		
		//new       ---------		   ---------
		//old	--------    	     -------
		else if ((m_start<in.m_start) && (m_end>=in.m_start) && (m_end<in.m_end) ) {
			ret_vector.push_back(Interval(m_start,in.m_end-1));
		}
		
		//any other case?
				
		return ret_vector;
	}
};

class mycomparison
{
public: 

	// LESS operator for map 
	bool operator() (const Interval& in1, const Interval& in2) const {
		if (in1.m_end < in2.m_start) 
			return true; 
		return false; 
	}
	
};

struct fileEntry{
	string file_name;
	fileRecipe file_recipe;

	map<Interval,tr1::tuple<string, int> > MDWTokens; //map(key=interval,value=(IP,port) ) for writers
	vector<tr1::tuple<Interval, string, int> > MDRTokens;  //for readers
	
	fileEntry(string fn, fileRecipe fr){
		file_name = fn; 
		file_recipe.stripeWidth = fr.stripeWidth; 
		file_recipe.stripeMask = fr.stripeMask; 
	}

}; 

typedef map<string, fileEntry> FILETABLE;  
FILETABLE general_file_table; 


#endif
