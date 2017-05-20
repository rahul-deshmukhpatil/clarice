#ifndef __PRODUCTINFO_H_
#define __PRODUCTINFO_H__

#include "base/BaseCommon.h"
#include "base/Base.h"

namespace base
{
    enum Currency
    {
        UNKNOWN = 0,
        USD     = 1,	// US DOLLER
        EUR     = 2,	// EURO
        GBP     = 3,	// GB POUND
        CHF		= 4		// SWISS FRANK
    };

    enum ProductInfoFieldType
    {
        UKNOWN		= 0,
        SYMBOL		= 1,
        LOTSIZE		= 2,
        TICKSIZE	= 3,
        CURRENCY	= 4
    };

    class ProductInfo: public Base
    {
		public:
		ProductInfo(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, const char *symbol);
		ProductInfo(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t symbolID, const char *symbol, const char *isin);
        char		 _symbol[NAME_SIZE];
        uint64_t	 _symbolID;
        char		 _isin[12];
        uint32_t	 _lotSize;
        uint32_t	 _tickSize;
        Currency	 _currency;
    };
}
#endif //__PRODUCTINFO_H__
