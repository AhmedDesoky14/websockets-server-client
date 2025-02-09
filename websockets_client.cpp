/************************************************************************************************************************
 * 	Module: Websockets Client
 * 	File Name: websockets_client.cpp
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Source file for websockets clients. This file contains all member functions implementations
 *               of all WebSocket and WebSocket Secure base and derived classes.
 *               Please read header file documentation for more understanding.
 ***********************************************************************************************************************/
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include "websockets_client.h"
constexpr int connection_timeout = 30;  //in seconds
/***********************************************************************************************************************
 *                     					    FUNCTIONS DEFINTITIONS
 ***********************************************************************************************************************/
/************************************************************************************************************************
* Function Name: read_message
* Class name: ws_client_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): read message if there's one in the queue or empty message
* Return value: vector of unsigned char (message)
* Description: User function to read messages received and stored in the queue
*              access to the queue and shared variables is secured by "read_mutex" mutex
************************************************************************************************************************/
std::vector<unsigned char> ws_client_base::read_message(void)
{
    std::vector<unsigned char> message;
    if(read_messages_queue.size() == 0)
        return message;
    read_mutex.lock();
    message = read_messages_queue.front();
    read_messages_queue.pop_front();
    read_mutex.unlock();
    return message;
}
/************************************************************************************************************************
* Function Name: send_message
* Class name: ws_client_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): message to send as vector of unsigned characters
* Parameters (out): NONE
* Return value: NONE
* Description: User function to send messages, first stored in the queue then written to the stream
*              access to the queue and shared varibales is secured by "send_mutex" mutex
************************************************************************************************************************/
void ws_client_base::send_message(const std::vector<unsigned char>& message)
{
    send_mutex.lock();
    send_messages_queue.push_back(std::move(message));
    send_mutex.unlock();
    this->write_message();  //call write message and give the write order
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending and writing
    return;
}
/************************************************************************************************************************
* Function Name: check_connection
* Class name: ws_client_base
* Access: Public
* Specifiers: inline
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): Client's connection status
* Return value: Client's connection status as boolean
* Description: User function to check if client is connected or not. accessing atomic variable for thread safety.
************************************************************************************************************************/
inline bool ws_client_base::check_connection(void)
{
    return ongoing_connection.load();
}
/************************************************************************************************************************
* Function Name: check_inbox
* Class name: ws_client_base
* Access: Public
* Specifiers: inline
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): Client's Rx inbox status
* Return value: Client's Rx inbox status as boolean
* Description: User function to check if there are new received messages to the client or not.
************************************************************************************************************************/
inline bool ws_client_base::check_inbox(void)
{
    if(read_messages_queue.size()>0)
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name: check_failed_connection
* Class name: ws_client_base
* Access: Public
* Specifiers: inline
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): Client's connection failure status
* Return value: Client's connection failure status as boolean
* Description: User function to check if the client's connection failed ungracefully
*              A failure is considered in the following cases:
*              - server closed the connection
*              - client's ongoing connection failed.
*              # In these cases "client_abstract::reset" function is called automatically at first new connect call.
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
inline bool ws_client_base::check_failed_connection(void)
{
    return self_disconnected.load();
}
/************************************************************************************************************************
* Function Name: receive_message
* Class name: ws_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Parameters (in): NONE
* Expected  Exception: No
* Parameters (out): NONE
* Return value: NONE
* Description: Internal Asynchronous function called after a connection establishment.
*              It contains its own handler as Lambda functions called upon receiving new message then
*              "ws_client::receive_message" is called again after handler execution.
*              This function exits completely at connection closure.
*              Access to the queue and shared variables is secured by "read_mutex" mutex
************************************************************************************************************************/
void ws_client::receive_message(void)
{
    if(!stream->is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(-1);
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream->async_read(*buffer,net::bind_executor(*strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode)
        {
            self_object->disconnect(-1);
            return;
        }
        if (buffer->size() == 0) //empty buffer, do nothing
            self_object->receive_message();
        auto date_buffer_ptr = net::buffer_cast<unsigned char*>(buffer->data()); //get ptr to the buffer for the "vector of unsigned char"
        std::size_t data_size = buffer->size();
        std::vector<unsigned char> received_data(date_buffer_ptr,date_buffer_ptr + data_size);  //store
        self_object->read_mutex.lock();
        self_object->read_messages_queue.push_back(received_data);  //push received data into the queue
         self_object->read_mutex.unlock();
        buffer->consume(bytes_received);   //clear the buffer
        buffer->clear();
        std::string rec_string(received_data.begin(),received_data.end());
        self_object->receive_message();
    }));
}
/************************************************************************************************************************
* Function Name: write_message
* Class name: ws_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Parameters (in): NONE
* Expected  Exception: No
* Parameters (out): NONE
* Return value: NONE
* Description: Internal Asynchronous function called after a each "ws_client_base::send_message" function call
*              It contains its own handler as Lambda functions called upon sending the new message to handle any error occurs.
*              Access to the queue and shared variables is secured by "send_mutex" mutex
************************************************************************************************************************/
void ws_client::write_message(void)
{
    if(!stream->is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(-1);
        return;
    }
    send_mutex.lock();
    std::vector<unsigned char> message = send_messages_queue.front();
    send_messages_queue.pop_front();    //read front then pop
    send_mutex.unlock();
    net::const_buffer buffer(message.data(), message.size());
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream->async_write(buffer,net::bind_executor(*strand,[self_object](beast::error_code errcode, std::size_t bytes_sent_dummy)
    {
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode) //failed to send, disconnect
        {
            self_object->disconnect(-1);
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
/************************************************************************************************************************
* Function Name: disconnect (1)
* Class name: ws_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Internal function called by all member functions to disconnect and release resources.
*              upon any connection failure. After that, "client_abstract::reset" function is called automatically at first new connect call.
*              any other function. Accessing atomic variable for thread safety.
************************************************************************************************************************/
void ws_client::disconnect(int code)
{
     if(!ongoing_connection.load())  //if already disconnected
         return; //do nothing and return
    ongoing_connection = false;
    if(code == 0)
    {
        try
        {
            if (stream->is_open())
                stream->close(websocket::close_code::normal);
        }
        catch(...){} //suppress exceptions, object is deleted afterwards
    }
    else if(code == -1)
    {
        try
        {
            if (stream->is_open())
                stream->close(websocket::close_code::protocol_error);
        }
        catch(...){} //suppress exceptions, object is deleted afterwards
    }
    read_messages_queue.clear();
    send_messages_queue.clear();
    connected_ip.clear();
    connected_port = 0;
    self_disconnected = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: connect
* Class name: ws_client
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): Server's IP address/host as string reference
*                  Server's port dedicated for WebSocket connections
* Parameters (out): connection status, successful/failed
* Return value: connection status as boolean
* Description: User function to connect to a host server. It tries to connect, if its timeout (30 seconds by default)
*              passed it will reset connection and release resources and return false indicating failed connection.
*              If connection is successful, it returns true.
************************************************************************************************************************/
bool ws_client::connect(std::string& ip_address, unsigned short port)
{
    std::string host_ip = ip_address;
    std::string host_port = std::to_string(port);
    if(connected_ip == ip_address && connected_port == port && ongoing_connection.load() == true)
        return true;    //if I tried to connect to already connected endpoint
    else if(ongoing_connection.load() == true)
        return false;   //if I tried to connect to another endpoint while I am connected to some endpoint
    if(self_disconnected.load() == true)
        reset();    //if previous connection failed, reset resources
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    resolver.async_resolve(host_ip,host_port,[host_ip,self_object](boost::system::error_code errcode, tcp::resolver::results_type result)   //resolve IP and port
    {
        if(errcode)
        {
            self_object->ongoing_connection = true;
            self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
            return;
        }
        net::async_connect(self_object->stream->next_layer(),result,    //TCP async connection (start connection)
        [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
        {
            if(errcode2 == net::error::connection_refused)
            {
                return;
            }
            else if(errcode2)
            {
                self_object->ongoing_connection = true;
                self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                return;
            }
            //**update host string, to provide the Host HTTP header during the websocket handshake**
            std::string http_header = host_ip + ':' + std::to_string(endpoint.port());
            //set the suggested timeout settings for the websocket as the client
            self_object->stream->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            self_object->stream->set_option(websocket::stream_base::decorator([](websocket::request_type& request) //***
            {
                request.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-client-async");
            }));
            self_object->stream->async_handshake(http_header,"/",[self_object](boost::system::error_code errcode3)   //websocket handshake
            {
                if(errcode3)
                {
                    self_object->ongoing_connection = true;
                    self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                    return;
                }
                self_object->ongoing_connection = true;
                //all function are successfull
                self_object->receive_message(); //trigger receive message call
            });
        });
    });
    client_pool = std::make_unique<net::thread_pool>(2); //create thread pool object of 2 threads, 1read/1write
    //run client context in different threads for read and write
    net::post(*client_pool,[this](){io_ctx->run();});
    net::post(*client_pool,[this](){io_ctx->run();});
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //timeout check loop
    while((i++<connection_timeout) && (!ongoing_connection.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_connection.load())  //if connection is not successfull
    {
        self_object->ongoing_connection = true; //setting "ongoing_connection" by true, to be able to run disconnect
        self_object->disconnect(); //disconnect if not disconnected and reset
        return false;
    }
    connected_ip = ip_address;
    connected_port = port;
    return true;
}
/************************************************************************************************************************
* Function Name: disconnect (2)
* Class name: ws_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: User function called to disconnect and release all resources. Accessing atomic variable for thread safety.
************************************************************************************************************************/
void ws_client::disconnect(void)
{
    if(!ongoing_connection.load())  //if already disconnected
        return; //do nothing and return
    ongoing_connection = false;
    try
    {
        if (stream->is_open())
            stream->close(websocket::close_code::normal);
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    stream.reset(); //reset stream
    strand.reset();//reset strand
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<ws_stream>(*io_ctx);//create new stream binded to the new io_context
    read_messages_queue.clear();
    send_messages_queue.clear();
    connected_ip.clear();
    connected_port = 0;
    self_disconnected = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: reset
* Class name: ws_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: User function called to clear all resources and reset the client to be ready for another connection.
*              Called only upon ungracefully failed connection.
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
void ws_client::reset(void)
{
    if(!self_disconnected.load())   //if no connection failed ungracefully, return, don't reset
        return;
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    stream.reset(); //reset stream
    strand.reset();
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<ws_stream>(*io_ctx);//create new stream binded to the new io_context
    self_disconnected = false;  //reset the boolean
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before reseting
}
/************************************************************************************************************************
* Function Name: receive_message
* Class name: wss_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Parameters (in): NONE
* Expected  Exception: No
* Parameters (out): NONE
* Return value: NONE
* Description: Internal Asynchronous function called after a connection establishment.
*              It contains its own handler as Lambda functions called upon receiving new message then
*              "wss_client::receive_message" is called again after handler execution.
*              This function exits completely at connection closure.
*              Access to the queue and shared variables is secured by "read_mutex" mutex
************************************************************************************************************************/
void wss_client::receive_message(void)
{
    if(!stream->is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(-1);
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream->async_read(*buffer,net::bind_executor(*strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode)
        {
            self_object->disconnect(-1);
            return;
        }
        if (buffer->size() == 0) //empty buffer, do nothing
            self_object->receive_message();
        auto date_buffer_ptr = net::buffer_cast<unsigned char*>(buffer->data()); //get ptr to the buffer for the "vector of unsigned char"
        std::size_t data_size = buffer->size();
        std::vector<unsigned char> received_data(date_buffer_ptr,date_buffer_ptr + data_size);  //store
        self_object->read_mutex.lock();
        self_object->read_messages_queue.push_back(received_data);  //push received data into the queue
        self_object->read_mutex.unlock();
        buffer->consume(bytes_received);   //clear the buffer
        buffer->clear();
        std::string rec_string(received_data.begin(),received_data.end());
        self_object->receive_message();
    }));
}
/************************************************************************************************************************
* Function Name: write_message
* Class name: wss_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Parameters (in): NONE
* Expected  Exception: No
* Parameters (out): NONE
* Return value: NONE
* Description: Internal Asynchronous function called after a each "ws_client_base::send_message" function call
*              It contains its own handler as Lambda functions called upon sending the new message to handle any error occurs.
*              Access to the queue and shared variables is secured by "send_mutex" mutex
************************************************************************************************************************/
void wss_client::write_message(void)
{
    if(!stream->is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(-1);
        return;
    }
    send_mutex.lock();
    std::vector<unsigned char> message = send_messages_queue.front();
    send_messages_queue.pop_front();    //read front then pop
    send_mutex.unlock();
    net::const_buffer buffer(message.data(), message.size());
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream->async_write(buffer,net::bind_executor(*strand,[self_object](beast::error_code errcode, std::size_t bytes_sent_dummy)
    {
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->disconnect(0);   //stop session
            return;
        }
        else if(errcode) //failed to send, disconnect
        {
            self_object->disconnect(-1);
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
/************************************************************************************************************************
* Function Name: disconnect (1)
* Class name: wss_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Internal function called by all member functions to disconnect and release resources.
*              upon any connection failure. After that, "client_abstract::reset" is called automatically at first new connect call.
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
void wss_client::disconnect(int code)
{
    if(!ongoing_connection.load())  //if already disconnected
         return; //do nothing and return
    ongoing_connection = false;
    if(code == 0)
    {
        try
        {
            if (stream->is_open())
                stream->close(websocket::close_code::normal);
        }
        catch(...){} //suppress exceptions, object is deleted afterwards
    }
    else if(code == -1)
    {
        try
        {
            if (stream->is_open())
                stream->close(websocket::close_code::protocol_error);
        }
        catch(...){} //suppress exceptions, object is deleted afterwards
    }
    read_messages_queue.clear();
    send_messages_queue.clear();
    connected_ip.clear();
    connected_port = 0;
    self_disconnected = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: connect
* Class name: wss_client
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): Server's IP address/host as string reference
*                  Server's port dedicated for WebSocket connections
* Parameters (out): connection status, successful/failed
* Return value: connection status as boolean
* Description: User function to connect to a host server. It tries to connect, if its timeout (30 seconds by default)
*              passed it will reset connection and release resources and return false indicating failed connection.
*              If connection is successful, it returns true.
************************************************************************************************************************/
bool wss_client::connect(std::string& ip_address, unsigned short port)
{
    std::string host_ip = ip_address;
    std::string host_port = std::to_string(port);
    if(connected_ip == ip_address && connected_port == port && ongoing_connection.load() == true)
        return true;    //if I tried to connect to already connected endpoint
    else if(ongoing_connection.load() == true)
        return false;   //if I tried to connect to another endpoint while I am connected to some endpoint
    if(self_disconnected.load() == true)
        reset();    //if previous connection failed, reset resources
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    resolver.async_resolve(host_ip,host_port,[host_ip,self_object](boost::system::error_code errcode, tcp::resolver::results_type result)   //resolve IP and port
    {
        if(errcode)
        {
            self_object->ongoing_connection = true;
            self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
            return;
        }
        net::async_connect(self_object->stream->next_layer().next_layer(),result,    //TCP async connection (start connection)
        [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
        {
            if(errcode2 == net::error::connection_refused)
            {
                return;
            }
            else if(errcode2)
            {
                self_object->ongoing_connection = true;
                self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                return;
            }
            //**update host string, to provide the Host HTTP header during the websocket handshake**
            std::string http_header = host_ip + ':' + std::to_string(endpoint.port());
            //**Set Server Name Indication (SNI) hostname, (many hosts need this to handshake successfully)**
            if(!SSL_set_tlsext_host_name(self_object->stream->next_layer().native_handle(),http_header.c_str()))
            {
                self_object->ongoing_connection = true;
                self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                return;
            }
            //SSL handshake, client side
            self_object->stream->next_layer().async_handshake(ssl::stream_base::client,[http_header,self_object](boost::system::error_code errcode3)
            {
                if(errcode3)
                {
                    self_object->ongoing_connection = true;
                    self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                    return;
                }
                //set the suggested timeout settings for the websocket as the client
                self_object->stream->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                self_object->stream->set_option(websocket::stream_base::decorator([](websocket::request_type& request) //***
                {
                    request.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-client-async-ssl");
                }));
                self_object->stream->async_handshake(http_header,"/",[self_object](boost::system::error_code errcode4) //websocket handshake
                {
                    if(errcode4)
                    {
                        self_object->ongoing_connection = true;
                        self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                        return;
                    }
                    self_object->ongoing_connection = true;
                    //all function are successfull
                    self_object->receive_message(); //trigger receive message call
                });
            });
        });
    });
    client_pool = std::make_unique<net::thread_pool>(2); //create thread pool object of 2 threads, 1read/1write
    //run client context in different threads for read and write
    net::post(*client_pool,[this](){io_ctx->run();});
    net::post(*client_pool,[this](){io_ctx->run();});
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0; //timeout check loop
    while((i++<connection_timeout) && (!ongoing_connection.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_connection.load())  //if connection is not successfull
    {
        self_object->ongoing_connection = true; //setting "ongoing_connection" by true, to be able to run disconnect
        self_object->disconnect(); //disconnect if not disconnected and reset
        return false;
    }
    connected_ip = ip_address;
    connected_port = port;
    return true;
}
/************************************************************************************************************************
* Function Name: disconnect (2)
* Class name: wss_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: User function called to disconnect and release all resources. Accessing atomic variable for thread safety.
************************************************************************************************************************/
void wss_client::disconnect(void)
{
    if(!ongoing_connection.load())  //if already disconnected
        return; //do nothing and return
    ongoing_connection = false;
    try
    {
        if (stream->is_open())
            stream->close(websocket::close_code::normal);
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    stream.reset(); //reset stream
    strand.reset(); //reset strand
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<wss_stream>(*io_ctx,ssl_ctx);//create new stream binded to the new io_context
    read_messages_queue.clear();
    send_messages_queue.clear();
    connected_ip.clear();
    connected_port = 0;
    self_disconnected = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: reset
* Class name: wss_client
* Access: Protected
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: User function called to clear all resources and reset the client to be ready for another connection.
*              Called only upon ungracefully failed connection.
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
void wss_client::reset(void)
{
    if(!self_disconnected.load())   //if no connection failed ungracefully, return, don't reset
        return;
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    stream.reset(); //reset stream
    strand.reset();
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<wss_stream>(*io_ctx,ssl_ctx);//create new stream binded to the new io_context
    self_disconnected = false;  //reset the boolean
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before reseting
}
