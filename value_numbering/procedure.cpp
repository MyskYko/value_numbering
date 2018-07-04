#include <boost/variant/recursive_variant.hpp>

#include <iostream>
#include <map>

#include "node.h"
#include "procedure.h"

namespace VN {
	struct get_name : boost::static_visitor<std::string> {
		std::string operator()(op_node const & x) const {
			return x.name;
		}

		std::string operator()(std::string const & name) const {
			return name;
		}

		std::string operator()(double const & c) const {
			return std::to_string(c);
		}
	};
}

namespace VN {
	struct set_name : boost::static_visitor<int> {
		std::string str;
		set_name(std::string str) : str(str) {}

		int operator()(op_node & x) const {
			x.name = str;
			return 0;
		}

		int operator()(std::string & name) const {
			name = str;
			return 0;
		}

		int operator()(double & c) const {
			std::cerr << "change const name\n";
			return 1;
		}
	};
}

namespace VN {
	struct get_op_node : boost::static_visitor<op_node *> {

		op_node * operator()(op_node & x) const {
			return &x;
		}

		op_node * operator()(std::string const & name) const {
			//std::cerr << "not op_node\n";
			return NULL;
		}

		op_node * operator()(double const & c) const {
			//std::cerr << "not op_node\n";
			return NULL;
		}
	};
}

namespace VN {
	int renamer::operator()(op_node & x) {
		if(x.type == '=') {
			std::string left_name = boost::apply_visitor(get_name(), x.left);
			std::string right_name = boost::apply_visitor(get_name(), x.right);
			if (right_name.empty()) {
				int r = boost::apply_visitor(set_name(left_name), x.right);
				if (r) {
					return 1;
				}
				x.type = 'n';
				boost::apply_visitor(*this, x.right);
			}
			else {
				boost::apply_visitor(*this, x.right);
				std::string new_name = std::to_string(name_count);
				boost::apply_visitor(set_name(new_name), x.left);
				rename_map[left_name].push_back(new_name);
				name_count++;
			}
		}
		else if (x.type == '+' || x.type == '-' || x.type == '*' || x.type == '/') {
			boost::apply_visitor(*this, x.left);
			boost::apply_visitor(*this, x.right);
			std::string tmp_name = x.name;
			x.name = std::to_string(name_count);
			if (!tmp_name.empty()) {
				rename_map[tmp_name].push_back(x.name);
			}
			name_count++;
		}
		else if (x.type == 'r') {
			boost::apply_visitor(*this, x.left);
		}
		else if (x.type == 'c') {
			std::string tmp_name = x.name;
			x.name = std::to_string(name_count);
			if (!tmp_name.empty()) {
				rename_map[tmp_name].push_back(x.name);
			}
			name_count++;
		}
		else {
			std::cerr << "type is " << x.type << std::endl;
			return 1;
		}
		return 0;
	}

	int renamer::operator()(std::string & name) {
		if (rename_map[name].empty()) {
			std::string tmp_name = name;
			name = std::to_string(name_count);
			rename_map[tmp_name].push_back(name);
			name_count++;
		}
		else {
			name = rename_map[name].back();
		}
		return 0;
	}

	int renamer::operator()(double const & c) {
		std::cerr << "rename const\n";
		return 1;
		
	}

	int renamer::rename(std::vector<node> & st) {
		for (node & s : st) {
			int r = boost::apply_visitor(*this, s);
			if (r) {
				std::cerr << "rename error\n";
				return 1;
			}
		}
		return 0;
	}
}

