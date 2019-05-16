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
#include <map>
#include <set>
#include <regex>
#include <stdexcept>
#include <algorithm>
#include <locale>
#include <functional>
#include <cstdint>
#include <utility>
#include "../metronome32/src/instruction.h"
#include "../metronome32/src/memory.h"
#include "../metronome32/src/vm.h"
#include "assemble.h"
#include "transforms.h"
#include "except.h"

#define EXCEPT_FILE std::string(__FILE__)
#define EXCEPT_LINE std::to_string(__LINE__)
#define EXCEPT_HEAD "<" + EXCEPT_FILE + ":" + EXCEPT_LINE + "> "

namespace maag32 = metroaag32;
namespace metro32 = metronome32;
using metro32::register_value;
using metro32::memory_value;

// Returns whether there are any duplicate labels within a parsed program.
// If there is a duplicate, the original directive is placed in dup1 and the
// duplicate directive is placed in dup2.
static bool has_no_duplicate_labels(
	const maag32::parse_results& results,
	maag32::directive& dup1,
	maag32::directive& dup2) noexcept
{
	typedef maag32::parse_results::const_iterator parse_iter;
	
	if (results.size() < 2) return true;
	
	for (parse_iter i = results.cbegin(); i < results.cend() - 1; i++) {
		const maag32::directive& dir1 = *i;
		
		for (parse_iter j = i + 1; j < results.cend(); j++) {
			if (dir1.label.size() != 0 and dir1.label == j->label) {
				dup1 = dir1;
				dup2 = *j;
				
				return false;
			}
		}
	}
	
	return true;
}

// The set of valid mnemonics of actual hardware instructions.
static const std::set<std::string> valid_realops {
	"add", "addi", "and", "andi", "beq", "bgez", "bgezal", "bgtz", "blez",
	"bltz", "bltzal", "bne", "cf", "exchange", "j", "jal", "jalr", "jr",
	"nor", "neg", "or", "ori", "rl", "rlv", "rr", "rrv", "sll", "sllv",
	"slt", "slti", "sra", "srav", "srl", "srlv", "sub", "xor", "xori", "exch"
};

// The set of valid pseudo instructions of the assembler.
static const std::set<std::string> valid_pseudops {
	// Reserves (but doesn't define) arg1 (default 1) 32-bit words.
	"resw",
	// Reserves enough space to fill the given ASCII string arg1 with one
	// ASCII character per word. Does NOT zero-terminate. Arg2 (efault 1)
	// defines how many copies of the string to make.
	"ress",
	// Reserves enough space to fill the given ASCII string arg1 with one
	// ASCII character per word. Does zero-terminate. Arg2 (default 1)
	// defines how many copies of the string to make.
	"ressz",
	// Defines arg2 (default 1) 32-bit words of value arg1 (default 0).
	"dw",
	// Defines a given ASCII string arg1 with one ASCII character per word.
	// Does NOT zero-terminate. Arg2 (default 1) defines how many copies of
	// the string to make.
	"ds",
	// Defines a given ASCII string arg1 with one ASCII character per word.
	// Does zero-terminate. Arg2 (default 1) defines how many copies of the
	// string to make.
	"dsz",
};

// Returns the size of a pseudo instruction "dw".
// Strong exception guarantee.
static long long pseudop_addrdelta_dw(const maag32::directive& dir)
{
	bool success2 = true;
	long long arg2 = maag32::tonumber(dir.data.second, success2);
	
	if (dir.data.first == "" or dir.data.second == "") {
		return 1;
	} else if (not success2) {
		throw maag32::invalid_argument(
			EXCEPT_HEAD,
			dir.original,
			"Argument two is not a number."
		);
	} else if (arg2 < 0) {
		throw maag32::underflow_except(
			EXCEPT_HEAD,
			dir.original,
			"Argument two must be at least 0."
		);
	} else {
		return arg2;
	}
}

