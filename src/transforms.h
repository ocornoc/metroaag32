/*
Copyright (c) 2019 Grayson Burton ( https://github.com/ocornoc/ )

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef METROAAG32_HEADER_TRANSFORMS
#define METROAAG32_HEADER_TRANSFORMS
#include <string>
#include <vector>
#include <utility>
#include <metronome32/instruction.h>

namespace metroaag32 {
	namespace patterns {
		const std::string hws = "(?:[\\t ])*";
		const std::string rhws = "(?:[\\t ])+";
		const std::string decnum = "(?:[+-]?\\d+)";
		const std::string hexnum = "(?:[+-]?0x[0-9a-f]+)";
		const std::string octnum = "(?:[+-]?0[0-7]*)";
		const std::string anynum = \
			"(?:" + hexnum + "|" + octnum + "|" + decnum + ")";
		const std::string name = \
			"(?:[_a-z][_a-z0-9]*(?:\\.[_a-z][_a-z0-9]*)*)";
		const std::string reg = "(?:%r\\d{1,2})";
		const std::string str1 = "(?:\"(?:\\\"|\\\\|.)*?\")";
		const std::string str2 = "(?:'(?:\\'|\\\\|.)*?')";
		const std::string str = "(?:" + str1 + "|" + str2 + ")";
		const std::string datum = \
			"(?:" + hws + "(" + anynum + "|" + name + "|" + str + "|" + reg + ")" + hws + ")";
		const std::string data = \
			"(?:(?:" + datum + hws + "," + hws + ")?" + datum + ")";
		const std::string label = \
			"(?:(" + name + ")" + hws + ":" + hws + ")";
		const std::string instr = \
			"(?:(" + name + ")(?:" + rhws + "(" + data + "))?)";
		const std::string comment = "(?:" + hws + ";[^\\n]*)";
		const std::string directive = \
			"(?:" + hws + label + "?" + instr + "?" + comment + \
			"?\\n+)";
	}
	
	typedef std::pair<std::string, std::string> directive_data;

	struct directive {
		std::string original = "";
		std::string label = "";
		std::string instr = "";
		directive_data data = {};
		metronome32::register_value address = 0;
	};
	
	typedef std::vector<directive> parse_results;
	
	// Unescapes all backslash escapes in a string.
	std::string unescape_chars(const std::string& str);
	// Returns whether a string consists of only directives.
	// Add a '\n' to the end of the string if one isn't present.
	bool consists_of_directives(const std::string& str);
	// Returns an iterator to the first non-directive character.
	// If the return value is str.cend(), it's all directives.
	// Add a '\n' to the end of the string if one isn't present.
	std::string::const_iterator \
		find_first_nondirective(const std::string& str);
	// Returns all directives of a source string.
	// If the entire string isn't directives, {} is returned.
	parse_results parse_source(const std::string& str);
	// Converts a number string to a LL. Only well-defined if success is
	// true.
	long long tonumber(const std::string& str, bool& success) noexcept;
}

#endif
