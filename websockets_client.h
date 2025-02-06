/************************************************************************************************************************
 * 	Module: Websockets Client
 * 	File Name: websockets_client.h
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Header file for websockets clients. This file contains all classes related to WebSocket
 *               and WebSocket Secure client part, their member variables, their member functions and
 *               their constructors and destructors implementations.
 *               Classes hierarchy is shown as following...
 *
 *                                              #client_abstract#
 *                                                      |
 *                                                      |
 *                                                      |
 *                                               #ws_client_base#
 *                                                      |
 *                                                      |
 *                                                      |
 *                                   /----------------------------------------\
 *                                   |                                        |
 *                               #ws_client#                             #wss_client#
 *
 *                           +Please read each class documentation for further information+
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
* Class Name: client_abstract
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
    virtual void receive_message(void) = 0; //receive message from stream
    virtual void write_message(void) = 0;   //write message into stream
    virtual void disconnect(int) = 0;   //self disconnection
public:
    virtual bool connect(std::string& host, unsigned short) = 0;
    virtual void disconnect(void) = 0;  //gracefull disconnection
    virtual bool check_failed_connection(void) = 0; //check if there's failed connection, if so, call reset
    virtual void reset(void) = 0;   //reset option, necessary to re-initialize client if connection failed and self-disconnected
    virtual std::vector<unsigned char> read_message(void) = 0;  //read message from Queue
    virtual void send_message(const std::vector<unsigned char>&) = 0; //send message to Queue
    virtual bool check_connection(void) = 0;    // check client connection
    virtual bool check_inbox(void) = 0; //check read inbox
};
/************************************************************************************************************************
* Class Name: ws_client_base
* Purpose: Base class for more derived classes with different options
* Abstract/Concrete: Abstract
* #Instances: No Instances
* Exception Expected: No
* Inherited Classes: client_abstract
* Constructors:
*               1- Non-parameterized constructor for member variables initialization - protected
*
* Description: Base class for derived classes for WebSocket and WebSocket secure.
*              It implements only common member functions for a WebSocket client.
*              And defines new member variables related to Boost.Asio library for handling
*              tasks, input/output and threads.
************************************************************************************************************************/
class ws_client_base : public client_abstract
{
protected:
    std::unique_ptr<net::io_context> io_ctx;     //the client io context for asynchronous I/O operations, unique pointer
    tcp::resolver resolver;     //ip and port resolver
    std::unique_ptr<net::thread_pool> client_pool;  //threads pool the client (unique pointer), default=2, 1read/1write
    std::unique_ptr<net::strand<net::io_context::executor_type>> strand;//strand to io_context to prevent racing between the threads for the async operations
protected:
    explicit ws_client_base(void)
        : io_ctx(std::make_unique<net::io_context>()), resolver(*io_ctx), strand(std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor())) {}
    virtual ~ws_client_base(void) = default;//join threads until all finish their work
public:
    std::vector<unsigned char> read_message(void) override;
    void send_message(const std::vector<unsigned char>&) override;
    bool check_connection(void) override;
    bool check_inbox(void) override;
    bool check_failed_connection(void) override;
};
/************************************************************************************************************************
* Class Name: ws_client
* Purpose: WebSocket class for all WebSocket client side operations
* Abstract/Concrete: Concrete / created dynamically only
* #Instances: Unlimited
* Exception Expected: No
* Inherited Classes: ws_client_base
* Constructors:
*               1- Non-parameterized constructor for base class and member variables initialization - public
*
* Description: derived final class for WebSocket client operations with no SSL/TLS underlayer.
*              It defines new member variables dedicated to its functionality.
*              And implements the rest of the pure virtual function to define its complete
*              behavior and functionaility.
************************************************************************************************************************/
class ws_client : public ws_client_base, public std::enable_shared_from_this<ws_client> //to allow creating shared objects inside async handlers
{
protected:
    std::unique_ptr<ws_stream> stream; //I/O stream, unique pointer
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
* Purpose: WebSocketSecure class for all WebSocket Secure client side operations
* Abstract/Concrete: Concrete / created dynamically only
* #Instances: Unlimited
* Exception Expected: Yes, due to "Set_SSL_CTX" function calling - only at object construction
* Inherited Classes: ws_client_base
* Constructors:
*               1- Default Non-parameterized constructor - Deleted
*               2- Constructor for SSL inputs: - public
*                       - user private key file path as constant string
*                       - user digital certificate file path as constant string
*                       - Certificate Authority digital certificate file path as constant string
*
* Description: derived final class for WebSocket Secure client operations with a SSL/TLS underlayer.
*              It defines new member variables dedicated to its functionality.
*              And implements the rest of the pure virtual function to define its complete
*              behavior and functionaility.
************************************************************************************************************************/
class wss_client : public ws_client_base, public std::enable_shared_from_this<wss_client> //to allow creating shared objects inside async handlers
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
