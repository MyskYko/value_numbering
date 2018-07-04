#pragma once
#include <vector>
#include "node.h"

namespace VN {
	class parser {
	private:
	public:
		std::vector<VN::node> st;
		int parse(std::string filename);
	};
}