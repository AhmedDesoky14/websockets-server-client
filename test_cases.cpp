// #include "test_cases.h"

// using namespace std;
// //Global variables


// std::string server_key_file_path = "../../credentials/server-key.pem";
// std::string server_certificate_file_path = "../../credentials/server-cert.pem";
// std::string client_key_file_path = "../../credentials/client-key.pem";
// std::string client_certificate_file_path = "../../credentials/client-cert.pem";
// std::string ip = "127.0.0.1";

// std::string tx_sample_message1 = "This is message 1 - Alfa";
// std::string tx_sample_message2 = "This is message 2 - Beta";
// std::string tx_sample_message3 = "This is message 3 - Gamma";
// std::string tx_sample_message4 = "This is message 4 - Delta";
// std::vector<unsigned char> tx_sample1(tx_sample_message1.begin(),tx_sample_message1.end());
// std::vector<unsigned char> tx_sample2(tx_sample_message2.begin(),tx_sample_message2.end());
// std::vector<unsigned char> tx_sample3(tx_sample_message3.begin(),tx_sample_message3.end());
// std::vector<unsigned char> tx_sample4(tx_sample_message4.begin(),tx_sample_message4.end());

// std::string rx_sample_string1;
// std::string rx_sample_string2;
// std::string rx_sample_string3;
// std::string rx_sample_string4;
// std::vector<unsigned char> rx_sample1;
// std::vector<unsigned char> rx_sample2;
// std::vector<unsigned char> rx_sample3;
// std::vector<unsigned char> rx_sample4;