// Returns the size of a pseudo instruction "resw".
// Strong exception guarantee.
static long long pseudop_addrdelta_resw(const maag32::directive& dir)
{
	bool success1 = true;
	long long arg1 = maag32::tonumber(dir.data.first, success1);
	
	if (dir.data.first == "") {
		return 1;
	} else if (not success1) {
		throw maag32::invalid_argument(
			EXCEPT_HEAD,
			dir.original,
			"Argument one is not a number."
		);
	} else if (arg1 < 0) {
		throw maag32::underflow_except(
			EXCEPT_HEAD,
			dir.original,
			"Argument one must be at least 0."
		);
	} else {
		return arg1;
	}
}

static const std::regex_constants::syntax_option_type regexopt =
	std::regex_constants::icase | std::regex_constants::optimize;
static const std::regex regex_str (maag32::patterns::str, regexopt);
static const std::regex regex_reg (maag32::patterns::reg, regexopt);

// Returns the size of a pseudo instruction with suffix "s".
// Strong exception guarantee.
static long long pseudop_addrdelta_s(const maag32::directive& dir)
{
	bool success1 = std::regex_match(dir.data.first, regex_str);
	bool success2 = true;
	std::string arg1 = maag32::unescape_chars(dir.data.first);
	long long arg2 = maag32::tonumber(dir.data.second, success2);
	
	size_t arg1len = 0;
	
	if (success1) {
		// Because of the start/end quotes.
		arg1len = arg1.size() - 2;
	} else {
		throw maag32::invalid_argument(
			EXCEPT_HEAD,
			dir.original,
			"Argument one is not a string."
		);
	}
	
	if (dir.data.second == "") {
		return arg1len;
	} else if (not success2) {
		throw maag32::invalid_argument(
			EXCEPT_HEAD,
			dir.original,
			"Argument two is not a number."
		);
	} else if (arg2 < 0) {
		throw maag32::underflow_except(
			EXCEPT_HEAD,
			dir.original,
			"Argument two must be at least 0."
		);
	} else {
		return arg1len * arg2;
	}
}

// Returns the size of a pseudo instruction with suffix "sz".
// Strong exception guarantee.
static long long pseudop_addrdelta_sz(const maag32::directive& dir)
{
	return pseudop_addrdelta_s(dir) + 1;
}

typedef std::map<std::string, metronome32::register_value> label_addr_map;

// Returns a label_addr_map containing all of the resolved labels of the parsed
// program.
// Strong exception guarantee.
static label_addr_map resolve_labels(maag32::parse_results& results)
{
	label_addr_map resolutions = {};
	register_value current_addr = 0;
	
	{
		maag32::directive dir1;
		maag32::directive dir2;
		if (not has_no_duplicate_labels(results, dir1, dir2)) {
			throw maag32::duplicate_label(
				EXCEPT_HEAD,
				dir1,
				dir2
			);
		}
	}
	
	for (maag32::directive& dir : results) {
		if (dir.label != "")
			resolutions.emplace(dir.label, current_addr);
		
		dir.address = current_addr;
		
		if (dir.instr == "") {
			continue;
		} else if (dir.instr == "resw") {
			current_addr += pseudop_addrdelta_resw(dir);
		} else if (dir.instr == "dw") {
			current_addr += pseudop_addrdelta_dw(dir);
		} else if (dir.instr == "ress" or dir.instr == "ds") {
			current_addr += pseudop_addrdelta_s(dir);
		} else if (dir.instr == "ressz" or dir.instr == "dsz") {
			current_addr += pseudop_addrdelta_sz(dir);
		} else if (valid_realops.count(dir.instr) == 0) {
			throw maag32::unknown_instruction(
				EXCEPT_HEAD,
				dir
			);
		} else current_addr++;
	}
	
	return resolutions;
}

static void lowercase_instr_names(maag32::parse_results& results) noexcept
{
	for (maag32::directive& dir : results) {
		std::transform(
			dir.instr.begin(),
			dir.instr.end(),
			dir.instr.begin(),
			::tolower
		);
	}
}

