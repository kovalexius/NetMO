#ifdef __linux__ 

#include <sstream>
#include <string>
#include <thread>
#include <map>
#include <iostream>

#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "net_tcp.h"
#include "ip_tools.h"

using namespace mmo;

const uint32_t MAXEVENTS = 65536;
const uint32_t LISTEN_BACKLOG = 1024;

///////////////////////Global functions///////////////////////////

static inline int create_tcp_socket()
{
    struct protoent *proto;
    if( (proto = ::getprotobyname("tcp")) == nullptr )
        throw std::string("Tcp protocol doesn't supproted");
    int sfd;
	//if( (sfd = ::socket( AF_INET, SOCK_STREAM, proto->p_proto )) == -1 )
    if( (sfd = ::socket( AF_INET, SOCK_STREAM|O_NONBLOCK, proto->p_proto )) == -1 )
        throw std::string("Couldn't create connection: new socket doesn't created");

	/*
    int flags = ::fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
        throw std::string("Couldn't take socket flags");
    if( ::fcntl(sfd, F_SETFL, flags|O_NONBLOCK) == -1 )
        throw std::string("Couldn't make socket nonblocked");
	*/
    
    return sfd;
}

static inline void bind( const int sfd, sockaddr_in &bind_addr )
{
    if( ::bind( sfd, (struct sockaddr*)&bind_addr, sizeof(bind_addr) ) == -1 )
    {
        ::close(sfd);
        throw std::string(strerror(errno));
    }
}

static inline void bind_to_interface( const int sfd, 
                                      const std::string& iface_addr, 
                                      const std::string& iface_name, 
                                      const std::string& iface_port)
{
    sockaddr_in bind_addr;
    memset( &bind_addr, 0, sizeof(bind_addr) );
    bind_addr.sin_family = AF_INET;

	if(!iface_addr.empty())
	{
		if(0 == inet_aton(iface_addr.c_str(), &(bind_addr.sin_addr)))
		{
			std::cout << std::string("Invalid interface address. Bind to any address") << std::endl;
			bind_addr.sin_addr.s_addr = INADDR_ANY;
		}
	}
    
    if(!iface_port.empty())
		bind_addr.sin_port = htons(std::atoi(iface_port.c_str()));
    
    // Second layer bind to interface
    int res_devbind = 0;
    if(!iface_name.empty())
    {
		int on = 1;
		setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<void *>(&on), sizeof(int));
		
        res_devbind = ::setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE,  iface_name.data(), iface_name.size());
        if(res_devbind != 0)      
            std::cout << "Failed to set SO_BINDTODEVICE sockopt. errno = " << strerror(errno) << std::endl;
    }
    
    bind(sfd, bind_addr);
}

static inline void connect_to_destination(const int sfd, const std::string& dest_address, const std::string& dest_port )
{
    // connecting to destination
    struct sockaddr_in dest_addr;
    memset( &dest_addr, 0, sizeof(dest_addr) );
    dest_addr.sin_family = AF_INET;
    if( ::inet_aton(dest_address.c_str(), &(dest_addr.sin_addr) ) == 0 )
        throw std::string(strerror(errno));
	if(!dest_port.empty())
		dest_addr.sin_port = htons(std::atoi(dest_port.c_str()));
    int code = ::connect( sfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr) );
    if(code)
    {
        if( EINPROGRESS != errno )
        {
            ::close(sfd);
            throw std::string(strerror(errno));
        }
    }
}


////////////////////////////////////////////////////
////////////////// class CNetTcp ///////////////////
////////////////////////////////////////////////////

CNetTcp::CNetTcp() : m_data_thread_isrun(false)
{
    m_efd = epoll_create(1);
    
    //self pipe initializing
    try
    {
        m_spfd = create_tcp_socket();
        if( listen( m_spfd, LISTEN_BACKLOG ) == -1 )
            throw std::string(strerror(errno));
        add_in_to_epoll(m_spfd);
    }
    catch(...)
    {
        std::cout << "self-pipe not initialized" << std::endl;
    }
}

CNetTcp::~CNetTcp()
{
    shutdown(m_efd, SHUT_RDWR);
    m_data_thread_isrun.store( false );
    if(m_data_thread.joinable())
    {
        //char c;
        //write( m_spfd, &c, sizeof(c) );       // sending self-pipe event to exit thread
        std::cout << "destructor()" << std::endl;
        m_data_thread.join();
    }
    
    close(m_efd);
    close(m_lsfd);
    close(m_spfd);
    
    for( auto it = m_connect_socks.begin(); it != m_connect_socks.end(); it++ )
        close(*it);
    for( auto it = m_data_socks.begin(); it != m_data_socks.end(); it++ )
        close(*it);
}

