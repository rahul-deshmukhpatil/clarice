#include <boost/tokenizer.hpp>
#include "infra/utils/StringUtils.h"

using namespace infra;
/**
 * Used by
 * saperating playback files in FeedHandler.cpp
 */
extern "C" std::vector<std::string> tokenize(std::string str, const char *delimeters)
{
	std::vector<std::string> result;
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(delimeters);
	tokenizer tokens(str, sep);
	auto tok_iter = tokens.begin();
	while(tok_iter != tokens.end())
	{
		result.push_back(*tok_iter);
		++tok_iter;
	}
	return result;
}
