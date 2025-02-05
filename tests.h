// ***********************************************************************************************************************
//  * 	Module: Testing module
//  * 	File Name: tests.h
//  *  Authors: Ahmed Desoky
//  *	Date: 26/1/2025
//  *	*********************************************************************************************************************
//  *	Description: testing suites for websocket and websocket secure modules
//  *
//  *
//  ***********************************************************************************************************************/
// #pragma once
// /************************************************************************************************************************
//  *                     							   INCLUDES
//  ***********************************************************************************************************************/
// #include "websockets_server.h"
// #include "websockets_client.h"
// #include <gtest/gtest.h>



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

// ws_server* server = ws_server::Create(8081,20);
// wss_server* server_secured = wss_server::Create(8082,4,server_key_file_path,server_certificate_file_path,server_certificate_file_path);
// wss_server* server_less_secure = wss_server::Create(8083,4,server_key_file_path);
// /************************************************************************************************************************
//  *                     						WEBSOCKET TEST CASES
//  ***********************************************************************************************************************/
// TEST(WebSocketsTests, OneClientTest)
// {
//     server->start();
//     ASSERT_TRUE(server->is_running());
//     std::shared_ptr<client_abstract> client = std::make_shared<ws_client>();
//     EXPECT_TRUE(client->connect(ip,8081));
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     EXPECT_TRUE(client->check_connection());
//     EXPECT_TRUE(server->is_serving());
//     EXPECT_TRUE(server->check_session(1));
//     EXPECT_FALSE(server->check_session(2));
//     EXPECT_EQ(server->sessions_count(),1);
//     client->disconnect();
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     EXPECT_FALSE(client->check_connection());
//     EXPECT_FALSE(server->check_session(1));
//     EXPECT_FALSE(server->is_serving());
//     server->stop();
//     EXPECT_FALSE(server->is_running());
// }
// /*=====================================================================================================================*/
// TEST(WebSocketsTests, OneClientTestSecured)
// {
//     server_secured->start();
//     std::shared_ptr<client_abstract> client = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
//     EXPECT_TRUE(client->connect(ip,8082));
//     EXPECT_TRUE(client->check_connection());
//     ASSERT_TRUE(server_secured->is_running());
//     EXPECT_TRUE(server_secured->is_serving());
//     EXPECT_TRUE(server_secured->check_session(1));
//     EXPECT_FALSE(server_secured->check_session(2));
//     EXPECT_EQ(server_secured->sessions_count(),1);
//     client->disconnect();
//     EXPECT_FALSE(client->check_connection());
//     server_secured->stop();
//     EXPECT_FALSE(server_secured->is_running());
// }
// /*=====================================================================================================================*/
// TEST(WebSocketsTests, OneClientTestNOVerification)
// {
//     server_less_secure->start();
//     std::shared_ptr<client_abstract> client = std::make_shared<wss_client>(client_key_file_path,client_certificate_file_path,server_certificate_file_path);
//     EXPECT_TRUE(client->connect(ip,8083));
//     EXPECT_TRUE(client->check_connection());
//     ASSERT_TRUE(server_less_secure->is_running());
//     EXPECT_TRUE(server_less_secure->is_serving());
//     EXPECT_TRUE(server_less_secure->check_session(1));
//     EXPECT_FALSE(server_less_secure->check_session(2));
//     EXPECT_EQ(server_less_secure->sessions_count(),1);
//     client->disconnect();
//     EXPECT_FALSE(client->check_connection());
//     server_less_secure->stop();
//     EXPECT_FALSE(server_less_secure->is_running());
// }


// /*=====================================================================================================================*/
// TEST(WebSocketsTests, LimitedClientsTest)
// {
//     std::shared_ptr<client_abstract> client1 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client2 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client3 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client4 = std::make_shared<ws_client>();
//     std::shared_ptr<client_abstract> client5 = std::make_shared<ws_client>();
//     server->start();
//     ASSERT_TRUE(server->is_running());
//     EXPECT_TRUE(client1->connect(ip,8081));
//     EXPECT_TRUE(client1->check_connection());
//     EXPECT_TRUE(client2->connect(ip,8081));
//     EXPECT_TRUE(client2->check_connection());
//     EXPECT_TRUE(server->is_serving());
//     EXPECT_TRUE(server->check_session(1));
//     EXPECT_TRUE(server->check_session(2));
//     EXPECT_FALSE(server->check_session(3));
//     EXPECT_EQ(server->sessions_count(),2);
//     EXPECT_TRUE(client3->connect(ip,8081));
//     EXPECT_TRUE(client4->connect(ip,8081));
//     EXPECT_FALSE(client5->connect(ip,8081));
//     EXPECT_TRUE(server->is_serving());
//     EXPECT_TRUE(server->check_session(3));
//     EXPECT_TRUE(server->check_session(4));
//     EXPECT_FALSE(server->check_session(5));
//     client1->disconnect();
//     EXPECT_EQ(server->sessions_count(),3);
//     client2->disconnect();
//     EXPECT_EQ(server->sessions_count(),2);
//     client3->disconnect();
//     EXPECT_EQ(server->sessions_count(),1);
//     client4->disconnect();
//     EXPECT_EQ(server->sessions_count(),0);
//     client5->disconnect();
//     EXPECT_EQ(server->sessions_count(),0);
//     EXPECT_FALSE(server->is_serving());
//     server->stop();
// }
// /*=====================================================================================================================*/

// /************************************************************************************************************************
//  *                     				    WEBSOCKET SECURE TEST CASES
//  **********************************************************************************************************************
