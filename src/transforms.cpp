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

#include <string>
#include <regex>
#include <map>
#include <vector>
#include <utility>
#include <stdexcept>
#include "transforms.h"
namespace maag32 = metroaag32;
namespace maag32pat = maag32::patterns;
using std::regex;
namespace regexc = std::regex_constants;

typedef regexc::syntax_option_type regexopt;
typedef std::map<std::string, std::string> escape_map;

static const escape_map unescape = {
	{"\\a", "\a"}, {"\\b", "\b"}, {"\\?", "\?"},
	{"\\f", "\f"}, {"\\n", "\n"}, {"\\r", "\r"},
	{"\\t", "\t"}, {"\\v", "\v"}, {"\\\\", "\\"},
};

static const regexopt regex_opts = regexc::icase | regexc::optimize;
static const regexopt regex_consists_opts = regex_opts | regexc::nosubs;
static const regex directive_consists \
	(maag32pat::directive + "*", regex_consists_opts);
static const regex directive_pat (maag32pat::directive, regex_opts);
static const regex data_pat (maag32pat::datum, regex_opts);

std::string maag32::unescape_chars(const std::string& str)
{
	typedef std::string::const_iterator strit;
	std::string newstr = "";
	std::string strat = "";
	
	for (strit i = str.begin(); i < str.cend(); ++i) {
		if (i + 1 == str.cend()) {
			newstr += *i;
		} else {
			strat = "";
			strat += *i;
			strat += *(i + 1);
			
			if (unescape.count(strat) != 0) {
				newstr += unescape.at(strat);
				++i;
			} else {
				newstr += *i;
			}
		}
	}
	
	return newstr;
}

bool maag32::consists_of_directives(const std::string& str)
{
	return std::regex_match(str, directive_consists);
}

std::string::const_iterator maag32::find_first_nondirective(const std::string& str)
{
	std::smatch results;
	bool any_matches = std::regex_search(str, results, directive_pat);
	
	if (not any_matches) return str.cbegin();
	if (results.prefix().matched) return results.prefix().first;
	std::string leftovers = results.suffix();
	std::string::const_iterator scit = find_first_nondirective(leftovers);
	if (scit != leftovers.cend()) return scit;
	
	return str.cend();
}

static maag32::directive_data parse_directive(const std::string& str)
{
	maag32::directive_data data ("", "");
	
	if (str == "") return data;
	
	std::smatch results;
	bool matched = std::regex_search(str, results, data_pat);
	std::string leftovers = results.suffix();
	
	if (matched and results[0] != "") {
		data.first = results[1];
		matched = std::regex_search(leftovers, results, data_pat);
		
		if (matched and results[0] != "") {
			data.second = results[1];
		}
	}
	
	return data;
}

static bool is_empty_directive(const maag32::directive& dir) noexcept
{
	return dir.label == "" and dir.instr == "";
}

maag32::parse_results maag32::parse_source(const std::string& str)
{
	if (not consists_of_directives(str)) return {};
	
	std::smatch results;
	bool matched = std::regex_search(str, results, directive_pat);
	parse_results parsed {};
	maag32::directive dir;
	std::string leftovers = results.suffix();
	
	while (matched and results[0] != "") {
		dir.original = results[0];
		dir.label = results[1];
		dir.instr = results[2];
		dir.data = parse_directive(results[3]);
		
		if (not is_empty_directive(dir))
			parsed.push_back(dir);
		
		leftovers = results.suffix();
		matched = std::regex_search(leftovers, results, directive_pat);
	}
	
	return parsed;
}

// Has a strong exception guarantee.
long long maag32::tonumber(
	const std::string& str,
	bool& success) noexcept
{
	try {
		success = true;
		
		return std::stoll(str, nullptr, 0);
	} catch (...) {
		success = false;
		
		return 0;
	}
}
