/************************************************************************************************************************
 * 	Module: Websockets Server
 * 	File Name: websockets_server.cpp
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Source file for websockets servers and sessions handling
 *
 *
 ***********************************************************************************************************************/
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include "websockets_server.h"

ws_server* ws_server::server_instance = nullptr; // Initialize static pointer to nullptr
std::mutex ws_server::access_mutex;             // Initialize static mutex

wss_server* wss_server::server_instance = nullptr; // Initialize static pointer to nullptr
std::mutex wss_server::access_mutex;             // Initialize static mutex
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
void ws_server_base::accept_connection(void)
{
    //current connection being in establishment finishes then to the next new establishment
    if(!server_running.load())  //if server is not running. accept no more connections
        return
    session_establishment_mutex.lock();
    tcp_acceptor.async_accept([this](boost::system::error_code errcode,tcp::socket socket)
    {
        if(errcode)
        {
            socket.close();
            session_establishment_mutex.unlock();
        }
        else if(session_count < max_sessions)
        {
            //Push session metadata and handler to the map, then pop them from the session object at exit
            session_count.fetch_add(1); //thread-safely increment the session counter, locked by mutex
            std::cout << "current session: " << session_count.load() << std::endl;
            g_sessions.lock();
            auto min_id_iter = std::min_element(sessions_ids.begin(),sessions_ids.end());
            int new_session_id = *min_id_iter;  //get the min available id
            sessions_ids.erase(min_id_iter);
            std::shared_ptr<session_abstract> new_session;  //shared_ptr to the new session
            if(secure) //if server is secure
                new_session = std::make_shared<wss_session>(new_session_id,session_count,sessions_ids,sessions,
                    g_sessions,io_ctx,std::move(socket),*ssl_ctx);
            else    //not secure
                new_session = std::make_shared<ws_session>(new_session_id,session_count,sessions_ids,sessions,
                    g_sessions,io_ctx,std::move(socket));
            sessions.insert({new_session_id,new_session});  //push the session handler and id to the map to allow its handle
            g_sessions.unlock();
            try{new_session->start();}   //start session. to handle "start" exceptions running in this thread
            catch(...)  //in case of exception and error, remove inserted metadata of the session
            {
                g_sessions.lock();
                sessions.erase(new_session_id); //erase the session handler from the map
                sessions_ids.insert(new_session_id); //insert the id back to the container to release the id
                g_sessions.unlock();
                session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
                std::cout << "current sessions: " << session_count.load() << std::endl;
            }
            session_establishment_mutex.unlock();
        }
        else
        {
            socket.close();
            std::cout << "session rejected" << std::endl;
            session_establishment_mutex.unlock();
        }
        accept_connection();  // Continue accepting new connections
    });
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
bool ws_server_base::start(void)
{
    if(server_running)
        return true;    //already running
    server_running = true;
    this->accept_connection();
    for(std::size_t k=0;k<max_sessions;++k) //initialize IDs
        sessions_ids.insert(k+1);
    //creating number of threads equal to max allowed number of sessions x2 and run the io_context in all these threads
    //this will make the server handle different sessions in different threads concurrently, each session 2threads, 1read/1write
    for (std::size_t i=0; i < max_sessions*2; ++i)
        net::post(threads_pool, [this](){io_ctx.run();});
    return true;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_server_base::stop(void)
{
    if(!server_running)
        return; //already stopped
    server_running = false; //stop server
    tcp_acceptor.cancel();  //cancel all tcp connections
    tcp_acceptor.close();   //stop accepting tcp connections
    threads_pool.stop();    //stop all threads
    io_ctx.stop();  //stop context
    std::this_thread::sleep_for(std::chrono::milliseconds(500));//sleep until all handlers are executed and threads are ended
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
inline bool ws_server_base::is_serving(void)
{
    if(session_count.load() > 0)
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
inline bool ws_server_base::is_running(void)
{
    if(server_running.load())
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
inline int ws_server_base::sessions_count(void)
{
    return static_cast<int>(session_count.load());
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
bool ws_server_base::send_message(int id, const std::vector<unsigned char>& message)
{
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
        return false;
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    session_handler.lock()->send_message(message);
    return true;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
std::vector<unsigned char> ws_server_base::read_message(int id)
{
    std::vector<unsigned char> msg;
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
        return msg;
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    msg = session_handler.lock()->read_message();
    return msg;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
bool ws_server_base::check_inbox(int id)
{
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
        return false;
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    if(!session_handler.lock()->check_inbox()) //if inbox empty
        return false;
    return true;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
bool ws_server_base::check_session(int id)
{
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
        return false;
    return true;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_server_base::close_session(int id)
{
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
        return;
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    session_handler.lock()->stop();
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
inline bool wss_server::start(void)
{
    if(server_running)
        return true;    //already running
    if(!Set_SSL_CTX(ssl_ctx,key,certificate))   //if failed to set ssl_ctx options
        return false;
    ws_server_base::start();    //start server by the inherited function
    return true;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
std::vector<unsigned char> ws_session_base::read_message(void)
{
    read_mutex.lock();
    std::vector<unsigned char> message = read_messages_queue.front();
    read_messages_queue.pop_front();
    read_mutex.unlock();
    std::cout << "message received by server: " << std::string(message.begin(),message.end()) << ", by session num: " << session_id << std::endl;
    return message;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_session_base::send_message(const std::vector<unsigned char>& message)
{
    send_mutex.lock();
    send_messages_queue.push_back(std::move(message));
    send_mutex.unlock();
    return;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
inline bool ws_session_base::check_inbox(void)
{
    if(read_messages_queue.size()>0)
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
inline bool ws_session_base::check_session(void)
{
    return ongoing_session.load();
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_session::start(void)
{
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    //set the suggested timeout settings for the websocket as the server
    self_object->stream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    self_object->stream.set_option(websocket::stream_base::decorator([](websocket::response_type& response)//****
    {
        response.set(http::field::server,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-server-async");
    }));
    stream.async_accept([self_object](boost::system::error_code errcode) mutable //mutable lambda expression
    {
        if(errcode)
        {
            try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
            catch(...)
            {
                self_object->ongoing_session = true;
                self_object->stop(1);   //set "ongoing_session" to true to allow session closing
            }
        }
        // All functions are successfull
        std::cout << "Server acquired new connection" << std::endl;
        self_object->ongoing_session = true;
        self_object->receive_message();
        while(self_object->ongoing_session.load())
        {
            if(self_object->send_messages_queue.size() > 0) //there's a message to send
                self_object->write_message();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
        }
    });
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //if connection is timeout throw an exception and it will be handled by server
    while((i++<30) && (!ongoing_session.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_session.load())  //if connection is not successfull
    {
        self_object->ongoing_session = true;
        self_object->stop(1);   //set "ongoing_session" to true to allow session closing
    }
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_session::stop(void)
{
    if(!ongoing_session.load()) //if session is already stopped
        return; //do nothing and return
    ongoing_session = false;
    g_sessions.lock(); //shared mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1); //thread-safely decrement the session counter at server class, locked by static mutex
    std::cout << "session closed" << std::endl;
    try
    {stream.close(websocket::close_code::normal);}
    catch(...) {} //suppress exception
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_session::stop(int)
{
    if(!ongoing_session.load()) //if session is already stopped
        return; //do nothing and return
    ongoing_session = false;
    g_sessions.lock(); //shared mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
    std::cout << "session closed" << std::endl;
    try
    {stream.close(websocket::close_code::protocol_error);}
    catch(...){}  //suppress exception
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_session::receive_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(1);   //stop session
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_read(*buffer,net::bind_executor(strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode)
        {
            self_object->stop(1);   //stop session
            return;
        }
        if (buffer->size() == 0) //empty buffer, receive again
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
        std::cout << "Server received Message: " << rec_string << std::endl;

        //self_object->send_message(received_data);   //for testing //------------------------

        self_object->receive_message(); //receive again
    }));
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void ws_session::write_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(1);   //stop session
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
        if(errcode) //failed to send
        {
            self_object->stop(1);   //stop session
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void wss_session::start(void)
{
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.next_layer().async_handshake(ssl::stream_base::server, //make the SSL handshake,server side, session key sharing
    [self_object](boost::system::error_code errcode)   //and certificates verification if exists
    {
        if(errcode)
        {
            try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
            catch(...)
            {
            self_object->ongoing_session = true;
            self_object->stop(1);   //set "ongoing_session" to true to allow session closing
            }
        }
        //set the suggested timeout settings for the websocket as the server
        self_object->stream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        self_object->stream.set_option(websocket::stream_base::decorator([](websocket::response_type& response)//****
        {
            response.set(http::field::server,std::string(BOOST_BEAST_VERSION_STRING)+"websocket-server-async-ssl");
        }));
        self_object->stream.async_accept([self_object](boost::system::error_code errcode2) mutable  //mutable lambda expression
        {
            if(errcode2)
            {
                try{self_object->stream.close(websocket::close_code::protocol_error);} /*close stream, due to protocol error*/
                catch(...)
                {
                    self_object->ongoing_session = true;
                    self_object->stop(1);//set "ongoing_session" to true to allow session closing
                }
            }
            // All functions are successfull
            std::cout << "Server acquired new connection" << std::endl;
            self_object->ongoing_session = true;
            self_object->receive_message();
            while(self_object->ongoing_session.load())
            {
                if(self_object->send_messages_queue.size() > 0) //there's a message to send
                    self_object->write_message();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
            }
        });
    });
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //if connection is timeout throw an exception and it will be handled by server
    while((i++<30) && (!ongoing_session.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_session.load())  //if connection is not successfull
    {
        self_object->ongoing_session = true;
        self_object->stop(1);   //set "ongoing_session" to true to allow session closing
    }
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void wss_session::stop(void)
{
    if(!ongoing_session.load()) //if session is already stopped
        return; //do nothing and return
    ongoing_session = false;
    g_sessions.lock(); //static mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
    std::cout << "session closed" << std::endl;
    try
    {stream.close(websocket::close_code::normal);}
    catch(...) {} //suppress exception
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void wss_session::stop(int)
{
    if(!ongoing_session.load()) //if session is already stopped
        return; //do nothing and return
    ongoing_session = false;
    g_sessions.lock(); //static mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
    std::cout << "session closed" << std::endl;
    try
    {stream.close(websocket::close_code::protocol_error);}
    catch(...) {} //suppress exception
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void wss_session::receive_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(1);   //stop session
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_read(*buffer,net::bind_executor(strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode)
        {
            self_object->stop(1);   //stop session
            return;
        }
        if (buffer->size() == 0) //empty buffer, receive again
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
        std::cout << "Server received Message: " << rec_string << std::endl;

        //self_object->send_message(received_data);   //for testing //------------------------

        self_object->receive_message(); //receive again
    }));
}
/************************************************************************************************************************
* Function Name:
* Class name:
* Access:
* Specifiers:
* Running Thread: Mainthread
* Sync/Async: Asynchronous
* Reentrancy: Non-reentrant
* Parameters (in):
* Parameters (out):
* Return value:
* Description:
*
*
************************************************************************************************************************/
void wss_session::write_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(1);   //stop session
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
        if(errcode) //failed to send
        {
            self_object->stop(1);   //stop session
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}