namespace VN {
	int value_numberer::operator()(op_node const & x) {
		if (x.type == '=') {
			int right_VN = boost::apply_visitor(*this, x.right);
			if (right_VN < 0) {
				return -1;
			}
			std::string left_name = boost::apply_visitor(get_name(), x.left);
			value_number[left_name] = right_VN;
		}
		else if (x.type == 'n') {
			int r = boost::apply_visitor(*this, x.right);
			return r;
		}
		else if (x.type == '+' || x.type == '*') {
			int left_VN = boost::apply_visitor(*this, x.left);
			int right_VN = boost::apply_visitor(*this, x.right);
			if (left_VN < 0 || right_VN < 0) {
				return -1;
			}
			std::string left_str = std::to_string(left_VN);
			std::string right_str = std::to_string(right_VN);
			std::string operation;
			if (left_VN < right_VN) {
				operation = x.type + "_" + left_str + "_" + right_str;
			}
			else {
				operation = x.type + "_" + right_str + "_" + left_str;
			}
			if (operation_map[operation] == 0) {
				value_number[x.name] = number_count;
				operation_map[operation] = number_count;
				number_count++;
			}
			else {
				value_number[x.name] = operation_map[operation];
			}
		}
		else if (x.type == '-' || x.type == '/') {
			int left_VN = boost::apply_visitor(*this, x.left);
			int right_VN = boost::apply_visitor(*this, x.right);
			if (left_VN < 0 || right_VN < 0) {
				return -1;
			}
			std::string left_str = std::to_string(left_VN);
			std::string right_str = std::to_string(right_VN);
			std::string operation = x.type + "_" + left_str + "_" + right_str;
			if (operation_map[operation] == 0) {
				value_number[x.name] = number_count;
				operation_map[operation] = number_count;
				number_count++;
			}
			else {
				value_number[x.name] = operation_map[operation];
			}
		}
		else if (x.type == 'r') {
			int r = boost::apply_visitor(*this, x.left);
			return r;
		}
		else if (x.type == 'c') {
			std::string left_name = boost::apply_visitor(get_name(), x.left);
			std::string operation = "c_" + left_name;
			if (operation_map[operation] == 0) {
				value_number[x.name] = number_count;
				operation_map[operation] = number_count;
				number_count++;
			}
			else {
				value_number[x.name] = operation_map[operation];
			}
		}
		else {
			std::cerr << "undefined type node\n";
			return -1;
		}
		return value_number[x.name];
	}

	int value_numberer::operator()(std::string const & name) {
		if (value_number[name] == 0) {
			value_number[name] = number_count;
			number_count++;
		}
		return value_number[name];
	}

	int value_numberer::operator()(double const & c) {
		std::cerr << "numbering constant\n";
		return -1;
	}

	int value_numberer::number(std::vector<node> & st) {
		for (node & s : st) {
			int r = boost::apply_visitor(*this, s);
			if (r < 0) {
				std::cerr << "number error\n";
				return -1;
			}
		}
		//for (auto a : value_number) {
		//	std::cout << a.first << ":" << a.second << std::endl;
		//}

		return 0;
	}
}

namespace VN {
	struct change_name : boost::static_visitor<> {
		std::string & before;
		std::string & after;
		change_name(std::string & before, std::string & after) : before(before), after(after)
		{}

		void operator()(op_node & x) const {
			if (x.type == '=') {
				boost::apply_visitor(*this, x.right);
			}
			else if (x.type == 'n') {
				boost::apply_visitor(*this, x.right);
			}
			else if (x.type == '+' || x.type == '-' || x.type == '*' || x.type == '/') {
				boost::apply_visitor(*this, x.left);
				boost::apply_visitor(*this, x.right);
			}
			else if (x.type == 'r') {
				boost::apply_visitor(*this, x.left);
			}
			else if (x.type == 'c') {
			}
			else {
				std::cerr << "undefined type node\n";
			}
		}

		void operator()(std::string & name) const {
			if (before == name) {
				name = after;
			}
		}

		void operator()(double & c) const {}
	};
}

namespace VN {
	int remove_redundancy(std::vector<node> & nodes, std::map<std::string, std::vector<std::string>> & rename_map, std::map<std::string, int> & value_number) {
		for (auto & pair : rename_map) {
			std::vector<std::string> & names = pair.second;
			for (size_t i = 0; i < names.size() - 1; i++) {
				for (size_t j = i + 1; j < names.size(); j++) {
					if (value_number[names[i]] == value_number[names[j]]) {
						for (auto itr = nodes.begin(); itr != nodes.end(); ++itr) {
							op_node * x = boost::apply_visitor(get_op_node(), *itr);
							if (x == NULL) {
								std::cerr << "error remove redundancy\n";
								return 1;
							}
							if (boost::apply_visitor(get_name(), x->left) == names[j]) {
								nodes.erase(itr);
								break;
							}
						}
						for (node & x : nodes) {
							boost::apply_visitor(change_name(names[j], names[i]), x);
						}
						names.erase(names.begin() + j);
						j--;
					}
				}
			}
		}
		return 0;
	}
}

namespace VN {
	struct find_op_node : boost::static_visitor<op_node *> {
		std::string name;
		find_op_node (std::string name) : name(name)
		{}

		op_node * operator()(op_node & x) const {
			if (x.name == name) {
				return &x;
			}
			if (x.type == '=') {
				if (boost::apply_visitor(get_name(), x.left) == name) {
					return &x;
				}
			}
			op_node * l = boost::apply_visitor(*this, x.left);
			if (l != NULL) {
				return l;
			}
			op_node * r = boost::apply_visitor(*this, x.right);
			if (r != NULL) {
				return r;
			}
			return NULL;
		}

