/************************************************************************************************************************
 * 	Module: Websockets Server
 * 	File Name: websockets_server.cpp
 *  Authors: Ahmed Desoky
 *	Date: 18/1/2025
 *	*********************************************************************************************************************
 *	Description: Source file for websockets servers and sessions handling.
 *               This file contains all member functions implementations
 *               of all WebSocket and WebSocket Secure base and derived classes.
 *               Please read header file documentation for more understanding.
 ***********************************************************************************************************************/
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include "websockets_server.h"
ws_server* ws_server::server_instance = nullptr; // Initialize static pointer to nullptr
std::mutex ws_server::access_mutex;             // Initialize static mutex
wss_server* wss_server::server_instance = nullptr; // Initialize static pointer to nullptr
std::mutex wss_server::access_mutex;             // Initialize static mutex
wss_server* wss_server::server_instance2 = nullptr;  // Initialize static pointer to nullptr
std::mutex wss_server::access_mutex2;             // Initialize static mutex
constexpr int connection_timeout = 4;  //in seconds
/***********************************************************************************************************************
 *                     					    FUNCTIONS DEFINTITIONS
 ***********************************************************************************************************************/
/************************************************************************************************************************
* Function Name: accept_connection
* Class name: ws_server_base
* Access: Protected
* Specifiers: NONE
* Running Thread: Caller thread for first time only then by Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Internal function called at first by "ws_server_base::start". Then called each time by its async handler.
*              to accept new connection. As long as server is running.
*              access to the async handler is secured by "session_establishment_mutex" mutex.
************************************************************************************************************************/
void ws_server_base::accept_connection(void)
{
    //current connection being in establishment finishes then to the next new establishment
    if(!server_running.load())  //if server is not running. accept no more connections
        return;
    session_establishment_mutex.lock();
    tcp_acceptor.async_accept([this](boost::system::error_code errcode,tcp::socket socket)
    {
        if(errcode == boost::asio::error::operation_aborted)    //operation canceled
        {
            socket.close();
            session_establishment_mutex.unlock();
            return;
        }
        else if(errcode)    //any other error
        {
            socket.close();
        }
        else if(session_count < max_sessions)
        {
            //Push session metadata and handler to the map, then pop them from the session object at exit
            session_count.fetch_add(1); //thread-safely increment the session counter, locked by mutex
            g_sessions.lock();
            auto min_id_iter = std::min_element(sessions_ids.begin(),sessions_ids.end());
            int new_session_id = *min_id_iter;  //get the min available id
            sessions_ids.erase(min_id_iter);
            std::shared_ptr<session_abstract> new_session;  //shared_ptr to the new session
            if(secure) //if server is secure
                new_session = std::make_shared<wss_session>(new_session_id,session_count,sessions_ids,sessions,
                    g_sessions,*io_ctx,std::move(socket),*ssl_ctx);
            else    //not secure
                new_session = std::make_shared<ws_session>(new_session_id,session_count,sessions_ids,sessions,
                    g_sessions,*io_ctx,std::move(socket));
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
            }
        }
        else
        {
            socket.close();
        }
        session_establishment_mutex.unlock();   //if locked,
        accept_connection();  // Continue accepting new connections
    });
}
/************************************************************************************************************************
* Function Name: start
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: User function to start server and start server worker threads.
*              shared access to the function from many threads is secured by "start_mutex" mutex. For thread safety
************************************************************************************************************************/
void ws_server_base::start(void)
{
    std::lock_guard<std::mutex> lock(start_mutex);
    if(server_running)
        return; //already running
    server_running = true;
    for(std::size_t k=0;k<max_sessions;++k) //initialize IDs
        sessions_ids.insert(k+1);
    tcp_acceptor = tcp::acceptor(*io_ctx,tcp::endpoint(tcp::v4(),server_port));  //initialze the tcp_accpetor
    tcp_acceptor.listen();
    this->accept_connection();
    threads_pool = std::make_unique<net::thread_pool>(max_sessions*2);
    for (std::size_t i=0; i < max_sessions*2; ++i)  //here check after stopping and starting again
        net::post(*threads_pool, [this](){io_ctx->run();});
    return;
}
/************************************************************************************************************************
* Function Name: stop
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: User function to stop server and release are resources and stop all working threads.
*              shared access to the function from many threads is secured by "stop_mutex" mutex. For thread safety
************************************************************************************************************************/
void ws_server_base::stop(void)
{
    std::lock_guard<std::mutex> lock(stop_mutex);
    if(!server_running)
        return;
    server_running = false; //stop server
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before stopping
    tcp_acceptor.cancel();  //cancel all tcp connections
    for(auto& sess_iter : sessions) //close all running sessions
    {
        if(this->sessions.size() == 0)
            break;
        this->close_session(sess_iter.first);
    }
    tcp_acceptor.close();   //close port, all connections will be closed automatically
    io_ctx->stop();  //make sure to stop context
    threads_pool->join();    //join threads until it ends
    threads_pool.reset();   //destory/delete threads pool object
    io_ctx.reset();
    io_ctx = std::make_unique<net::io_context>();
    sessions.clear();   //clear threads pool
    sessions_ids.clear();
    session_count = 0;
    if(!session_establishment_mutex.try_lock()) //unlock this mutex if it is locked, assure its unlocked
        session_establishment_mutex.unlock();
    else
        session_establishment_mutex.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));    //delay before ending
}
/************************************************************************************************************************
* Function Name: is_serving
* Class name: ws_server_base
* Access: Public
* Specifiers: inline
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): server serving status
* Return value: server serving status as boolean
* Description: User function to check whether server is serving clients and there are ongoing sessions or not
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
inline bool ws_server_base::is_serving(void)
{
    if(session_count.load() > 0)
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name: is_running
* Class name: ws_server_base
* Access: Public
* Specifiers: inline
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): server running status
* Return value: server running status as boolean
* Description: User function to check whether server is running or not
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
inline bool ws_server_base::is_running(void)
{
    if(server_running.load())
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name: sessions_count
* Class name: ws_server_base
* Access: Public
* Specifiers: inline
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): number of ongoing sessions/clients being served
* Return value: number of ongoing sessions/clients being served as integer
* Description: User function to get number of ongoing sessions by server
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
inline int ws_server_base::sessions_count(void)
{
    return static_cast<int>(session_count.load());
}
/************************************************************************************************************************
* Function Name: send_message
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): session id to send the message to
*                  message to be sent as const reference to vector of unsigned characters
* Parameters (out): Operation status whether successful or not
* Return value: Operation status whether successful or not as boolean.
* Description: User function to send a message to specific session by the server
*              shared access to the function from many threads is secured by "send_message_mutex" mutex. For thread safety
************************************************************************************************************************/
bool ws_server_base::send_message(int id, const std::vector<unsigned char>& message)
{
    std::lock_guard<std::mutex> lock(send_message_mutex);
    auto session_iter = sessions.find(id);  //get session handler from sessions map container
    if(session_iter == sessions.end())   //id not found, not running
        return false;
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    session_handler.lock()->send_message(message);
    return true;
}
/************************************************************************************************************************
* Function Name: read_message
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): session id to read the message from its queue
* Parameters (out): message read, empty if read failed
* Return value: message read as vector of unsigned characters, empty if read failed
* Description: User function to read messages from specific session's queue by the server
*              shared access to the function from many threads is secured by "read_message_mutex" mutex. For thread safety
************************************************************************************************************************/
std::vector<unsigned char> ws_server_base::read_message(int id)
{
    std::lock_guard<std::mutex> lock(read_message_mutex);
    std::vector<unsigned char> msg;
    auto session_iter = sessions.find(id);  //get session handler from sessions map container
    if(session_iter == sessions.end())   //id not found, not running
        return msg; //empty message
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    msg = session_handler.lock()->read_message();
    return msg;
}
/************************************************************************************************************************
* Function Name: check_inbox
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): session id to check its inbox/queue
* Parameters (out): whether there are messages to read or not
* Return value: whether there are messages to read or not as boolean
* Description: User function to check the read queue/inbox of specific session by server
*              shared access to the function from many threads is secured by "check_inbox_mutex" mutex. For thread safety
************************************************************************************************************************/
bool ws_server_base::check_inbox(int id)
{
    std::lock_guard<std::mutex> lock(check_inbox_mutex);
    auto session_iter = sessions.find(id);  //get session handler from sessions map container
    if(session_iter == sessions.end())   //id not found, not running
        return false;   //id not found or not running
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    if(!session_handler.lock()->check_inbox()) //if inbox empty
        return false;   //inbox empty
    return true;
}
/************************************************************************************************************************
* Function Name: check_session
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): session id to check its status
* Parameters (out): whether the session is ongoing or not
* Return value: whether the session is ongoing or not as boolean
* Description: User function to check the status of specific session if it's ongoing or not by server
*              shared access to the function from many threads is secured by "session_check_mutex" mutex. For thread safety
************************************************************************************************************************/
bool ws_server_base::check_session(int id)
{
    std::lock_guard<std::mutex> lock(session_check_mutex);
    auto session_iter = sessions.find(id);  //get session handler from sessions map container
    if(session_iter == sessions.end())   //id not found, not running
        return false;
    return true;
}
/************************************************************************************************************************
* Function Name: close_session
* Class name: ws_server_base
* Access: Public
* Specifiers: NONE
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): session id to close
* Parameters (out): NONE
* Return value: NONE
* Description: User function to close a specific session by server using its ID
*              shared access to the function from many threads is secured by "session_close_mutex" mutex. For thread safety
************************************************************************************************************************/
void ws_server_base::close_session(int id)
{
    std::lock_guard<std::mutex> lock(session_close_mutex);
    auto session_iter = sessions.find(id);  //get session handler from sessions map container
    if(session_iter == sessions.end())   //id not found, not running
        return;
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    session_handler.lock()->stop();
}
/************************************************************************************************************************
* Function Name: read_message
* Class name: ws_session_base
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): message from the session read queue
* Return value: message from the session read queue as vector of unsigned characters, empty if queue is empty
* Description: Protected function to read message from sessin by server
*              shared variables and racing to the function is secured by "read_mutex" mutex.
************************************************************************************************************************/
std::vector<unsigned char> ws_session_base::read_message(void)
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
* Class name: ws_session_base
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): message to be sent as const reference to vector of unsigned characters
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to send message to a session by server
*              shared variables and racing to the function is secured by "send_mutex" mutex.
************************************************************************************************************************/
void ws_session_base::send_message(const std::vector<unsigned char>& message)
{
    send_mutex.lock();
    send_messages_queue.push_back(std::move(message));
    send_mutex.unlock();
    this->write_message();  //call write message and give the write order
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending and writing
    return;
}
/************************************************************************************************************************
* Function Name: check_inbox
* Class name: ws_session_base
* Access: Protected - Accessed only by server classes
* Specifiers: inline
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): read queue/inbox status whether there are messages to read or empty
* Return value: read queue/inbox status as boolean whether there are messages to read or empty
* Description: Protected function to check read queue of a session by server.
************************************************************************************************************************/
inline bool ws_session_base::check_inbox(void)
{
    if(read_messages_queue.size()>0)
        return true;
    return false;
}
/************************************************************************************************************************
* Function Name: check_session
* Class name: ws_session_base
* Access: Protected - Accessed only by server classes
* Specifiers: inline
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): session status whether its ongoing or not
* Return value: session status as boolean whether its ongoing or not
* Description: Protected function to check session statu by server whether ongoing or not.
*              Accessing atomic variable for thread safety.
************************************************************************************************************************/
inline bool ws_session_base::check_session(void)
{
    return ongoing_session.load();
}
/************************************************************************************************************************
* Function Name: start
* Class name: ws_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to start a new session by server. session with no TLS/SSL underlayer.
*              server starts the new session as new client tries to connect to the server.
*              if the session is started successfully, it starts receiving operations and update its status.
*              if session failed to start, it stops and disconnects the client and delete its metadata
*              metadata: session id, session's handler in sessions map, decrement sessions counter by 1.
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
            self_object->ongoing_session = true;
            self_object->stop(-1);   //set "ongoing_session" to true to allow session closing
            return;
        }
        // All functions are successfull
        self_object->ongoing_session = true;
        self_object->receive_message(); //trigger receive message call
    });
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //timeout check loop
    while((i++<connection_timeout) && (!ongoing_session.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_session.load())  //if connection is not successfull
    {
        self_object->stop();   //stop if not stopped
        return;
    }
}
/************************************************************************************************************************
* Function Name: stop(1)
* Class name: ws_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to stop thesession by server willingly. session with no TLS/SSL underlayer.
*              It stops and disconnects the client and delete its metadata
*              metadata: session id, session's handler in sessions map, decrement sessions counter by 1.
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
    try
    {
        if (stream.is_open())
            stream.close(websocket::close_code::normal);
    }
    catch(...) {} //suppress exceptions
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: stop(2)
* Class name: ws_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to stop the session by session itself in error cases. session with no TLS/SSL underlayer.
*              It stops and disconnects the client and delete its metadata
*              metadata: session id, session's handler in sessions map, decrement sessions counter by 1.
************************************************************************************************************************/
void ws_session::stop(int code)
{
    if(!ongoing_session.load()) //if session is already stopped
        return; //do nothing and return
    ongoing_session = false;
    g_sessions.lock(); //shared mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
    if(code == 0)
    {
        try
        {
            if (stream.is_open())
                stream.close(websocket::close_code::normal);
        }
        catch(...) {} //suppress exception
    }
    else if(code == -1)
    {
        try
        {
            if (stream.is_open())
                stream.close(websocket::close_code::protocol_error);
        }
        catch(...) {} //suppress exception
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: receive_message
* Class name: ws_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to receieve messages for the session. session with no TLS/SSL underlayer.
*              It contains its own handler as Lambda functions called upon receiving new message then
*              "ws_session::receive_message" is called again after handler execution.
*              This function exits completely at session closure.
*              Access to the queue and shared variables is secured by "read_mutex" mutex
************************************************************************************************************************/
void ws_session::receive_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(-1);   //stop session
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_read(*buffer,net::bind_executor(strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode)
        {
            self_object->stop(-1);   //stop session
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
        self_object->receive_message(); //receive again
    }));
}
/************************************************************************************************************************
* Function Name: write_message
* Class name: ws_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Parameters (in): NONE
* Expected  Exception: No
* Parameters (out): NONE
* Return value: NONE
* Description: Internal Asynchronous function called after a each "ws_server_base::send_message" function call
*              It contains its own handler as Lambda functions called upon sending the new message to handle any error occurs.
*              Access to the queue and shared variables is secured by "send_mutex" mutex
*              session is not with TLS/SSL underlayer.
************************************************************************************************************************/
void ws_session::write_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(-1);   //stop session
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
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode) //failed to send
        {
            self_object->stop(-1);   //stop session
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
/************************************************************************************************************************
* Function Name: start
* Class name: wss_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to start a new session by server. session with TLS/SSL underlayer.
*              server starts the new session as new client tries to connect to the server.
*              if the session is started successfully, it starts receiving operations and update its status.
*              if session failed to start, it stops and disconnects the client and delete its metadata
*              metadata: session id, session's handler in sessions map, decrement sessions counter by 1.
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
            self_object->ongoing_session = true;
            self_object->stop(-1);   //set "ongoing_session" to true to allow session closing
            return;
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
                self_object->ongoing_session = true;
                self_object->stop(-1);//set "ongoing_session" to true to allow session closing
                return;
            }
            // All functions are successfull
            self_object->ongoing_session = true;
            //all function are successfull
            self_object->receive_message(); //trigger receive message call
        });
    });
    //Boost's default handshake timeout connection for websocket is 30seconds
    int i = 0;  //timeout check loop
    while((i++<connection_timeout) && (!ongoing_session.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_session.load())  //if connection is not successfull
    {
        self_object->stop();   //stop if not stopped
        return;
    }
}
/************************************************************************************************************************
* Function Name: stop(1)
* Class name: wss_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to stop thesession by server willingly. session with TLS/SSL underlayer.
*              It stops and disconnects the client and delete its metadata
*              metadata: session id, session's handler in sessions map, decrement sessions counter by 1.
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
    try
    {
        if (stream.is_open())
            stream.close(websocket::close_code::normal);
    }
    catch(...) {} //suppress exception
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: stop(2)
* Class name: wss_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to stop the session by session itself in error cases. session with TLS/SSL underlayer.
*              It stops and disconnects the client and delete its metadata
*              metadata: session id, session's handler in sessions map, decrement sessions counter by 1.
************************************************************************************************************************/
void wss_session::stop(int code)
{
    if(!ongoing_session.load()) //if session is already stopped
        return; //do nothing and return
    ongoing_session = false;
    g_sessions.lock(); //static mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
    if(code == 0)
    {
        try
        {
            if (stream.is_open())
                stream.close(websocket::close_code::normal);
        }
        catch(...) {} //suppress exception
    }
    else if(code == -1)
    {
        try
        {
            if (stream.is_open())
                stream.close(websocket::close_code::protocol_error);
        }
        catch(...) {} //suppress exception
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));    //delay before ending
}
/************************************************************************************************************************
* Function Name: receive_message
* Class name: wss_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Expected  Exception: No
* Parameters (in): NONE
* Parameters (out): NONE
* Return value: NONE
* Description: Protected function to receieve messages for the session. session with TLS/SSL underlayer.
*              It contains its own handler as Lambda functions called upon receiving new message then
*              "ws_session::receive_message" is called again after handler execution.
*              This function exits completely at session closure.
*              Access to the queue and shared varibales is secured by "read_mutex" mutex
************************************************************************************************************************/
void wss_session::receive_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(-1);   //stop session
        return;
    }
    std::shared_ptr<beast::flat_buffer> buffer = std::make_shared<beast::flat_buffer>();
    //to avoid object destroying during async operations and keep the object alive until end of the scope of "self_object" shared_ptr
    auto self_object = shared_from_this();
    stream.async_read(*buffer,net::bind_executor(strand,[buffer,self_object](beast::error_code errcode,std::size_t bytes_received) mutable //mutable lambda expression
    {
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode)
        {
            self_object->stop(-1);   //stop session
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
        self_object->receive_message(); //receive again
    }));
}
/************************************************************************************************************************
* Function Name: write_message
* Class name: wss_session
* Access: Protected - Accessed only by server classes
* Specifiers: NONE
* Running Thread: Pool thread
* Sync/Async: Asynchronous
* Reentrancy: Reentrant
* Parameters (in): NONE
* Expected  Exception: No
* Parameters (out): NONE
* Return value: NONE
* Description: Internal Asynchronous function called after a each "ws_server_base::send_message" function call
*              It contains its own handler as Lambda functions called upon sending the new message to handle any error occurs.
*              Access to the queue and shared varibales is secured by "send_mutex" mutex.
*              session is with TLS/SSL underlayer.
************************************************************************************************************************/
void wss_session::write_message(void)
{
    if(!stream.is_open())   //if stream is closed or there's no connection
    {
        this->stop(-1);   //stop session
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
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode == boost::asio::error::eof)
        {
            self_object->stop(0);   //stop session
            return;
        }
        else if(errcode) //failed to send
        {
            self_object->stop(-1);   //stop session
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}
