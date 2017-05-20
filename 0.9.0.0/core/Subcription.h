#ifndef __SUBCRIPTION_H__
#define __SUBCRIPTION_H__

#include "book.h"
#include "trade.h"

namespace core
{
    enum SymbolType
    {
        SymbolType_Numbric		= 0,
        SymbolType_Alphanumbric	= 1
    };

    class Symbol
    {
            union
            {
                unsigned long long  _numericSymbol;
                char[16]            _alphanumricSymbol;
            };
            SymbolType  _symbolType;
    };

    class Subscription
    {
            Symbol	_symbol;
            bool	_isSubscribed;
            bool	_isComposite;
            unsigned long long  *_clientMap;
            Book    _book;
            Trade	_trade;
			/** 
			 * Callback Thread which raises the callbacks for subscription 
			 */
			Thread	_thread;	
    };
}

#endif //__SUBCRIPTION_H__
