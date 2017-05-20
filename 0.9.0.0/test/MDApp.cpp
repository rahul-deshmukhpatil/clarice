#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <string.h> 
#include "infra/utils/StringUtils.h"
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "base/LineGroup.h"
#include "base/Line.h"
#include "base/Packet.h"
#include "base/Subscription.h"
#include "base/Base.h"
#include "base/LineGroup.h"
#include "base/Line.h"
#include "base/Order.h"
#include "base/Book.h"
#include "base/Quote.h"
#include "base/PriceLevel.h"
#include "base/Quote.h"
#include "base/Trade.h"
#include "base/Status.h"
#include "base/Custom.h"
#include "base/ProductInfo.h"
#include "MDApp.h"

using namespace base;

#define FPRINTF fprintf 

base::FeedAPI feedAPI;
MDApp *app = nullptr;

// typedef for signals
typedef void (*SignalHandler)(int, siginfo_t *, void *);

void register_signals(int signal, SignalHandler sig_func);

// register the signalhandler for siginfo 
void signal_handler(int signum, siginfo_t *siginfo, void *data)
{
	fprintf(stderr, "Recieved signal %d\n", signum);	
	app->_shutdownApp = true;
	register_signals(signum, nullptr);
}

//register the signalhandler for siginfo 
void register_signals(int signal, SignalHandler sig_func)
{
	struct sigaction act;	
	struct sigaction oldact;	
	memset(&act, 0, sizeof(act));
	act.sa_handler = nullptr;
	act.sa_sigaction = sig_func;
	act.sa_flags =  SA_SIGINFO;

	int result = sigaction(signal, &act, &oldact);
	if(result)
	{
		perror("Could not register signal SIGINT");
		exit(-1);
	}
}

void MDApp::onFeedStarted(const FeedHandler *feedHandler)
{
	fprintf(stdout, "update onFeedStarted, feed %s\n", feedHandler->_name);
}

void MDApp::onFeedStopped(const FeedHandler *feedHandler)
{
	fprintf(stdout, "update onFeedStopped, feed %s\n", feedHandler->_name);
}

void MDApp::onLineGroupStarted(const LineGroup *lineGroup)
{
	fprintf(stdout, "update onLineGroupStarted, feed %s, lineGroup %s\n", lineGroup->_feedHandler->_name, lineGroup->_name);
}

void MDApp::onLineGroupStopped(const LineGroup *lineGroup)
{
	fprintf(stdout, "update onLineGroupStarted, feed %s, lineGroup %s\n", lineGroup->_feedHandler->_name, lineGroup->_name);
}

void MDApp::onLineStarted(const Line *pLine)
{
	fprintf(stdout, "update onLineStarted, feed %s, lineGroup %s, line: %s\n", pLine->_lineGroup->_feedHandler->_name, pLine->_lineGroup->_name, pLine->_name);
}

void MDApp::onLineStopped(const Line *pLine)
{
	fprintf(stdout, "update onLineStopped, feed %s, lineGroup %s, line: %s\n", pLine->_lineGroup->_feedHandler->_name, pLine->_lineGroup->_name, pLine->_name);
}

void MDApp::onPacketStart(const Line *pLine)
{
//	fprintf(stdout, "sq %lu, update onPacketStart, endsq %lu, nmsg %lu, %s\n", pLine->_headerSeqNo, pLine->_endSeqNo, pLine->_endSeqNo - pLine->_headerSeqNo+1, pLine->_packet->_packetAddress->toString().c_str());
}

void MDApp::onPacketEnd(const Line *pLine)
{
//	fprintf(stdout, "sq %lu, update onPacketEnd, endsq %lu, nmsg %lu, %s\n", pLine->_headerSeqNo, pLine->_endSeqNo, pLine->_endSeqNo - pLine->_headerSeqNo+1, pLine->_packet->_packetAddress->toString().c_str());
}

/**
 * Checks if ID has valid type and valid value
 */
void assertID(const ID &id)
{
	// Either it is numeric or alphanum
	ASSERT(id.type() == NUMERIC || id.type() == ALPHANUM, "Wrong Order ID type");

	// if numeric should not be 0, if alphanum should not be equal to "" 
 	ASSERT(	id.type() == NUMERIC ? id.numeric() != 0 : strcmp(id.alphanum(), ""), "Order ID is not initialized");
}

void assertBase(const Base *base)
{
	ASSERT(base != nullptr, "Base is nullptr");
	ASSERT(base->_sub->_symbolID != 0, "SymbolID is 0");
	ASSERT(base->_sub->_prodInfo != nullptr, "ProductInfo is nullptr");
	ASSERT(base->_seqNum != 0, "Seq num is 0");
	ASSERT(base->_xt.tv_sec != 0, "Exchange Time is 0");
	ASSERT(base->_type != UNKNOWN_UPDATE, "Object type unknown");
}