// ws_server* server = ws_server::GetInstance(8081,50);
// wss_server* server_secured = wss_server::GetInstance(8082,4,server_key_file_path,server_certificate_file_path,server_certificate_file_path);
// wss_server* server_less_secure = wss_server::GetInstance(8083,4,server_key_file_path);
// /************************************************************************************************************************
// * Test Name: WebSocket connection test
// * Test#: 1
// * Description: Test WebSocket connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase1(void)
// {
//     cout << "Test Case 1 Start - WebSocket" << endl;
//     server->start();
//     std::shared_ptr<client_abstract> client = std::make_shared<ws_client>();
//     bool connection = client->connect(ip,8081);
//     cout << "client->connect: " << connection << endl;
//     cout << "client->check_connection: " << client->check_connection() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
//     cout << "server->is_running: " << server->is_running() << endl;
//     cout << "server->is_serving: " << server->is_serving() << endl;
//     cout << "server check session 1: " << server->check_session(1) << endl;
//     cout << "server check session 2: " << server->check_session(2) << endl;
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     client->disconnect();
//     cout << "client disconnected" << connection << endl;
//     server->stop();
//     cout << "server stopped" << connection << endl;
//     cout << "Test Case 1 End" << endl;
// }
// /************************************************************************************************************************
// * Test Name: WebSocket Secure connection test
// * Test#: 2
// * Description: Test WebSocket Secure connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase2(void)
// {
//     cout << "Test Case 2 Start - WebSocket Secure" << endl;
//     server_secured->start();
//     std::shared_ptr<client_abstract> client = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
//     bool connection = client->connect(ip,8082);
//     cout << "client->connect: " << connection << endl;
//     cout << "client->check_connection: " << client->check_connection() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
//     cout << "server->is_running: " << server_secured->is_running() << endl;
//     cout << "server->is_serving: " << server_secured->is_serving() << endl;
//     cout << "server check session 1: " << server_secured->check_session(1) << endl;
//     cout << "server check session 2: " << server_secured->check_session(2) << endl;
//     cout << "server->session_count(): " << server_secured->sessions_count() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     client->disconnect();
//     cout << "client disconnected: " << client->check_connection() << endl;
//     server_secured->stop();
//     cout << "server stopped: " << server_secured->is_running() << endl;
//     cout << "Test Case 2 End" << endl;
// }
// /************************************************************************************************************************
// * Test Name: WebSocket Secure connection test
// * Test#: 3
// * Description: Test WebSocket Secure connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase3(void)
// {
//     cout << "Test Case 3 Start - WebSocket Secure, no certificate verification" << endl;
//     server_less_secure->start();
//     std::shared_ptr<client_abstract> client = std::make_shared<wss_client>(client_key_file_path);
//     bool connection = client->connect(ip,8083);
//     cout << "client->connect: " << connection << endl;
//     cout << "client->check_connection: " << client->check_connection() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));    //sleep for 100ms
//     cout << "server->is_running: " << server_less_secure->is_running() << endl;
//     cout << "server->is_serving: " << server_less_secure->is_serving() << endl;
//     cout << "server check session 1: " << server_less_secure->check_session(1) << endl;
//     cout << "server check session 2: " << server_less_secure->check_session(2) << endl;
//     cout << "server->session_count(): " << server_less_secure->sessions_count() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     client->disconnect();
//     cout << "client disconnected: " << client->check_connection() << endl;
//     server_less_secure->stop();
//     cout << "server stopped: " << server_less_secure->is_running() << endl;
//     cout << "Test Case 3 End" << endl;
// }
// /************************************************************************************************************************
// * Test Name: WebSocket Secure connection test
// * Test#: 4
// * Description: Test WebSocket Secure connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase4(void)
// {
//     cout << "Test Case 4 Start - WebSocket: exceeding server session limit" << endl;
//     std::shared_ptr<client_abstract> client1 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client2 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client3 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client4 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client5 = std::make_shared<ws_client>();
//     server->start();
//     bool connection1 = client1->connect(ip,8081);
//     cout << "client2->connect: " << connection1 << endl;
//     cout << "client1->check_connection: " << client1->check_connection() << endl;
//     bool connection2 = client2->connect(ip,8081);
//     cout << "client3->connect: " << connection2 << endl;
//     cout << "client2->check_connection: " << client2->check_connection() << endl;
//     cout << "server->is_running: " << server->is_running() << endl;
//     cout << "server->is_serving: " << server->is_serving() << endl;
//     cout << "server check session 1: " << server->check_session(1) << endl;
//     cout << "server check session 2: " << server->check_session(2) << endl;
//     cout << "server check session 3: " << server->check_session(3) << endl;
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     bool connection3 = client3->connect(ip,8081);
//     bool connection4 = client4->connect(ip,8081);
//     bool connection5 = client5->connect(ip,8081);
//     cout << "client3->check_connection: " << client3->check_connection() << endl;
//     cout << "client4->check_connection: " << client4->check_connection() << endl;
//     cout << "client5->check_connection: " << client5->check_connection() << endl;
//     cout << "server->is_serving: " << server->is_serving() << endl;
//     cout << "server check session 3: " << server->check_session(3) << endl;
//     cout << "server check session 4: " << server->check_session(4) << endl;
//     cout << "server check session 5: " << server->check_session(5) << endl;
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     client1->disconnect();
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     client2->disconnect();
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     client3->disconnect();
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     client4->disconnect();
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     client5->disconnect();
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     cout << "server->session_count(): " << server->sessions_count() << endl;
//     server->stop();
//     cout << "Test Case 4 End" << endl;
// }
// /************************************************************************************************************************
// * Test Name: WebSocket Secure connection test
// * Test#: 5
// * Description: Test WebSocket Secure connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase5(void)
// {
//     cout << "Test Case 5 Start - WebSocket Secure: multiple clients multiple messages" << endl;
//     server_secured->start();
//     std::shared_ptr<client_abstract> client1 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
//     std::shared_ptr<client_abstract> client2 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
//     std::shared_ptr<client_abstract> client3 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
//     bool connection1 = client1->connect(ip,8082);
//     bool connection2 = client2->connect(ip,8082);
//     bool connection3 = client3->connect(ip,8082);
//     cout << "client1->check_connection: " << client1->check_connection() << endl;
//     cout << "client2->check_connection: " << client2->check_connection() << endl;
//     cout << "client3->check_connection: " << client3->check_connection() << endl;
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     cout << "server->session_count(): " << server_secured->sessions_count() << endl;
//     if(server_secured->check_session(1))
//     {
//         server_secured->send_message(1,tx_sample1);
//         server_secured->send_message(1,tx_sample2);
//         server_secured->send_message(1,tx_sample3);
//         cout << "session 1 is ongoing and messages sent" << endl;
//     }
//     if(server_secured->check_session(2))
//     {
//         server_secured->send_message(2,tx_sample1);
//         server_secured->send_message(2,tx_sample2);
//         server_secured->send_message(2,tx_sample3);
//         cout << "session 2 is ongoing and messages sent" << endl;
//     }
//     if(server_secured->check_session(3))
//     {
//         server_secured->send_message(3,tx_sample1);
//         server_secured->send_message(3,tx_sample2);
//         server_secured->send_message(3,tx_sample3);
//         cout << "session 3 is ongoing and messages sent" << endl;
//     }
//     cout << "server check session 4: " << server_secured->check_session(4) << endl;
//     cout << "server check session 5: " << server_secured->check_session(5) << endl;
//     cout << "server check session 6: " << server_secured->check_session(6) << endl;
//     if(client1->check_connection())
//         client1->send_message(tx_sample1);
//     if(client2->check_connection())
//         client2->send_message(tx_sample2);
//     if(client3->check_connection())
//         client3->send_message(tx_sample3);
//     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     while(server_secured->check_inbox(1))
//     {
//         rx_sample1 = server_secured->read_message(1);
//         rx_sample_string1 = std::string(rx_sample1.begin(),rx_sample1.end());
//         cout << "server from session 1, received message: " << rx_sample_string1 << endl;
//     }
//     while(server_secured->check_inbox(2))
//     {
//         rx_sample2 = server_secured->read_message(2);
//         rx_sample_string2 = std::string(rx_sample2.begin(),rx_sample2.end());
//         cout << "server from session 2, received message: " << rx_sample_string2 << endl;
//     }
//     while(server_secured->check_inbox(3))
//     {
//         rx_sample3 = server_secured->read_message(3);
//         rx_sample_string3 = std::string(rx_sample3.begin(),rx_sample3.end());
//         cout << "server from session 3, received message: " << rx_sample_string3 << endl;
//     }
//     while(client1->check_inbox())
//     {
//         rx_sample1 = client1->read_message();
//         rx_sample_string1 = std::string(rx_sample1.begin(),rx_sample1.end());
//         cout << "client 1, received message: " << rx_sample_string1 << endl;
//     }
//     while(client2->check_inbox())
//     {
//         rx_sample2 = client2->read_message();
//         rx_sample_string2 = std::string(rx_sample2.begin(),rx_sample2.end());
//         cout << "client 2, received message: " << rx_sample_string2 << endl;
//     }
//     while(client3->check_inbox())
//     {
//         rx_sample3 = client3->read_message();
//         rx_sample_string3 = std::string(rx_sample3.begin(),rx_sample3.end());
//         cout << "client 3, received message: " << rx_sample_string3 << endl;
//     }
//     client3->disconnect();
//     client2->disconnect();
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     cout << "server->session_count(): " << server_secured->sessions_count() << endl;
//     client1->disconnect();
//     std::this_thread::sleep_for(std::chrono::milliseconds(300));
//     cout << "server->is_serving: " << server->is_serving() << endl;
//     cout << "server->session_count(): " << server_secured->sessions_count() << endl;
//     server_secured->stop();
//     cout << "server->is_running: " << server->is_running() << endl;
//     cout << "server->is_serving: " << server->is_serving() << endl;
//     wss_server::Destroy(server_secured);
//     cout << "Test Case 5 End" << endl;
// }
// /************************************************************************************************************************
// * Test Name: WebSocket Secure connection test
// * Test#: 6
// * Description: Test WebSocket Secure connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase6(void)
// {
//     cout << "Test Case 6 Start - WebSocket Secure: mixed connections and disconnections" << endl;
//     std::shared_ptr<client_abstract> client1 = std::make_shared<wss_client>(client_key_file_path);
//     std::shared_ptr<client_abstract> client2 = std::make_shared<wss_client>(client_key_file_path);
//     std::shared_ptr<client_abstract> client3 = std::make_shared<wss_client>(client_key_file_path);
//     std::shared_ptr<client_abstract> client4 = std::make_shared<wss_client>(client_key_file_path);
//     server_less_secure->start();
//     std::this_thread::sleep_for(std::chrono::milliseconds(250));
//     server_less_secure->stop();
//     std::this_thread::sleep_for(std::chrono::milliseconds(250));    //delay so ongoing operations exit
//     server_less_secure->start();
//     std::this_thread::sleep_for(std::chrono::milliseconds(250));
//     cout << "server->is_running: " << server_less_secure->is_running() << endl;
//     cout << "server->is_serving: " << server_less_secure->is_serving() << endl;
//     bool connection1 = client1->connect(ip,8083);
//     if(client1->check_failed_connection()) //if connection failed during it
//         client1->reset();
//     std::this_thread::sleep_for(std::chrono::milliseconds(250));
//     cout << "server->is_serving: " << server_less_secure->is_serving() << endl;
//     cout << "server->session_count(): " << server_less_secure->sessions_count() << endl;
//     cout << "server check session 1: " << server_less_secure->check_session(1) << endl;
//     cout << "server check session 2: " << server_less_secure->check_session(2) << endl;
//     server_less_secure->send_message(1,tx_sample1);
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     rx_sample1 = client1->read_message();
//     rx_sample_string1 = std::string(rx_sample1.begin(),rx_sample1.end());
//     cout << "client 1, received message: " << rx_sample_string1 << endl;
//     server_less_secure->stop();
//     std::this_thread::sleep_for(std::chrono::milliseconds(250));
//     if(client1->check_failed_connection()) //if connection failed during it
//         client1->reset();
//     bool connection2 = client2->connect(ip,8083);
//     if(client2->check_failed_connection()) //if connection failed during it
//         client2->reset();
//     wss_server::Destroy(server_less_secure);
//     wss_server* new_server_less_secure = wss_server::GetInstance(8083,4,server_key_file_path);
//     bool connection3 = client3->connect(ip,8083);
//     if(client3->check_failed_connection()) //if connection failed during it
//         client3->reset();
//     new_server_less_secure->start();
//     bool connection4 = client4->connect(ip,8083);
//     if(client4->check_failed_connection()) //if connection failed during it
//         client4->reset();
//     std::this_thread::sleep_for(std::chrono::milliseconds(250));
//     new_server_less_secure->close_session(1);
//     cout << "Server closed session #1"<< endl;
//     new_server_less_secure->stop();
//     cout << "server stopped" << endl;
//     if(client4->check_failed_connection()) //if connection failed during it
//         client4->reset();
//     wss_server::Destroy(new_server_less_secure);
//     cout << "Test Case 6 End" << endl;
// }
// /************************************************************************************************************************
// * Test Name: WebSocket Secure connection test
// * Test#: 6
// * Description: Test WebSocket Secure connection between client and server
// *
// *
// ************************************************************************************************************************/
// void TestCase7(void)
// {
//     cout << "Test Case 7 Start - server max limit connections" << endl;
//     unsigned int clients_num = 50;
//     server->start();
//     std::vector<std::shared_ptr<client_abstract>> clients;
//     for(unsigned int i=0;i<clients_num;++i)
//         clients.push_back(std::make_shared<ws_client>());
//     for(unsigned int i=0;i<clients_num;++i)
//     {
//         clients.at(i)->connect(ip,8081);
//         cout << "Server current connections: " << server->sessions_count() << endl;
//     }
//     for(unsigned int i=0;i<clients.size();++i)
//     {
//         clients.at(i)->disconnect();
//     }
//     cout << "Test Case 7 End" << endl;

// }

