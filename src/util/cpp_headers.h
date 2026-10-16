/**
 * @file cpp_headers.h
 *
 * @brief The C++ and Boost headers used by Edax.
 *
 * all.cpp reads this file first: libedax_utf8.h (the library on Windows) replaces fopen and
 * remove by macros, which would rename std::remove and other names in the headers read after it.
 * The sources that use these headers also include this file (it is read once).
 *
 * Edax is built without exceptions: where Boost would throw, it calls boost::throw_exception, defined in
 * boost_support.cpp (it prints the error and aborts). pdqsort neither allocates memory nor throws.
 */

#ifndef EDAX_CPP_HEADERS_H
#define EDAX_CPP_HEADERS_H

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <new>
#include <utility>
#include <type_traits>

#include <boost/assert/source_location.hpp>
#include <boost/sort/pdqsort/pdqsort.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#endif /* EDAX_CPP_HEADERS_H */
