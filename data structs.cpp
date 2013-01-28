//data structs.cpp
//Implementation of relational operators

#include "data structs.h"

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

bool operator>(const occurrences& occ1, const occurrences& occ2){
	if(occ1.count != occ2.count){
		return (occ1.count > occ2.count);
	}
	else{
		return (occ1.entity > occ2.entity);
	}
}

bool operator<(const critical_file_owner& cfo1, const critical_file_owner& cfo2){
	if(cfo1.owner != cfo2.owner){
		return (cfo1.owner < cfo2.owner);
	}
	else{
		return (cfo1.file < cfo2.file);
	}
}
