#ifndef __FEEDMANAGER_H__
#define __FEEDMANAGER_H__

#include <set>
#include <map>
#include <mutex>
#include <unordered_map>
#include <regex>

#include "infra/InfraCommon.h"
#include "infra/judy/JudySArray.h"
#include "base/BaseCommon.h"

using namespace infra;

namespace base
{
	enum class FeedID
	{
		UNKNOWN_FEED = 0,
		MILLENIUM = 1,
		OMXNORDIC = 2,
		MAX_FEEDS
	};

    class FeedHandler
    {
        public:
			typedef LineGroup* (*LineGroupConstructor)(const MarketDataApplication *, FeedHandler *, const pugi::xml_node &, LineGroupAPI *);

			FeedHandler(const MarketDataApplication *, const pugi::xml_node &, FeedID, LineGroupConstructor, FeedAPI *);
			~FeedHandler();

			void start(const Thread *) const; 
			void stop(const Thread *) const; 
			void printStats(const Thread *) const;

			int32_t subscribe(const uint64_t symbolID, Subscription **sub);
			int32_t subscribe(const char *, Subscription **);

			Subscription* getIdSub(uint64_t );
			Subscription* _getIdSub(uint64_t );
			Subscription* getSymbolSub(const char *);
			Subscription* _getSymbolSub(const char *);


			ProductInfo* getProductInfo(Line *line, uint32_t, const char* symbol, uint64_t symbolID, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds);
			ProductInfo* _getProductInfo(Line *line, uint32_t, const char* symbol, uint64_t symbolID, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds);

			ProductInfo* getProductInfo(Line *line, uint32_t, uint64_t symbolID, const char* symbol, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds);
			ProductInfo* _getProductInfo(Line *line, uint32_t, uint64_t symbolID, const char* symbol, const char *isin, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds);

			Subscription* getValidIdIsinSymbolSub(uint64_t symbolID, const char* symbol, const char *isin);

			static FeedID getFeedID(const std::string& feedHandlerType);
			static const char* getFeedType(FeedID id);

			const FeedID	_feedID;
            const char 	_name[NAME_SIZE];
			const PlaybackMode _mode;
			const std::vector<std::string>	_playbackFilesVector;
			bool _reRecord;
			bool _maintainOrders;
			std::mutex _recordingMutex;
			Recording *_recording;

			void registerCallbacks(CallBacks );
			void registerCallbacks(std::string );

        private:

			void createLineGroups(const pugi::xml_node &configNode);

			const LineGroupConstructor _lineGroupConstructor;

			const MarketDataApplication   *_appInstance;
            typedef std::map<std::string , LineGroup *> NameToLineGroupMap;
			NameToLineGroupMap _lineGroupMap;
			LoggerClientHandle	*_logger;
			
			bool _recordPlayback;
			std::string _pbLocation;
				
			// Mutex covering _regexMap, _idSubMap, _symbolSubMap, _symbolProdInfoMap 
			std::mutex _subMapMutex;
			std::vector<std::regex>	_regexMap;
			std::unordered_map<uint64_t, Subscription *> _idSubMap;
			//@TODO: give a thought to replace with vector of char[20]
			// this will make the code slow as I think string will dynamically
			// allocate memory for characters
			//std::map<std::string, Subscription *> _symbolSubMap;
			judySArray<Subscription *> _symbolSubMap;
			judySArray<Subscription *> _isinSubMap;
			judySArray<ProductInfo *> _prodInfoMap;
			judySArray<ProductInfo *> _prodInfoMapISIN;
			// could be intentionally kept std::map instead of unordered_map
			// as it is not read/write for much time
			std::map<uint64_t, ProductInfo *> _prodInfoMapID;
			FeedAPI *_feedAPI;
    };
}

#endif //__FEEDMANAGER_H__
