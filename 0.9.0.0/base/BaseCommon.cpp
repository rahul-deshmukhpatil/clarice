#include <stdexcept>  
#include <string.h>


#include "base/BaseCommon.h"

using namespace base;

const char *CLARICE_HEADER = "CLARICE_FEEDS__RECORD___";

ID::ID()
: _IDType(IDTYPE_UNKNOWN)
{
	_numericID = 0;
}

ID::ID(char *strID)
: _IDType(ALPHANUM)
{
	strncpy(_alphaNumID, strID, sizeof(_alphaNumID));
}

ID::ID(uint64_t numericID)
	: _numericID(numericID)
	  , _IDType(NUMERIC)
{
}

ID::~ID()
{
	_IDType = IDTYPE_UNKNOWN;
	_numericID = 0;
}

std::string ID::to_string() const
{
	if(_IDType == NUMERIC)
	{
		return std::to_string(_numericID); 
	}
	else
	{
		return std::string(_alphaNumID); 
	}
}

uint64_t ID::numeric() const 
{
	return _numericID;
}

const char* ID::alphanum() const
{
	return _alphaNumID;
}

/**
 * \brief: getPlaybackModeFromString
 *			Used to get enum PlaybackMode from string at the time of
 *			reading config for setting up the Feed Handler 
 *
 */
extern "C"
PlaybackMode playbackMode(const std::string &strMode)
{
	if(strMode == "file")
	{
		return PlaybackMode::FILE_PLAYBACK;
	}
	else if(strMode == "live")
	{
		return PlaybackMode::LIVE_PLAYBACK;
	}
}

/**
 * \brief: getPlaybackModeFromString
 *			Used to get enum PlaybackMode from string at the time of
 *			reading config for setting up the Feed Handler 
 *
 */
extern "C"
const char* playbackModeToStr(const PlaybackMode mode)
{
	switch(mode)
	{
		case PlaybackMode::FILE_PLAYBACK:
				return "file";

		case PlaybackMode::LIVE_PLAYBACK:
				return "live";
		
		default:
				///< Throw an error exception duplicate lineGroup name 
				char errorBuff[512];

				snprintf(errorBuff, sizeof(errorBuff), "Tried to convert invalid playback mode");
				std::string error(errorBuff);

				throw std::invalid_argument(error);
	}
}
