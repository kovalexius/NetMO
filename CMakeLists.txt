cmake_minimum_required(VERSION 2.8)

project(NetMMO)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions( -Wall -Wl,--no-as-needed -pthread -lpthread -std=c++11 )
elseif (MSVC)
    ## add appropriate keys
endif()

source_group(src)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_library( netmmo
        src/netmmo/shared_mutex.h
        src/netmmo/net_tcp_linux.cpp
        src/netmmo/net_tcp.h
        src/netmmo/ip_tools.cpp
        src/netmmo/ip_tools.h
        )
elseif(MSVC)
    add_library( netmmo
        src/netmmo/shared_mutex.h
        src/netmmo/net_tcp_windows.cpp
        src/netmmo/net_tcp.h
        )
endif()

configure_file( ./delete_ip_link.sh ${CMAKE_CURRENT_BINARY_DIR}/delete_ip_link.sh COPYONLY )
configure_file( ./make_ip_link.sh ${CMAKE_CURRENT_BINARY_DIR}/make_ip_link.sh COPYONLY )

## testnet module
add_executable( testnet
                src/tests/main.cpp
                src/tests
              )
include_directories( 	./ 
                        ./src/netmmo/
                    )            
if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries( testnet netmmo "-pthread")
elseif(MSVC)
	target_link_libraries( testnet netmmo ws2_32 )
endif()


#test shared mutex module
add_executable( test_sh_mutex
                src/netmmo/shared_mutex.h
                src/tests/test_shared_mutex.h
                src/tests/test_shared_mutex.cpp
                )
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries( test_sh_mutex "-pthread")
endif()

#getip module
add_executable( getip
                src/getip.cpp
              )

              
#echoserver module
add_executable( echoserver
                src/tests/echoserver/echoserver.cpp
              )
include_directories(    ./ 
                        ./src/netmmo/
                   )  
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries( echoserver netmmo "-pthread")
elseif(MSVC)
    target_link_libraries( echoserver netmmo ws2_32 )
endif()


#echoclient module
add_executable( echoclient
                src/tests/echoserver/echoclient.cpp
              )
include_directories(    ./ 
                        ./src/netmmo/
                   )  
if(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries( echoclient netmmo "-pthread")
elseif(MSVC)
    target_link_libraries( echoclient netmmo ws2_32 )
endif()