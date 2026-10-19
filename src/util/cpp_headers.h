/**
 * @file cpp_headers.h
 *
 * @brief The C++ and Boost headers used by Edax.
 *
 * all.cpp reads this file first; the sources that use these headers also include it (it is read once).
 *
 * Edax is built without exceptions: where Boost would throw, it calls boost::throw_exception, defined in
 * boost_support.cpp (it prints the error and aborts). pdqsort neither allocates memory nor throws.
 * The containers allocate with HeapAllocator (allocator.h): no operator new, no C++ runtime to link.
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

/** A function the compiler must inline (a template made for a constant: no call left). */
#if defined(_MSC_VER)
	#define EDAX_FORCE_INLINE __forceinline
#elif defined(__GNUC__)
	#define EDAX_FORCE_INLINE inline __attribute__((always_inline))
#else
	#define EDAX_FORCE_INLINE inline
#endif

/** Put before a statement: the calls in it are inlined (MSVC 19.37 or later; no effect with other compilers). */
#if defined(_MSC_VER) && _MSC_VER >= 1937 && !defined(__clang__)
	#define EDAX_INLINE_CALLS [[msvc::forceinline_calls]]
#else
	#define EDAX_INLINE_CALLS
#endif

#include <boost/assert/source_location.hpp>
#include <boost/sort/pdqsort/pdqsort.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#endif /* EDAX_CPP_HEADERS_H */