void CNetTcp::add_in_to_epoll( const int sfd )
{
    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
    if( ::epoll_ctl( m_efd, EPOLL_CTL_ADD, sfd, &event ) == -1 )
    {
        ::close(sfd);
        throw std::string(strerror(errno));
    }
}

void CNetTcp::add_out_to_epoll( const int sfd )
{
    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLOUT | EPOLLHUP | EPOLLRDHUP;
    if( ::epoll_ctl( m_efd, EPOLL_CTL_ADD, sfd, &event ) == -1 )
    {
        ::close(sfd);
        throw std::string(strerror(errno));
    }
}

void CNetTcp::mod_in_epoll(const int sfd)
{
    struct epoll_event event;
    event.data.fd = sfd;
    event.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
    if( ::epoll_ctl( m_efd, EPOLL_CTL_MOD, sfd, &event ) == -1 )
    {
        ::close(sfd);
        throw std::string(strerror(errno));
    }
}

void CNetTcp::del_from_epoll( const int sfd )
{
    if(::epoll_ctl(m_efd, EPOLL_CTL_DEL, sfd, NULL) == -1)
    {
        ::close(sfd);
        throw std::string(strerror(errno));
    }
}

int CNetTcp::_connect( const std::string& dst_address, 
					   const std::string& dst_port,
                       const std::string& iface_addr, 
					   const std::string& iface_name, 
                       const std::string& iface_port)
{
    int sfd = create_tcp_socket();
    
    {
    std::lock_guard<std::mutex> lk(m_connect_mutex);
    m_connect_socks.insert(sfd);
    }

	bind_to_interface( sfd, iface_addr, iface_name, iface_port );

    add_out_to_epoll(sfd);
    create_data_thread();
    connect_to_destination(sfd, dst_address, dst_port);

    return sfd;
}

int CNetTcp::connect( const std::string& dst_address, 
					  const std::string& dst_port)
{
    return _connect(dst_address, dst_port, std::string(), std::string(), std::string());
}

int CNetTcp::connect( const std::string& dst_address, 
					  const std::string& dst_port,
                      const std::string& iface_addr, 
					  const std::string& iface_port
                    )
{
    return _connect( dst_address, dst_port, iface_addr, std::string(), iface_port );
}

int CNetTcp::connect( const std::string& dst_address, 
					  const std::string& dst_port,
                      const std::string& iface_addr, 
					  const std::string& iface_port,
                      const std::string& iface_name = std::string()
                    )
{
    return _connect( dst_address, dst_port, iface_addr, iface_name, iface_port );
}

bool CNetTcp::send( const int& sock_id, const std::string& message )
{
    if( ::send( sock_id, message.data(), message.size(), 0 ) == -1 )
        throw std::string( strerror(errno) );
    
    return true;
}

void CNetTcp::_subscribe_on_new_connection( OnConnect connect_handle, 
                                            const std::string& iface_addr, 
                                            const std::string& iface_name, 
                                            const std::string& iface_port)
{
    m_connect_handle = connect_handle;
    m_lsfd = create_tcp_socket();

	bind_to_interface(m_lsfd, iface_addr, iface_name, iface_port);
    
    if(listen( m_lsfd, LISTEN_BACKLOG ) == -1)
	{
        throw std::string(std::string("listen() failed: \'") + std::string(strerror(errno)) + "\' m_lsfd: \'" + std::to_string(m_lsfd) + "\'");
	}
   
    add_in_to_epoll(m_lsfd);
    create_data_thread();
}

void CNetTcp::subscribe_on_new_connection(OnConnect connect_handle)
{   
    _subscribe_on_new_connection(connect_handle, std::string(), std::string(), std::string());
}

void CNetTcp::subscribe_on_new_connection( OnConnect connect_handle, const std::string& iface_port )
{
    _subscribe_on_new_connection( connect_handle, std::string(), std::string(), iface_port );
}

void CNetTcp::subscribe_on_new_connection( OnConnect connect_handle, 
                                           const std::string& iface_addr, 
                                           const std::string& iface_port)
{
    _subscribe_on_new_connection( connect_handle, iface_addr, std::string(), iface_port );
}

void CNetTcp::subscribe_on_new_connection( OnConnect connect_handle, 
                                           const std::string& iface_addr, 
                                           const std::string& iface_name, 
                                           const std::string& iface_port )
{
    _subscribe_on_new_connection(connect_handle, iface_addr, iface_name, iface_port);
}

void CNetTcp::subscribe_on_data( OnReceive receive_handle )
{
    m_receive_handle = receive_handle;
    create_data_thread();
}

