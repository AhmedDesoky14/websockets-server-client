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

wss_server* wss_server::server_instance2 = nullptr;  // Initialize static pointer to nullptr
std::mutex wss_server::access_mutex2;             // Initialize static mutex


constexpr int connection_timeout = 3;  //in seconds




/***********************************************************************************************************************
 *                     					    FUNCTIONS DEFINTITIONS
 ***********************************************************************************************************************/

void ws_server_base::lock_resources(ws_server_base* instance_ptr)
{
    //Safety for multithreads if block code
    if(dynamic_cast<wss_server*>(instance_ptr)) //if the instance calling this function is "wss_server" (the ptr can be cast to it)
    {
        if(instance_ptr->verification_on) //if it's the one secured by verification
            wss_server::access_mutex.lock();
        else    //if it's the one not secured by verification
            wss_server::access_mutex2.lock();
    }
    else    //if the instance calling this function is "ws_server"
        ws_server::access_mutex.lock();
}

void ws_server_base::unlock_resources(ws_server_base* instance_ptr)
{
    //Safety for multithreads if block code
    if(dynamic_cast<wss_server*>(instance_ptr)) //if the instance calling this function is "wss_server" (the ptr can be cast to it)
    {
        if(instance_ptr->verification_on) //if it's the one secured by verification
            wss_server::access_mutex.unlock();
        else    //if it's the one not secured by verification
            wss_server::access_mutex2.unlock();
    }
    else    //if the instance calling this function is "ws_server"
        ws_server::access_mutex.unlock();
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
void ws_server_base::accept_connection(void)
{
    //current connection being in establishment finishes then to the next new establishment
    if(!server_running.load())  //if server is not running. accept no more connections
        return;
    session_establishment_mutex.lock();
    tcp_acceptor.async_accept([this](boost::system::error_code errcode,tcp::socket socket)
    {
        if(!server_running.load())  //if server is not running. accept no more connections
            return;
        ws_server_base::lock_resources(this);
        if(errcode)
        {
            socket.close();
            std::cout << "A session failed to start" << std::endl;
            //ws_server_base::unlock_resources(this);
            //session_establishment_mutex.unlock();
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
            }
            //ws_server_base::unlock_resources(this);
            //session_establishment_mutex.unlock();
        }
        else
        {
            socket.close();
            std::cout << "A session rejected" << std::endl;
            //ws_server_base::unlock_resources(this);
            //session_establishment_mutex.unlock();
        }
        ws_server_base::unlock_resources(this);
        session_establishment_mutex.unlock();
        accept_connection();  // Continue accepting new connections
    });

    //current connection being in establishment finishes then to the next new establishment
    // if(!server_running.load())  //if server is not running. accept no more connections
    //     return
    //         session_establishment_mutex.lock();
    // tcp_acceptor.async_accept([this](boost::system::error_code errcode,tcp::socket socket)
    //                           {
    //                               if(errcode)
    //                               {
    //                                   socket.close();
    //                                   session_establishment_mutex.unlock();
    //                               }
    //                               else if(session_count < max_sessions)
    //                               {

    //                                   // auto& new_io_context = sessions_contexts[next_session_context_index()];
    //                                   // net::io_context new_io_context;
    //                                   // auto new_socket = tcp::socket(new_io_context, socket);
    //                                   // net::executor_work_guard<net::io_context::executor_type> work_guard(new_io_context);
    //                                   // new_io_context.run();
    //                                   // threads_pool.emplace_back([&,this]
    //                                   // {
    //                                   //     (std::make_shared<wss_session_base>(io_ctx,std::move(socket),ssl_ctx))->start();
    //                                   // });
    //                                   //Push session metadata and handler to the map, then pop them from the session object at exit
    //                                   session_count.fetch_add(1); //thread-safely increment the session counter, locked by mutex
    //                                   //++session_count;
    //                                   std::cout << "current session: " << session_count.load() << std::endl;
    //                                   g_sessions.lock();
    //                                   //int new_session_id = Get_ID();
    //                                   auto min_id_iter = std::min_element(sessions_ids.begin(),sessions_ids.end());
    //                                   int new_session_id = *min_id_iter;  //get the min available id
    //                                   sessions_ids.erase(min_id_iter);
    //                                   //auto new_session = std::make_shared<wss_session_base>(std::move(socket),ssl_ctx,session_count,sessions_ids,sessions,new_session_id);
    //                                   // auto new_session = std::make_shared<wss_session>(std::move(socket),ssl_ctx,session_count,sessions_ids,sessions,new_session_id,io_ctx);
    //                                   auto new_session = std::make_shared<wss_session>(new_session_id,session_count,sessions_ids,sessions,
    //                                       g_sessions,io_ctx,std::move(socket),*ssl_ctx);

    //                                   sessions.insert({new_session_id,new_session});  //push the session handler and id to the map to allow its handle
    //                                   g_sessions.unlock();
    //                                   try{new_session->start();}   //start session. to handle "start" exceptions running in this thread
    //                                   catch(...)  //in case of exception and error, remove inserted metadata of the session
    //                                   {
    //                                       g_sessions.lock();
    //                                       sessions.erase(new_session_id); //erase the session handler from the map
    //                                       sessions_ids.insert(new_session_id); //insert the id back to the container to release the id
    //                                       //Release_ID(new_session_id); //release id back to the set to be used later
    //                                       g_sessions.unlock();
    //                                       session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
    //                                       std::cout << "current sessions: " << session_count.load() << std::endl;
    //                                   }
    //                                   //new_session->start();   //start session
    //                                   //connection started, unlock the mutex
    //                                   //std::cout << "ongoing sessions: " << session_count.load() << std::endl;
    //                                   session_establishment_mutex.unlock();

    //                                   // auto session = std::make_shared<wss_session_base>(std::move(socket), ssl_ctx);
    //                                   // net::post(threads_pool, [this,session]()
    //                                   // {
    //                                   //     sessions_count.fetch_add(1);    //thread-safely increment the session counter
    //                                   //     session->start();
    //                                   //     sessions_count.fetch_sub(1);    //thread-safely decrement the session counter
    //                                   // });
    //                               }
    //                               else
    //                               {
    //                                   socket.close();
    //                                   std::cout << "session rejected" << std::endl;
    //                                   session_establishment_mutex.unlock();
    //                               }
    //                               //if there's error. ignore the connection session
    //                               accept_connection();  // Continue accepting new connections
    //                           });
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
void ws_server_base::start(void)
{
    ws_server_base::lock_resources(this);
    if(server_running)
    {
        ws_server_base::unlock_resources(this);
        return; //already running
    }
    server_running = true;
    tcp_acceptor.listen();
    this->accept_connection();
    std::cout << "Server started" << std::endl;
    for(std::size_t k=0;k<max_sessions;++k) //initialize IDs
        sessions_ids.insert(k+1);
    //creating number of threads equal to max allowed number of sessions x2 and run the io_context in all these threads
    //this will make the server handle different sessions in different threads concurrently, each session 2threads, 1read/1write
    for (std::size_t i=0; i < max_sessions*2; ++i)  //here check after stopping and starting again
        net::post(threads_pool, [this](){io_ctx.run();});
    ws_server_base::unlock_resources(this);
    //this is to make sure that the mutex is unlocked to accept new connections. if and only if the mutex is not locked
    if(!session_establishment_mutex.try_lock()) //if not locked, lock then unlock
        session_establishment_mutex.unlock();
    else
        session_establishment_mutex.unlock();   //if locked, unlock
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
void ws_server_base::stop(void)
{
    ws_server_base::lock_resources(this);
    if(!server_running)
    {
        ws_server_base::unlock_resources(this);
        return;
    }
    server_running = false; //stop server
    tcp_acceptor.cancel();  //cancel all tcp connections
    for(auto& sess_iter : sessions) //close all running sessions
        this->close_session(sess_iter.first);
    io_ctx.stop();  //stop context
    io_ctx.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));//sleep until all handlers are executed and threads are ended
    std::cout << "Server stoped gracefully" << std::endl;
    ws_server_base::unlock_resources(this);
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
    ws_server_base::lock_resources(this);
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
    {
        ws_server_base::unlock_resources(this);
        return false;
    }
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    session_handler.lock()->send_message(message);
    ws_server_base::unlock_resources(this);
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
    ws_server_base::lock_resources(this);
    std::vector<unsigned char> msg;
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
    {
        ws_server_base::unlock_resources(this);
        return msg; //empty message
    }
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    msg = session_handler.lock()->read_message();
    ws_server_base::unlock_resources(this);
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
    ws_server_base::lock_resources(this);
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
    {
        ws_server_base::unlock_resources(this);
        return false;   //id not found or not running
    }
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    if(!session_handler.lock()->check_inbox()) //if inbox empty
    {
        ws_server_base::unlock_resources(this);
        return false;   //inbox empty
    }
    ws_server_base::unlock_resources(this);
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
    ws_server_base::lock_resources(this);
    auto session_iter = sessions.find(id);
    ws_server_base::unlock_resources(this);
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
    ws_server_base::lock_resources(this);
    auto session_iter = sessions.find(id);
    if(session_iter == sessions.end())   //id not found, not running
    {
        ws_server_base::unlock_resources(this);
        return;
    }
    auto session_handler = session_iter->second;    //stored weak_ptr to the session
    session_handler.lock()->stop();
    ws_server_base::unlock_resources(this);
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
    std::vector<unsigned char> message;
    if(read_messages_queue.size() == 0)
        return message;
    read_mutex.lock();
    message = read_messages_queue.front();
    read_messages_queue.pop_front();
    read_mutex.unlock();
    //std::cout << "message received by server: " << std::string(message.begin(),message.end()) << ", by session num: " << session_id << std::endl;
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
            std::cout << "Server, failed WebSocket handshake, num: "<< self_object->session_id << std::endl;
            try
            {   /*close stream, due to protocol error*/
                self_object->stream.close(websocket::close_code::protocol_error);
                self_object->ongoing_session = true;
                self_object->stop(1);   //set "ongoing_session" to true to allow session closing
                return;
            }
            catch(...)
            {
                self_object->ongoing_session = true;
                self_object->stop(1);   //set "ongoing_session" to true to allow session closing
                return;
            }
        }
        // All functions are successfull
        std::cout << "Server acquired new connection, session started, num: "<< self_object->session_id << std::endl;
        std::cout << "current connected sessions: " << self_object->session_count.load() << std::endl;
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
    int i = 0;  //timeout check loop
    while((i++<connection_timeout) && (!ongoing_session.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_session.load())  //if connection is not successfull
    {
        std::cout << "session timeout, num: " << session_id << std::endl;
        self_object->stop(1);   //stop if not stopped
        return;
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
    std::cout << "session gracefully stopped, num: " << session_id << std::endl;
    ongoing_session = false;
    g_sessions.lock(); //shared mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1); //thread-safely decrement the session counter at server class, locked by static mutex
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
    std::cout << "session ungracefully stopped, num: " << session_id << std::endl;
    ongoing_session = false;
    g_sessions.lock(); //shared mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
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
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop();   //stop session
            return;
        }
        else if(errcode)
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
        //std::cout << "Server received message: " << rec_string << ", by session: " << self_object->session_id << std::endl;

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
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop();   //stop session
            return;
        }
        else if(errcode) //failed to send
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
            std::cout << "Server, failed SSL handshake, num: "<< self_object->session_id << std::endl;
            try
            {   /*close stream, due to protocol error*/
                self_object->stream.close(websocket::close_code::protocol_error);
                self_object->ongoing_session = true;
                self_object->stop(1);   //set "ongoing_session" to true to allow session closing
                return;
            }
            catch(...)
            {
                self_object->ongoing_session = true;
                self_object->stop(1);   //set "ongoing_session" to true to allow session closing
                return;
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
                std::cout << "Server, failed WebSocket handshake, num: "<< self_object->session_id << std::endl;
                try
                {   /*close stream, due to protocol error*/
                    self_object->stream.close(websocket::close_code::protocol_error);
                    self_object->ongoing_session = true;
                    self_object->stop(1);//set "ongoing_session" to true to allow session closing
                    return;
                }
                catch(...)
                {
                    self_object->ongoing_session = true;
                    self_object->stop(1);//set "ongoing_session" to true to allow session closing
                    return;
                }
            }
            // All functions are successfull
            std::cout << "Server acquired new connection, session started, num: "<< self_object->session_id << std::endl;
            std::cout << "current connected sessions: " << self_object->session_count.load() << std::endl;
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
    int i = 0;  //timeout check loop
    while((i++<connection_timeout) && (!ongoing_session.load()))
        std::this_thread::sleep_for(std::chrono::seconds(1));
    if(!ongoing_session.load())  //if connection is not successfull
    {
        std::cout << "session timeout, num: " << session_id << std::endl;
        self_object->stop(1);   //stop if not stopped
        return;
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
    std::cout << "session gracefully stopped, num: " << session_id << std::endl;
    ongoing_session = false;
    g_sessions.lock(); //static mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
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
    std::cout << "session ungracefully stopped, num: " << session_id << std::endl;
    ongoing_session = false;
    g_sessions.lock(); //static mutex for all sessions
    sessions.erase(session_id); //erase the session handler from the map
    sessions_ids.insert(session_id);
    g_sessions.unlock();
    session_count.fetch_sub(1);//thread-safely decrement the session counter at server class, locked by static mutex
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
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop();   //stop session
            return;
        }
        else if(errcode)
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
        //std::cout << "Server received message: " << rec_string << ", by session: " << self_object->session_id << std::endl;

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
        if(errcode == boost::beast::websocket::error::closed)
        {
            self_object->stop();   //stop session
            return;
        }
        else if(errcode) //failed to send
        {
            self_object->stop(1);   //stop session
            return;
        }
        boost::ignore_unused(bytes_sent_dummy); //ignore the dummy parameter
    }));
}

