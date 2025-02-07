/**********************************************************************************************************************
 * 	Module: Testing module
 * 	File Name: tests.h
 *  Authors: Ahmed Desoky
 *	Date: 26/1/2025
 *	*********************************************************************************************************************
 *	Description: testing suites for websocket and websocket secure modules
 ***********************************************************************************************************************/
#pragma once
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include "websockets_server.h"
#include "websockets_client.h"
#include <gtest/gtest.h>


std::string server_key_file_path = "../../credentials/server-key.pem";
std::string server_certificate_file_path = "../../credentials/server-cert.pem";
std::string client_key_file_path = "../../credentials/client-key.pem";
std::string client_certificate_file_path = "../../credentials/client-cert.pem";
std::string ip = "127.0.0.1";

std::string tx_sample_message1 = "This is message 1 - Alfa";
std::string tx_sample_message2 = "This is message 2 - Beta";
std::string tx_sample_message3 = "This is message 3 - Gamma";
std::string tx_sample_message4 = "This is message 4 - Delta";
std::vector<unsigned char> tx_sample1(tx_sample_message1.begin(),tx_sample_message1.end());
std::vector<unsigned char> tx_sample2(tx_sample_message2.begin(),tx_sample_message2.end());
std::vector<unsigned char> tx_sample3(tx_sample_message3.begin(),tx_sample_message3.end());
std::vector<unsigned char> tx_sample4(tx_sample_message4.begin(),tx_sample_message4.end());

std::string rx_sample_string1;
std::string rx_sample_string2;
std::string rx_sample_string3;
std::string rx_sample_string4;
std::vector<unsigned char> rx_sample1;
std::vector<unsigned char> rx_sample2;
std::vector<unsigned char> rx_sample3;
std::vector<unsigned char> rx_sample4;

ws_server* server = ws_server::GetInstance(8081,4);
wss_server* server_secured  = wss_server::GetInstance(8082,4,server_key_file_path,server_certificate_file_path,server_certificate_file_path);
wss_server* server_less_secure = wss_server::GetInstance(8083,4,server_key_file_path);
/************************************************************************************************************************
 *                     						WEBSOCKET TEST CASES
 ***********************************************************************************************************************/
