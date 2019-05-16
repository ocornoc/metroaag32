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

#ifndef METROAGG32_HEADER_EXCEPT
#define METROAGG32_HEADER_EXCEPT
#include <exception>
#include <stdexcept>
#include <string>
#include "transforms.h"

namespace metroaag32 {
	// General Pendulum32 exceptions.
	class exception;
	// An argument of invalid type or value.
	class invalid_argument;
	// An argument that's too small/low to be valid.
	class underflow_except;
	// An instruction that's not known.
	class unknown_instruction;
	// A label showed up multiple times.
	class duplicate_label;
	// Incorrect number of arguments.
	class incorrect_arg_num;
}

class metroaag32::exception : public std::runtime_error
{
	using std::runtime_error::runtime_error;
	
	public:
		exception() noexcept;
		exception(const exception&) noexcept
			= default;
		exception& operator=(const exception&) noexcept
			= default;
		virtual ~exception()
			= default;
};

class metroaag32::invalid_argument : public metroaag32::exception
{
	using metroaag32::exception::exception;
	
	public:
		invalid_argument(
			const std::string& head,
			const std::string& line,
			const std::string& why
		) noexcept;
		invalid_argument(const invalid_argument&) noexcept
			= default;
		invalid_argument& operator=(const invalid_argument&) noexcept
			= default;
		virtual ~invalid_argument()
			= default;
};

class metroaag32::underflow_except : public metroaag32::invalid_argument
{
	using metroaag32::invalid_argument::invalid_argument;
	
	public:
		underflow_except(const underflow_except&) noexcept
			= default;
		underflow_except& operator=(const underflow_except&) noexcept
			= default;
		virtual ~underflow_except()
			= default;
};

class metroaag32::unknown_instruction : public metroaag32::exception
{
	using metroaag32::exception::exception;
	
	public:
		unknown_instruction(const unknown_instruction&) noexcept
			= default;
		unknown_instruction(
			const std::string& head,
			const directive& dir
		) noexcept;
		unknown_instruction& operator=(const unknown_instruction&) noexcept
			= default;
		virtual ~unknown_instruction()
			= default;
};

class metroaag32::duplicate_label : public metroaag32::exception
{
	using metroaag32::exception::exception;
	
	public:
		duplicate_label(const duplicate_label&) noexcept
			= default;
		duplicate_label(
			const std::string& head,
			const directive& dup1,
			const directive& dup2
		) noexcept;
		duplicate_label& operator=(const duplicate_label&) noexcept
			= default;
		virtual ~duplicate_label()
			= default;
};

class metroaag32::incorrect_arg_num : public metroaag32::exception
{
	using metroaag32::exception::exception;
	
	public:
		incorrect_arg_num(
			const std::string& head,
			const directive& dir,
			unsigned long long correct_num
		) noexcept;
		incorrect_arg_num(const incorrect_arg_num&) noexcept
			= default;
		incorrect_arg_num& operator=(const incorrect_arg_num&) noexcept
			= default;
		virtual ~incorrect_arg_num()
			= default;
};

#endif
