#include <iostream>
#include <string>
#include <unistd.h>
#include "../netmmo/net_tcp.h"
#include "test_shared_mutex.h"

int main()
{
    test_sh_mutex();
    
    std::cout << "Press any key" << std::endl;
    std::cin.get();
    
    mmo::CNetTcp net_;
    /*
    net_.subscribe_on_new_connection(
        []( const uint64_t &id_abonent )
        {
            std::cout << "new abonent = " << id_abonent << std::endl;
        },
        50023
    );
    sleep(1);
    */
    try
    {
        int id = net_.connect( std::string("127.0.0.1"), (uint16_t)50023 );
        //net_.send( id, "XYU" );
    }
    catch( const std::string &what )
    {
        std::cout << what << std::endl;
    }

    std::cin.get();

    return 0;
}
