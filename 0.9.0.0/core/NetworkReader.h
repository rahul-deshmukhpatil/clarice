#ifndef __NETWORK_READER_H__
#define __NETWORK_READER_H__

#include "infra/app/MarketDataApplication.h"

namespace core
{
	enum IPType
	{
		PRIM_MC,
		SEC_MC,
		PRIM_MC_SNAP,
		SEC_MC_SNAP,
		PRIM_MC_RETRANS,
		SEC_MC_RETRANS,
		PRIM_TCP,
		SEC_TCP,
		PRIM_TCP_SNAP,
		SEC_TCP_SNAP,
		PRIM_TCP_RETRANS,
		SEC_TCP_RETRANS,
		IPTYPE_MAX
	};

	class IP
	{
		uint32_t	IPv4;
		uint16_t	port;
	};

	class Line
	{
		IP	ips[IPTYPE_MAX];
	};

	class NetworkReader
	{
		public:
			NetworkReader(const infra::MarketDataApplication *app, infra::LoggerClientHandleSharedPtr logger, const pugi::xml_node &readerNode, std::string readerName);

		private:
			std::string 	_name;	///< Name of thread belonging to network reader
			std::set<Line>	_lines;	///< All the lines beloning to this network reader 
			
	};

	typedef std::shared_ptr<NetworkReader> SharedReaderPtr;
}

#endif //__NETWORK_READER_H__
