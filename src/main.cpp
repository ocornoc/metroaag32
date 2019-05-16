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

#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include "../metronome32/src/vm.h"
#include "transforms.h"
#include "assemble.h"
namespace maag32 = metroaag32;

namespace warnmsg {
	static const std::string multiarg =
		"Warning: only the first argument is used.";
}

namespace errmsg {
	static const std::string realpathfail =
		"Failed to get the realpath of a file.";
	static const std::string filenonexist =
		"File failed to open.";
	static const std::string expectingarg =
		"No argument provided.";
	static const std::string notsource =
		"Provided file doesn't contain valid source code.";
}

std::string get_realpath(const std::string& path, bool& success)
{
	char resolved[PATH_MAX];
	const char* const good = realpath(path.c_str(), resolved);
	
	if (good == NULL) {
		success = false;
		
		return "";
	} else {
		success = true;
		
		return static_cast<std::string>(resolved);
	}
}

std::string get_file_contents(const std::string& rp, bool& success)
{
	std::string contents;
	std::ifstream fst(rp, std::ios::binary);
	
	success = fst.is_open();
	
	while (fst.good() and fst.is_open()) {
		contents += fst.get();
	}
	
	contents.erase(contents.cend() - 1, contents.cend());

	return contents;
}

void error(const std::string& str)
{
	std::cout << str << std::endl;
	std::exit(EXIT_FAILURE);
}

void print_registers(const metronome32::vm& vm)
{
	std::cout << "Register values:" << std::endl;
	const metronome32::context_data& cont = vm.get_context();
	const auto& regs = cont.registers;
	
	for (unsigned int i = 0; i < regs.size(); i++) {
		std::cout << std::dec << "Register [" << i << "]:\t";
		std::cout << regs[i] << " (0x" << std::hex;
		std::cout << regs[i] <<  ")" << std::endl;
	}
}

void print_counter(const metronome32::vm& vm)
{
	const auto& counter = vm.get_context().counter;
	std::cout << "Counter: " << std::dec << counter;
	std::cout << std::hex << " (0x" << counter << ")" << std::endl;
}

metronome32::vm load_file_and_assemble(const int argc, const char** argv)
{
	if (argc > 2) {
		std::cout << warnmsg::multiarg << std::endl;
	} else if (argc < 2) error(errmsg::expectingarg);
	
	std::string file_path = argv[1];
	bool success = true;
	std::string real_path = get_realpath(file_path, success);
	if (not success) error(errmsg::realpathfail);
	
	std::string file_data = get_file_contents(real_path, success);
	if (not success) error(errmsg::filenonexist);
	
	std::cout << "Data:" << std::endl << file_data << std::endl;
	
	bool is_valid_source = maag32::consists_of_directives(file_data);
	if (not is_valid_source) error(errmsg::notsource);
	
	maag32::parse_results results = maag32::parse_source(file_data);
	
	return maag32::assemble(results);
}

int main(const int argc, const char** argv)
{
	auto vm = load_file_and_assemble(argc, argv);
	constexpr metronome32::context_error naidef = \
		metronome32::context_error::naidefault;
	unsigned int steps = 0;
	const auto start_counter = vm.get_context().counter;
	
	while (not vm.halted() and vm.get_error_code() != naidef) {
		vm.step();
		steps++;
	}
	
	print_counter(vm);
	print_registers(vm);
	
	if (vm.is_error_trivial()) {
		std::cout << std::endl << "Reversing!" << std::endl;
		vm.reverse();
		vm.halt(false);
		
		for (unsigned int i = 0; i <= steps; i++) vm.step();
		
		print_counter(vm);
		std::cout << std::dec << "(should be " << start_counter;
		std::cout << " (0x" << std::hex << start_counter;
		std::cout << "))" << std::endl;
		
		if (vm.is_error_trivial()) {
			return EXIT_SUCCESS;
		} else {
			std::cout << "Error: " << vm.get_error_name();
			std::cout << std::endl;
			
			return EXIT_FAILURE;
		}
	} else {
		std::cout << "Error: " << vm.get_error_name() << std::endl;
		
		return EXIT_FAILURE;
	}
}
