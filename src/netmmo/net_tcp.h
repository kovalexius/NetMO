#ifndef NETTCP_H
#define NETTCP_H 

#include <exception>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <utility>
#include <vector>
#include <set>
#include "shared_mutex.h"

namespace mmo
{
    template<typename T, void (T::*ptr)()>
    void invoke(T *obj)
    {
        (obj->*ptr)();
    }

    enum CONNECT_ERROR
    {
        DISCONNECTED = 1
    };

    typedef void ( * OnReceive ) (const uint64_t&, const std::string&);
    typedef void ( * OnConnect ) (const uint64_t& id_abonent);
    typedef void ( * OnError ) ( CONNECT_ERROR );

    class CNetTcp
    {
    public:
        CNetTcp();
        ~CNetTcp();

        // Connect to host, you may connect to any hosts, any times
        // even though you are client or server
        int connect( const std::string& dest_address, const uint16_t dest_port );
        int connect( const uint32_t& dest_address, const uint16_t dest_port );
        int connect( const std::string& interface_addr, const uint16_t interface_port,
                          const std::string& dest_address, const uint16_t dest_port );
        int connect( const uint32_t& interface_addr, const uint16_t interface_port,
                          const uint32_t& address, const uint16_t port);

        // Disconnect from host
        void disconnect( const uint64_t& id_abonent );

        //  send message to one host or to all hosts
        bool send( const int& id_abonent, const std::string& message );
        bool send_to_group( const std::vector<uint64_t> &group, const std::string& message );
        bool send_to_group( const std::vector<std::pair<uint64_t, std::string> > &messages_to_id );
        bool send_to_all( const std::string& message );

        // Subscribe on received message
        void subscribe( OnReceive receive_handle );
	
        // Subscribe on error
        void subscribe_on_error( OnError error_handle );
	
        // Subscribe on receive messages from required host
        void subscribe( OnReceive receive_handle, uint64_t& id_abonent );
	
        // Subscribe on new connection - start listening as server
        // return socket id as integer type
        void subscribe_on_new_connection( OnConnect connect_handle );
        void subscribe_on_new_connection( OnConnect connect_handle, const uint16_t interface_port );

        // Set to blocking or unblocking mode, while blocking
        // doesn't called any subscribers
        // A.k.a serial perfomance
        void block();
        void unblock();

    private:
        void create_data_thread();
        void create_connect_thread();
        // epoll thread - we may have one thread for epoll waiting and sockets IO, or
        // we may have one thread for epoll waiting and pool of threads for sockets IO
        void data_thread();
        // thread for connection occurs check
        void connect_thread();
        
        void add_to_epoll( const int sfd );

        OnConnect   m_connect_handle;
        OnError     m_error_handle;
        OnReceive   m_receive_handle;
        
        std::unordered_map<int, OnReceive>  m_receive_handles_map;
        std::unordered_set<int> m_data_socks;
        std::unordered_set<int> m_connect_socks;
        
        // epoll descriptor for signaling data receive
        int m_efd;
        
        // poll descriptor for signaling connect occurs
        int m_pfd;
        
        // listen socket descriptor
        int m_lsfd;
        
        std::atomic<bool> m_data_thread_isrun;
        std::atomic<bool> m_connect_thread_isrun;
        std::thread *m_data_thread;
        std::thread *m_connect_thread;
    };
}

#endif
