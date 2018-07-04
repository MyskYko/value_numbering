#pragma once
#include <string>
#include <boost/variant/recursive_variant.hpp>

namespace VN {

	struct op_node;

	typedef
		boost::variant<
		double,
		std::string,
		boost::recursive_wrapper<op_node>
		> node;

	struct op_node {
		std::string name;
		char type;
		node left;
		node right;
		int mark;

		op_node(std::string name, char type, node &left, node &right, int mark) :
			name(name), type(type), left(left), right(right), mark(mark) {}
	};
}