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
#include <functional>

#include "shared_mutex.h"

namespace mmo
{
    const uint32_t MESSAGE_MAX_LEN = 100;
    
    template<typename T, void (T::*ptr)()>
    void invoke(T *obj)
    {
        (obj->*ptr)();
    }

    enum CONNECT_ERROR
    {
        DISCONNECTED = 1
    };
    
    typedef std::function<void (const std::string &message)> OnReceiveAbonent;
    typedef std::function<void (const int &id_abonent, const std::string &message)> OnReceive;
    typedef std::function<void (const int &id_abonent)> OnConnect;
    typedef std::function<void (const CONNECT_ERROR &error_code, const int &id_abonent)> OnError;

    class CNetTcp
    {
    public:
        CNetTcp();
        ~CNetTcp();

        // Connect to host, you may connect to any hosts, any times
        // through any interface
        int connect( const std::string& dst_address,
					 const std::string& dst_port);
		
        int connect( const std::string& dst_address, 
                     const std::string& dst_port,
                     const std::string& iface_addr, 
                     const std::string& iface_port );
		
        int connect( const std::string& dst_address, 
                     const std::string& dst_port,
                     const std::string& iface_addr, 
                     const std::string& iface_port,
                     const std::string& iface_name );

        // Disconnect from host
        void disconnect( const int& id_abonent );

        //  send message to one host or to all hosts
        bool send( const int& id_abonent, const std::string& message );
        bool send_to_group( const std::vector<int> &group, const std::string& message );
        bool send_to_group( const std::vector<std::pair<int, std::string> > &messages_to_id );
        bool send_to_all( const std::string& message );

        // Subscribe on received message
        void subscribe_on_data( OnReceive receive_handle );
        
        // Subscribe on receive messages from required point
        void subscribe_on_data( OnReceiveAbonent receive_handle, const int& id_abonent );
        void subscribe_on_data( OnReceive receive_handle, const int& id_abonent );

        // Subscribe on error
        void subscribe_on_error( OnError error_handle );

        // Subscribe on new connection - start listening as server
        // return socket id as integer type
        void subscribe_on_new_connection(OnConnect connect_handle);
        
		void subscribe_on_new_connection(OnConnect connect_handle,
			const std::string& iface_port);
		
        void subscribe_on_new_connection(OnConnect connect_handle,
			const std::string& iface_addr,
			const std::string& iface_port);
		
        void subscribe_on_new_connection(OnConnect connect_handle, 
			const std::string& iface_addr, 
			const std::string& iface_name, 
			const std::string& iface_port);

        // Set to blocking or unblocking mode, while blocking
        // doesn't called any subscribers
        // A.k.a serial perfomance
        void block();
        void unblock();

    private:
        int _connect(const std::string& dst_address, 
					 const std::string& dst_port,
                     const std::string& iface_addr, 
                     const std::string& iface_name,
                     const std::string& iface_port);

		void _subscribe_on_new_connection(OnConnect connect_handle, 
                                          const std::string& iface_addr, 
                                          const std::string& iface_name, 
                                          const std::string& iface_port);
        
        void _process_input_events(const struct epoll_event &evnt );
        void _process_output_events(const struct epoll_event &evnt );
        void _process_hup_events(const struct epoll_event &evnt );
        void create_data_thread();
        void data_thread();
        
        void add_in_to_epoll(const int sfd);
        void add_out_to_epoll(const int sfd);
        void mod_in_epoll(const int sfd);
        void del_from_epoll(const int sfd);
        
        // epoll descriptor for signaling data receive
        int m_efd;
        
        std::atomic<bool> m_data_thread_isrun;
        std::thread m_data_thread;
        

        std::unordered_set<int> m_data_socks;
        std::mutex m_data_mutex;
        
        std::unordered_set<int> m_connect_socks;
        std::mutex m_connect_mutex;

        OnConnect   m_connect_handle;
        OnError     m_error_handle;
        OnReceive   m_receive_handle;
        std::unordered_map<int, OnReceiveAbonent>  m_receive_handles_map;
        std::unordered_map<int, OnReceive>   m_ex_receive_handles_map;
        
        // listen socket descriptor
        int m_lsfd;
        
        // self-pipe socket
        int m_spfd;
        
        char m_recv_buf[MESSAGE_MAX_LEN];
    };
}

#endif
