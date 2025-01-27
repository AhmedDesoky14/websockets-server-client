/************************************************************************************************************************
 * 	Module: Testing module
 * 	File Name: tests.h
 *  Authors: Ahmed Desoky
 *	Date: 26/1/2025
 *	*********************************************************************************************************************
 *	Description: testing suites for websocket and websocket secure modules
 *
 *
 ***********************************************************************************************************************/
#pragma once
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include "websockets_server.h"
#include "websockets_client.h"
#include <gtest/gtest.h>

/************************************************************************************************************************
 *                     							   FIXTURES
 ***********************************************************************************************************************/
class WS_TEST_FIXTURE : public ::testing::Test
{
protected:
    ws_server* server = ws_server::Create(7071,5); //port:7071, max sessions:5;
    std::string ip = "127.0.0.1";
    std::shared_ptr<ws_client> client1 = std::make_shared<ws_client>();
    std::shared_ptr<ws_client> client2 = std::make_shared<ws_client>();
    std::shared_ptr<ws_client> client3 = std::make_shared<ws_client>();
    std::shared_ptr<ws_client> client4 = std::make_shared<ws_client>();
    std::shared_ptr<ws_client> client5 = std::make_shared<ws_client>();
    std::shared_ptr<ws_client> client6 = std::make_shared<ws_client>();
    std::shared_ptr<ws_client> client7 = std::make_shared<ws_client>();
    void SetUp() override //initialization, runs before each test
    {
    }


    void TearDown() override    //destroying resources, runs after each test
    {
        ws_server::Destroy();   //destroy server
    }
};
/*======================================================================================================================*/
class WSS_TEST_FIXTURE : public ::testing::Test
{
protected:
    std::string server_key_file_path = "../../credentials/server-key.pem";
    std::string server_certificate_file_path = "../../credentials/server-cert.pem";
    std::string client_key_file_path = "../../credentials/client-key.pem";
    std::string client_certificate_file_path = "../../credentials/client-cert.pem";
    wss_server* server = wss_server::Create(7071,5,server_key_file_path,server_certificate_file_path); //port:7071, max sessions:5;
    std::string ip = "127.0.0.1";
    std::shared_ptr<wss_client> client1 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);
    std::shared_ptr<wss_client> client2 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);
    std::shared_ptr<wss_client> client3 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);
    std::shared_ptr<wss_client> client4 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);
    std::shared_ptr<wss_client> client5 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);
    std::shared_ptr<wss_client> client6 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);
    std::shared_ptr<wss_client> client7 = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path);



    void TearDown() override    //destroying resources, runs after each test
    {
        ws_server::Destroy();   //destroy server
    }
};
/*======================================================================================================================*/
class CAPACITY_LIMIT_TEST_FIXTURE : public ::testing::Test
{
protected:
    unsigned long capacity = 10;
    std::string ip = "127.0.0.1";
    ws_server* server = ws_server::Create(7071,capacity); //port:7071
    std::vector<std::shared_ptr<ws_client>> clients;

    void SetUp() override //initialization, runs before each test
    {
        for(unsigned long i=0;i<capacity;++i)
            clients[i] = std::make_shared<ws_client>();

    }


    void TearDown() override    //destroying resources, runs after each test
    {
        ws_server::Destroy();   //destroy server
    }
};
/************************************************************************************************************************
 *                     						WEBSOCKET TEST CASES
 ***********************************************************************************************************************/
TEST(Normal_Tests, Normal1_one_Client_Server)
{
    ws_server* server = ws_server::Create(7071,5); //port:7071, max sessions:5;
    std::string ip = "127.0.0.1";
    std::shared_ptr<ws_client> client1 = std::make_shared<ws_client>();
    server->start();    //server start
    ASSERT_TRUE(server->is_running());
    ASSERT_TRUE(client1->connect(ip,9091));
    ASSERT_TRUE(client1->check_connection());
    ASSERT_TRUE(server->is_serving());
    ASSERT_EQ(server->sessions_count(),1);  //1 session must be ongoing
    std::string cli_mes = "Hello, Websocket, client 1\n";
    std::vector<unsigned char> cli_mes_vec(cli_mes.begin(),cli_mes.end());
    client1->send_message(cli_mes_vec);
    ASSERT_TRUE(server->check_session(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  //delay for 100ms
    ASSERT_TRUE(server->check_inbox(1));
    std::vector<unsigned char> serv_rec_vect = server->read_message(1);
    std::string serv_rec_str(serv_rec_vect.begin(),serv_rec_vect.end());
    ASSERT_STREQ(cli_mes.c_str(),serv_rec_str.c_str());  //sent and received messages
    std::string server_mes = "Hello, Websocket, server\n";
    std::vector<unsigned char> server_mes_vec(server_mes.begin(),server_mes.end());
    ASSERT_TRUE(server->send_message(1,server_mes_vec));
    ASSERT_TRUE(client1->check_queue());
    std::vector<unsigned char> clirec_vect = client1->read_message();
    std::string cli_rec_str(clirec_vect.begin(),clirec_vect.end());
    ASSERT_STREQ(server_mes.c_str(),cli_rec_str.c_str());  //sent and received messages
    client1->disconnect();
    ASSERT_FALSE(client1->check_connection());
    ASSERT_FALSE(server->check_session(1));
    ASSERT_EQ(server->sessions_count(),0);  //0 session must be ongoing
    ASSERT_FALSE(server->is_serving());
    server->stop();
    ASSERT_FALSE(server->is_running());
}
/*=====================================================================================================================*/
TEST_F(WS_TEST_FIXTURE, Normal2_one_Client_Server)
{
    server->start();    //server start
    ASSERT_TRUE(server->is_running());
    ASSERT_TRUE(client1->connect(ip,9091));
    ASSERT_TRUE(client1->check_connection());
    ASSERT_TRUE(server->is_serving());
    ASSERT_EQ(server->sessions_count(),1);  //1 session must be ongoing
    std::string cli_mes = "Hello, Websocket, client 1\n";
    std::vector<unsigned char> cli_mes_vec(cli_mes.begin(),cli_mes.end());
    client1->send_message(cli_mes_vec);
    ASSERT_TRUE(server->check_session(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  //delay for 100ms
    ASSERT_TRUE(server->check_inbox(1));
    std::vector<unsigned char> serv_rec_vect = server->read_message(1);
    std::string serv_rec_str(serv_rec_vect.begin(),serv_rec_vect.end());
    ASSERT_STREQ(cli_mes.c_str(),serv_rec_str.c_str());  //sent and received messages
    std::string server_mes = "Hello, Websocket, server\n";
    std::vector<unsigned char> server_mes_vec(server_mes.begin(),server_mes.end());
    ASSERT_TRUE(server->send_message(1,server_mes_vec));
    ASSERT_TRUE(client1->check_queue());
    std::vector<unsigned char> clirec_vect = client1->read_message();
    std::string cli_rec_str(clirec_vect.begin(),clirec_vect.end());
    ASSERT_STREQ(server_mes.c_str(),cli_rec_str.c_str());  //sent and received messages
    server->close_session(1);
    ASSERT_FALSE(client1->check_connection());
    ASSERT_FALSE(server->check_session(1));
    ASSERT_EQ(server->sessions_count(),0);  //0 session must be ongoing
    ASSERT_FALSE(server->is_serving());
    server->stop();
    ASSERT_FALSE(server->is_running());
}
/*=====================================================================================================================*/
TEST_F(WS_TEST_FIXTURE, Limit_Client_Server)
{


}
/************************************************************************************************************************
 *                     				    WEBSOCKET SECURE TEST CASES
 ***********************************************************************************************************************/
