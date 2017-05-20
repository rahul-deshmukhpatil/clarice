#include <string.h>
#include "base/ProductInfo.h"

using namespace base;

ProductInfo::ProductInfo(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, const char *symbol)
	: Base(line, sub, msgType, seqNo, exSeconds, exUSeconds, PRODUCTINFO_UPDATE)
{
	strncpy(_symbol, symbol, sizeof(_symbol));	
}

ProductInfo::ProductInfo(Line *line, Subscription *sub, uint32_t msgType, uint64_t seqNo, uint32_t exSeconds, uint32_t exUSeconds, uint64_t symbolID, const char *symbol, const char *isin)
	: Base(line, sub, msgType, seqNo, exSeconds, exUSeconds, PRODUCTINFO_UPDATE)
	, _symbolID(symbolID)
{
	strncpy(_symbol, symbol, sizeof(_symbol));	
	strncpy(_isin, isin, sizeof(_isin));	
}
