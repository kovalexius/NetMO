cmake_minimum_required(VERSION 2.8)

project(NetMMO)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions( -Wall -Wl,--no-as-needed -pthread -lpthread -std=gnu++11 )
elseif (MSVC)
	## add appropriate keys
endif()

source_group(src)

if(CMAKE_COMPILER_IS_GNUCXX)
	add_library( netmmo
		src/netmmo/net_tcp_linux.cpp
		src/netmmo/net_tcp.h
		)
elseif(MSVC)
	add_library( netmmo
		src/netmmo/net_tcp_windows.cpp
		src/netmmo/net_tcp.h
		)
endif()

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