TEST(WSTESTING, OneClientTest)  //Test Case #1
{
    server->start();
    ASSERT_TRUE(server->is_running());
    std::shared_ptr<client_abstract> client = std::make_shared<ws_client>();
    EXPECT_TRUE(client->connect(ip,8081));
    EXPECT_TRUE(client->check_connection());
    EXPECT_TRUE(server->is_serving());
    EXPECT_TRUE(server->check_session(1));
    EXPECT_FALSE(server->check_session(2));
    EXPECT_EQ(server->sessions_count(),1);
    client->disconnect();
    EXPECT_FALSE(client->check_connection());
    EXPECT_FALSE(server->check_session(1));
    EXPECT_FALSE(server->is_serving());
    server->stop();
    EXPECT_FALSE(server->is_running());
}
/*=====================================================================================================================*/
TEST(WSSTESTING, OneClientTestSecured) //Test Case #2
{
    server_secured->start();
    std::shared_ptr<client_abstract> client = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
    EXPECT_TRUE(client->connect(ip,8082));
    EXPECT_TRUE(client->check_connection());
    ASSERT_TRUE(server_secured->is_running());
    EXPECT_TRUE(server_secured->is_serving());
    EXPECT_TRUE(server_secured->check_session(1));
    EXPECT_FALSE(server_secured->check_session(2));
    EXPECT_EQ(server_secured->sessions_count(),1);
    client->disconnect();
    EXPECT_FALSE(client->check_connection());
    server_secured->stop();
    EXPECT_FALSE(server_secured->is_running());
}
/*=====================================================================================================================*/
TEST(WSSTESTING, OneClientTestSecuredNoVerification) //Test Case #3
{
    server_less_secure->start();
    std::shared_ptr<client_abstract> client = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
    EXPECT_TRUE(client->connect(ip,8083));
    EXPECT_TRUE(client->check_connection());
    ASSERT_TRUE(server_less_secure->is_running());
    EXPECT_TRUE(server_less_secure->is_serving());
    EXPECT_TRUE(server_less_secure->check_session(1));
    EXPECT_FALSE(server_less_secure->check_session(2));
    EXPECT_EQ(server_less_secure->sessions_count(),1);
    client->disconnect();
    EXPECT_FALSE(client->check_connection());
    server_less_secure->stop();
    EXPECT_FALSE(server_less_secure->is_running());
}
/*=====================================================================================================================*/
TEST(WSTESTING, LimitedClientsTest) //Test Case #4
{
    std::shared_ptr<client_abstract> client1 = std::make_shared<ws_client>();
    std::shared_ptr<client_abstract> client2 = std::make_shared<ws_client>();
    std::shared_ptr<client_abstract> client3 = std::make_shared<ws_client>();
    std::shared_ptr<client_abstract> client4 = std::make_shared<ws_client>();
    std::shared_ptr<client_abstract> client5 = std::make_shared<ws_client>();
    server->start();
    ASSERT_TRUE(server->is_running());
    EXPECT_TRUE(client1->connect(ip,8081));
    EXPECT_TRUE(client1->check_connection());
    EXPECT_TRUE(client2->connect(ip,8081));
    EXPECT_TRUE(client2->check_connection());
    EXPECT_TRUE(server->is_serving());
    EXPECT_TRUE(server->check_session(1));
    EXPECT_TRUE(server->check_session(2));
    EXPECT_FALSE(server->check_session(3));
    EXPECT_EQ(server->sessions_count(),2);
    EXPECT_TRUE(client3->connect(ip,8081));
    EXPECT_TRUE(client4->connect(ip,8081));
    EXPECT_FALSE(client5->connect(ip,8081));
    EXPECT_TRUE(server->is_serving());
    EXPECT_TRUE(server->check_session(3));
    EXPECT_TRUE(server->check_session(4));
    EXPECT_FALSE(server->check_session(5));
    client1->disconnect();
    EXPECT_EQ(server->sessions_count(),3);
    client2->disconnect();
    EXPECT_EQ(server->sessions_count(),2);
    client3->disconnect();
    EXPECT_EQ(server->sessions_count(),1);
    client4->disconnect();
    EXPECT_EQ(server->sessions_count(),0);
    client5->disconnect();
    EXPECT_EQ(server->sessions_count(),0);
    EXPECT_FALSE(server->is_serving());
    server->stop();
}
/*=====================================================================================================================*/
TEST(WSTESTING, MultipleClientsMultipleMessages) //Test Case #5
{
    server_secured->start();
    ASSERT_TRUE(server_secured->is_running());
    std::shared_ptr<client_abstract> client1 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
    std::shared_ptr<client_abstract> client2 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
    std::shared_ptr<client_abstract> client3 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
    EXPECT_TRUE(client1->connect(ip,8082));
    EXPECT_TRUE(client1->check_connection());
    EXPECT_TRUE(client2->connect(ip,8082));
    EXPECT_TRUE(client2->check_connection());
    EXPECT_TRUE(client3->connect(ip,8082));
    EXPECT_TRUE(client3->check_connection());
    EXPECT_EQ(server_secured->sessions_count(),3);
    //Server send messages
    EXPECT_TRUE(server_secured->send_message(1,tx_sample1));
    EXPECT_TRUE(server_secured->send_message(1,tx_sample2));
    EXPECT_TRUE(server_secured->send_message(1,tx_sample3));
    EXPECT_TRUE(server_secured->send_message(2,tx_sample1));
    EXPECT_TRUE(server_secured->send_message(2,tx_sample2));
    EXPECT_TRUE(server_secured->send_message(2,tx_sample3));
    EXPECT_TRUE(server_secured->send_message(3,tx_sample1));
    EXPECT_TRUE(server_secured->send_message(3,tx_sample2));
    EXPECT_TRUE(server_secured->send_message(3,tx_sample3));
    EXPECT_FALSE(server_secured->check_session(4));
    EXPECT_FALSE(server_secured->check_session(5));
    EXPECT_FALSE(server_secured->check_session(6));
    //Clients send messages
    client1->send_message(tx_sample1);
    client2->send_message(tx_sample2);
    client3->send_message(tx_sample3);
    //server read clients' messages
    EXPECT_TRUE(server_secured->check_inbox(1));
    rx_sample1 = server_secured->read_message(1);
    rx_sample_string1 = std::string(rx_sample1.begin(),rx_sample1.end());
    EXPECT_STREQ(rx_sample_string1.c_str(),tx_sample_message1.c_str());
    EXPECT_TRUE(server_secured->check_inbox(2));
    rx_sample2 = server_secured->read_message(2);
    rx_sample_string2 = std::string(rx_sample2.begin(),rx_sample2.end());
    EXPECT_STREQ(rx_sample_string2.c_str(),tx_sample_message2.c_str());
    EXPECT_TRUE(server_secured->check_inbox(3));
    rx_sample3 = server_secured->read_message(3);
    rx_sample_string3 = std::string(rx_sample3.begin(),rx_sample3.end());
    EXPECT_STREQ(rx_sample_string3.c_str(),tx_sample_message3.c_str());
    //Clients read server's messages
    for(int i=1;i<3;++i)
    {
        EXPECT_TRUE(client1->check_inbox());
        std::vector<unsigned char> receive_buffer = client1->read_message();
        std::string receive_str = std::string(receive_buffer.begin(),receive_buffer.end());
        switch (i)
        {
            case 1:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message1.c_str());
                break;
            case 2:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message2.c_str());
                break;
            case 3:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message3.c_str());
                break;
        }
    }
    for(int i=1;i<3;++i)
    {
        EXPECT_TRUE(client2->check_inbox());
        std::vector<unsigned char> receive_buffer = client2->read_message();
        std::string receive_str = std::string(receive_buffer.begin(),receive_buffer.end());
        switch (i)
        {
            case 1:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message1.c_str());
                break;
            case 2:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message2.c_str());
                break;
            case 3:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message3.c_str());
                break;
        }
    }
    for(int i=1;i<3;++i)
    {
        EXPECT_TRUE(client3->check_inbox());
        std::vector<unsigned char> receive_buffer = client3->read_message();
        std::string receive_str = std::string(receive_buffer.begin(),receive_buffer.end());
        switch (i)
        {
            case 1:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message1.c_str());
                break;
            case 2:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message2.c_str());
                break;
            case 3:
                EXPECT_STREQ(receive_str.c_str(),tx_sample_message3.c_str());
                break;
        }
    }
    client3->disconnect();
    EXPECT_FALSE(client3->check_connection());
    client2->disconnect();
    EXPECT_FALSE(client2->check_connection());
    EXPECT_EQ(server_secured->sessions_count(),1);
    EXPECT_TRUE(server_secured->is_serving());
    client1->disconnect();
    EXPECT_FALSE(client1->check_connection());
    EXPECT_EQ(server_secured->sessions_count(),0);
    EXPECT_FALSE(server_secured->is_serving());
    server_secured->stop();
    ASSERT_FALSE(server->is_running());
}
/*=====================================================================================================================*/
TEST(WSSTESTING, ConnectDisconnectCornerCases) //Test Case #6
{
    std::shared_ptr<client_abstract> client1 = std::make_shared<wss_client>(client_key_file_path);
    std::shared_ptr<client_abstract> client2 = std::make_shared<wss_client>(client_key_file_path);
    std::shared_ptr<client_abstract> client3 = std::make_shared<wss_client>(client_key_file_path);
    std::shared_ptr<client_abstract> client4 = std::make_shared<wss_client>(client_key_file_path);
    server_less_secure->start();
    ASSERT_TRUE(server_less_secure->is_running());
    server_less_secure->stop();
    ASSERT_FALSE(server_less_secure->is_running());
    server_less_secure->start();
    ASSERT_TRUE(server_less_secure->is_running());
    EXPECT_FALSE(server_less_secure->is_serving());
    EXPECT_TRUE(client1->connect(ip,8083));
    EXPECT_TRUE(server_less_secure->is_serving());
    EXPECT_EQ(server_less_secure->sessions_count(),1);
    EXPECT_TRUE(server_less_secure->check_session(1));
    EXPECT_FALSE(server_less_secure->check_session(2));
    EXPECT_TRUE(server_less_secure->send_message(1,tx_sample1));    //send message to session with id=1
    EXPECT_FALSE(server_less_secure->send_message(2,tx_sample1));   //no session with id=2
    rx_sample1 = client1->read_message();
    rx_sample_string1 = std::string(rx_sample1.begin(),rx_sample1.end());
    EXPECT_STREQ(rx_sample_string1.c_str(),tx_sample_message1.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server_less_secure->stop();
    EXPECT_FALSE(server_less_secure->is_serving());
    ASSERT_FALSE(server_less_secure->is_running());
    EXPECT_FALSE(client2->connect(ip,8083));    //server is stopped, no connection shall be established
    wss_server::Destroy(server_less_secure);    //destroy the created server, only 1 server can be created
    server_less_secure = nullptr;
    wss_server* new_server_less_secure = wss_server::GetInstance(8085,4,server_key_file_path);  //create new server on port 8085
    EXPECT_FALSE(new_server_less_secure->is_serving()); //not started yet
    ASSERT_FALSE(new_server_less_secure->is_running());
    EXPECT_FALSE(client3->connect(ip,8085));    //server is not running
    new_server_less_secure->start();
    ASSERT_TRUE(new_server_less_secure->is_running());
    EXPECT_TRUE(client4->connect(ip,8085));
    EXPECT_TRUE(new_server_less_secure->is_serving()); //new session established
    new_server_less_secure->close_session(1);   //the last established session is id=1
    EXPECT_FALSE(new_server_less_secure->is_serving()); //session closed
    EXPECT_FALSE(client4->check_connection());  //client session is closed
    new_server_less_secure->stop();
    ASSERT_FALSE(new_server_less_secure->is_running()); //server stopped
    wss_server::Destroy(new_server_less_secure);    //destroy the created server
    new_server_less_secure = nullptr;
    server_less_secure = wss_server::GetInstance(8083,4,server_key_file_path);  //create again the same first server object
    ASSERT_FALSE(server_less_secure == nullptr); //check that object is created
}
/*=====================================================================================================================*/
TEST(WSTESTING, ServerUpperLimit) //Test Case #7
{
    int upper_limit = 25;   //testing handling up to 25 clients
    ws_server::Destroy(server);    //destroy the already created server
    server = nullptr;
    ws_server* new_server = ws_server::GetInstance(8085,upper_limit);  //create again the same first server object but with different max limit and port
    std::vector<std::shared_ptr<client_abstract>> clients;
    for(unsigned int i=0;i<upper_limit;++i)
        clients.push_back(std::make_shared<ws_client>());
    new_server->start();
    ASSERT_TRUE(new_server->is_running());
    for(int i=0;i<upper_limit;++i)
        EXPECT_TRUE(clients.at(i)->connect(ip,8085));
    EXPECT_EQ(new_server->sessions_count(),upper_limit);
    EXPECT_TRUE(new_server->is_serving());
    for(int i=0;i<upper_limit;++i)
    {
        clients.at(i)->disconnect();
        EXPECT_FALSE(clients.at(i)->check_connection());  //client session is closed
    }
    EXPECT_EQ(new_server->sessions_count(),0);
    EXPECT_FALSE(new_server->is_serving());
    new_server->stop();
    ASSERT_FALSE(new_server->is_running());
    ws_server::Destroy(new_server);
    new_server = nullptr;
    server = ws_server::GetInstance(8081,4);  //bring back the old server options
}
/*=====================================================================================================================*/
TEST(WSTESTING, ClientSingleConnection) //Test Case #8
{
    std::shared_ptr<client_abstract> client = std::make_shared<ws_client>();
    server->start();
    ASSERT_TRUE(server->is_running());
    EXPECT_TRUE(client->connect(ip,8081));
    EXPECT_TRUE(client->connect(ip,8081));
    std::string another_ip = "192.168.1.1";
    EXPECT_FALSE(client->connect(another_ip,8888));
    client->disconnect();
    EXPECT_FALSE(client->check_connection());
}
