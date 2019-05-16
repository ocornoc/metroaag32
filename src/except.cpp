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

#include <exception>
#include <stdexcept>
#include <string>
#include "except.h"
#include "transforms.h"

namespace maag32 = metroaag32;

maag32::exception::exception() noexcept
	: maag32::exception::exception("Undefined MetroAAG32 error")
{}

maag32::invalid_argument::invalid_argument(
	const std::string& head,
	const std::string& line,
	const std::string& why
) noexcept
	: maag32::invalid_argument::invalid_argument(head + \
		"Line:\t" + line + "\n" + why
	)
{}

maag32::unknown_instruction::unknown_instruction(
	const std::string& head,
	const maag32::directive& dir
) noexcept
	: maag32::unknown_instruction::unknown_instruction(head + \
		"Unknown instruction:\nDirective:\n\t" + dir.original
	)
{}

maag32::duplicate_label::duplicate_label(
	const std::string& head,
	const maag32::directive& dup1,
	const maag32::directive& dup2
) noexcept
	: maag32::duplicate_label::duplicate_label(head + \
		"Duplicate labels found:\nDirective one:\n\t" + dup1.original + \
		"\n\nDirective two:\n\t" + dup2.original
	)
{}

maag32::incorrect_arg_num::incorrect_arg_num(
	const std::string& head,
	const directive& dir,
	unsigned long long correct_num
) noexcept
	: maag32::exception::exception(head + \
		"Incorrect number of arguments provided:\n\t" + dir.original + \
		"\n\tExpect arg number: " + std::to_string(correct_num)
	)
{}
