#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "node.h"
#include "parser.h"

namespace VN_parser {
	namespace fusion = boost::fusion;
	namespace phoenix = boost::phoenix;
	namespace qi = boost::spirit::qi;
	namespace ascii = boost::spirit::ascii;
}

BOOST_FUSION_ADAPT_STRUCT (
	VN::op_node,
	(std::string, name)
	(char, type)
	(VN::node, left)
	(VN::node, right)
	(int, mark)
)

namespace VN_parser {	
	struct printer : boost::static_visitor<> {

		void operator()(VN::op_node const & x) const {
			std::cout << x.type << " ";
			boost::apply_visitor(printer(), x.left);
			if (x.type != 'r' && x.type != 'c') {
				boost::apply_visitor(printer(), x.right);
			}
		}

		void operator()(std::string const & name) const {
			std::cout << name << " ";
		}

		void operator()(double const & c) const	{
			std::cout << c << " ";
		}
	};
	
	template <typename Iterator>
	struct ope_int_grammer : qi::grammar<Iterator, std::vector<VN::node>(), ascii::space_type> {
		ope_int_grammer() : ope_int_grammer::base_type(code) {
			using qi::lit;
			using qi::eps;
			using qi::double_;
			using qi::alnum;
			using qi::as_string;
			using ascii::alpha;
			using ascii::char_;
			using namespace qi::labels;

			using phoenix::push_back;
			using phoenix::construct;

			term = 
				as_string[
					alpha
					>> *(alpha | alnum)
				][_val = _1]
				;
			
			constant = 
				double_[_val = _1]
				>> eps[_val = construct<VN::op_node>("", 'c', _val, _val, 0)]
				;

			op = char_("+*/-")[_val = _1];

			sub_expr =
				(
					lit('(')
					>> sub_expr[_val = _1]
					>> op
					>> sub_expr
					>> lit(')')
					)[_val = construct<VN::op_node>("", _2, _1, _3, 0)]
				|(term)[_val = _1]
				|(constant)[_val = _1]
				;

			expr =
				(
					sub_expr[_val = _1]
					>> op
					>> sub_expr
					)[_val = construct<VN::op_node>("", _2, _1, _3, 0)]
				| (term)[_val = _1]
				| (constant)[_val = _1]
				;

			eq =
				term[_val = _1]
				>> lit('=')
				>> expr[_val = construct<VN::op_node>("", '=', _val, _1, 0)]
				>> lit(';')
				;

			ret = expr[_val = construct<VN::op_node>("", 'r', _1, _val, 0)];

			code = 
				*(eq[push_back(_val, _1)])
				>> lit("return")
				>> ret[push_back(_val, _1)]
				>> *(lit(',') >> ret[push_back(_val, _1)])
				>> lit(';')
				;
		}

		qi::rule<Iterator, std::vector<VN::node>(), ascii::space_type> code;
		qi::rule<Iterator, VN::node(), ascii::space_type > term;
		qi::rule<Iterator, VN::node(), ascii::space_type> eq;
		qi::rule<Iterator, VN::node(), ascii::space_type> expr;
		qi::rule<Iterator, VN::node(), ascii::space_type> constant;
		qi::rule<Iterator, VN::node(), ascii::space_type> sub_expr;
		qi::rule<Iterator, char(), ascii::space_type> op;
		qi::rule<Iterator, VN::node(), ascii::space_type> ret;
	};
}

namespace VN {
	int parser::parse(std::string filename)	{
		std::ifstream in(filename, std::ios_base::in);

		if (!in) {
			std::cerr << "Error: Could not open input file: " << filename << std::endl;
			return 1;
		}

		std::string storage;
		in.unsetf(std::ios::skipws);
		std::copy(
			std::istream_iterator<char>(in),
			std::istream_iterator<char>(),
			std::back_inserter(storage));

		typedef VN_parser::ope_int_grammer<std::string::const_iterator> ope_int_grammar;
		ope_int_grammar grm;

		using boost::spirit::ascii::space;
		std::string::const_iterator iter = storage.begin();
		std::string::const_iterator end = storage.end();
		bool r = phrase_parse(iter, end, grm, space, st);

		if (r && iter == end) {
			//std::cout << "-------------------------\n";
			//std::cout << "Parsing succeeded\n";
			//std::cout << "-------------------------\n";
			//for (VN::node s : st) {
			//	boost::apply_visitor(VN_parser::printer(), s);
			//	std::cout << std::endl;
			//}
			return 0;
		}
		else {
			std::cout << "-------------------------\n";
			std::cout << "Parsing failed\n";
			std::cout << "-------------------------\n";
			return 1;
		}
	}
}
