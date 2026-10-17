/**
 * @file boost_support.cpp
 *
 * @brief What Boost needs from a program built without exceptions.
 *
 * Without exceptions (BOOST_NO_EXCEPTIONS, set by Boost.Config from the compiler options),
 * Boost calls boost::throw_exception where it would throw, and the program defines it.
 * Here it prints the error and aborts the program, as fatal_error() does (the containers of
 * Boost call it when a size is too large or memory is exhausted).
 */

#include "util/cpp_headers.h"

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace boost {

	void throw_exception(const std::exception &e)
	{
		std::fprintf(stderr, "\nFATAL ERROR: %s\n", e.what());
		std::abort();
	}

	void throw_exception(const std::exception &e, const boost::source_location&)
	{
		throw_exception(e);
	}

}