/**
 * Assert if all the order fields have valid values 
 */
void assertOrder(const Order *order)
{
	assertBase(order);
	// orderType is either NORMAL_ORDER or MARKET_ORDER
	ASSERT(order->_orderType == NORMAL_ORDER || 
			order->_orderType == MARKET_ORDER, "Invalid Order type");

	// order side is BUY or SELL
	ASSERT(order->_side == BUY || order->_side == SELL, "Invalid Order Side");

	//Check if orderID is  not empty 
	//assertID()

	// if not MARKET_ORDER then price should not be zero
	if(order->_orderType != MARKET_ORDER && (order->_action == ADDED || order->_action == MODIFIED))
	{
		ASSERT(order->_px != 0.0, "Order Price is 0.0");
	}

	// if modified or added, size should not be zero 
	ASSERT(order->_action == ADDED ||
			order->_action == MODIFIED ||
			order->_action == DELETED ||
			order->_action == EXECUTED, "Invalid Order Action");

	// if modified or added, size should not be zero 
	if(order->_action == ADDED || order->_action == MODIFIED)
	{
		ASSERT(order->_size != 0, "Order size is 0 at the time of add/modify");
	}
}

/**
 * Assert if all the quote fields have valid values 
 */
void assertQuote(const Quote *quote)
{
	assertBase(quote);
	
	ASSERT(_sub->_buys.empty() ||  _buy == _sub->_buys.begin()->second, "quote->_buy != TOB");
	ASSERT(_sub->_sells.empty() ||  _sell == _sub->_sells.begin()->second, "quote->_sell != TOB");
}

/**
 * Assert if all the trade fields have valid values 
 */
void assertTrade(const Trade *trade)
{
	assertBase(trade);
	// trade side is BUY or SELL
	//ASSERT(trade->_side == BUY || 
	//		trade->_side == SELL || trade->_side == SIDE_UNKNOWN);
	ASSERT(trade->_px != 0.0, "Traded price is 0.0");
}

void calcLatency(const Base *base)
{
	LineGroup *lineGroup = base->_line->_lineGroup;
#ifdef __MSG_LATENCY__
	getLatency(lineGroup->_ml, lineGroup->_mt);
#endif

#ifdef __PACKET_LATENCY__
	getLatency(lineGroup->_rl, base->_line->_packet->_rt);
#endif 
}

void MDApp::printBase(char *buf, const Base *base)
{
	timespec ct = globalClock();

	timespec mt = base->_line->_lineGroup->_mt;

	timespec rt = base->_line->_packet->_rt;

	//FPRINTF(_output, "pt %s, line %s, instrID %lu, sq %lu, msg %u, recordTime %ld.%.09ld, xt %ld.%.09ld, rt %ld.%.09ld, mt %ld.%.09ld, ct %ld.%.09ld, ut %c, ", base->_line->_lineGroup->_name, base->_line->_name, base->_sub->_symbolID, base->_seqNum, base->_msgType, base->_line->_packet->_recordTime.tv_sec, base->_line->_packet->_recordTime.tv_nsec, base->_xt.tv_sec, base->_xt.tv_nsec, rt.tv_sec, rt.tv_nsec, mt.tv_sec, mt.tv_nsec, ct.tv_sec, ct.tv_nsec, base->_type);
	sprintf(buf, "pt %s, line %s, instrID %lu, sq %lu, msg %u, recordTime %ld.%.09ld, xt %ld.%.09ld, ut %c", base->_line->_lineGroup->_name, base->_line->_name, base->_sub->_symbolID, base->_seqNum, base->_msgType, base->_line->_packet->_recordTime.tv_sec, base->_line->_packet->_recordTime.tv_nsec, base->_xt.tv_sec, base->_xt.tv_nsec, base->_type);
}

void MDApp::printProductInfo(const ProductInfo *product)
{
	if(_print)
	{
		char buf[512];
		printBase(buf, product);
		FPRINTF(_output, "%s, update onProductInfo, symbol %s, isin %.12s\n", buf, product->_symbol, product->_isin);
	}
}

void MDApp::onProductInfo(const ProductInfo *product)
{
	printProductInfo(product);
}

