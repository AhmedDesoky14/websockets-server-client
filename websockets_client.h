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
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class client_abstract
{
protected:
    std::atomic<bool> ongoing_connection = false;   //atomic boolean to indetify connection status
    std::mutex read_mutex;      //mutex for reading buffer
    std::mutex send_mutex;      //mutex for sending buffer
    std::deque<std::vector<unsigned char>> read_messages_queue;  //queue to store messages to read
    std::deque<std::vector<unsigned char>> send_messages_queue;  //queue to store messages to send
protected:
    client_abstract(void) = default;    //default non-parameterized constructor
    virtual ~client_abstract(void) = default;
    virtual void receive_message(void) = 0;
    virtual void write_message(void) = 0;
    virtual void disconnect(int) = 0;   //ungracefull disconnection
public:
    virtual bool connect(std::string& host, unsigned short) = 0;
    virtual void disconnect(void) = 0;  //ungracefull disconnection
    virtual std::vector<unsigned char> read_message(void) = 0;
    virtual void send_message(const std::vector<unsigned char>&) = 0;
    virtual bool check_connection(void) = 0;
    virtual bool check_queue(void) = 0;
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class ws_client_base : public client_abstract
{
protected:
    net::io_context& io_ctx;    //the client io context
    tcp::resolver resolver;     //ip and port resolver
    boost::asio::thread_pool client_pool;   //threads pool the client, default=2, 1read/1write
    net::strand<net::io_context::executor_type> strand;//strand to io_context to prevent racing between the threads for the async operations
protected:
    ws_client_base(void) = delete;  //default non-parameterized constructor
    explicit ws_client_base(net::io_context& context)
        : io_ctx(context), resolver(context), client_pool(2), strand(context.get_executor()) {} //2threads, 1read/1write
    virtual ~ws_client_base(void) = default;
public:
    std::vector<unsigned char> read_message(void) override;
    void send_message(const std::vector<unsigned char>&) override;
    bool check_connection(void) override;
    bool check_queue(void) override;
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class ws_client : public ws_client_base, public std::enable_shared_from_this<ws_client>
{
protected:
    net::io_context io_ctx;    //IO_context for asynchronous operations
    ws_stream stream;   //I/O stream
protected:
    void receive_message(void);
    void write_message(void);
    void disconnect(int);
public:
    explicit ws_client(void) : ws_client_base(io_ctx), stream(io_ctx) {}
    ~ws_client(void) = default;
    bool connect(std::string&, unsigned short);
    void disconnect(void);
};
/************************************************************************************************************************
* Class Name: server_abstract
* Purpose: abstract class, including main server's member variables and interface functions
* Abstract/Concrete:
* #Instances:
* Inherited Classes:
* Description:
*
*
************************************************************************************************************************/
class wss_client : public ws_client_base, public std::enable_shared_from_this<wss_client>
{
protected:
    net::io_context io_ctx;    //IO_context for asynchronous operations
    wss_stream stream;   //I/O stream
    ssl::context ssl_ctx{ssl::context::tls};  //SSL context reference
    const std::string key;  //key file path
    const std::string certificate;  //certificate file path
protected:
    void receive_message(void);
    void write_message(void);
    void disconnect(int);
public:
    wss_client(void) = delete;   //default non-parameterized constructor
    explicit wss_client(const std::string key_file,const std::string certificate_file) : ws_client_base(io_ctx),
        key(key_file), certificate(certificate_file), stream(io_ctx,ssl_ctx) {}
    ~wss_client(void) = default;
    bool connect(std::string&, unsigned short);
    void disconnect(void);
};



