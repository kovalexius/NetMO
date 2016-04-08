#ifdef __linux__ 

#include <sstream>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <ifaddrs.h>
#include <map>
#include <sys/epoll.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include "net_tcp.h"

using namespace mmo;

const uint32_t MAXEVENTS = 65536;
const uint32_t LISTEN_BACKLOG = 50;

void get_interfaces()
{
    // Head of the interface address linked list
    ifaddrs* ifap = NULL;

    if( getifaddrs(&ifap) == -1 )
        throw std::string( "Can't get interfaces" );

    //std::map< std::string, std::pair<struct sockaddr*, struct sockaddr*> > if_to_addr;

    ifaddrs* current = ifap;
    while( current != NULL )
    {
        if( (current->ifa_addr!=NULL) && (current->ifa_addr->sa_family==AF_INET) && (current->ifa_flags & IFF_UP) )
        {	
            current->ifa_addr;
            current->ifa_netmask;	
        }
        current = current->ifa_next;	
    }

    ::freeifaddrs(ifap);
    	ifap = NULL;
}

static inline int create_tcp_socket()
{
    struct protoent *proto;
    if( (proto = ::getprotobyname("tcp")) == nullptr )
        throw std::string("Tcp protocol doesn't supproted");
    int sfd;
    if( (sfd = ::socket( AF_INET, SOCK_STREAM, proto->p_proto )) == -1 )
        throw std::string("Couldn't create connection: new socket doesn't created");

    int flags = ::fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
        throw std::string("Couldn't take socket flags");
    if( ::fcntl(sfd, F_SETFL, flags|O_NONBLOCK) == -1 )
        throw std::string("Couldn't make socket nonblocked");
    
    return sfd;
}

static inline void bind_to_any_interface( const int sfd )
{
    // Bind to any interface
    sockaddr_in bind_addr;
    memset( &bind_addr, 0, sizeof(bind_addr) );
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    if( ::bind( sfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr) ) == -1 )
    {
        ::close(sfd);
        throw std::string("Bind error");
    }
}

static inline void bind_to_any_interface( const int sfd, const uint16_t interface_port )
{
    // Bind to any interface
    sockaddr_in bind_addr;
    memset( &bind_addr, 0, sizeof(bind_addr) );
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons( interface_port );
    if( ::bind( sfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr) ) == -1 )
    {
        ::close(sfd);
        throw std::string("Bind error");
    }
}

static inline void bind_to_interface( const int sfd, const std::string& interface_addr, const uint16_t interface_port )
{
    // Bind to appropriate interface (all packets will be transport through this intreface)
    sockaddr_in bind_addr;
    memset( &bind_addr, 0, sizeof(bind_addr) );
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons( interface_port );
    if( inet_aton( interface_addr.c_str(), &(bind_addr.sin_addr) ) == 0 )
        throw std::string("Invalid interface address");
    if( ::bind( sfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr) ) == -1 )
    {
        throw std::string("Bind error");
        close(sfd);
    }
}

static inline void connect_to_destination(const int sfd, const std::string& dest_address, const uint16_t dest_port )
{
    // connecting to destination
    struct sockaddr_in dest_addr;
    memset( &dest_addr, 0, sizeof(dest_addr) );
    dest_addr.sin_family = AF_INET;
    if( ::inet_aton(dest_address.c_str(), &(dest_addr.sin_addr) ) == 0 )
        throw std::string("Invalid destination address");
    dest_addr.sin_port = htons(dest_port);
    int code = ::connect( sfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr) );
    if( code )
    {
        if( EINPROGRESS != errno )
        {
            ::close(sfd);
            throw std::string("Can't connect");
        }
    }
}


///////////// class CNetTcp //////////////

void CNetTcp::add_to_epoll( const int sfd )
{
    // adding socket descriptor to epoll
    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLPRI | EPOLLERR | EPOLLONESHOT | EPOLLWAKEUP;
    if( ::epoll_ctl( m_efd, EPOLL_CTL_ADD, sfd, &event ) == -1 )
    {
        ::close(sfd);
        throw std::string("Can't add file descriptor to epoll");
    }
}

CNetTcp::CNetTcp() : m_connect_handle(nullptr),
                     m_error_handle(nullptr),
                     m_receive_handle(nullptr),
                     m_data_thread_isrun(false),
                     m_connect_thread_isrun(false),
                     m_data_thread(nullptr),
                     m_connect_thread(nullptr)
{
        m_efd = epoll_create(1);
}

