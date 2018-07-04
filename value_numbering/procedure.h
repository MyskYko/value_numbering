#pragma 
#include <vector>
#include "node.h"

namespace VN {
	struct printer : boost::static_visitor<> {
		int mode = 0;
		std::map<std::string, int> * value_number;
		std::map<int, std::string> * representative;
		std::map<std::string, std::string> * print_name;
		int returning = 0;

		void operator()(op_node const & x);
		void operator()(std::string const & name) const;
		void operator()(double const & c) const;

		void print(std::vector<node> & st);
	};

	struct renamer : boost::static_visitor<int>	{
		std::map<std::string, std::vector<std::string>> rename_map;
		int name_count = 0;

		int operator()(op_node & x);
		int operator()(std::string & name);
		int operator()(double const & c);

		int rename(std::vector<node> & st);
	};

	struct value_numberer : boost::static_visitor<int> {
		std::map<std::string, int> value_number;
		std::map<std::string, int> operation_map;
		int number_count = 1;

		int operator()(op_node const & x);
		int operator()(std::string const & name);
		int operator()(double const & c);

		int number(std::vector<node> & st);
	};

	int remove_redundancy(std::vector<node> & nodes, std::map<std::string, std::vector<std::string>> & rename_map, std::map<std::string, int> & value_number);

	struct marker {
		std::map<int, std::string> representative;

		void set_representative(std::map<std::string, int> & value_number);
		int mark_nodes(std::vector<node> & nodes, std::map<std::string, int> & value_number);
	};
}