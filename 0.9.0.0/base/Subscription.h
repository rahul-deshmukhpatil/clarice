#ifndef __SUBSCRIPTION_H__
#define __SUBSCRIPTION_H__

#include <atomic>
#include "base/Book.h" 
#include "base/Quote.h" 
#include "base/Trade.h" 
#include "base/Status.h" 

namespace base
{
	const char * InstrStateToString(InstrState state);

	class Subscription
	{
		public:
			Subscription(const uint64_t symbolID, bool activate);
			Subscription(const char *symbol, bool activate);
			//bool hasInterest(CallBacks callback) const;

			//@TODO: take look if needed atomic
			std::atomic<bool>	_isActive;
			char				_symbol[NAME_SIZE];
			uint64_t			_symbolID;
			char				_isin[13];

			//Atomic so that wrong callback is not raised
			//and could be overide by main thread subscribe
			ProductInfo	*_prodInfo;
			Status _status;
			Trade _trade;
			Quote _quote;	
			Book _book;
	};
}

#endif// __SUBSCRIPTION_H__