		op_node * operator()(std::string const & name) const {
			return NULL;
		}

		op_node * operator()(double const & c) const {
			return NULL;
		}
	};
}

namespace VN {
	void marker::set_representative(std::map<std::string, int > & value_number) {
		std::map<int, std::vector<int>> members;

		for (auto & pair : value_number) {
			if (pair.second != 0) {
				members[pair.second].push_back(std::stoi(pair.first));
			}
		}

		for (auto & pair : members) {
			std::sort(pair.second.begin(), pair.second.end());
			std::string representative_name = std::to_string(*pair.second.begin());
			representative[pair.first] = representative_name;
		}
		
		//for (auto pair : representative) {
		//	std::cout << pair.first << ":" << pair.second << std::endl;
		//}
	}

	int marker::mark_nodes(std::vector<node> & nodes, std::map<std::string, int> & value_number) {
		std::vector<std::string> targets;
		std::vector<std::string> marked;
		
		for (node & x : nodes) {
			op_node * y = boost::apply_visitor(get_op_node(), x);
			if (y->type == 'r') {
				y->mark = 1;
				std::string left_name = boost::apply_visitor(get_name(), y->left);
				targets.push_back(left_name);
			}
		}

		std::sort(targets.begin(), targets.end());
		targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

		while (!targets.empty()) {
			std::string target = targets.back();
			targets.pop_back();
			marked.push_back(target);

			op_node * y = NULL;
			for (node & x : nodes) {
				y = boost::apply_visitor(find_op_node(target), x);
				if (y != NULL) {
					break;
				}
			}
			if (y == NULL) {
				//std::cerr << "cannot find necessary node\n";
				//return 1;
				continue;
			}
			y->mark = 1;

			std::string new_name = representative[value_number[y->name]];
			if (y->name != new_name) {
				auto i1 = find(targets.begin(), targets.end(), new_name);
				if (i1 == targets.end()) {
					auto i2 = find(marked.begin(), marked.end(), new_name);
					if (i2 == marked.end()) {
						targets.push_back(new_name);
					}
				}
				continue;
			}

			if (y->type != '=' && y->type != 'c') {
				std::string left_name = boost::apply_visitor(get_name(), y->left);
				int left_VN = value_number[left_name];
				std::string next = representative[left_VN];
				auto i1 = find(targets.begin(), targets.end(), next);
				if (i1 == targets.end()) {
					auto i2 = find(marked.begin(), marked.end(), next);
					if (i2 == marked.end()) {
						targets.push_back(next);
					}
				}
			}

			if (y->type != 'c') {
				std::string right_name = boost::apply_visitor(get_name(), y->right);
				int right_VN = value_number[right_name];
				std::string next = representative[right_VN];
				auto i1 = find(targets.begin(), targets.end(), next);
				if (i1 == targets.end()) {
					auto i2 = find(marked.begin(), marked.end(), next);
					if (i2 == marked.end()) {
						targets.push_back(next);
					}
				}
			}
		}
		return 0;
	}
}

namespace VN {
	struct preprinter : boost::static_visitor<> {
		std::map<std::string, std::string> * print_name;
		std::map<std::string, int> * value_number;
		std::map<int, std::string> * representative;
		int mode = 0;
		int name_count = 0;

		void operator()(op_node const & x) {
			if (x.type == '=') {
				boost::apply_visitor(*this, x.right);
				std::string left_name = boost::apply_visitor(get_name(), x.left);
				if (mode == 0) {
					(*print_name)[left_name] = "r";
				}
				if (mode == 1 && x.mark == 1) {
					(*print_name)[left_name] = "r" + std::to_string(name_count);
					name_count++;
				}
			}
			else if (x.type == 'n') {
				boost::apply_visitor(*this, x.right);
			}
			else if (x.type == '+' || x.type == '-' || x.type == '*' || x.type == '/') {
				boost::apply_visitor(*this, x.left);
				boost::apply_visitor(*this, x.right);
				if (mode == 0) {
					(*print_name)[x.name] = "r";
				}
				if (mode == 1 && x.mark == 1) {
					(*print_name)[x.name] = "r" + std::to_string(name_count);
					name_count++;
				}
			}
			else if (x.type == 'r') {
				boost::apply_visitor(*this, x.left);
			}
			else if (x.type == 'c') {
				if (mode == 0) {
					(*print_name)[x.name] = "r";
				}
				if (mode == 1 && x.mark == 1) {
					(*print_name)[x.name] = "r" + std::to_string(name_count);
					name_count++;
				}
			}
			else {
				std::cerr << "undefined type node\n";
			}
		}