static const std::string entry_label = "_ENTRY";

static register_value get_entry_point(const label_addr_map& labels) noexcept
{
	if (labels.count(entry_label) == 0) {
		return 0;
	} else {
		return labels.at(entry_label);
	}
}

typedef metronome32::gpregister reg_t;
typedef metronome32::immediate_t imm_t;
typedef metronome32::offset_t offs_t;
typedef metronome32::shrot_t shr_t;
template <class... Args>
using m32func_t = std::function<memory_value(const reg_t&, Args...)>;
typedef m32func_t<const reg_t&> r1typenew;
typedef m32func_t<const shr_t&> r2typenew;
typedef m32func_t<const imm_t&> itypenew;
typedef m32func_t<const offs_t&> b1typenew;
typedef m32func_t<const reg_t&, const offs_t&> b2typenew;

static const std::map<std::string, r1typenew> r1_new_instr {
	{"add", metronome32::new_add},
	{"and", metronome32::new_and},
	{"exchange", metronome32::new_exchange},
	{"exch", metronome32::new_exchange},
	{"jalr", metronome32::new_jalr},
	{"or", metronome32::new_or},
	{"rlv", metronome32::new_rlv},
	{"rrv", metronome32::new_rrv},
	{"sllv", metronome32::new_sllv},
	{"slt", metronome32::new_slt},
	{"srav", metronome32::new_srav},
	{"srlv", metronome32::new_srlv},
	{"sub", metronome32::new_sub},
	{"xor", metronome32::new_xor}
};

static const std::map<std::string, r2typenew> r2_new_instr {
	{"rl", metronome32::new_rl},
	{"rr", metronome32::new_rr},
	{"sll", metronome32::new_sll},
	{"slt", metronome32::new_slt},
	{"sra", metronome32::new_sra},
	{"srl", metronome32::new_srl}
};

static const std::map<std::string, itypenew> i_new_instr {
	{"addi", metronome32::new_addi},
	{"andi", metronome32::new_andi},
	{"ori", metronome32::new_ori},
	{"slti", metronome32::new_slti},
	{"xori", metronome32::new_xori}
};

static const std::map<std::string, b1typenew> b1_new_instr {
	{"bgez", metronome32::new_bgez},
	{"bgtz", metronome32::new_bgtz},
	{"blez", metronome32::new_blez},
	{"bltz", metronome32::new_bltz},
	{"jal", metronome32::new_jal}
};

static const std::map<std::string, b2typenew> b2_new_instr {
	{"beq", metronome32::new_beq},
	{"bgezal", metronome32::new_bgezal},
	{"bltzal", metronome32::new_bltzal},
	{"blne", metronome32::new_blne}
};

static constexpr unsigned long long regmaxval = (1 << 6) - 1;
static constexpr signed long long immmaxval = (1 << 21) - 1;
static constexpr signed long long immminval = -(1 << 21);
static constexpr unsigned long long shrotmaxval = (1 << 6) - 1;
static constexpr unsigned long long tarmaxval = (1 << 27) - 1;
static constexpr signed long long offmaxval = (1 << 16) - 1;
static constexpr signed long long offminval = -(1 << 16);

// If success, returns the register number of a register.
static unsigned long long get_register_num(
	const maag32::directive& dir,
	const std::string& str,
	bool& success)
{
	success = std::regex_match(str, regex_reg);
	
	if (success) {
		unsigned long long num = std::stoull(str.substr(2));
		
		if (num > regmaxval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Register number must be between 0 and " + \
				std::to_string(regmaxval) + "."
			);
		} else return num;
	} else return 0;
}

// Returns the address referred to by the label if success.
static memory_value get_label_addr(
	const maag32::directive& dir,
	const label_addr_map& labels,
	const std::string& label,
	bool& success) noexcept
{
	success = labels.count(label) != 0;
	
	if (success) {
		return labels.at(label);
	} else if (label == "_HERE") {
		success = true;
		
		return dir.address;
	} else return 0;
}

