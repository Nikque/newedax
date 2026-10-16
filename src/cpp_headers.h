/**
 * @file cpp_headers.h
 *
 * @brief The C++ and Boost headers used by Edax.
 *
 * all.cpp reads this file first: libedax_utf8.h (the library on Windows) replaces fopen and
 * remove by macros, which would rename std::remove and other names in the headers read after it.
 * The sources that use these headers also include this file (it is read once).
 *
 * Edax is built without exceptions: the Boost code used here must not throw (pdqsort does not
 * allocate memory and does not throw).
 */

#ifndef EDAX_CPP_HEADERS_H
#define EDAX_CPP_HEADERS_H

#include <algorithm>

#include <boost/sort/pdqsort/pdqsort.hpp>

#endif /* EDAX_CPP_HEADERS_H */
