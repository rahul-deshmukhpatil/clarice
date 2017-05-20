#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h>
#include <sys/un.h>
#include <stdint.h>

namespace infra
{
	class Socket
	{
		public:
			Socket();
			Socket(const struct sockaddr_in *serverAddress, int32_t type);
			~Socket();
			int32_t readSocket(char *buffer, uint32_t size);

			int32_t socketfd() const
			{
				return _sockfd;
			}

		private:	
			int32_t _sockfd;
			bool	_isActive;
	};
}

#endif // __SOCKET_H__
