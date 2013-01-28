/*

data structs.h

Contains the data structures used in WWF Analyzer

*/

#ifndef _DATA_STRUCTS_H_
#define _DATA_STRUCTS_H_

#include <string>
using std::string;

//stores data about a given owner on a given server
struct WWF_data{
	string owner; //the owner of the WWF
	string server; //name of the server where the WWF is found
	unsigned long count; //the number of files
	unsigned long critical; //the number of files that are critical
};

//overload > operator for use in sorting
//ordered by count, then by owner name, then by server name, then by number of critical files
//.sort(greater<WWF_data>()) to sort descending
bool operator>(const WWF_data& WWF1, const WWF_data& WWF2);



//used to sum all of the files for owners/servers to determine which have the most files
struct occurrences{
	string entity; //name of the owner/server
	unsigned long count; //the number of files for the owner/server
};

//overload > operator for use in sorting
//ordered by count, then by name
//.sort(greater<occurrences>()) to sort descending
bool operator>(const occurrences& occ1, const occurrences& occ2);



//used to store owner/file pairs for critical files
struct critical_file_owner{
	string owner; //name of the owner
	string file; //the critical file
};

//overload < operator for use in sorting
//ordered by owner name, then by file name
bool operator<(const critical_file_owner& cfo1, const critical_file_owner& cfo2);

#endif
