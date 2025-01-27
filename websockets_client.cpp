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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
std::vector<unsigned char> ws_client_base::read_message(void)
{
    read_mutex.lock();
    std::vector<unsigned char> message = read_messages_queue.front();
    read_messages_queue.pop_front();
    read_mutex.unlock();
    std::cout << "message received by client: " << std::string(message.begin(),message.end()) << std::endl;
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
inline bool ws_client_base::check_connection(void)
{
    std::cout << "connection is: " <<  ongoing_connection.load() << std::endl;
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
bool ws_client_base::check_queue(void)
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void ws_client::receive_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(1);
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_read(*buffer,net::bind_executor(strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode)
        {
            self_object->disconnect(1);
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
        std::cout << "Client received Message: " << rec_string << std::endl;
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void ws_client::write_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(1);
        return;
    }
    send_mutex.lock();
    std::vector<unsigned char> message = send_messages_queue.front();
    send_messages_queue.pop_front();    //read front then pop
    send_mutex.unlock();
    net::const_buffer buffer(message.data(), message.size());
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_write(buffer,net::bind_executor(strand,[self_object](beast::error_code errcode, std::size_t bytes_sent_dummy)
    {
        if(errcode) //failed to send, disconnect
        {
            self_object->disconnect(1);
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void ws_client::disconnect(int)
{
    if(!ongoing_connection.load())  //if already disconnected
        return; //do nothing and return
    ongoing_connection = false;
    if(!io_ctx.stopped())   //stop context
        io_ctx.stop();
    try
    {
        stream.close(websocket::close_code::protocol_error);
        std::cout << "read canceled and mutex locked for session close" << std::endl;
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
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
        std::cout << "Lambda called" << std::endl;
        if(errcode)
        {
            self_object->ongoing_connection = true;
            self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
        }
        net::async_connect(self_object->stream.next_layer(),result,    //TCP async connection (start connection)
        [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
        {
            if(errcode2)
            {
                try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                catch(...)
                {
                    self_object->ongoing_connection = true;
                    self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
                }
            }
            //**update host string, to provide the Host HTTP header during the websocket handshake**
            std::string http_header = host_ip + ':' + std::to_string(endpoint.port());
            //set the suggested timeout settings for the websocket as the client
            self_object->stream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            self_object->stream.set_option(websocket::stream_base::decorator([](websocket::request_type& request) //***
            {
                request.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-client-async");
            }));
            self_object->stream.async_handshake(http_header,"/",[self_object](boost::system::error_code errcode3)   //websocket handshake
            {
                if(errcode3)
                {
                    try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                    catch(...)
                    {
                        self_object->ongoing_connection = true;
                        self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
                    }
                }
                std::cout << "Client connected successfully" << std::endl;
                self_object->ongoing_connection = true;
                //all function are successfull
                self_object->receive_message(); //trigger receive message call
                while(self_object->ongoing_connection.load())
                {
                    if(self_object->send_messages_queue.size() > 0) //there's a message to send
                        self_object->write_message();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
                }
                std::cout << "Connection is ended" << std::endl; //ongoing_connection is false now
            });
        });
    });
    //run client context in different threads for read and write
    net::post(client_pool,[this](){io_ctx.run();});
    net::post(client_pool,[this](){io_ctx.run();});
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //if connection is timeout and not successfull return false, else true
    while((i++<30) && (!ongoing_connection.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_connection.load())  //if connection is not successfull
    {
        io_ctx.stop();
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
    if(!io_ctx.stopped())   //stop context
        io_ctx.stop();
    try
    {
        stream.close(websocket::close_code::normal);
        std::cout << "read canceled and mutex locked for session close" << std::endl;
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
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
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void wss_client::receive_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(1);
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_read(*buffer,net::bind_executor(strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode)
        {
            self_object->disconnect(1);
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
        std::cout << "Client received Message: " << rec_string << std::endl;
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void wss_client::write_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->disconnect(1);
        return;
    }
    send_mutex.lock();
    std::vector<unsigned char> message = send_messages_queue.front();
    send_messages_queue.pop_front();    //read front then pop
    send_mutex.unlock();
    net::const_buffer buffer(message.data(), message.size());
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_write(buffer,net::bind_executor(strand,[self_object](beast::error_code errcode, std::size_t bytes_sent_dummy)
    {
        if(errcode) //failed to send, disconnect
        {
            self_object->disconnect(1);
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
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
void wss_client::disconnect(int)
{
    if(!ongoing_connection.load())  //if already disconnected
        return; //do nothing and return
    ongoing_connection = false;
    if(!io_ctx.stopped())   //stop context
        io_ctx.stop();
    try
    {
        stream.close(websocket::close_code::protocol_error);
        std::cout << "read canceled and mutex locked for session close" << std::endl;
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
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
* Parameters (out): NONE
* Return value: NONE
* Description:
*
*
************************************************************************************************************************/
bool wss_client::connect(std::string& ip_address, unsigned short port)
{
    if(!Set_SSL_CTX(ssl_ctx,key,certificate))   //if failed to set SSL options
        return false;
    std::string host_ip = ip_address;
    std::string host_port = std::to_string(port);
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    resolver.async_resolve(host_ip,host_port,[host_ip,self_object](boost::system::error_code errcode, tcp::resolver::results_type result)   //resolve IP and port
    {
        std::cout << "Lambda called" << std::endl;
        if(errcode)
        {
            self_object->ongoing_connection = true;
            self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
        }
        net::async_connect(self_object->stream.next_layer().next_layer(),result,    //TCP async connection (start connection)
        [host_ip,self_object](boost::system::error_code errcode2, const tcp::endpoint endpoint)
        {
            if(errcode2)
            {
                try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                catch(...)
                {
                    self_object->ongoing_connection = true;
                    self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
                }
            }
            //**update host string, to provide the Host HTTP header during the websocket handshake**
            std::string http_header = host_ip + ':' + std::to_string(endpoint.port());
            //**Set Server Name Indication (SNI) hostname, (many hosts need this to handshake successfully)**
            if(!SSL_set_tlsext_host_name(self_object->stream.next_layer().native_handle(),http_header.c_str()))
            {
                try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                catch(...)
                {
                self_object->ongoing_connection = true;
                self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
                }
            }
            //SSL handshake, client side
            self_object->stream.next_layer().async_handshake(ssl::stream_base::client,[http_header,self_object](boost::system::error_code errcode3)
            {
                if(errcode3)
                {
                    try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                    catch(...)
                    {
                        self_object->ongoing_connection = true;
                        self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
                    }
                }
                //set the suggested timeout settings for the websocket as the client
                self_object->stream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                self_object->stream.set_option(websocket::stream_base::decorator([](websocket::request_type& request) //***
                {
                    request.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-client-async-ssl");
                }));
                self_object->stream.async_handshake(http_header,"/",[self_object](boost::system::error_code errcode4) //websocket handshake
                {
                    if(errcode4)
                    {
                        try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                        catch(...)
                        {
                            self_object->ongoing_connection = true;
                            self_object->disconnect(1); //setting "ongoing_connection" by true, to be able to disconnect
                        }
                    }
                    std::cout << "Client connected successfully" << std::endl;
                    self_object->ongoing_connection = true;
                    //all function are successfull
                    self_object->receive_message(); //trigger receive message call
                    while(self_object->ongoing_connection.load())
                    {
                        if(self_object->send_messages_queue.size() > 0) //there's a message to send
                            self_object->write_message();
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
                    }
                    std::cout << "Connection is ended" << std::endl; //ongoing_connection is false now

                });
            });
        });
    });
    //run client context in different threads for read and write
    net::post(client_pool,[this](){io_ctx.run();});
    net::post(client_pool,[this](){io_ctx.run();});
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //if connection is timeout and not successfull return false, else true
    while((i++<30) && (!ongoing_connection.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_connection.load())  //if connection is not successfull
    {
        io_ctx.stop();
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
    if(!io_ctx.stopped())   //stop context
        io_ctx.stop();
    try
    {
        stream.close(websocket::close_code::normal);
        std::cout << "read canceled and mutex locked for session close" << std::endl;
    }
    catch(...){} //suppress exceptions, object is deleted afterwards
}
