/*

WWF Analyzer.cpp
By: Kevin McNee

This program analyzes world writable files reports and summarizes the results.

*/

#include <windows.h>
#include <cstdlib>
#include <cctype>
#include <limits>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <list>
#include <iterator>
using namespace std;

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
bool operator>(const WWF_data& WWF1, const WWF_data& WWF2){
	if(WWF1.count != WWF2.count){
		return (WWF1.count > WWF2.count);
	}
	else if(WWF1.owner != WWF2.owner){
		return (WWF1.owner > WWF2.owner);
	}
	else if(WWF1.owner != WWF2.owner){
		return (WWF1.server > WWF2.server);
	}
	else{
		return (WWF1.critical > WWF2.critical);
	}
}


//used to sum all of the files for owners/servers to determine which have the most files
struct occurrences{
	string entity; //name of the owner/server
	unsigned long count; //the number of files for the owner/server
};

//overload > operator for use in sorting
//ordered by count, then by name
//.sort(greater<occurrences>()) to sort descending
bool operator>(const occurrences& occ1, const occurrences& occ2){
	if(occ1.count != occ2.count){
		return (occ1.count > occ2.count);
	}
	else{
		return (occ1.entity > occ2.entity);
	}
}

//used to store owner/file pairs for critical files
struct critical_file_owner{
	string owner; //name of the owner
	string file; //the critical file
};

//overload < operator for use in sorting
//ordered by owner name, then by file name
bool operator<(const critical_file_owner& cfo1, const critical_file_owner& cfo2){
	if(cfo1.owner != cfo2.owner){
		return (cfo1.owner < cfo2.owner);
	}
	else{
		return (cfo1.file < cfo2.file);
	}
}



void pause(void);
string trim(const string str);
unsigned long analyze(ifstream& file, string directory, string server_name, list<WWF_data>& lst_WWF);
void summarize(ofstream& report, list<WWF_data>& lst_WWF);
string get_server_name(string file_name);
bool is_ignored(string file_name);
bool is_critical(string file_name);
bool set_prefs(void);

const int COL_WIDTH = 16; //the width of the columns in the reports

//user preferences
//summary report max items to show (default is unlimited)
static int SUMMARY_SERVERS = numeric_limits<int>::max(); //max number of servers to show
static int SUMMARY_OWNERS = numeric_limits<int>::max(); //max number of owners to show
static int HIGH_VOLUME = numeric_limits<int>::max(); //max number of the highest volume owners for each server (and vice versa) to show

static unsigned long MAX_CRITICAL = numeric_limits<unsigned long>::max(); //max number of critical files to show (default is unlimited)

//files to ignore
static list<string> lst_ignored_extensions; //file extensions to ignore
static list<string> lst_ignored_substrings; //substrings within the file name to ignore
//critical files
static list<string> lst_critical_extensions; //file extensions to consider critical
static list<string> lst_critical_substrings; //substrings within the file name to consider critical
//


