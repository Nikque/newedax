/**
 * @file allocator.h
 *
 * @brief An allocator of the C heap for the containers of the C++ library and Boost.
 *
 * Edax is built without exceptions. The default allocator calls operator new, which throws on
 * failure: with a static C++ library (Android) it brings the exception runtime into the program.
 * This allocator takes its memory from the C heap, aligned as the type needs, and stops the program
 * with fatal_error() when memory is exhausted, as the C code of Edax does.
 */

#ifndef EDAX_ALLOCATOR_H
#define EDAX_ALLOCATOR_H

#include "util/util.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cerrno>

/** @brief Memory aligned on @p alignment bytes (a power of 2), or NULL. */
static inline void* aligned_malloc(const size_t size, const size_t alignment)
{
#ifdef _WIN32
	return _aligned_malloc(size ? size : 1, alignment);
#else
	void *p = NULL;
	return posix_memalign(&p, alignment < sizeof (void*) ? sizeof (void*) : alignment, size ? size : 1) == 0 ? p : NULL;
#endif
}

/** @brief Free memory of aligned_malloc(). */
static inline void aligned_free(void *p)
{
#ifdef _WIN32
	_aligned_free(p);
#else
	free(p);
#endif
}

/** Allocator of the C heap (see the file comment). */
template <class T> struct HeapAllocator {
	using value_type = T;

	HeapAllocator() noexcept = default;
	template <class U> HeapAllocator(const HeapAllocator<U>&) noexcept {}

	T* allocate(const size_t n)
	{
		void *p = aligned_malloc(n * sizeof (T), alignof(T));
		if (p == NULL) fatal_error("cannot allocate %llu bytes\n", (unsigned long long) (n * sizeof (T)));
		return static_cast<T*>(p);
	}

	void deallocate(T *p, size_t) noexcept
	{
		aligned_free(p);
	}

	template <class U> bool operator==(const HeapAllocator<U>&) const noexcept { return true; }
	template <class U> bool operator!=(const HeapAllocator<U>&) const noexcept { return false; }
};

#endif /* EDAX_ALLOCATOR_H */
