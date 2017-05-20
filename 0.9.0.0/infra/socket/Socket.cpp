#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <string>
#include <stdexcept>  // std::invalid_argument 

#include <unistd.h> // close
#include <fcntl.h> // get/set fcntl
#include "infra/socket/Socket.h"

using namespace infra;

Socket::Socket()
	: _sockfd(-1) 
	, _isActive(false)
{	
}

/*
 *	Socket: create socket
 *
 *	@input: 
 *			@sockaddr_in: Address of socket
 *			@type		: SOCk_DGRAM/SOCK_STREAM 
 *
 *
 */
Socket::Socket(const struct sockaddr_in *serverAddress, int32_t type)
{
	in_addr_t interfaceIP = 0;
	int set = 1;
	int unset = 0;
  
  	//create socket to join multicast group on
	_sockfd = socket(AF_INET, type, IPPROTO_UDP);
	if(_sockfd < 0)
	{
		char errorBuff[512];
		snprintf(errorBuff, sizeof(errorBuff), "Client socket could not be created to an address %s:%hu", inet_ntoa(serverAddress->sin_addr), ntohs(serverAddress->sin_port));
		std::string error = errorBuff;
		throw std::invalid_argument(error);
	}

	//set reuse port to on to allow multiple binds per host
	int retval = setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set));
	if(retval < 0)
	{
		std::string error =  "Could not setsockopt to SO_REUSEADDR";
		throw std::invalid_argument(error);
	}

	//set the interface
	{
		struct ifconf conf;
		char data[4096];
		struct ifreq *ifr;
		char addrbuf[1024];

		conf.ifc_len = sizeof(data);
		conf.ifc_buf = (caddr_t) data;
		if (ioctl(_sockfd , SIOCGIFCONF, &conf) < 0) 
		{
			std::string error =  "Could not cal SIOCGIFCONF";
			throw std::invalid_argument(error);
		}

		int i = 0;
		ifr = (struct ifreq*)data;
		while ((char*)ifr < data+conf.ifc_len) 
		{
			switch (ifr->ifr_addr.sa_family) 
			{
				case AF_INET:
					++i;
					//printf("%d. %s : %s\n", i, ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr, addrbuf, sizeof(addrbuf)));
					if(std::string("lo") == ifr->ifr_name)
					{
						interfaceIP = ((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr.s_addr;
					}
					break;
#if 0
				case AF_INET6:
					++i;
					printf("%d. %s : %s\n", i, ifr->ifr_name, inet_ntop(ifr->ifr_addr.sa_family, &((struct sockaddr_in6*)&ifr->ifr_addr)->sin6_addr, addrbuf, sizeof(addrbuf)));
					break;
#endif
			}
			ifr = (struct ifreq*)((char*)ifr + sizeof(*ifr));
		}
	}

	int flags = fcntl(_sockfd, F_GETFL, 0);
    if (flags < 0)
	{
		std::string error = "Could not get the socket flags";
	}

	flags = flags|O_NONBLOCK;
	if(fcntl(_sockfd, F_SETFL, flags) != 0) 
	{
		std::string error = "Could not set the socket flag O_NONBLOCK";
		throw std::invalid_argument(error);
	}

	char* mc_addr_str;            /* multicast IP address */
	struct sockaddr_in mc_addr;   /* socket address structure */
	unsigned short mc_port;       /* multicast port */

	/* construct a multicast address structure */
	memset(&mc_addr, 0, sizeof(mc_addr));
	mc_addr.sin_family      = AF_INET;
	mc_addr.sin_addr.s_addr = /*htonl(INADDR_ANY);*/serverAddress->sin_addr.s_addr;
	mc_addr.sin_port        = serverAddress->sin_port;

	/* bind to multicast address to socket */
	retval = bind(_sockfd, (struct sockaddr *) &mc_addr, sizeof(mc_addr));
	if(retval < 0) 
	{
		std::string error = "Could not bind the socket to mc addr";
		throw std::invalid_argument(error);
	}

	/* construct an IGMP join request structure */
	struct ip_mreq mc_req;        /* multicast request structure */
	mc_req.imr_multiaddr.s_addr = serverAddress->sin_addr.s_addr;
	//mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
	mc_req.imr_interface.s_addr = interfaceIP;

	/* send an ADD MEMBERSHIP message via setsockopt */
	retval = setsockopt(_sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mc_req, sizeof(mc_req));
	if( retval < 0) 
	{
		std::string error = "Could not set the socket option for mc addr membership";
		throw std::invalid_argument(error);
	}

	_isActive = true;
}

int32_t Socket::readSocket(char *buffer, uint32_t size)
{
	return recv(_sockfd, buffer, size, MSG_DONTWAIT);
}

Socket::~Socket()
{
	_isActive = false;
	close(_sockfd);
}