int main(void){

	cout << "WWF Analyzer" << endl
		<< "This program analyzes the world writable files reports" << endl
		<< "and summarizes the results." << endl << endl;

	//load the user's preferences, if the file does not exist, create it and have the user rerun the program
	if(!set_prefs()) return EXIT_SUCCESS;

	//these are for finding the files to be analyzed
	WIN32_FIND_DATA file_data;
	HANDLE hFind;

	string directory; //the directory where the files are located
	const string file_name_pattern = "*.*"; //the structure of the file name (we will allow any file name)

	//loop until we get a valid directory
	do{
		cout << "Enter the directory containing the files:" << endl;

		do{
		    getline(cin,directory);
		} while(directory.empty());

		//add trailing backslash if the user did not
		if((directory.at(directory.length() - 1) != '\\') && (directory.at(directory.length() - 1) != '/')){
			directory += '\\';
		}

		//get the first file in the given directory
		hFind = FindFirstFile((directory + file_name_pattern).c_str(), &file_data);

		//if no such file can be found
		if(hFind == INVALID_HANDLE_VALUE){
			cerr << "Error: The directory could not be found." << endl << endl;
		}

	} while(hFind == INVALID_HANDLE_VALUE);


	cout << endl << endl << "Beginning analysis." << endl << endl;

	list<WWF_data> lst_WWF; //list of WWF

	//for each file in the directory
    do{
		//ignore directories and hidden files
		if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			|(file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) continue;

		//determine the name of the server for the file that we're opening
		string server_name = get_server_name(file_data.cFileName);

		//if the file is a previously generated report, ignore it
		string cmp1 = server_name;
		cmp1 += " WWF Details Report.txt";
		const string cmp2 = "WWF Summary Report.txt";
		if((cmp1.compare(file_data.cFileName) == 0) || (cmp2.compare(file_data.cFileName) == 0)) continue;

        //open the file
		ifstream file ((directory + file_data.cFileName).c_str());
		if(file.is_open()){

			unsigned long WWF_found; //stores the number of WWFs found from the analysis

			cout << "Analyzing WWFs on " << server_name << "..." << endl;
			WWF_found = analyze(file, directory, server_name, lst_WWF);
			cout << "Done analyzing " << server_name << ". " << WWF_found << " WWFs have been found." << endl << endl;

        }
        else{
            cerr << "Error: " << file_data.cFileName << " could not be opened." << endl;
        }

    } while(FindNextFile(hFind, &file_data));

    //if the reason that we stopped is something other than running out of files, something went wrong
	if(GetLastError() != ERROR_NO_MORE_FILES){
        cerr << "Error: The directory was not searched successfully." << endl;
		pause();
		return EXIT_FAILURE;
	}

	//make summary report
	ofstream report;
	report.open ((directory + "WWF Summary Report.txt").c_str());

	if(report.is_open()){

		summarize(report, lst_WWF);

		report.close();

		cout << endl << "Analysis has finished." << endl << endl
			<< "The WWF Summary Report has been created in the" << endl
			<< "same directory as the analyzed files." << endl << endl;

		pause();

		//open the Summary Report before the termination of this program
		ShellExecute(NULL, "open", (directory + "WWF Summary Report.txt").c_str(), NULL, NULL, 1);

	}
	else{
		cerr << "Error: Unable create report." << endl
			<< "Make sure that you have write access for the directory" << endl
			<< "and that any previously created reports are not in use," << endl
			<< "so that they can be overwritten." << endl << endl;

		pause();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//prompts the user to press enter to continue
void pause(void){
	cout << "Press enter to continue." << endl;
	cin.ignore();
	return;
}

//remove leading and trailing white space from str
//e.g. "       Hello World!  " -> "Hello World!"
string trim(const string str){

    const string whitespaces = " \t\n\r"; //these characters are to be trimmed

    //first non-whitespace
    size_t first = str.find_first_not_of(whitespaces);

    //the string is all whitespace just return an empty string
    if(first == string::npos) return "";

    //last non-whitespace
    size_t last = str.find_last_not_of(whitespaces);

    //return the substring from first to last
    return str.substr(first, (last - first + 1));
}

//determine the server name from the given file name
//e.g. "MyServer-WWFiles-01012011.out" -> "MyServer"
string get_server_name(string file_name){

	string server_name = "";

	//we append the characters from the file name until we get to a non-alphanumeric character
	string::iterator i;
	i = file_name.begin();
	while(i != file_name.end() && isalnum(*i)){
		server_name += *i;
		i++;
	}

	return server_name;
}

//get the user's preferences from the preferences file
//returns false if no preferences file exists, true otherwise
bool set_prefs(void){

	const string pref_file = "WWF Analyzer.pref";

	//open the preferences file and load the contents
	ifstream file (pref_file.c_str());
	if(file.is_open()){

		//while we still have lines to read
		while(file.good()){

			//read the line
			string line;
			getline(file,line);

			//get values
			if((line.compare(0,16,"SUMMARY_SERVERS=") == 0) || (line.compare(0,17,"SUMMARY_SERVERS =") == 0)){
				istringstream ss;
				ss.str(line.substr(line.find_last_of('=') + 1));

				int val;
				ss >> val;

				if(!ss.fail()){
					SUMMARY_SERVERS = val;
				}
			}
			else if((line.compare(0,15,"SUMMARY_OWNERS=") == 0) || (line.compare(0,16,"SUMMARY_OWNERS =") == 0)){
				istringstream ss;
				ss.str(line.substr(line.find_last_of('=') + 1));

				int val;
				ss >> val;

				if(!ss.fail()){
					SUMMARY_OWNERS = val;
				}
			}
			else if((line.compare(0,12,"HIGH_VOLUME=") == 0) || (line.compare(0,13,"HIGH_VOLUME =") == 0)){
				istringstream ss;
				ss.str(line.substr(line.find_last_of('=') + 1));

				int val;
				ss >> val;

				if(!ss.fail()){
					HIGH_VOLUME = val;
				}
			}
			else if((line.compare(0,13,"MAX_CRITICAL=") == 0) || (line.compare(0,14,"MAX_CRITICAL =") == 0)){
				istringstream ss;
				ss.str(line.substr(line.find_last_of('=') + 1));

				unsigned long val;
				ss >> val;

				if(!ss.fail()){
					MAX_CRITICAL = val;
				}
			}
			else if(line.compare(0,2,"i.") == 0){
				string val = trim(line.substr(2));

				if(val != ""){
					lst_ignored_extensions.push_front(val);
				}
			}
			else if(line.compare(0,2,"i:") == 0){
				string val = trim(line.substr(2));

				if(val != ""){
					lst_ignored_substrings.push_front(val);
				}
			}
			else if(line.compare(0,2,"c.") == 0){
				string val = trim(line.substr(2));

				if(val != ""){
					lst_critical_extensions.push_front(val);
				}
			}
			else if(line.compare(0,2,"c:") == 0){
				string val = trim(line.substr(2));

				if(val != ""){
					lst_critical_substrings.push_front(val);
				}
			}
		}
    }

	//if the preferences file does not exist, we create a new one and ask the user to rerun the program
	else{
        cout << "The preferences file (" << pref_file << ") does not exist." << endl
			<< "A default preferences file will be created in the directory" << endl
			<< "where this program is located." << endl << endl
			<< "Take a look at the file, make any changes you want, and then rerun this program.";

		ofstream pref;
		pref.open (pref_file.c_str());

		if(pref.is_open()){
			pref << "# WWF Analyzer Preferences File (" << pref_file << ")" << endl << endl
				<< "# Comments are indicated by a hash character (#) in the first position of a line." << endl
				<< "# Malformed lines are also ignored." << endl << endl
				<< "# Customizing the Summary Report:" << endl
				<< "# To limit the number of results shown, include the following lines:" << endl
				<< "# For the max number of servers to show:" << endl
				<< "#SUMMARY_SERVERS=X" << endl
				<< "# For the max number of owners to show:" << endl
				<< "#SUMMARY_OWNERS=X" << endl
				<< "# For the max number of the highest volume owners for each server (and vice versa) to show:" << endl
				<< "#HIGH_VOLUME=X" << endl
				<< "# Where X is the desired value." << endl
				<< "# If the corresponding value is not set here, the program will default to not limiting the number shown." << endl << endl
				<< "# Ignoring files:" << endl
				<< "# To ignore a certain file extension, type i.[extension] on a new line." << endl
				<< "# e.x. to ignore log files:" << endl
				<< "#i.log" << endl
				<< "# To ignore any file that contains a certain substring, type i:[substring] on a new line." << endl
				<< "# e.x. to ignore all files which contain the substring \"harmless\":" << endl
				<< "#i:harmless" << endl
				<< "# This can be used to ignore directories as well by including the slashes." << endl
				<< "# e.x. to ignore all files in any directory called /tmp/:" << endl
				<< "#i:/tmp/" << endl
				<< "# Note that this will ignore */tmp/*, so you may want to specify" << endl
				<< "# the entire path upto that directory, if there are multiple directories with the same name" << endl
				<< "# but with different parent directories." << endl << endl
				<< "# Critical files:" << endl
				<< "# To have the program flag certain files as critical," << endl
				<< "# use the same method for ignoring extensions and substrings, but with \"c\" instead of \"i\"" << endl
				<< "# e.x. to flag sh files:" << endl
				<< "#c.sh" << endl
				<< "# e.x. to flag all files in the \"home\" directory:" << endl
				<< "#c:/home/" << endl
				<< "# The program will default to not limiting the number of critical files shown in the details reports." << endl
				<< "# To limit the number shown, include:" << endl
				<< "#MAX_CRITICAL=X" << endl
				<< "# Where X is the desired value." << endl;

		}
		else{
			cerr << "Error: Unable to create preferences file." << endl
				<< "Make sure that you have write access for the directory." << endl << endl;
		}

		pause();
		return false;
    }

	return true;
}

//returns true if the file is to be ignored
bool is_ignored(string file_name){

	//for each substring
	for(list<string>::iterator i = lst_ignored_substrings.begin(); i != lst_ignored_substrings.end(); i++){
		//if the substring can be found in the file name, then we ignore the file
		if(file_name.find(*i) != string::npos){
			return true;
		}
	}

	//get the file extension
	string extension = file_name.substr(file_name.find_last_of("\\/.") + 1);

	//for each file extension to be ignored
	for(list<string>::iterator i = lst_ignored_extensions.begin(); i != lst_ignored_extensions.end(); i++){
		//ignore the file if the extensions are the same
		if(extension.compare(*i) == 0){
			return true;
		}
	}

	//by this point we know that the file is not to be ignored
	return false;
}

//returns true if the file is considered critical
bool is_critical(string file_name){

	//for each substring
	for(list<string>::iterator i = lst_critical_substrings.begin(); i != lst_critical_substrings.end(); i++){
		//if the substring can be found in the file name, then it is critical
		if(file_name.find(*i) != string::npos){
			return true;
		}
	}

	//get the file extension
	string extension = file_name.substr(file_name.find_last_of("\\/.") + 1);

	//for each file extension to be considered critical
	for(list<string>::iterator i = lst_critical_extensions.begin(); i != lst_critical_extensions.end(); i++){
		//the file is critical if the extensions are the same
		if(extension.compare(*i) == 0){
			return true;
		}
	}

	//by this point we know that the file is not critical
	return false;
}

//analyze the file for the given server and store the data in the list of WWFs
//returns the number of WWFs found
unsigned long analyze(ifstream& file, string directory, string server_name, list<WWF_data>& lst_WWF){

	unsigned long WWFs = 0;
	unsigned long ignored_files = 0;
	unsigned long critical_files = 0;

	list<critical_file_owner> lst_critical_files;

	//check if the first character in the file is non-ascii
	//if so, then the file likely contains formatted text, which we can't read easily
	if(file.good() && (file.peek() != EOF)){
		if(!__isascii(file.peek())){
			cerr << endl << "Error:" << endl
				<< "The contents of this file are unintelligible." << endl
				<< "This is probably due to formatting being applied to the text." << endl
				<< "Try copying the text into a new text file and redo the analysis." << endl
				<< "If you chose to continue, the analysis of the remaining files will be done," << endl
				<< "but this file will not be analyzed and no WWFs will be recorded." << endl << endl;
			pause();
			return 0;
		}
	}

	//while we still have lines to read
	while(file.good()){

		//read the line
		string line;
		getline(file,line);

		//we extract the permissions, owner, and file name from each line and discard the rest
		istringstream ss;
		ss.str(line);
		string permissions, owner, file_name, discard;
		ss >> permissions >> discard >> owner >> discard >> discard >> discard >> discard >> discard;
		//the file name is just the rest of the line
		getline(ss,file_name);
		file_name = trim(file_name);

		//only continue to process the line if the line contains valid data
		//we expect the permissions symbolic notation to be 10 characters long
		//the first character in permissions must be one of the following:
		//'-' for regular file, 'd' for directory, or 'l' for link, or b, c, p, s
		//else, we have an invalid line
		if(!ss.fail() && (file_name != "") && (permissions.size() == 10) && (permissions.at(0) == '-'
			|| permissions.at(0) == 'd' || permissions.at(0) == 'l'
			|| permissions.at(0) == 'b' || permissions.at(0) == 'c'
			|| permissions.at(0) == 'p' || permissions.at(0) == 's')){
			//if the file is world writable then the second last char will be "w", not "-"
			//also we make sure that the file is not one of the ones to be ignored
			if((permissions.at(8) == 'w') && (!is_ignored(file_name))){

				//stores whether or not we can add an occurrence to an existing owner/server pair
				bool existing_element = false;

				//check if there is an element for this owner on this server
				for (list<WWF_data>::iterator i = lst_WWF.begin(); i != lst_WWF.end(); i++){
					if((i->owner == owner) && (i->server == server_name)){
						//increment the number of occurrences of this owner on this server
						i->count += 1;

						//if the file is critical
						if(is_critical(file_name)){
							//count it
							i->critical += 1;
							critical_files++;

							//include the file in the list of critical files
							critical_file_owner new_critical_file;

							new_critical_file.owner = owner;
							new_critical_file.file = file_name;

							lst_critical_files.push_back(new_critical_file);
						}

						existing_element = true; //we added to an existing pair
						break; //we can stop searching
					}
				}

				//if there were no existing pairs, we add a new one to the list
				if(!existing_element){
					WWF_data new_WWF;

					new_WWF.server = server_name;
					new_WWF.owner = owner;
					new_WWF.count = 1;

					if(is_critical(file_name)){
						new_WWF.critical = 1;
						critical_files++;

						critical_file_owner new_critical_file;

						new_critical_file.owner = owner;
						new_critical_file.file = file_name;

						lst_critical_files.push_back(new_critical_file);
					}
					else{
						new_WWF.critical = 0;
					}


					lst_WWF.push_front(new_WWF);
				}

				WWFs++;
			}
			else{
				ignored_files++;
			}
		}
	}

	lst_WWF.sort(greater<WWF_data>());

	//create server details report
	ofstream report;
	report.open ((directory + server_name + " WWF Details Report.txt").c_str());

	if(report.is_open()){

		report << server_name << " WWF Details Report" << endl << endl;

		report << "Files Found:\t" << (WWFs + ignored_files) << endl
		<< "Files Ignored:\t" << ignored_files << endl
		<< "# of WWFs:\t" << WWFs << endl
		<< "Critical Files:\t" << critical_files << endl << endl << endl;

		if(WWFs > 0){
			report << ' ' << setw(COL_WIDTH) << left << "Owner" << "# of Files (# Critical)" << endl
				<< setfill('-') << setw(COL_WIDTH) << right << "+" << setw(COL_WIDTH + COL_WIDTH/2) << "" << setfill(' ') << endl;

			//for each element for the current server, print the owner and the number of files (and any critical files)
			for (list<WWF_data>::iterator i = lst_WWF.begin(); i != lst_WWF.end(); i++){
				if(i->server == server_name){
					report << ' ' << setw(COL_WIDTH - 2)  << left << i->owner << "| " << i->count;

					if(i->critical > 0){
						report << " (" << i->critical << ")";
					}

					report << endl;
				}
			}

		}
		else{
			report << "No WWFs to report.";
		}


		//display the critical files
		if(critical_files > 0){

			lst_critical_files.sort();

			report << endl << endl << endl << "Critical files:";

			//for each critical file, or until the max number to display is reached...
			list<critical_file_owner>::iterator i = lst_critical_files.begin();
			unsigned long num_file = 0;
			for(; (num_file < MAX_CRITICAL) && (i != lst_critical_files.end()); num_file++, i++){
				report << endl << ' ' << setw(COL_WIDTH - 2)  << left << i->owner << i->file;
			}

			//if we've hit the max number to display but there are still more files, inform the user there are too many critical files
			if((num_file == MAX_CRITICAL) && (i != lst_critical_files.end())){
				report << endl << endl << MAX_CRITICAL
					<< " critical files have been displayed. However, there are more critical files than this." << endl
					<< "They have been omitted as per the value of the \"MAX_CRITICAL\" setting.";
			}
		}

	}
	else{
		cerr << "Error: Unable create report." << endl
			<< "Make sure that you have write access for the directory" << endl
			<< "and that any previously created reports are not in use," << endl
			<< "so that they can be overwritten." << endl << endl;
	}

	return WWFs;
}

void summarize(ofstream& report, list<WWF_data>& lst_WWF){

	lst_WWF.sort(greater<WWF_data>());

	report << "WWF Summary Report" << endl << endl;

	//if there are no WWFs
	if(lst_WWF.size() == 0){
		report << "There are no WWFs to report.";
		return;
	}

	report << " Most Files by Server" << endl
		<< setfill('-') << setw(25) << "" << setfill(' ') << endl << endl
		<< ' ' << setw(COL_WIDTH) << left << "Server"
		<< ' ' << setw(COL_WIDTH) << left << "# of Files"
		<< "Top Owners" << endl
		<< setfill('-') << setw(COL_WIDTH) << right << "+"
		<< setw(COL_WIDTH) << right << "+"
		<< setw(COL_WIDTH + COL_WIDTH/2 + 1) << "" << setfill(' ') << endl;

	list<occurrences> lst_servers; //list of servers by number of WWFs

	//determine order of the servers based on number of files
	for(list<WWF_data>::iterator i = lst_WWF.begin(); i != lst_WWF.end(); i++){

		//for each server/owner pair, we look for the sum of the files for the server and update it with the new files
		bool existing_element = false;
		for (list<occurrences>::iterator j = lst_servers.begin(); j != lst_servers.end(); j++){
			if(i->server == j->entity){
				j->count += i->count;

				existing_element = true;
				break;
			}
		}

		//if the server does not have a sum yet, we add it
		if(!existing_element){
			occurrences new_server;

			new_server.entity = i->server;
			new_server.count = i->count;

			lst_servers.push_front(new_server);
		}
	}

	lst_servers.sort(greater<occurrences>());

	//for each server, or until the max number of servers to display is reached...
	list<occurrences>::iterator i = lst_servers.begin();
	int server = 0;
	for(; (server < SUMMARY_SERVERS) && (i != lst_servers.end()); server++, i++){

		//display the total number of WWFs for the server
		report << ' ' << setw(COL_WIDTH - 2) << left << i->entity
			<< "| " << setw(COL_WIDTH - 2) << left << i->count << "| " << endl;

		//for each owner on the server, or until the max number of top owners to display is reached...
		list<WWF_data>::iterator j = lst_WWF.begin();
		int top_owner = 0;
		for(; (top_owner < HIGH_VOLUME) && (j != lst_WWF.end()); j++){
			if(j->server == i->entity){

				//display the number of WWFs for the owner
				report << setw(COL_WIDTH) << right << '|' << setw(COL_WIDTH + 1) << right << " | "
					<< setw(COL_WIDTH) << left << j->owner
					<< setw(COL_WIDTH/2) << right << j->count << endl;

				top_owner++;
			}
		}
		report << setw(COL_WIDTH) << right << '|' << setw(COL_WIDTH + 1) << right << " | " << endl;
	}

	report << endl << endl << endl;


	report << " Most Files by Owner" << endl
		<< setfill('-') << setw(25) << "" << setfill(' ') << endl << endl
		<< ' ' << setw(COL_WIDTH) << left << "Owner"
		<< ' ' << setw(COL_WIDTH) << left << "# of Files"
		<< "Servers" << endl
		<< setfill('-') << setw(COL_WIDTH) << right << "+"
		<< setw(COL_WIDTH) << right << "+"
		<< setw(COL_WIDTH + COL_WIDTH/2 + 1) << "" << setfill(' ') << endl;

	list<occurrences> lst_owners; //list of owners by number of WWFs

	//determine order of the owners based on number of files
	for(list<WWF_data>::iterator k = lst_WWF.begin(); k != lst_WWF.end(); k++){

		//for each server/owner pair, we look for the sum of the files for the owner and update it with the new files
		bool existing_element = false;
		for (list<occurrences>::iterator j = lst_owners.begin(); j != lst_owners.end(); j++){
			if(k->owner == j->entity){
				j->count += k->count;

				existing_element = true;
				break;
			}
		}

		//if the owner does not have a sum yet, we add it
		if(!existing_element){
			occurrences new_owner;

			new_owner.entity = k->owner;
			new_owner.count = k->count;

			lst_owners.push_front(new_owner);
		}
	}

	lst_owners.sort(greater<occurrences>());

	//for each server, or until the max number of owners to display is reached...
	list<occurrences>::iterator k = lst_owners.begin();
	int owner = 0;
	for(; (owner < SUMMARY_OWNERS) && (k != lst_owners.end()); owner++, k++){

		//display the total number of WWFs for the owner
		report << ' ' << setw(COL_WIDTH - 2) << left << k->entity
			<< "| " << setw(COL_WIDTH - 2) << left << k->count << "| " << endl;

		//for each server which has the owner, or until the max number of top servers to display is reached...
		list<WWF_data>::iterator j = lst_WWF.begin();
		int top_server = 0;
		for(; (top_server < HIGH_VOLUME) && (j != lst_WWF.end()); j++){
			if(j->owner == k->entity){

				//display the number of WWFs for the server
				report << setw(COL_WIDTH) << right << '|' << setw(COL_WIDTH + 1) << right << " | "
					<< setw(COL_WIDTH) << left << j->server
					<< setw(COL_WIDTH/2) << right << j->count << endl;

				top_server++;
			}
		}
		report << setw(COL_WIDTH) << right << '|' << setw(COL_WIDTH + 1) << right << " | " << endl;
	}


	list<occurrences> lst_num_critical; //list of the number of critical files per server

	//count the number of critical files an each server
	for(list<WWF_data>::iterator i = lst_WWF.begin(); i != lst_WWF.end(); i++){

		//if this entry has no critical files, move on
		if(i->critical == 0) continue;

		//for each server/owner pair, we look for the sum of the critical files for the server and update it with the new files
		bool existing_element = false;
		for (list<occurrences>::iterator j = lst_num_critical.begin(); j != lst_num_critical.end(); j++){
			if(i->server == j->entity){
				j->count += i->critical;

				existing_element = true;
				break;
			}
		}

		//if the server does not have a sum yet, we add it
		if(!existing_element){
			occurrences new_server;

			new_server.entity = i->server;
			new_server.count = i->critical;

			lst_num_critical.push_front(new_server);
		}
	}

	lst_num_critical.sort(greater<occurrences>());

	//display the servers with critical files
	if(lst_num_critical.size() > 0){

		report << endl << endl << endl << "Critical files have been found on the following servers:";

		for(list<occurrences>::iterator i = lst_num_critical.begin(); i != lst_num_critical.end(); i++){
			report << endl << i->entity << " (" << i->count << ")";
		}
	}

	return;
}
