#include <iostream>
#include <unistd.h>
#include <string>
#include <cstdint>

#include <stdio.h>

#include <net_tcp.h>

void usage()
{
    std::cout << "usage: echoserver [ -p interface_port [[-i interface_name] [-a interface_address]] [-d dst_port -t dst_addr]]" << std::endl <<
                 "    interface_port - tcp port which echo server will listen" << std::endl <<
                 "    interface_name - string name of net device which echo server will listen" << std::endl <<
                 "    interface_address - ip address of net device which echo server will listen" << std::endl <<
                 "    dst_port - port of destination host" << std::endl <<
                 "    dst_addr - address of destination host" << std::endl;
}

int main(int argc, char* argv[])
{   
    int opt;
    std::string eth_name;
    std::string eth_addr;
    std::string dst_addr;
    int32_t port = 23;
    int32_t dst_port;
    
    while( (opt = getopt(argc, argv, "a:i:p:d:t:h")) != -1 )
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
            case 'd':
                dst_port = atoi(optarg);
                break;
            case 't':
                dst_addr = std::string(optarg);
                break;
            case 'h':  
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }
    
    std::cout << "port=" << port << " eth_name=" << eth_name << " eth_addr=" << eth_addr << " dst_port=" << dst_port << " dst_addr=" << dst_addr << std::endl;
    
    mmo::CNetTcp net_client;
    
    try
    {
        net_client.subscribe_on_error(
                                        [&net_client] (mmo::CONNECT_ERROR error, const int id)
                                        {
                                            std::cout << "coonnect was not established with peer: " << id << std::endl;
                                        }
        );
        net_client.subscribe_on_data( []( const int& id_abonent, const std::string& message )
                                        {
                                            std::cout << "abonent = " << id_abonent << " message=" << message << std::endl;
                                        }
                                    );
        int id = net_client.connect( dst_addr, dst_port, eth_addr, port, eth_name );
        mmo::OnReceiveAbonent handle = []( const std::string& message )
                                       {
                                            std::cout << "message = " << message << std::endl;
                                       };
        net_client.subscribe_on_data( handle, id );
        net_client.send( id, std::string("abc_client") );
    }
    catch(  const std::string &what )
    {
        std::cout << what << std::endl;
    }

    std::cin.get();
    
}