#include <iostream>
#include <unistd.h>
#include <string>
#include <cstdint>

#include <stdio.h>

#include <net_tcp.h>

void usage()
{
    std::cout << "usage: echoserver [ -p listen_port [[-i interface_name] [-a interface_address]] ]" << std::endl <<
                 "    listen_port - tcp port which echo server will listen" << std::endl <<
                 "    interface_name - string name of net device which echo server will listen" << std::endl <<
                 "    interface_address - ip address of net device which echo server will listen" << std::endl;
}

int main(int argc, char* argv[])
{
    int opt;
    std::string eth_name;
    std::string eth_addr;
    int32_t port = 23;
    
    while( (opt = getopt(argc, argv, "a:i:p:h")) != -1 )
    {
        switch(opt)
        {
            case 'i':
                eth_name = std::string(optarg);
                break;
            case 'a':
                eth_addr = std::string(optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'h':  
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }
    
    std::cout << "port=" << port << " eth_name=" << eth_name << " eth_addr=" << eth_addr << std::endl;
    
    mmo::CNetTcp net_server;
    try
    {
        net_server.subscribe_on_error(
                                        [&net_server] (mmo::CONNECT_ERROR error, const int id)
                                        {
                                            std::cout << "connect was hanged up by peer: " << id << std::endl;
                                        }
        );
        net_server.subscribe_on_new_connection( [&net_server]( const int &id_abonent ) 
                                                {
                                                    std::cout << "new abonent = " << id_abonent << std::endl;
                                                    mmo::OnReceive handle = [&]( const int id, const std::string& message )
                                                                            {
                                                                                std::cout << "message: " << message << " from abonent: " << id << std::endl;
                                                                               
                                                                                try
                                                                                {
                                                                                    net_server.send( id, message );
                                                                                }
                                                                                catch(const std::string &what)
                                                                                {
                                                                                    std::cout << what <<std::endl;
                                                                                }
                                                                            };
                                                    net_server.subscribe_on_data( handle, id_abonent );
                                                    try
                                                    {
                                                        //net_server.send( id_abonent, std::string("abc_server") );
                                                    }
                                                    catch( const std::string &what )
                                                    {
                                                        std::cout << what << std::endl;
                                                    }
                                                },
                                                eth_addr,
                                                eth_name,
                                                port
                                              );
        
    }
    catch(  const std::string &what )
    {
        std::cout << what << std::endl;
    }
    
    std::cin.get();
}