/************************************************************************************************************************
 * 	Module: Websockets Client
 * 	File Name: websockets_client.cpp
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Source file for websockets clients
 *
 *
 ***********************************************************************************************************************/
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include "websockets_client.h"

constexpr int connection_timeout = 4;  //in seconds
/***********************************************************************************************************************
 *                     					    FUNCTIONS DEFINTITIONS
 ***********************************************************************************************************************/
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
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
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
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
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
inline bool ws_client_base::check_connection(void)
{
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before checking
    return ongoing_connection.load();
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
inline bool ws_client_base::check_inbox(void)
{
    if(read_messages_queue.size()>0)
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
inline bool ws_client_base::check_failed_connection(void)
{
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before checking
    return self_disconnected.load();
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in): NONE
* Expected  Exception:
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
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
            std::cout << "Client failed to receive message: " << errcode.message() << std::endl;
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
        //std::cout << "Client received Message: " << rec_string << std::endl;
        self_object->receive_message();
    }));
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
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
            std::cout << "Client failed to write message: " << errcode.message() << std::endl;
            self_object->disconnect(-1);
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void ws_client::disconnect(int code)
{
     if(!ongoing_connection.load())  //if already disconnected
         return; //do nothing and return
    ongoing_connection = false;
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before closing
    //client_pool.stop();    //stop all threads
    //if(!io_ctx.stopped())   //stop context
    //io_ctx.stop();    //stop context
    //io_ctx.restart();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay until ongoing operations stop
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
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    //io_ctx.restart();
    //client_pool = net::thread_pool(2);  //re-initialze the threads pool with 2 new threads
    read_messages_queue.clear();
    send_messages_queue.clear();
    self_disconnected = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
    std::cout << "client disconnected ungracefully" << std::endl;
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
bool ws_client::connect(std::string& ip_address, unsigned short port)
{
    std::string host_ip = ip_address;
    std::string host_port = std::to_string(port);
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    resolver.async_resolve(host_ip,host_port,[host_ip,self_object](boost::system::error_code errcode, tcp::resolver::results_type result)   //resolve IP and port
    {
        if(errcode)
        {
            std::cout << "Client failed to resolve ip and port: " << errcode.message() << std::endl;
            self_object->ongoing_connection = true;
            self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
            return;
        }
        net::async_connect(self_object->stream->next_layer(),result,    //TCP async connection (start connection)
        [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
        {
            if(errcode2 == net::error::connection_refused)
            {
                std::cout << "Client failed to resolve ip and port: " << errcode2.message() << std::endl;
                return;
            }
            else if(errcode2)
            {
                std::cout << "Client, failed TCP connection: " << errcode2.message() <<std::endl;
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
                    std::cout << "Client, failed WebSocket handshake: " << errcode3.message() << std::endl;
                    self_object->ongoing_connection = true;
                    self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                    return;
                }
                std::cout << "Client connected successfully" << std::endl;
                self_object->ongoing_connection = true;
                //all function are successfull
                self_object->receive_message(); //trigger receive message call
                // while(self_object->ongoing_connection.load())
                // {
                //     if(self_object->send_messages_queue.size() > 0) //there's a message to send
                //         self_object->write_message();
                //     std::this_thread::sleep_for(std::chrono::milliseconds(50));    //sleep for 50ms
                // }
                // std::cout << "Client connection ended" << std::endl;
            });
        });
    });
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
    client_pool = std::make_unique<net::thread_pool>(2); //create thread pool object of 2 threads, 1read/1write
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay
    //run client context in different threads for read and write
    net::post(*client_pool,[this](){io_ctx->run();});
    net::post(*client_pool,[this](){io_ctx->run();});
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //timeout check loop
    while((i++<connection_timeout) && (!ongoing_connection.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_connection.load())  //if connection is not successfull
    {
        self_object->disconnect(); //disconnect if not disconnected and reset
        std::cout << "client connection timeout" << std::endl;
        return false;
    }
    return true;
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void ws_client::disconnect(void)
{
    if(!ongoing_connection.load())  //if already disconnected
        return; //do nothing and return
    ongoing_connection = false;
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before closing
    try
    {
        if (stream->is_open())
            stream->close(websocket::close_code::normal);
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
    //client_pool.stop();    //stop all threads
    // read_messages_queue.clear();
    // send_messages_queue.clear();
    while(!io_ctx->stopped());   //wait until io_context return
    // io_ctx.restart(); //reset io_context to exit all running tasks
    io_ctx->stop();  //make sure to stop context
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay
    //io_ctx->restart(); //reset io_context to run again at re-start
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    stream.reset(); //reset stream
    strand.reset();//reset strand
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<ws_stream>(*io_ctx);//create new stream binded to the new io_context
    //io_ctx.restart();
    //client_pool.join();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay until ongoing operations stop
    // try{stream->close(websocket::close_code::normal);}
    // catch(...){} //suppress exceptions, object is deleted afterwards
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    //io_ctx.restart();
    //client_pool = net::thread_pool(2);  //re-initialze the threads pool with 2 new threads
    read_messages_queue.clear();
    send_messages_queue.clear();
    self_disconnected = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
    std::cout << "client disconnected gracefully" << std::endl;
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void ws_client::reset(void)
{
    if(!self_disconnected.load())   //if no connection failed ungracefully, return, don't reset
        return;
    while(!io_ctx->stopped());   //wait until io_context return
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    //io_ctx->restart(); //reset io_context to run again at re-start
    //while(!io_ctx->stopped());   //wait until io_context return
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay
    stream.reset(); //reset stream
    strand.reset();
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<ws_stream>(*io_ctx);//create new stream binded to the new io_context
    //connection is closed and io_context is stopped. just handle threads and destroy them then restart io_context
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    //io_ctx->restart();   //restart io_context for futher operations
    self_disconnected = false;  //reset the boolean
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before reseting
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
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
            std::cout << "Client failed to receive message: " << errcode.message() << std::endl;
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
        //std::cout << "Client received Message: " << rec_string << std::endl;
        self_object->receive_message();
    }));
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
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
            std::cout << "Client failed to write message: " << errcode.message() << std::endl;
            self_object->disconnect(-1);
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void wss_client::disconnect(int code)
{
    if(!ongoing_connection.load())  //if already disconnected
         return; //do nothing and return
    ongoing_connection = false;
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before closing
    //client_pool.stop();    //stop all threads
    // read_messages_queue.clear();
    // send_messages_queue.clear();
    //while(!io_ctx.stopped());   //wait until io_context return
    //io_ctx.stop();  //make sure to stop context
    //io_ctx.restart();
    //client_pool.join();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay until ongoing operations stop
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
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    //io_ctx.restart();
    //client_pool = net::thread_pool(2);  //re-initialze the threads pool with 2 new threads
    read_messages_queue.clear();
    send_messages_queue.clear();
    self_disconnected = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
    std::cout << "client disconnected ungracefully" << std::endl;
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
bool wss_client::connect(std::string& ip_address, unsigned short port)
{
    std::string host_ip = ip_address;
    std::string host_port = std::to_string(port);
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    resolver.async_resolve(host_ip,host_port,[host_ip,self_object](boost::system::error_code errcode, tcp::resolver::results_type result)   //resolve IP and port
    {
        if(errcode)
        {
            std::cout << "Client failed to resolve ip and port: " << errcode.message() << std::endl;
            self_object->ongoing_connection = true;
            self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
            return;
        }
        net::async_connect(self_object->stream->next_layer().next_layer(),result,    //TCP async connection (start connection)
        [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
        {
            if(errcode2 == net::error::connection_refused)
            {
                std::cout << "Client failed to resolve ip and port: " << errcode2.message() << std::endl;
                return;
            }
            else if(errcode2)
            {
                std::cout << "Client, failed TCP connection: " << errcode2.message() << std::endl;
                self_object->ongoing_connection = true;
                self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                return;
            }
            //**update host string, to provide the Host HTTP header during the websocket handshake**
            std::string http_header = host_ip + ':' + std::to_string(endpoint.port());
            //**Set Server Name Indication (SNI) hostname, (many hosts need this to handshake successfully)**
            if(!SSL_set_tlsext_host_name(self_object->stream->next_layer().native_handle(),http_header.c_str()))
            {
                std::cout << "Client, failed to set SNI hostname" << std::endl;
                self_object->ongoing_connection = true;
                self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                return;
            }
            //SSL handshake, client side
            self_object->stream->next_layer().async_handshake(ssl::stream_base::client,[http_header,self_object](boost::system::error_code errcode3)
            {
                if(errcode3)
                {
                    std::cout << "Client, failed SSL handshake: " << errcode3.message() << std::endl;
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
                        std::cout << "Client, failed WebSocket handshake: " << errcode4.message() << std::endl;
                        self_object->ongoing_connection = true;
                        self_object->disconnect(-1); //setting "ongoing_connection" by true, to be able to disconnect
                        return;
                    }
                    std::cout << "Client connected successfully" << std::endl;
                    self_object->ongoing_connection = true;
                    //all function are successfull
                    self_object->receive_message(); //trigger receive message call
                    // while(self_object->ongoing_connection.load())
                    // {
                    //     if(self_object->send_messages_queue.size() > 0) //there's a message to send
                    //         self_object->write_message();
                    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));    //sleep for 50ms
                    // }
                    // std::cout << "Client connection ended" << std::endl;                    //all function are successfull
                });
            });
        });
    });
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
    client_pool = std::make_unique<net::thread_pool>(2); //create thread pool object of 2 threads, 1read/1write
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay
    //run client context in different threads for read and write
    net::post(*client_pool,[this](){io_ctx->run();});
    net::post(*client_pool,[this](){io_ctx->run();});
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0; //timeout check loop

    while((i++<connection_timeout) && (!ongoing_connection.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_connection.load())  //if connection is not successfull
    {
        self_object->disconnect(); //disconnect if not disconnected and reset
        std::cout << "client connection timeout" << std::endl;
        return false;
    }
    return true;
//     std::string host_ip = ip_address;
//     std::string host_port = std::to_string(port);
//     //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
//     auto self_object = shared_from_this();
//     resolver.async_resolve(host_ip,host_port,[host_ip,self_object](boost::system::error_code errcode, tcp::resolver::results_type result)   //resolve IP and port
// {
// std::cout << "Lambda called" << std::endl;
// if(errcode)
// {
// self_object->ongoing_connection = true;
// self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
// }
// //throw std::runtime_error("Client failed to connect: " + errcode.message());
// net::async_connect(self_object->stream->next_layer().next_layer(),result,    //TCP async connection (start connection)
// [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
// {
// if(errcode2)
// {
// try{self_object->stream->close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
// catch(...)
// {
// self_object->ongoing_connection = true;
// self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
// }
// //throw std::runtime_error("Client TCP connection failed: " + errcode2.message());
// }
// std::string http_header = host_ip + ':' + std::to_string(endpoint.port());
// //host_ip += ':' + std::to_string(endpoint.port());   //**update host string, to provide the Host HTTP header during the websocket handshake**
// //**Set Server Name Indication (SNI) hostname, (many hosts need this to handshake successfully)**
// if(!SSL_set_tlsext_host_name(self_object->stream->next_layer().native_handle(),http_header.c_str()))
// {
// try{self_object->stream->close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
// catch(...)
// {
// self_object->ongoing_connection = true;
// self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
// }
// // catch(std::exception& e)
// // {throw std::runtime_error("Client failed to connect: set SNI error: " + std::string(e.what()));}
// }
// //SSL handshake, client side
// self_object->stream->next_layer().async_handshake(ssl::stream_base::client,[http_header,self_object](boost::system::error_code errcode3)
// {
// if(errcode3)
// {
// try{self_object->stream->close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
// catch(...)
// {
// self_object->ongoing_connection = true;
// self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
// }
// // throw std::runtime_error("Client SSL handshake failed: " + errcode3.message());
// }
// //set the suggested timeout settings for the websocket as the client
// self_object->stream->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
// self_object->stream->set_option(websocket::stream_base::decorator([](websocket::request_type& request) //***
//                                             {
//                                                 request.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-client-async-ssl");
//                                             }));
// self_object->stream->async_handshake(http_header,"/",[self_object](boost::system::error_code errcode4)
//                {
//                    if(errcode4)
//                    {
//                        try{self_object->stream->close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
//                        catch(...)
//                        {
//                            self_object->ongoing_connection = true;
//                            self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
//                        }
//                        // throw std::runtime_error("Client websocket handshake failed: " + errcode4.message());
//                    }
//                    std::cout << "Client connected successfully" << std::endl;
//                    self_object->ongoing_connection = true;
//                    //all function are successfull
//                    self_object->receive_message(); //trigger receive message call
//                    while(self_object->ongoing_connection.load())
//                    {
//                        if(self_object->send_messages_queue.size() > 0) //there's a message to send
//                            self_object->write_message();
//                        std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
//                    }
//                    // while(self_object->ongoing_connection.load())   //while connection is ongoing - Loop
//                    // {
//                    //     if(self_object->send_messages_queue.size() != 0)    //if there are messages to send
//                    //         self_object->write_message();
//                    //     if(!self_object->read_called)  //if async_read is not called
//                    //         self_object->receive_message();
//                    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//                    //     // if((self_object->send_messages_queue.size() == 0) && (self_object->read_called))
//                    //     // {
//                    //     //     //if all of the queues are empty. sleep for 100ms and continue
//                    //     //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                    //     //     continue;
//                    //     // }
//                    //     // self_object->write_message();
//                    //     // self_object->receive_message();
//                    //     //std:: cout << "ongoing connection status: " << self_object->ongoing_session.load() << std::endl;
//                    //     //std:: cout << "ongoing connection status: " << self_object->ongoing_connection.load() << std::endl;
//                    // }
//                    //connection ended and stream is closed
//                    std::cout << "Connection is ended" << std::endl; //ongoing_connection is false now
//                    //self_object->disonnect_mutex.lock();
//                    // try
//                    // {
//                    //     if(!self_object->io_ctx.stopped())   //stop context
//                    //         self_object->io_ctx.stop();
//                    //     self_object->stream.close(websocket::close_code::normal);
//                    //     std::cout << "read canceled and mutex locked for session close" << std::endl;
//                    // }
//                    // catch(...){}    //suppress exception, client object will be deleted afterwards

//                });
// });
// });
//                        });
//     //run client context in different threads for read and write
//     net::post(client_pool,[this](){io_ctx.run();});
//     net::post(client_pool,[this](){io_ctx.run();});


//     // std::thread client_thread([&,this]()
//     // { io_ctx.run();
//     // //io_ctx.run();
//     // std::cout << "client context ended" << std::endl;}); //run the server context
//     // client_thread.detach(); //detach from the client_thread

//     // std::thread client2_thread([&,this]()
//     //                           { io_ctx.run();
//     // //io_ctx.run();
//     // std::cout << "client context ended" << std::endl;}); //run the server context
//     // client2_thread.detach(); //detach from the client_thread

//     //Boost's default handshake timeout connection for websocket is 30seconds
//     int i = 0;  //if connection is timeout and not successfull return false, else true
//     while((i++<30) && (!ongoing_connection.load()))
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//     if(!ongoing_connection.load())  //if connection is not successfull
//     {
//         io_ctx.stop();
//         return false;
//     }
//     return true;

}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void wss_client::disconnect(void)
{
    if(!ongoing_connection.load())  //if already disconnected
        return; //do nothing and return
    ongoing_connection = false;
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before closing
    try
    {
        if (stream->is_open())
            stream->close(websocket::close_code::normal);
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
    //client_pool.stop();    //stop all threads
    // read_messages_queue.clear();
    // send_messages_queue.clear();
    // io_ctx.restart(); //reset io_context to exit all running tasks
    while(!io_ctx->stopped());   //wait until io_context return
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay
    //io_ctx->restart(); //reset io_context to run again at re-start
    //while(!io_ctx->stopped());   //wait until io_context return
    stream.reset(); //reset stream
    strand.reset(); //reset strand
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<wss_stream>(*io_ctx,ssl_ctx);//create new stream binded to the new io_context
    //client_pool.join();
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay until ongoing operations stop
    // try{stream->close(websocket::close_code::normal);}
    // catch(...){} //suppress exceptions, object is deleted afterwards
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    //io_ctx.restart();
    //client_pool = net::thread_pool(2);  //re-initialze the threads pool with 2 new threads
    read_messages_queue.clear();
    send_messages_queue.clear();
    self_disconnected = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
    std::cout << "client disconnected gracefully" << std::endl;
}
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers:
* Running Thread: Mainthread -> A pool thread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Expected  Exception:
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void wss_client::reset(void)
{
    if(!self_disconnected.load())   //if no connection failed ungracefully, return, don't reset
        return;
    while(!io_ctx->stopped());   //wait until io_context return
    io_ctx->stop();  //make sure to stop context
    client_pool->join();    //join threads until they finish
    client_pool.reset();    //destory/delete threads pool object
    //io_ctx->restart(); //reset io_context to run again at re-start
    //while(!io_ctx->stopped());   //wait until io_context return
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay
    stream.reset(); //reset stream
    strand.reset();
    io_ctx.reset(); //reset the unique pointer to destroy the underlaying object
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    io_ctx = std::make_unique<net::io_context>();   //create new object
    strand = std::make_unique<net::strand<net::io_context::executor_type>>(io_ctx->get_executor());
    stream = std::make_unique<wss_stream>(*io_ctx,ssl_ctx);//create new stream binded to the new io_context
    //connection is closed and io_context is stopped. just handle threads and destroy them then restart io_context
    //client_pool->join();    //join threads until they finish
    //client_pool.reset();    //destory/delete threads pool object
    //io_ctx->restart();   //restart io_context for futher operations
    self_disconnected = false;  //reset the boolean
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before reseting
}