void MDApp::printLevels(const Book *book, char *buf, uint32_t &wrote)
{
	{
		PriceLevelMap::const_iterator itr = book->_buys.begin();
		PriceLevelMap::const_iterator itrEnd = book->_buys.end();
		int i = 1;
		wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "	Bid Levels : size %lu\n", book->_buyQuantity);
		for(; itr != itrEnd && i <= _printLevels; itr++, i++)
		{
			wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "	%d] px %f, size %lu, num %lu\n", i, itr->second->_px, itr->second->_size, itr->second->_numOrders);
			if(_printOrders)
			{
				Order *order = itr->second->_orders;
				while(order)
				{
					wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "		|__ px %f, size %u, orderID %s\n", order->_px, order->_size, order->_orderID.to_string().c_str());
					order = order->_next;
				}
			}
		}
	}
	{
		PriceLevelMap::const_iterator itr = book->_sells.begin();
		PriceLevelMap::const_iterator itrEnd = book->_sells.end();
		int i = 1;
		wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "	Ask Levels : size %lu\n", book->_sellQuantity);
		for(; itr != itrEnd && i <= _printLevels; itr++, i++)
		{
			wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "	%d] px %f, size %lu, num %lu\n", i, itr->second->_px, itr->second->_size, itr->second->_numOrders);
			if(_printOrders)
			{
				Order *order = itr->second->_orders;
				while(order)
				{
					wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "		|__ px %f, size %u, orderID %s\n", order->_px, order->_size, order->_orderID.to_string().c_str());
					order = order->_next;
				}
			}
		}
	}
}

void MDApp::printOrder(const Order *order)
{
	assertOrder(order);
	if(_print)
	{
		char buf1[512];
		char buf[65536 * 10];
		printBase(buf1, order);
		uint32_t wrote = snprintf(buf, sizeof(buf), "%s, px %f, size %u, side %c, action %c, orderID %s\n", buf1, order->_px, order->_size, order->_side, order->_action, order->_orderID.to_string().c_str());
		fwrite(buf, 1, wrote, _output);
	}
}

void MDApp::onOrder(const Order *order)
{
	calcLatency(order);
	printOrder(order);
}

void MDApp::printBook(const Book *book)
{
	//assertOrder(book);
	if(_print)
	{
		char buf1[512];
		char buf[65536 * 10];
		printBase(buf1, book);

		uint32_t wrote = snprintf(buf, sizeof(buf), "%s", buf1);

		if(!book->_buys.empty())
		{
			wrote += snprintf(buf + wrote , sizeof(buf) - wrote, ", bp %f, bs %lu, bnum %lu, blevels %lu, cbs %lu", book->_buys.begin()->second->_px, book->_buys.begin()->second->_size, book->_buys.begin()->second->_numOrders, book->_buys.size(), book->_buyQuantity);
		}
	
		if(!book->_sells.empty())
		{
			wrote += snprintf(buf + wrote , sizeof(buf) - wrote, ", ap %f, as %lu, snum %lu, alevels %lu, cas %lu \n", book->_sells.begin()->second->_px, book->_sells.begin()->second->_size, book->_sells.begin()->second->_numOrders, book->_sells.size(), book->_sellQuantity);
		}

		if(_printLevels)
		{
			printLevels(book, buf, wrote);
		}
		fwrite(buf, 1, wrote, _output);
	}
}

void MDApp::onBook(const Book *book)
{
	calcLatency(book);
	printBook(book);
}

void MDApp::printQuote(const Quote *quote)
{
	assertQuote(quote);
	if(_print)
	{
		char buf1[512];
		char buf[512];
		printBase(buf1, quote);
		uint32_t wrote = snprintf(buf, sizeof(buf), "%s", buf1);
		if(quote->_buy)
		{
			wrote += snprintf(buf + wrote, sizeof(buf) - wrote, ", bp %f, bs %lu, bnum %lu", quote->_buy->_px, quote->_buy->_size, quote->_buy->_numOrders);
		}

		if(quote->_sell)
		{
			wrote += snprintf(buf + wrote, sizeof(buf) - wrote, ", ap %f, as %lu, anum %lu", quote->_sell->_px, quote->_sell->_size, quote->_sell->_numOrders);
		}
		wrote += snprintf(buf + wrote, sizeof(buf) - wrote, "\n");
		fwrite(buf, 1, wrote, _output);
	}
}


void MDApp::onQuote(const Quote *quote)
{
	calcLatency(quote);
	printQuote(quote);
}

void MDApp::printTrade(const Trade *trade)
{
	if(_print)
	{
		char buf[512];
		printBase(buf, trade);
		FPRINTF(_output, "%s, px %f, size %u, side %c, tradeID %s\n", buf, trade->_px, trade->_size, trade->_side, trade->_tradeID.to_string().c_str());
	}
}

void MDApp::onTrade(const Trade *trade, const Order *order)
{
	calcLatency(trade);
	assertTrade(trade);
	printTrade(trade);
	if(order)
	{
		printOrder(order);
	}
}

void MDApp::printStatus(const Status *status)
{
	if(_print)
	{
		char buf[512];
		printBase(buf, status);
		FPRINTF(_output, "%s, st %s, prev %s\n", buf, Status::StateToCStr(status->_state), Status::StateToCStr(status->_prevState));
	}
}

void MDApp::onStatus(const Status *status)
{
	calcLatency(status);
	printStatus(status);
}

void MDApp::onCustom(const Custom *custom)
{
}