// Returns the shift/rotate number if success.
static unsigned long long get_shrot_num(
	const maag32::directive& dir,
	const std::string& str,
	bool& success)
{
	unsigned long long num = maag32::tonumber(str, success);
	
	if (success) {
		if (num > shrotmaxval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Shift/rotate amount must be between 0 and " + \
				std::to_string(shrotmaxval) + "."
			);
		} else return num;
	} else return 0;
}

// Returns the immediate number if success.
static unsigned long long get_imm_num(
	const maag32::directive& dir,
	const label_addr_map& labels,
	const std::string& str,
	bool& success)
{
	signed long long num = maag32::tonumber(str, success);
	
	if (success) {
		if (num > immmaxval or num < immminval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Immediate must be between " + \
				std::to_string(immminval) + " and " + \
				std::to_string(immmaxval) + "."
			);
		} else return num;
	}
	
	num = get_label_addr(dir, labels, str, success);
	
	if (success) {
		if (num > immmaxval or num < immminval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Immediate must be between " + \
				std::to_string(immminval) + " and " + \
				std::to_string(immmaxval) + "."
			);
		} else return num;
	} else return 0;
}

// Returns the offset number if success.
static unsigned long long get_offset_num(
	const maag32::directive& dir,
	const label_addr_map& labels,
	const std::string& str,
	bool& success)
{
	signed long long num = maag32::tonumber(str, success);
	
	if (success) {
		if (num > offmaxval or num < offminval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Offset must be between " + \
				std::to_string(offminval) + " and " + \
				std::to_string(offmaxval) + "."
			);
		} else return num;
	}
	
	signed long long addr = dir.address;
	num = get_label_addr(dir, labels, str, success) - addr;
	
	if (success) {
		if (num > offmaxval or num < offminval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Offset must be between " + \
				std::to_string(offminval) + " and " + \
				std::to_string(offmaxval) + "."
			);
		} else return num;
	} else return 0;
}

// Returns the target number if success.
static unsigned long long get_tar_num(
	const maag32::directive& dir,
	const label_addr_map& labels,
	const std::string& str,
	bool& success)
{
	unsigned long long num = maag32::tonumber(str, success);
	
	if (success) {
		if (num > tarmaxval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Target must be between 0 and " + \
				std::to_string(tarmaxval) + "."
			);
		} else return num;
	};
	
	num = get_label_addr(dir, labels, str, success) + 1;
	
	if (success) {
		if (num > tarmaxval) {
			throw maag32::invalid_argument(
				EXCEPT_HEAD,
				dir.original,
				"Target must be between 0 and " + \
				std::to_string(tarmaxval) + "."
			);
		} else return num;
	} else return 0;
}

// Return the value to fill memory with when using dw.
static unsigned long long get_dw_num(
	const maag32::directive& dir,
	const label_addr_map& labels,
	const std::string& str,
	bool& success)
{
	unsigned long long num = maag32::tonumber(str, success);
	
	if (success) return num;
	
	num = get_label_addr(dir, labels, str, success) - dir.address;
	
	if (success) return num;
		else return 0;
}

// Throws if success is false.
static void assert_is_reg(
	const maag32::directive& dir,
	const std::string& arg,
	bool success)
{
	if (not success) throw maag32::invalid_argument(
		EXCEPT_HEAD,
		dir.original,
		"Expected '" + arg + "' to be a register."
	);
}

// Throws if success is false.
static void assert_is_shrot(
	const maag32::directive& dir,
	const std::string& arg,
	bool success)
{
	if (not success) throw maag32::invalid_argument(
		EXCEPT_HEAD,
		dir.original,
		"Expected '" + arg + "' to be a shift/rotate amount."
	);
}