void CNetTcp::subscribe_on_data( OnReceiveAbonent receive_handle, const int& id_abonent )
{
    m_receive_handles_map.insert( { id_abonent, receive_handle } );
    create_data_thread();
}

void CNetTcp::subscribe_on_data( OnReceive receive_handle, const int& id_abonent)
{
    m_ex_receive_handles_map.insert( { id_abonent, receive_handle } );
    create_data_thread();
}

void CNetTcp::subscribe_on_error( OnError error_handle )
{
    m_error_handle = error_handle;
    create_data_thread();
}

void CNetTcp::create_data_thread()
{
    if( !m_data_thread_isrun.load() )
    {
        m_data_thread_isrun.store( true );
        m_data_thread = std::thread( &CNetTcp::data_thread, this );
    }
}

void CNetTcp::data_thread()
{
    struct epoll_event *events;
    events = (epoll_event*)calloc( MAXEVENTS, sizeof(epoll_event) );
    while( m_data_thread_isrun )
    {   
        int n = epoll_wait( m_efd, events, MAXEVENTS, -1 );
        for(int i = 0; i < n; i++)
        {
            std::string str_event;
            ip_tools::epoll_events_to_str(events[i].events, str_event);   //print
            //std::cout << "events: " << str_event << std::endl;
            
            if(events[i].data.fd == m_spfd)        // self-pipe event
                break;
            
            if(events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP )        // somebody was hanged up, aka closed peer
            {
                _process_hup_events(events[i]);
                continue;
            }
            if(events[i].events & EPOLLIN)         // ready to read or accept connections
                _process_input_events(events[i]);
            if(events[i].events & EPOLLOUT )        // ready to write or connect was established
                _process_output_events(events[i]);
        }
        sleep(1);
        std::cout << "thread is run: " << m_data_thread_isrun << std::endl;
    }
    free( events );
}

void CNetTcp::_process_input_events( const struct epoll_event &evnt )
{
    int cur_sock = evnt.data.fd;
    // new connection on listen socket
    if( cur_sock == m_lsfd )
    {
        struct sockaddr local;
        socklen_t addrlen = sizeof( local );
        int data_sock = accept(m_lsfd, (struct sockaddr *)&local, &addrlen);
        if( data_sock >= 0 )
        {
            add_in_to_epoll( data_sock );
            std::lock_guard<std::mutex> lk(m_data_mutex);
            m_data_socks.insert( data_sock );
        }
        if( m_connect_handle )
            m_connect_handle( data_sock );
    }
    // socket receive data
    else if( m_data_socks.find(cur_sock) != m_data_socks.end() )
    {
        std::string msg;
        auto recv_size = ::read( cur_sock, m_recv_buf, MESSAGE_MAX_LEN );
        if( recv_size > 0 )
            msg.insert( 0, m_recv_buf, recv_size );
        
        if( m_receive_handle )
            m_receive_handle(cur_sock, msg );
        auto receive_handle_it = m_receive_handles_map.find(cur_sock);
        if( receive_handle_it != m_receive_handles_map.end() )
            receive_handle_it->second( msg );
        auto receive_ex_handle_it = m_ex_receive_handles_map.find(cur_sock);
        if( receive_ex_handle_it != m_ex_receive_handles_map.end() )
            receive_ex_handle_it->second( cur_sock, msg );
    }
}

void CNetTcp::_process_output_events( const struct epoll_event &evnt )
{
    int cur_sock = evnt.data.fd;
    // socket made connect
    auto it = m_connect_socks.find(cur_sock);
    if( it != m_connect_socks.end() )
    {
        mod_in_epoll(cur_sock);
        std::lock(m_connect_mutex, m_data_mutex);
        m_connect_socks.erase(it);
        m_data_socks.insert(cur_sock);
        m_connect_mutex.unlock();
        m_data_mutex.unlock();
    }
}

void CNetTcp::_process_hup_events(const struct epoll_event &evnt)
{
    int cur_sock = evnt.data.fd;
    auto it = m_connect_socks.find(cur_sock);
    if( it != m_connect_socks.end() )
    {
        del_from_epoll(cur_sock);
        ::close(cur_sock);
        m_error_handle(CONNECT_ERROR::DISCONNECTED, cur_sock);
        std::lock_guard<std::mutex> lk(m_connect_mutex);
        m_connect_socks.erase(it);
        return;
    }
    it = m_data_socks.find(cur_sock);
    if( it != m_data_socks.end() )
    {
        del_from_epoll(cur_sock);
        ::close(cur_sock);
        m_error_handle(CONNECT_ERROR::DISCONNECTED, cur_sock);
        std::lock_guard<std::mutex> lk(m_data_mutex);
        m_data_socks.erase(it);
        return;
    }
}

#endif
