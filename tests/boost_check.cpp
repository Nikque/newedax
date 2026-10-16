/**
 * @file boost_check.cpp
 *
 * @brief Check that the Boost headers used by new edax build without exceptions and RTTI.
 *
 * Built apart from Edax (it is not included by all.cpp), with the options of Edax and
 * -I <boost directory>. With BOOST_NO_EXCEPTIONS, Boost calls boost::throw_exception,
 * which is defined here: it ends the program, as a failed allocation would.
 */

#define BOOST_NO_EXCEPTIONS
#include <boost/version.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/assert/source_location.hpp>

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace boost {
	void throw_exception(const std::exception &e)
	{
		std::fprintf(stderr, "boost exception: %s\n", e.what());
		std::abort();
	}
	void throw_exception(const std::exception &e, const boost::source_location&)
	{
		throw_exception(e);
	}
}

int main(void)
{
	boost::unordered_flat_map<unsigned long long, int> map;
	boost::container::static_vector<int, 8> v;
	unsigned long long key = 0x0123456789abcdefULL;
	int sum = 0;

	for (int i = 0; i < 1000; ++i) map[key * (unsigned long long) i] = i;
	for (int i = 0; i < 8; ++i) v.push_back(i);
	for (int x : v) sum += x;

	std::printf("boost %d.%d.%d: map %zu, vector sum %d, big endian of 0x01020304: 0x%08x\n",
		BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100,
		map.size(), sum, (unsigned) boost::endian::native_to_big(0x01020304u));
	return (map.size() == 1000 && sum == 28) ? 0 : 1;
}
