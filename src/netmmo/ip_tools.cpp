#include "ip_tools.h"

#ifdef __linux__
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <string.h>
    
    #include <linux/version.h>
    
    // Match Linux to FreeBSD
    #define AF_LINK AF_PACKET
#else
    #include <net/if_dl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <cstdio>
#include <iostream>

#include <map>

namespace ip_tools
{
    
void Interface::setName( const char *name )
{
    m_name = std::string( name );
}

void Interface::setMac( struct ifaddrs *current )
{
#ifdef __linux__
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy( ifr.ifr_name, current->ifa_name );
    ioctl(fd, SIOCGIFHWADDR, &ifr);
    close(fd);
    uint8_t* MAC = reinterpret_cast<uint8_t*>(ifr.ifr_hwaddr.sa_data);
#else  // Posix/FreeBSD/Mac OS
    sockaddr_dl* sdl = (struct sockaddr_dl *)current->ifa_addr;
    uint8_t* MAC = reinterpret_cast<uint8_t*>(LLADDR(sdl));
#endif
    
    char buffer[20];
    snprintf( buffer, 20, "%02x:%02x:%02x:%02x:%02x:%02x", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5] );
    m_mac = std::string( buffer );
}

void Interface::setStatus( int16_t status )
{
    m_status = status;
}

void Interface::setType( int16_t type )
{
    m_type = type;
}

void Interface::setIp( struct sockaddr *address )
{
    struct sockaddr_in *addr;
    addr = reinterpret_cast<struct sockaddr_in*>(address);
    m_ip = std::string( inet_ntoa(addr->sin_addr) );
}

void Interface::setBroadcast( struct sockaddr *address )
{
    struct sockaddr_in *addr;
    addr = reinterpret_cast<struct sockaddr_in*>(address);
    m_broadcast = std::string( inet_ntoa(addr->sin_addr) );
}

void Interface::setSubnet( struct sockaddr *address )
{
    struct sockaddr_in *addr;
    addr = reinterpret_cast<struct sockaddr_in*>(address);
    m_subnet = std::string( inet_ntoa(addr->sin_addr) );
}

std::string Interface::getName()
{
    return m_name;
}

std::string Interface::getMac()
{
    return m_mac;
}

std::string Interface::getIp()
{
    return m_ip;
}

std::string Interface::getBroadcast()
{
    return m_broadcast;
}

std::string Interface::getSubnet()
{
    return m_subnet;
}

std::string Interface::getZeroIp()
{
    return m_ip_zero;
}

std::string Interface::getZeroBroadcast()
{
    return m_broad_zero;
}

std::string Interface::getZeroSubnet()
{
    return m_subnet_zero;
}

void get_interfaces( std::vector<Interface> &result )
{
    result.clear();
    
    // Head of the interface address linked list
    ifaddrs* ifap = NULL;

    if( getifaddrs(&ifap) == -1 )
        throw std::string( "Can't get interfaces" );

    std::map<std::string, Interface> interfaces;
    
    ifaddrs* current = ifap;
    while( current != NULL )
    {   
        Interface *cur_iface;
        
        auto it = interfaces.find( std::string(current->ifa_name) );
        if( it != interfaces.end() )
            cur_iface = &(it->second);
        else
        {
            auto new_pair = interfaces.insert( std::make_pair( std::string(current->ifa_name), Interface() ) );
            if( new_pair.second )
                cur_iface = &(new_pair.first->second);
            else
                cur_iface = nullptr;
        }
        
        cur_iface->setName( current->ifa_name );
        cur_iface->setStatus( current->ifa_flags );
        cur_iface->setType( current->ifa_addr->sa_family );
        
        if( (current->ifa_addr!=NULL) && (current->ifa_flags & IFF_UP) )
        {   
            switch( current->ifa_addr->sa_family )
            {
                case AF_INET:
                    cur_iface->setIp( current->ifa_addr );
                    cur_iface->setBroadcast( current->ifa_dstaddr );
                    cur_iface->setSubnet( current->ifa_netmask );
                    break;
                case AF_LINK:
                    cur_iface->setMac( current );
                    break;
            }
        }
        current = current->ifa_next;    
    }

    ::freeifaddrs(ifap);
    ifap = NULL;
    
    for( auto it=interfaces.begin(); it!=interfaces.end(); it++ )
        result.push_back( it->second );
}

void epoll_events_to_str( const uint32_t events, std::string &msg )
{
    msg.clear();
    if( EPOLLOUT & events )
        msg += "EPOLLOUT ";
    if( EPOLLIN & events )
        msg += "EPOLLIN ";
    if( EPOLLRDHUP & events )
        msg += "EPOLLRDHUP ";
    if( EPOLLPRI & events )
        msg += "EPOLLPRI ";
    if( EPOLLERR & events )
        msg += "EPOLLERR ";
    if( EPOLLHUP & events )
        msg += "EPOLLHUP ";
    if( EPOLLET & events )
        msg += "EPOLLET ";
    if( EPOLLONESHOT & events )
        msg += "EPOLLONESHOT ";
    if( EPOLLWAKEUP & events )
        msg += "EPOLLWAKEUP ";
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
    if( EPOLLEXCLUSIVE & events )
        msg += "EPOLLEXCLUSIVE ";
#endif
}

}