#ifndef __OMXNORIDCUTILS_H__
#define __OMXNORIDCUTILS_H__

#include <stdint.h>

namespace omxnordic
{
	// Assumes that str contains atleast one valid
	// non space char
	extern "C" uint32_t ltrim(const char *str);

	extern "C" uint64_t atoll(const char *str, uint32_t  size);
	extern "C" uint64_t getUint64(const char *str, uint32_t  size);
	extern "C" uint64_t getUint64Price(const char *str);
}

#endif // __OMXNORIDCUTILS_H__