// Throws if success is false.
static void assert_is_imm(
	const maag32::directive& dir,
	const std::string& arg,
	bool success)
{
	if (not success) throw maag32::invalid_argument(
		EXCEPT_HEAD,
		dir.original,
		"Expected '" + arg + "' to be an immediate or label."
	);
}

// Throws if success is false.
static void assert_is_offset(
	const maag32::directive& dir,
	const std::string& arg,
	bool success)
{
	if (not success) throw maag32::invalid_argument(
		EXCEPT_HEAD,
		dir.original,
		"Expected '" + arg + "' to be an offset or label."
	);
}

// Throws if success is false.
static void assert_is_tar(
	const maag32::directive& dir,
	const std::string& arg,
	bool success)
{
	if (not success) throw maag32::invalid_argument(
		EXCEPT_HEAD,
		dir.original,
		"Expected '" + arg + "' to be a target or label."
	);
}

// Throws if registers are equal.
static void assert_regs_unequal(
	const maag32::directive& dir,
	unsigned long long reg1,
	unsigned long long reg2)
{
	if (reg1 == reg2) throw maag32::invalid_argument(
		EXCEPT_HEAD,
		dir.original,
		"The two provided registers cannot be equal."
	);
}

// Creates an instruction of r1 type.
static void r1_create_instr(
	const maag32::directive& dir,
	metronome32::context_data& context)
{
	bool success = true;
	unsigned long long reg1 = get_register_num(
		dir,
		dir.data.first,
		success
	);
	assert_is_reg(dir, dir.data.first, success);
	unsigned long long reg2 = get_register_num(
		dir,
		dir.data.second,
		success
	);
	assert_is_reg(dir, dir.data.second, success);
	assert_regs_unequal(dir, reg1, reg2);
	
	context.sys_mem[context.counter] = r1_new_instr.at(dir.instr)(
		reg1,
		reg2
	);
	
	context.counter++;
}

// Creates an instruction of r1 type.
static void r2_create_instr(
	const maag32::directive& dir,
	metronome32::context_data& context)
{
	bool success = true;
	unsigned long long reg = get_register_num(
		dir,
		dir.data.first,
		success
	);
	assert_is_reg(dir, dir.data.first, success);
	unsigned long long shrot = get_shrot_num(
		dir,
		dir.data.second,
		success
	);
	assert_is_shrot(dir, dir.data.second, success);
	
	context.sys_mem[context.counter] = r2_new_instr.at(dir.instr)(
		reg,
		shrot
	);
	
	context.counter++;
}

// Creates an instruction of i type.
static void i_create_instr(
	const maag32::directive& dir,
	const label_addr_map& labels,
	metronome32::context_data& context)
{
	bool success = true;
	unsigned long long reg = get_register_num(
		dir,
		dir.data.first,
		success
	);
	assert_is_reg(dir, dir.data.first, success);
	unsigned long long imm = get_imm_num(
		dir,
		labels,
		dir.data.second,
		success
	);
	assert_is_imm(dir, dir.data.second, success);
	
	context.sys_mem[context.counter] = i_new_instr.at(dir.instr)(
		reg,
		imm
	);
	
	context.counter++;
}

// Creates an instruction of b1 type.
static void b1_create_instr(
	const maag32::directive& dir,
	const label_addr_map& labels,
	metronome32::context_data& context)
{
	bool success = true;
	unsigned long long reg = get_register_num(
		dir,
		dir.data.first,
		success
	);
	assert_is_reg(dir, dir.data.first, success);
	unsigned long long offset = get_offset_num(
		dir,
		labels,
		dir.data.second,
		success
	);
	assert_is_offset(dir, dir.data.second, success);
	
	context.sys_mem[context.counter] = b1_new_instr.at(dir.instr)(
		reg,
		offset
	);
	
	context.counter++;
}

