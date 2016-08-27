#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <net_tcp.h>
#include "test_shared_mutex.h"
#include "../netmmo/ip_tools.h"

using namespace ip_tools;

int main()
{
    std::vector<Interface> interfaces;
    get_interfaces( interfaces );
    
    for( auto it=interfaces.begin(); it!=interfaces.end(); it++ )
    {
        std::cout << "Name: " << it->getName() << std::endl <<
        "Mac: " << it->getMac() << std::endl << 
        "Ip: " << it->getIp() << std::endl << 
        "Broadcast: " << it->getBroadcast() << std::endl << 
        "Subnet: " << it->getSubnet() << std::endl << std::endl;
    }
    
    mmo::CNetTcp net_server;
    
    try
    {
        net_server.subscribe_on_new_connection( []( const int &id_abonent )
                                                {
                                                    std::cout << "new abonent = " << id_abonent << std::endl;
                                                },
                                                50023
                                              );
    }
    catch(  const std::string &what )
    {
        std::cout << what << std::endl;
    }
    
    net_server.subscribe_on_data( []( const int& id_abonent, const std::string& message )
                                  {
                                        std::cout << "abonent = " << id_abonent << " message=" << message << std::endl;
                                  }
                                );
    
    sleep(1);

    
    mmo::CNetTcp net_client;
    try
    {
        int id = net_client.connect( std::string("127.0.0.1"), 50023 );
        
        std::cout << "connect client id = " << id << std::endl;
        //sleep(1);
        //net_client.send( id, "HELLO" );
    }
    catch( const std::string &what )
    {
        std::cout << what << std::endl;
    }

    std::cin.get();

    return 0;
}
