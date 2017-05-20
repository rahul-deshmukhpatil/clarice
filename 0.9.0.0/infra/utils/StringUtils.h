#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

#include <vector>
#include <string>

namespace infra 
{
	extern "C" std::vector<std::string> tokenize(std::string , const char *);
}
#endif//__STRING_UTILS_H__
