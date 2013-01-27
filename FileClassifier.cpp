//FileClassifier.cpp
//Implementation of FileClassifier class

#include "FileClassifier.h"

#include <list>
#include <string>
using std::list;
using std::string;


void FileClassifier::add_extension(string extension){
	extensions.push_front(extension);
}
void FileClassifier::add_substring(string substring){
	substrings.push_front(substring);
}

//returns true iff the extensions/substrings make the file have the classification
bool FileClassifier::satisfies(string file_name){
	
	//for each substring
	for(list<string>::iterator i = substrings.begin(); i != substrings.end(); i++){
		//if the substring can be found in the file name, we're done
		if(file_name.find(*i) != string::npos){
			return true;
		}
	}


	//get the file extension
	const string file_extension = file_name.substr(file_name.find_last_of("\\/.") + 1);

	//for each file extension
	for(list<string>::iterator i = extensions.begin(); i != extensions.end(); i++){
		//if the extensions match, we're done
		if(file_extension == *i){
			return true;
		}
	}


	//by this point we know the file does not have the classification
	return false;
}