int CNetTcp::connect( const std::string& dest_address, const uint16_t dest_port )
{
    int sfd = create_tcp_socket();
    bind_to_any_interface( sfd );
    add_to_epoll(sfd);
    create_connect_thread();
    connect_to_destination( sfd, dest_address, dest_port );
    m_connect_socks.insert(sfd);

    return sfd;
}

int CNetTcp::connect( const std::string& interface_addr, const uint16_t interface_port,
                           const std::string& dest_address, const uint16_t dest_port )
{
    int sfd = create_tcp_socket();
    bind_to_interface( sfd, interface_addr, interface_port );
    add_to_epoll(sfd);
    create_connect_thread();
    connect_to_destination( sfd, dest_address, dest_port );
    m_connect_socks.insert(sfd);

    return sfd;
}

bool CNetTcp::send( const int& sock_id, const std::string& message )
{
    if( ::send( sock_id, message.data(), message.size(), 0 ) == -1 )
    {
        std::ostringstream ostr;
        ostr << "Can't send errorcode=" << errno;
        throw ostr.str();
    }
    
    return true;
}

void CNetTcp::subscribe_on_new_connection( OnConnect connect_handle )
{
    m_connect_handle = connect_handle;
    m_lsfd = create_tcp_socket();
    bind_to_any_interface( m_lsfd );
    if( listen( m_lsfd, LISTEN_BACKLOG ) == -1 )
        throw std::string("Can't listen");
    add_to_epoll(m_lsfd);
    create_data_thread();
}

void CNetTcp::subscribe_on_new_connection( OnConnect connect_handle, const uint16_t interface_port )
{
    m_connect_handle = connect_handle;
    m_lsfd = create_tcp_socket();
    bind_to_any_interface( m_lsfd, interface_port );
    if( listen( m_lsfd, LISTEN_BACKLOG ) == -1 )
        throw std::string("Can't listen");
    add_to_epoll(m_lsfd);
    create_data_thread();
}

CNetTcp::~CNetTcp()
{
    for( auto it = m_connect_socks.begin(); it != m_connect_socks.end(); it++ )
        close( *it );
    for( auto it = m_data_socks.begin(); it != m_data_socks.end(); it++ )
        close( *it );
    if( m_data_thread )
    {
        m_data_thread_isrun = false;
        m_data_thread->join();
        delete m_data_thread;
    }
}

void CNetTcp::create_data_thread()
{
    if( m_data_thread_isrun == false )
    {
        m_data_thread_isrun = true;
        if( !m_data_thread )
            m_data_thread = new std::thread( mmo::invoke<CNetTcp, &CNetTcp::data_thread>, this );
    }
}

void CNetTcp::create_connect_thread()
{
    
}

void CNetTcp::data_thread()
{
    struct epoll_event *events;
    events = (epoll_event*)calloc( MAXEVENTS, sizeof(epoll_event) );
    while( true )
    {   
        int n = epoll_wait( m_efd, events, MAXEVENTS, -1 );
        std::cout << "listen thread  n="<< n << std::endl;
        for( int i = 0; i < n; i++ )
        {
            
            if ( (events[i].events & EPOLLERR) ||
               ( events[i].events & EPOLLHUP) ||
               ( !(events[i].events & EPOLLIN)))
            {
                close (events[i].data.fd);
                continue;
            }
            int cur_sock = events[i].data.fd;
            if( cur_sock == m_lsfd )
            // new connection on listen socket
            {
                struct sockaddr local;
                socklen_t addrlen = sizeof( local );
                int data_sock = accept( m_lsfd, (struct sockaddr *) &local, &addrlen );
                if( data_sock >= 0 )
                    m_data_socks.insert( data_sock );
                if( m_connect_handle )
                    m_connect_handle( data_sock );
            }
            else if( m_connect_socks.find(cur_sock) != m_connect_socks.end() )
            // socket made connect
            {
                std::cout << "connect = " << cur_sock << std::endl;
            }
            else if( m_data_socks.find(cur_sock) != m_data_socks.end() )
            // socket receive data
            {
                std::cout << "data = " << cur_sock << std::endl;
            }
        }
        
        // exit this thread
        if( m_data_thread_isrun == false )
            break;
    }
}

#endif