MDApp::MDApp()
	: FeedAPI(static_cast<OnFeedStarted>(&MDApp::onFeedStarted)
	, static_cast<OnFeedStopped>(&MDApp::onFeedStopped)
	, static_cast<OnLineGroupStarted>(&MDApp::onLineGroupStarted)
	, static_cast<OnLineGroupStopped>(&MDApp::onLineGroupStopped)
	, static_cast<OnLineStarted>(&MDApp::onLineStarted)
	, static_cast<OnLineStopped>(&MDApp::onLineStopped)
	, static_cast<OnPacketStart>(&MDApp::onPacketStart)
	, static_cast<OnPacketEnd>(&MDApp::onPacketEnd)
	, static_cast<OnOrder>(&MDApp::onOrder)
	, static_cast<OnBook>(&MDApp::onBook)
	, static_cast<OnQuote>(&MDApp::onQuote)
	, static_cast<OnTrade>(&MDApp::onTrade)
	, static_cast<OnStatus>(&MDApp::onStatus)
	, static_cast<OnCustom>(&MDApp::onCustom)
	, static_cast<OnProductInfo>(&MDApp::onProductInfo))
	, _config("")
	, _output(stdout)
	, _symbols()
	, _symbolFiles()
	, _marketDataApp(nullptr)
	, _print(false)
	, _printLevels(0)
	, _printOrders(false)
	, _shutdownApp(false)
{
}

MDApp::~MDApp()
{
	/**
	 * Will now release all the resources held.
	 * Destructs in exactly reverse order of the create base::MarketDataApplication
	 */	

	delete _marketDataApp;	
}

void MDApp::parseCmdLine(int argc, char *argv[])
{
	int opt;
	while((opt = getopt(argc, argv, "oc:s:S:f:p:")) != -1) 
	{
		switch (opt) 
		{
			case 'c':
				_config = optarg;
				break;

			case 's':
				_symbols = tokenize(optarg, ",");
				break;

			case 'S':
				_symbolFiles = tokenize(optarg, ",");
				break;

			case 'p':
				if(optarg)
					_printLevels = atoi(optarg);
				else
					_printLevels = 100000;
				break;

			case 'o':
				_printOrders = true;
				break;

			case 'f':
				if(!strcmp(optarg, "-"))
				{
					_output = stdout;
					_print = true;
				}
				else
				{
					_output = fopen(optarg, "w");
					_print = true;
					//@TODO: check return value and give exception
				}
				break;
			
			default: /* '?' */
				break;
		}
	}

	if(_config.empty())
	{
		fprintf(stderr, "Please provide config file with option -c \n");
		exit(-1);
	}
}

void MDApp::init()
{
	_marketDataApp = new base::MarketDataApplication(_config, this);	
}

void MDApp::subscribe()
{
	/*
	 * @TODO: Subscribe to the _marketDataApp.
	 * _marketDataApp->subscribe(SYMBOL, feed-name, CALLBACKS, *subscriptionInterface);	
	 */
	Subscription *ppSub;

	std::vector<string>::const_iterator	itr = _symbols.begin();
	for(; itr != _symbols.end(); itr++)
	{
		uint32_t ret = _marketDataApp->subscribe("millenium", (*itr).c_str(), &ppSub);
		if(ret < 0)
		{
			fprintf(stderr, "Could not create subscription symbols %s\n", (*itr).c_str());
		}
		else
		{
			fprintf(stderr, "Subscribed to %s\n", (*itr).c_str());
		}
	}
}

void MDApp::start()
{
	_marketDataApp->start();

	while(!_shutdownApp.load(std::memory_order_acquire))
	{
	}
}

void MDApp::stop()
{
	/** 
	 * Stop the managers, connections and flush all logs in logger at the end. We have not freed _thread yet.
	 */
	std::cerr << "Shutting down the application !!!" << endl;	
	_marketDataApp->stop();

}

int main(int argc, char *argv[])
{
	register_signals(SIGINT, signal_handler);
	register_signals(15, signal_handler);
	char buff[16 * 1024];
	//Set the stdout buffer
	setvbuf(stdout, buff, _IOFBF, sizeof(buff));
	fprintf(stderr, "Standard Market Data application: Clarice Feeds !!!\n");

	try	
	{
		app= new MDApp();
		app->parseCmdLine(argc, argv);
		app->init();
		app->subscribe();
		app->start();
		app->stop();
		usleep(5 * 1000000);
		delete  app;
	}
	catch(std::exception& e)
	{
		std::cerr << "exception caught: " << e.what() << '\n';
		app->stop();
		delete  app;
		exit(-1);
	}
	catch(...)
	{
		fprintf(stderr, "EXECPTION: Standard Market Data application: Clarice Feeds !!!\n");
		app->stop();
		delete  app;
		exit(-1);
	}
	return 0;
}
