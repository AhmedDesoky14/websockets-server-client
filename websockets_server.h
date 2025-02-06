/************************************************************************************************************************
 * 	Module: Websockets Server
 * 	File Name: websockets_server.h
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Header file for websockets servers and sessions handling. This file contains all classes related to WebSocket
 *               and WebSocket Secure server part and sessions handling, their member variables, their member functions and
 *               their constructors and destructors implementations.
 *               Server classes hierarchy is shown as following...
 *
 *                                              #server_abstract#
 *                                                      |
 *                                                      |
 *                                                      |
 *                                               #ws_server_base#
 *                                                      |
 *                                                      |
 *                                                      |
 *                                   /----------------------------------------\
 *                                   |                                        |
 *                               #ws_server#                             #wss_server#
 *
 *                           +Please read each class documentation for further information+
 *
 *======================================================================================================================
 *
 *               #Sessions classes are closed box, can not be accessed by user.
 *
 *               Sessions classes hierarchy is shown as following...
 *
 *                                              #session_abstract#
 *                                                      |
 *                                                      |
 *                                                      |
 *                                               #ws_session_base#
 *                                                      |
 *                                                      |
 *                                                      |
 *                                   /----------------------------------------\
 *                                   |                                        |
 *                              #ws_session#                             #wss_session#
 *
 *                           +Please read each class documentation for further information+
 *
 *
 ***********************************************************************************************************************/
#pragma once
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/asio/strand.hpp>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <deque>
#include <chrono>
#include <thread>
#include <functional>
#include "ssl_conf.h"
#include <iostream>
/************************************************************************************************************************
 *                     							   NAMESPACES
 ***********************************************************************************************************************/
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
/***********************************************************************************************************************
 *                     					      FORWARD DECLARATIONS
 ***********************************************************************************************************************/
class server_abstract;
class ws_server_base;
class ws_server;
class wss_server;
class session_abstract;
class ws_session_base;
class ws_session;
class ws_session;
/***********************************************************************************************************************
 *                                                  ALIASES
 ***********************************************************************************************************************/
