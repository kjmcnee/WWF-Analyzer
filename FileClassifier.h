/*

FileClassifier.h

FileClassifier is a class which determines
if files should be classified a certain way

In this project they are used to identify ignored and critical files

*/

#ifndef _FILECLASSIFIER_H_
#define _FILECLASSIFIER_H_

#include <list>
#include <string>
using std::list;
using std::string;


class FileClassifier{
	
	//having given extensions or containing given substrings makes the file have the classification
	list<string> extensions;
	list<string> substrings;

public:
	void add_extension(string extension);
	void add_substring(string substring);
	bool satisfies(string file_name); //returns true iff the extensions/substrings make the file have the classification
};


#endif