// Assembles pseudo instructions.
static void pseudop_create_instr(
	const maag32::directive& dir,
	const label_addr_map& labels,
	metronome32::context_data& context)
{
	if (dir.instr == "dw") {
		register_value start = context.counter;
		const register_value end = start + pseudop_addrdelta_dw(dir);
		bool success = true;
		memory_value val = get_dw_num(
			dir,
			labels,
			dir.data.first,
			success
		);
		
		if (not success) throw maag32::invalid_argument(
			EXCEPT_HEAD,
			dir.original,
			"Argument one is not a label or number."
		);
		
		for (; start < end; start++) {
			context.sys_mem.insert({start, val});
		}
		
		context.counter = end;
	} else if (dir.instr == "ds") {
		typedef std::string::size_type sindex_t;
		
		register_value start = context.counter;
		std::string str = maag32::unescape_chars(dir.data.first);
		str = str.substr(1, str.size() - 2);
		const register_value times = pseudop_addrdelta_s(dir) / str.size();
		
		for (register_value i = 0; i < times; i++) {
			for (sindex_t si = 0; si < str.size(); si++) {
				context.sys_mem.insert({
					start + si,
					str[si]
				});
			}
			
			start += str.size();
		}
		
		context.counter = start;
	} else if (dir.instr == "dsz") {
		typedef std::string::size_type sindex_t;
		
		register_value start = context.counter;
		std::string str = maag32::unescape_chars(dir.data.first);
		str = str.substr(1, str.size() - 2);
		const register_value times = pseudop_addrdelta_s(dir) / str.size();
		
		for (register_value i = 0; i < times; i++) {
			for (sindex_t si = 0; si < str.size(); si++) {
				context.sys_mem.insert({
					start + si,
					str[si]
				});
			}
			
			start += str.size();
		}
		
		context.sys_mem.insert({start, 0});
		start++;
		context.counter = start;
	} else if (dir.instr == "resw") {
		context.counter += pseudop_addrdelta_resw(dir);
	} else if (dir.instr == "ress") {
		context.counter += pseudop_addrdelta_s(dir);
	} else if (dir.instr == "ressz") {
		context.counter += pseudop_addrdelta_sz(dir);
	}
}

// Creates an instruction from a directive.
static void assemble_instruction(
	const maag32::directive& dir,
	const label_addr_map& labels,
	metronome32::context_data& context)
{
	if (dir.instr.size() == 0) {
		return;
	} else if (r1_new_instr.count(dir.instr) != 0) {
		return r1_create_instr(dir, context);
	} else if (r2_new_instr.count(dir.instr) != 0) {
		return r2_create_instr(dir, context);
	} else if (i_new_instr.count(dir.instr) != 0) {
		return i_create_instr(dir, labels, context);
	} else if (b1_new_instr.count(dir.instr) != 0) {
		return b1_create_instr(dir, labels, context);
	} else if (valid_pseudops.count(dir.instr) != 0) {
		return pseudop_create_instr(dir, labels, context);
	} else if (dir.instr == "cf") {
		context.sys_mem[context.counter] = metronome32::new_cf();
		context.counter++;
	} else if (dir.instr == "j") {
		bool success = true;
		unsigned long long target = get_tar_num(
			dir,
			labels,
			dir.data.first,
			success
		);
		assert_is_tar(dir, dir.data.first, success);
		context.sys_mem[context.counter] = metronome32::new_j(target);
		context.counter++;
	} else throw maag32::unknown_instruction(
		EXCEPT_HEAD,
		dir
	);
}

maag32::vm maag32::assemble(maag32::parse_results pr)
{
	lowercase_instr_names(pr);
	label_addr_map labels = resolve_labels(pr);
	metronome32::context_data context;
	context.counter = 0;
	
	for (const auto& dir : pr) {
		assemble_instruction(dir, labels, context);
	}
	
	context.counter = get_entry_point(labels);
	maag32::vm my_vm;
	my_vm.set_context(std::forward<metronome32::context_data>(context));
	
	return my_vm;
}
