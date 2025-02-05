/************************************************************************************************************************
 * 	Module: Websockets Client
 * 	File Name: websockets_client.h
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Header file for websockets clients
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
class client_abstract;
class ws_client_base;
class ws_client;
class wss_client;
/***********************************************************************************************************************
 *                                                  ALIASES
 ***********************************************************************************************************************/
using ws_stream = websocket::stream<tcp::socket>;   //stream type for websockets
using wss_stream = websocket::stream<beast::ssl_stream<tcp::socket>>; //stream type for websockets secure

/***********************************************************************************************************************
 *                                                  CLASSES
 ***********************************************************************************************************************/
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Exception Expected:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class client_abstract
{
protected:
    std::atomic<bool> ongoing_connection = false;   //atomic boolean to indetify connection status
    std::atomic<bool> self_disconnected = false;   //atomic boolean to if connection is lost during ongoing connection
    std::mutex read_mutex;      //mutex for reading buffer
    std::mutex send_mutex;      //mutex for sending buffer
    std::deque<std::vector<unsigned char>> read_messages_queue;  //queue to store messages to read
    std::deque<std::vector<unsigned char>> send_messages_queue;  //queue to store messages to send
protected:
    client_abstract(void) = default;    //default non-parameterized constructor
    virtual ~client_abstract(void) = default;
    virtual void receive_message(void) = 0;
    virtual void write_message(void) = 0;
    virtual void disconnect(int) = 0;   //self disconnection
public:
    virtual bool connect(std::string& host, unsigned short) = 0;
    virtual void disconnect(void) = 0;  //gracefull disconnection
    virtual bool check_failed_connection(void) = 0; //check if there's failed connection, if so, call reset
    virtual void reset(void) = 0;   //reset option, necessary to re-initialize client if connection failed and self-disconnected
    virtual std::vector<unsigned char> read_message(void) = 0;
    virtual void send_message(const std::vector<unsigned char>&) = 0;
    virtual bool check_connection(void) = 0;
    virtual bool check_inbox(void) = 0;
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Exception Expected:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class ws_client_base : public client_abstract
{
protected:
    std::unique_ptr<net::io_context> io_ctx;     //the client io context for asynchronous I/O operations, unique pointer
    //net::io_context io_ctx;    //the client io context for asynchronous I/O operations
    tcp::resolver resolver;     //ip and port resolver
    std::unique_ptr<net::thread_pool> client_pool;  //threads pool the client (unique pointer), default=2, 1read/1write
    //net::thread_pool client_pool;   //threads pool the client, default=2, 1read/1write
    std::unique_ptr<net::strand<net::io_context::executor_type>> strand;//strand to io_context to prevent racing between the threads for the async operations
    //net::strand<net::io_context::executor_type> strand;//strand to io_context to prevent racing between the threads for the async operations
protected:
    explicit ws_client_base(void)
        : io_ctx(std::make_unique<net::io_context>()), resolver(*io_ctx), /*client_pool(2),*/ strand(std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor())) {} //2threads, 1read/1write
    virtual ~ws_client_base(void) = default;//join threads until all finish their work
public:
    std::vector<unsigned char> read_message(void) override;
    void send_message(const std::vector<unsigned char>&) override;
    bool check_connection(void) override;
    bool check_inbox(void) override;
    bool check_failed_connection(void) override;
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Exception Expected:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class ws_client : public ws_client_base, public std::enable_shared_from_this<ws_client>
{
protected:
    std::unique_ptr<ws_stream> stream; //I/O stream, unique pointer
    //ws_stream stream;   //I/O stream
protected:
    void receive_message(void) override;
    void write_message(void) override;
    void disconnect(int) override;
public:
    explicit ws_client(void) : ws_client_base(), stream(std::make_unique<ws_stream>(*io_ctx)) {}
    ~ws_client(void){this->disconnect();} //disconnect before destruction
    bool connect(std::string&, unsigned short) override;
    void disconnect(void) override;
    void reset(void) override;
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Exception Expected:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class wss_client : public ws_client_base, public std::enable_shared_from_this<wss_client>
{
protected:
    std::unique_ptr<wss_stream> stream; //I/O stream, unique pointer
    ssl::context ssl_ctx{ssl::context::tls};  //SSL context reference
protected:
    void receive_message(void) override;
    void write_message(void) override;
    void disconnect(int) override;
public:
    wss_client(void) = delete;   //default non-parameterized constructor
    explicit wss_client(const std::string key_file,const std::string certificate_file,const std::string CA_cert_file) :
        ws_client_base()
        {
            Set_SSL_CTX(ssl_ctx,key_file,certificate_file,CA_cert_file);
            stream = std::make_unique<wss_stream>(*io_ctx,ssl_ctx);  //late initialization instead of the initialization list
        }
    explicit wss_client(const std::string key_file) :
        ws_client_base()
    {
        Set_SSL_CTX(ssl_ctx,key_file);
        stream = std::make_unique<wss_stream>(*io_ctx,ssl_ctx);  //late initialization instead of the initialization list
    }
    ~wss_client(void){this->disconnect();} //disconnect before destruction
    bool connect(std::string&, unsigned short) override;
    void disconnect(void) override;
    void reset(void) override;

};



