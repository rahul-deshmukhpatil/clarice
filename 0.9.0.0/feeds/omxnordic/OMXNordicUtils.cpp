#include "feeds/omxnordic/OMXNordicUtils.h"

using namespace omxnordic;
// Assumes that str contains atleast one valid
// non space char
extern "C" 
uint32_t  ltrim(const char *str)
{
	uint32_t  i = 0;	
	// Trim leading space, ascii 32
	while(*str++ == ' ') i++;
	return i;
}

extern "C" 
uint64_t atoll(const char *str, uint32_t  size)
{
	uint32_t  i = 0;
	uint64_t result = 0;
	while( i < size)
	{
		result *= 10;
		result += str[i] - '0';
		i++;
	}
	return result;
}

extern "C" 
uint64_t getUint64(const char *str, uint32_t  size)
{
	uint32_t  i = omxnordic::ltrim(str);	
	return omxnordic::atoll(str+i, size - i);
}

extern "C" 
uint64_t getUint64Price(const char *str)
{
	double result = omxnordic::getUint64(str, 6);
	return result + (omxnordic::getUint64(str+6, 4)/10000.0);
}