		void operator()(std::string const & name) {
			if (mode == 0 && (*print_name)[name].empty()) {
				(*print_name)[name] = "i" + std::to_string(name_count);
				name_count++;
			}
		}

		void operator()(double const & c) const {
			std::cerr << "undefined print of constant\n";
		}

		void preprint(std::vector<node> & st) {
			for (node & s : st) {
				boost::apply_visitor(*this, s);
			}
			mode = 1;
			for (node & s : st) {
				boost::apply_visitor(*this, s);
			}
		}
	};
}



namespace VN {
	void printer::operator()(op_node const & x) {
		if (x.type == '=') {
			boost::apply_visitor(*this, x.right);
			if (mode == 0) {
				std::string left_name = boost::apply_visitor(get_name(), x.left);
				std::string right_name = boost::apply_visitor(get_name(), x.right);
				std::cout << left_name << " = " << right_name << ";" << std::endl;
			}
			else if (x.mark == 1) {
				std::string left_name = boost::apply_visitor(get_name(), x.left);
				std::string right_name = boost::apply_visitor(get_name(), x.right);
				std::string new_right_name = (*representative)[(*value_number)[right_name]];
				std::cout << (*print_name)[left_name] << " = " << (*print_name)[new_right_name] << ";" << std::endl;
			}
		}
		else if (x.type == 'n') {
			boost::apply_visitor(*this, x.right);
		}
		else if (x.type == '+' || x.type == '-' || x.type == '*' || x.type == '/') {
			boost::apply_visitor(*this, x.left);
			boost::apply_visitor(*this, x.right);
			if (mode == 0) {
				std::string left_name = boost::apply_visitor(get_name(), x.left);
				std::string right_name = boost::apply_visitor(get_name(), x.right);
				std::cout << x.name << " = " << left_name << x.type << right_name << ";" << std::endl;
			}
			else if (x.mark == 1) {
				std::string new_name = (*representative)[(*value_number)[x.name]];
				if (x.name != new_name) {
					std::cout << (*print_name)[x.name] << " = " << (*print_name)[new_name] << ";" << std::endl;
				}
				else {
					std::string left_name = boost::apply_visitor(get_name(), x.left);
					std::string right_name = boost::apply_visitor(get_name(), x.right);
					std::string new_left_name = (*representative)[(*value_number)[left_name]];
					std::string new_right_name = (*representative)[(*value_number)[right_name]];
					std::cout << (*print_name)[x.name] << " = " << (*print_name)[new_left_name] << x.type << (*print_name)[new_right_name] << ";" << std::endl;
				}
			}
		}
		else if (x.type == 'r') {
			boost::apply_visitor(*this, x.left);
			std::string left_name = boost::apply_visitor(get_name(), x.left);
			if (returning == 0) {
				if (mode == 0) {
					std::cout << "return " << left_name;
				}
				else {
					std::cout << "return " << (*print_name)[left_name];
				}
				returning = 1;
			}
			else {
				if (mode == 0) {
					std::cout << ", " << left_name;
				}
				else {
					std::cout << ", " << (*print_name)[left_name];
				}
			}
		}
		else if (x.type == 'c') {
			if (mode == 0) {
				std::string left_name = boost::apply_visitor(get_name(), x.left);
				std::cout << x.name << " = " << left_name << ";" << std::endl;
			}
			else if (x.mark == 1) {
				std::string new_name = (*representative)[(*value_number)[x.name]];
				if (x.name != new_name) {
					std::cout << (*print_name)[x.name] << " = " << (*print_name)[new_name] << ";" << std::endl;
				}
				else {
					std::string left_name = boost::apply_visitor(get_name(), x.left);
					std::cout << (*print_name)[x.name] << " = " << left_name << ";" << std::endl;
				}
			}
		}
		else {
			std::cerr << "undefined type node\n";
		}
	}

	void printer::operator()(std::string const & name) const {}

	void printer::operator()(double const & c) const {
		std::cerr << "undefined print of constant\n";
	}

	void printer::print(std::vector<node> & st) {
		if (mode == 1) {
			preprinter preprinter_;
			preprinter_.value_number = value_number;
			preprinter_.representative = representative;
			print_name = new std::map<std::string, std::string>;
			preprinter_.print_name = print_name;
			preprinter_.preprint(st);
		}
		returning = 0;
		for (node & s : st) {
			boost::apply_visitor(*this, s);
		}
		std::cout << ";" << std::endl;
	}
}


