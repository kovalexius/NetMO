#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>

#include <string>
#include <vector>

namespace ip_tools
{
    class Interface
    {
    public:
        void setName( const char *name );
        void setMac( struct ifaddrs *current );
        void setIp( struct sockaddr *addr );
        void setBroadcast( struct sockaddr *address );
        void setSubnet( struct sockaddr *address );
        void setStatus( int16_t status );
        void setType( int16_t type );
        std::string getName();
        std::string getMac();
        
        std::string getIp();
        std::string getBroadcast();
        std::string getSubnet();
        
        std::string getZeroIp();
        std::string getZeroBroadcast();
        std::string getZeroSubnet();
    private:
        int16_t m_type;
        int16_t m_status;
        std::string m_name;
        std::string m_mac;
        std::string m_ip;
        std::string m_broadcast;
        std::string m_subnet;
        
        std::string m_ip_zero;
        std::string m_broad_zero;
        std::string m_subnet_zero;
    };
    
    void get_interfaces( std::vector<Interface> &interfaces );
    
    void epoll_events_to_str( const uint32_t events, std::string &msg );
}