using ws_stream = websocket::stream<tcp::socket>;   //stream type for websockets
using wss_stream = websocket::stream<beast::ssl_stream<tcp::socket>>; //stream type for websockets secure
using session_hndl = std::weak_ptr<session_abstract>;   //session hndl, for handling and dealing with sessions objects
/***********************************************************************************************************************
 *                                                  CLASSES
 ***********************************************************************************************************************/
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: Abstract base class for furher derived classes
* Abstract/Concrete: Abstract
* #Instances: No Instances
* Exception Expected: No
* Inherited Classes: NONE
* Description: Abstract base class that acts as interface for derived classes and declares all needed basic
*              and common member variables and member functions.
*              All member variables are protected.
*              Internally called member functions are protected.
*              Only functions dedicated for user interface are public
************************************************************************************************************************/
class server_abstract
{
protected:
    bool secure; //boolean to understand if the server is secured or not
    bool started_once = false;  //boolean to check if the server already started once or not
    unsigned short server_port; //opened port for the server
    //8 mutuxes for 8 functions to prevent racing from different threads to the same single instance
    std::mutex start_mutex;     //mutex for start function
    std::mutex stop_mutex;         //mutex for stop function
    std::mutex send_message_mutex;   //mutex for send message
    std::mutex read_message_mutex;   //mutex for read message
    std::mutex check_inbox_mutex;   //mutex for check inbox
    std::mutex session_check_mutex; //mutex for check_session function
    std::mutex session_close_mutex;   //mutex to prevent deadlock of calling close sessions inside stop function
    std::mutex session_establishment_mutex; //Mutex to prevent racing when creating new sessions
    std::mutex g_sessions;   //mutex to protect shared access to map of IDs and sessions
    std::size_t max_sessions;   //max allowed sessions num
    std::atomic<std::size_t> session_count = 0; //counter for sessions running
    std::atomic<bool> server_running = false;   //boolean to show if server is running or not
    std::unordered_set<int> sessions_ids;   //to track available IDs for sessions
    std::unordered_map<int,session_hndl> sessions;  //hash map for all sessions to control
protected:
    server_abstract(void) = delete; //deleted default non-parameterized constructor
    explicit server_abstract(std::size_t limit,unsigned short port,bool type) : max_sessions(limit), secure(type), server_port(port) {}
    virtual ~server_abstract(void) = default;
    virtual void accept_connection(void) = 0;
public:
    virtual void start(void) = 0;   //start server
    virtual void stop(void) = 0;    //stop server
    virtual bool is_serving(void) = 0;  //check if server is serving clients or not
    virtual bool is_running(void) = 0;  //check if server is running or not
    virtual int sessions_count(void) = 0;   //count number of running sessions
    virtual bool send_message(int, const std::vector<unsigned char>&) = 0;  //send message for session, add to queue
    virtual std::vector<unsigned char> read_message(int) = 0;   //read message for session, get from queue
    virtual bool check_inbox(int) = 0;  //check session inbox of a session
    virtual bool check_session(int) = 0;//check if a specific session is running
    virtual void close_session(int) = 0;//close specific session
};
/************************************************************************************************************************
* Class Name: ws_server_base
* Purpose: Base class for more derived classes with different options
* Abstract/Concrete: Abstract
* #Instances: No Instances
* Exception Expected: No
* Inherited Classes: server_abstract
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
*               2- Constructor with no SSL option - protected
*                   Inputs:
*                       - Server port as unsigned short.
*                       - Maximum sessions allowed to handle for the server as "std::size_t".
*                       - boolean set as false, used to indicate if digital certificates peer verification is used or not.
*               3- Constructor with SSL option - protected
*                   Inputs:
*                       - ssl_context reference, ready and initialized.
*                       - Server port as unsigned short.
*                       - Maximum sessions allowed to handle for the server as "std::size_t".
*                       - boolean, used to indicate if digital certificates peer verification is used or not.
*                       - Certificate Authority digital certificate file path as constant string
*
* Description: Base class for derived classes for WebSocket and WebSocket secure.
*              It implements only common member functions for a WebSocket server.
*              And defines new member variables related to Boost.Asio library for handling
*              tasks, input/output and threads.
************************************************************************************************************************/
class ws_server_base : public server_abstract
{
protected:
    ssl::context* ssl_ctx; //SSL context shared_ptr passed to sessions by reference
    std::unique_ptr<net::io_context> io_ctx;
    tcp::acceptor tcp_acceptor; //TCP acceptor for connections accpeting
    std::unique_ptr<net::thread_pool> threads_pool; //Pool of threads to run server and sessions, unique pointer
    bool verification_on;   //boolean variable to check whether the instance is not that secured by verification, used by "wss_server"
protected:
    ws_server_base(void) = delete;  //deleted default non-parameterized constructor
    //websocket server constructor
    explicit ws_server_base(unsigned short port, std::size_t sessions_num, bool vrf)
        : server_abstract(sessions_num,port,false), io_ctx(std::make_unique<net::io_context>()), tcp_acceptor(*io_ctx,tcp::endpoint(tcp::v4(),port)),
        ssl_ctx(nullptr), verification_on(vrf) {tcp_acceptor.cancel(); tcp_acceptor.close();}
    //websocket secure server constructor
    explicit ws_server_base(ssl::context& ssl_context, unsigned short port, std::size_t sessions_num, bool vrf)
        : server_abstract(sessions_num,port,true), io_ctx(std::make_unique<net::io_context>()), ssl_ctx(&ssl_context),
        tcp_acceptor(*io_ctx,tcp::endpoint(tcp::v4(),port)), verification_on(vrf) {tcp_acceptor.cancel(); tcp_acceptor.close();}
    virtual ~ws_server_base(void) = default;
    void accept_connection(void) override;
public:
    void start(void) override;
    void stop(void) override;
    bool is_serving(void) override;
    bool is_running(void) override;
    int sessions_count(void) override;
    bool send_message(int, const std::vector<unsigned char>&) override;
    std::vector<unsigned char> read_message(int) override;
    bool check_inbox(int) override;
    bool check_session(int) override;
    void close_session(int) override;
};
/************************************************************************************************************************
* Class Name: ws_server
* Purpose: WebSocket class for all WebSocket server side operations
* Abstract/Concrete: Concrete / created dynamically only once by a factory function
* #Instances: Only one instance
* Exception Expected: No
* Inherited Classes: ws_server_base
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
*               2- Constructor with no SSL option and it initializes the base class - protected
*                   Inputs:
*                       - Server port as unsigned short.
*                       - Maximum sessions allowed to handle for the server as "std::size_t".
*
* Description: Derived final class for WebSocket server operations with no SSL/TLS underlayer.
*              It defines new member variables dedicated to its functionality.
*              And implements the rest of the pure virtual function to define its complete
*              behavior and functionaility.
*              Copy constrctor and copy assignment operator are deleted to prevent object copying.
*              Singleton design pattern used to create only once instance using a factory function
************************************************************************************************************************/
class ws_server : public ws_server_base  //make all inherited members private
{
private:
    static ws_server* server_instance;  //static ptr to server to access in different places - "Singleton Design Pattern"
    static std::mutex access_mutex;  //mutex to access the instance in many threads safely
protected:
    ws_server(void) = delete;   //deleted default non-parameterized constructor
    explicit ws_server(unsigned short port, std::size_t sessions_num)
        : ws_server_base(port,sessions_num,false) {}
    ~ws_server(void) = default;
public:
    ws_server(const ws_server&) = delete; //delete copy constructor
    ws_server& operator=(const ws_server&) = delete;  //delete copy assignment operator
    /*====================== Class creation functions - "Singleton Design Pattern" ====================================*/
    inline static ws_server* GetInstance(unsigned short port, std::size_t sessions_num)    //create the instance function
    {
        std::lock_guard<std::mutex> lock(ws_server::access_mutex);
        if(server_instance == nullptr)
            server_instance = new ws_server(port,sessions_num);
        return server_instance;
    }
    inline static void Destroy(ws_server* inst_ptr)  //destroy the instance function
    {
        std::lock_guard<std::mutex> lock(ws_server::access_mutex);
        inst_ptr->stop(); //stop server, call stop
        delete server_instance;
        server_instance = nullptr;
    }
};
/************************************************************************************************************************
* Class Name: wss_server
* Purpose: WebSocket class for all WebSocket Secure server side operations
* Abstract/Concrete: Concrete / created dynamically only once by a factory function
* #Instances: Only one instance
* Exception Expected: Yes, due to "Set_SSL_CTX" function calling - only at object construction
* Inherited Classes: ws_server_base
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
*               2- Constructor with no digital certificate peer verification - protected
*                   Inputs:
*                       - Server port as unsigned short.
*                       - Maximum sessions allowed to handle for the server as "std::size_t".
*                       - user private key file path as constant string for the server.
*               3- Constructor with with digital certificate peer verification - protected
*                   Inputs:
*                       - Server port as unsigned short.
*                       - Maximum sessions allowed to handle for the server as "std::size_t".
*                       - user private key file path as constant string for the server.
*                       - user digital certificate file path as constant string for the server.
*                       - Certificate Authority digital certificate file path as constant string
*
* Description: Derived final class for WebSocket secure server operations with SSL/TLS underlayer.
*              It defines new member variables dedicated to its functionality.
*              And implements the rest of the pure virtual function to define its complete
*              behavior and functionaility.
*              Copy constrctor and copy assignment operator are deleted to prevent object copying.
*              Singleton design pattern used to create only once instance using a factory function
************************************************************************************************************************/
class wss_server : public ws_server_base  //make all inherited members private
{
private:
    static wss_server* server_instance;  //static ptr to server to access in different places
    static wss_server* server_instance2;  //static ptr to server to access in different places, for server with lower security
    static std::mutex access_mutex;  //mutex to access the instance in many threads safely
    static std::mutex access_mutex2;  //mutex to access the instance in many threads safely, for server with lower security
    ssl::context ssl_ctx{ssl::context::tls};  //SSL context reference
    const std::string key;  //key file path
    const std::string certificate;  //certificate file path
protected:
    wss_server(void) = delete;  //deleted default non-parameterized constructor
    explicit wss_server(unsigned short port, std::size_t sessions_num,
                        const std::string key_file, const std::string certificate_file, const std::string CA_cert_file)
        : ws_server_base(ssl_ctx,port,sessions_num,true) {Set_SSL_CTX(ssl_ctx,key_file,certificate_file,CA_cert_file);}
    explicit wss_server(unsigned short port, std::size_t sessions_num,
                        const std::string key_file)
        : ws_server_base(ssl_ctx,port,sessions_num,false)
    {Set_SSL_CTX(ssl_ctx,key_file);}
    ~wss_server(void) = default;
public:
    wss_server(const wss_server&) = delete; //delete copy constructor
    wss_server& operator=(const wss_server&) = delete;  //delete copy assignment operator
    /*====================== Class creation functions - "Singleton Design Pattern" ====================================*/
    inline static wss_server* GetInstance(unsigned short port, std::size_t sessions_num,const std::string key,
        const std::string certificate,const std::string CA_certificate)//create the instance function
    {
        std::lock_guard<std::mutex> lock(access_mutex);
        if(server_instance == nullptr)
            server_instance = new wss_server(port,sessions_num,key,certificate,CA_certificate);
        return server_instance;
    }
    inline static wss_server* GetInstance(unsigned short port, std::size_t sessions_num,const std::string key)//create the instance function
    {
        std::lock_guard<std::mutex> lock(access_mutex2);
        if(server_instance2 == nullptr)
            server_instance2 = new wss_server(port,sessions_num,key);
        return server_instance2;
    }
    inline static void Destroy(wss_server* inst_ptr)  //destroy the instance function
    {
        //Safety for multithreads if block code
        bool is_verification_on = inst_ptr->verification_on;
        if(is_verification_on)
            wss_server::access_mutex.lock();
        else
            wss_server::access_mutex2.lock();
        inst_ptr->stop();   //stop server, call stop
        if(is_verification_on)
        {
            delete server_instance;
            server_instance = nullptr;
        }
        else
        {
            delete server_instance2;
            server_instance2 = nullptr;
        }
        if(is_verification_on)
            wss_server::access_mutex.unlock();
        else
            wss_server::access_mutex2.unlock();
    }
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: Abstract base class for furher derived classes
* Abstract/Concrete: Abstract
* #Instances: No Instances
* Exception Expected: No
* Inherited Classes: NONE
* Description: Abstract base class that acts as interface for derived classes and declares all needed basic
*              and common member variables and member functions.
*              All member variables and member functions are protected.
*              Member functions are accessed by server friend classes, "ws_server_base", "ws_server" and "wss_server"
************************************************************************************************************************/
class session_abstract
{
protected:
    int session_id; //session id number
    std::atomic<bool> ongoing_session = false;  //boolean to check session state
    std::mutex read_mutex;  //mutex to prevent racing for "read_messages_queue"
    std::mutex send_mutex;  //mutex to prevent racing for "send_messages_queue"
    std::deque<std::vector<unsigned char>> read_messages_queue;  //queue to store messages to read
    std::deque<std::vector<unsigned char>> send_messages_queue;  //queue to store messages to send
    std::mutex& g_sessions;   //reference to the mutex to protect shared access to map of IDs and sessions
    std::atomic<std::size_t>& session_count; //reference to session_count to decrement it after session close
    std::unordered_set<int>& sessions_ids;  //reference to IDs set of the server to safely release the id
    std::unordered_map<int,session_hndl>& sessions; //reference to sessions map to safely release the session resources
protected:
    session_abstract(void) = delete;    //deleted default non-parameterized constructor
    explicit session_abstract(int id,std::atomic<std::size_t>& sessions_counter,
        std::unordered_set<int>& ids_set,std::unordered_map<int,session_hndl>& sessions_map, std::mutex& gmtx)
        : session_id(id), session_count(sessions_counter), sessions_ids(ids_set), sessions(sessions_map), g_sessions(gmtx) {}
    virtual ~session_abstract(void) = default;
    virtual void stop(int) = 0;   //for ungracefull disconnection
    virtual void receive_message(void) = 0; //receive messages asynchronously
    virtual void write_message(void) = 0;   //send messages asynchronously
    virtual void start(void) = 0;   //to start session connection by server
    virtual void stop(void) = 0;    //for gracefull disconnection
    virtual std::vector<unsigned char> read_message(void) = 0;  //read messages, add to queue
    virtual void send_message(const std::vector<unsigned char>&) =0; //send messages, get from queue
    virtual bool check_inbox(void) = 0;  //check session inbox
    virtual bool check_session(void) = 0;//check if session is running
public:
    friend class ws_server_base;    //friend class to access private/protected members
    friend class ws_server;    //friend class to access private/protected members
    friend class wss_server;    //friend class to access private/protected members
};
/************************************************************************************************************************
* Class Name: ws_session_base
* Purpose: Base class for more derived classes with different options
* Abstract/Concrete: Abstract
* #Instances: No Instances
* Exception Expected: No
* Inherited Classes: session_abstract
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
*               2- Constructor that accepts necessary paramaters and initialize the abstact class - protected
*                   Inputs:
*                       - Session ID
*                       - Reference to sessions counter
*                       - Reference to IDs set
*                       - Reference to sessions map container
*                       - Reference to shared mutex with the server
*                       - io_context reference
*                    # All refernces are used for resources release by the session object after session closure.
* Description: Base class for derived classes for WebSocket and WebSocket secure.
*              It implements only common member functions for a WebSocket server's sessions.
*              And defines new member variables related to Boost.Asio library for handling
*              tasks, input/output and threads.
************************************************************************************************************************/
class ws_session_base : public session_abstract
{
protected:
    net::io_context& io_ctx;    //reference to the io_context
    net::strand<net::io_context::executor_type> strand; //strand to manage handlers running on many threads sequentially
protected:
    ws_session_base(void) = delete; //deleted default non-parameterized constructor
    explicit ws_session_base(int id,std::atomic<std::size_t>& sessions_counter,std::unordered_set<int>& ids_set,
        std::unordered_map<int,session_hndl>& sessions_map,std::mutex& gmtx,net::io_context& context)
        : session_abstract(id,sessions_counter,ids_set,sessions_map,gmtx), io_ctx(context), strand(context.get_executor()){}
    virtual ~ws_session_base(void) = default;
    virtual void receive_message(void) = 0;
    virtual void write_message(void) = 0;
    virtual void stop(int) = 0;
    virtual void start(void) = 0;
    virtual void stop(void) = 0;
    std::vector<unsigned char> read_message(void) override;
    void send_message(const std::vector<unsigned char>&) override;
    bool check_inbox(void) override;
    bool check_session(void) override;
public:
     friend class ws_server_base;    //friend class to access private/protected members
     friend class ws_server;    //friend class to access private/protected members
     friend class wss_server;    //friend class to access private/protected members
};
/************************************************************************************************************************
* Class Name: ws_session
* Purpose: WebSocket class for WebSocket sessions handling by server
* Abstract/Concrete: Concrete
* #Instances: Based on server maximum sessions limit
* Exception Expected: No
* Inherited Classes: ws_session_base
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
                2- Constructor that accepts necessary paramaters and initialize the base class - protected
*                   Inputs:
*                       - Session ID
*                       - Reference to sessions counter
*                       - Reference to IDs set
*                       - Reference to sessions map container
*                       - Reference to shared mutex with the server
*                       - io_context reference
*                       - session connection socket
*                    # All refernces are used for resources release by the session object after session closure.
*
* Description: Derived final class for WebSocket sessions handling by server with no SSL/TLS underlayer.
*              It defines new member variables dedicated to its functionality.
*              And implements the rest of the pure virtual function to define its complete
*              behavior and functionaility.
*              Objects of this class are only created and destroyed by the server class.
*              Member functions are accessed by server friend classes, "ws_server_base", "ws_server" and "wss_server"
************************************************************************************************************************/
class ws_session : public ws_session_base, public std::enable_shared_from_this<ws_session>
{
private:
    ws_stream stream;   //I/O stream
protected:
    void stop(int) override;
    void receive_message(void) override;
    void write_message(void) override;
    void start(void) override;
    void stop(void) override;
public:
    ws_session(void) = delete;  //deleted default non-parameterized constructor
    explicit ws_session(int id,std::atomic<std::size_t>& sessions_counter,std::unordered_set<int>& ids_set,
        std::unordered_map<int,session_hndl>& sessions_map,std::mutex& gmtx,net::io_context& context,tcp::socket&& socket)
        : ws_session_base(id,sessions_counter,ids_set,sessions_map,gmtx,context), stream(std::move(socket)) {}
    ~ws_session(void) = default;
public:
    friend class ws_server_base;    //friend class to access private/protected members
    friend class ws_server;    //friend class to access private/protected members
    friend class wss_server;    //friend class to access private/protected members
};
/************************************************************************************************************************
* Class Name: wss_session
* Purpose: WebSocket class for WebSocket Secure sessions handling by server
* Abstract/Concrete: Concrete
* #Instances: Based on server maximum sessions limit
* Exception Expected: No
* Inherited Classes: ws_session_base
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
                2- Constructor that accepts necessary paramaters and initialize the base class - protected
*                   Inputs:
*                       - Session ID
*                       - Reference to sessions counter
*                       - Reference to IDs set
*                       - Reference to sessions map container
*                       - Reference to shared mutex with the server
*                       - io_context reference
*                       - session connection socket
*                       - server's ssl_context by reference
*                    # All refernces are used for resources release by the session object after session closure.
*
* Description: Derived final class for WebSocket Secure sessions handling by server with SSL/TLS underlayer.
*              It defines new member variables dedicated to its functionality.
*              And implements the rest of the pure virtual function to define its complete
*              behavior and functionaility.
*              Objects of this class are only created and destroyed by the server class.
*              Member functions are accessed by server friend classes, "ws_server_base", "ws_server" and "wss_server"
************************************************************************************************************************/
class wss_session : public ws_session_base, public std::enable_shared_from_this<wss_session>
{
private:
    wss_stream stream;  //I/O secured stream
protected:
    void stop(int) override;
    void receive_message(void) override;
    void write_message(void) override;
    void start(void) override;
    void stop(void) override;
public:
    wss_session(void) = delete;  //deleted default non-parameterized constructor
    explicit wss_session(int id,std::atomic<std::size_t>& sessions_counter,std::unordered_set<int>& ids_set,
        std::unordered_map<int,session_hndl>& sessions_map,std::mutex& gmtx,net::io_context& context,tcp::socket&& socket,ssl::context& ssl_ctx)
        : ws_session_base(id,sessions_counter,ids_set,sessions_map,gmtx,context), stream(std::move(socket),ssl_ctx) {}
    ~wss_session(void) = default;
public:
    friend class ws_server_base;    //friend class to access private/protected members
    friend class ws_server;    //friend class to access private/protected members
    friend class wss_server;    //friend class to access private/protected members
};
