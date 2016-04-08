#ifdef _WIN32

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "net_tcp.h"

using namespace mmo;

CNetTcp::CNetTcp() : m_is_running(false)
{
	//m_fd = epoll_create(1);
	m_is_running = false;
	m_thread = nullptr;

	WORD w = MAKEWORD(2, 2);
	WSADATA wsadata;
	WSAStartup(w, &wsadata);
}

CNetTcp::~CNetTcp()
{

}

uint64_t CNetTcp::connect( const std::string& address, const uint16_t port )
{
	struct protoent* proto;
	if ((proto = getprotobyname("tcp")) == nullptr)
	{
		//FormatMessage()
		//WSAGetLastError()
		throw std::string("Tcp protocol doesn't supported");
	}

	SOCKET sfd;
	sfd = socket( AF_INET, SOCK_STREAM, proto->p_proto);
	if (sfd == INVALID_SOCKET)
	{
		//WSAGetLastError()
		//FormatMessage()
		throw std::string("Can't create socket");
	}

	// Биндим к любому интерфейсу
	sockaddr_in bind_addr;
	memset(&bind_addr, 0, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	if(::bind(sfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR)
	{
		closesocket(sfd);
		throw std::string("Bind error");
	}

	// Устанавливаем соединение с указанным адресом
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	if(::InetPton(AF_INET, address.c_str(), &(dest_addr.sin_addr)) == 0)
		throw std::string("Invalid destination address");
	dest_addr.sin_port = htons(port);
	if (::connect(sfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
	{
		closesocket(sfd);
		throw std::string("Can't connect");
	}

	return sfd;
}

#endif
