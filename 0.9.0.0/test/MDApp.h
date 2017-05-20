#ifndef __MD_H__
#define __MD_H__

#include "base/API.h"

class MDApp: public FeedAPI
{
	std::string _config;
	std::vector<std::string> _symbols;
	std::vector<std::string> _symbolFiles;
	FILE *_output;
	base::MarketDataApplication* _marketDataApp;
	bool _print;
	uint32_t _printLevels; // printLevels in the book
	bool _printOrders; // printOrders in the levels

	public:

		//FeedAPI
		void onFeedStarted(const FeedHandler *feedHandler);
		void onFeedStopped(const FeedHandler *feedHandler);
		void onLineGroupStarted(const LineGroup *lineGroup);
		void onLineGroupStopped(const LineGroup *lineGroup);
		void onLineStarted(const Line *pLine);
		void onLineStopped(const Line *pLine);
		void onPacketStart(const Line *pLine);
		void onPacketEnd(const Line *pLine);

		void printBase(char *buf, const Base *);
		void printProductInfo(const ProductInfo *);
		void printLevels(const Book *book, char *buf, uint32_t &wrote);
		void printOrder(const Order *);
		void printQuote(const Quote *);
		void printBook(const Book *book);
		void printTrade(const Trade *);
		void printStatus(const Status *);

		//SubscriptionAPI
		void onOrder(const Order *);
		void onBook(const Book *);
		void onQuote(const Quote *);
		void onTrade(const Trade *, const Order *);
		void onStatus(const Status *);
		void onCustom(const Custom *);
		void onProductInfo(const ProductInfo *);

		MDApp();
		~MDApp();
		void parseCmdLine(int argc, char *argv[]);
		void init();
		void subscribe();
		void start();
		void stop();

		atomic<bool> _shutdownApp;
};

#endif //__MD_H__
