#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <map>
#include <algorithm>

#include "node.h"
#include "parser.h"
#include "procedure.h"

int main(int argc, char ** argv) {
	std::string filename;
	if (argc > 1) {
		filename = argv[1];
	}
	else {
		std::cerr << "no file name\n";
		return 1;
	}
	VN::parser p;
	int r = p.parse(filename);
	if (r) {
		std::cerr << "parse failed\n";
		return 1;
	}
	
	VN::renamer renamer_;
	r = renamer_.rename(p.st);
	if (r) {
		std::cerr << "rename failed\n";
		return 1;
	}

	VN::printer printer_;
	//printer_.print(p.st);

	VN::value_numberer numberer_;
	r = numberer_.number(p.st);
	if (r < 0) {
		std::cerr << "value number error\n";
		return 1;
	}

	r = VN::remove_redundancy(p.st, renamer_.rename_map, numberer_.value_number);
	if (r) {
		std::cerr << "remove redundancy error\n";
		return 1;
	}
	//printer_.print(p.st);

	VN::marker marker_;
	marker_.set_representative(numberer_.value_number);
	r = marker_.mark_nodes(p.st, numberer_.value_number);
	if (r) {
		std::cerr << "mark node error\n";
		return 1;
	}
	printer_.value_number = &numberer_.value_number;
	printer_.representative = &marker_.representative;
	printer_.mode = 1;
	printer_.print(p.st);

	return 0;
}
