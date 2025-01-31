#include <QCoreApplication>
//#include "websocket_old.h"
//#include "test_cases.h"
#include "tests.h"
int main(int argc, char *argv[])
{
    //QCoreApplication a(argc, argv);

    // Set up code that uses the Qt event loop here.
    // Call a.quit() or a.exit() to quit the application.
    // A not very useful example would be including
    // #include <QTimer>
    // near the top of the file and calling
    // QTimer::singleShot(5000, &a, &QCoreApplication::quit);
    // which quits the application after 5 seconds.

    // If you do not need a running Qt event loop, remove the call
    // to a.exec() or use the Non-Qt Plain C++ Application template.


    // net::io_context ioc;
    // net::io_context client1_ioc;
    // net::io_context client2_ioc;
    // net::io_context client3_ioc;
    // std::string certificate = "/home/desoky/Fade_Project/keys/server-cert.pem";

    // std::string server_key_file = "/home/desoky/Fade_Project/keys/server-key.pem";
    // ssl::context ssl_ctx{ssl::context::tlsv12_server};    //use TLS version 1.2
    // // ssl_ctx.set_options(ssl::context::default_workarounds | //set options, Enables workarounds for known bugs in SSL libraries.
    // //                     ssl::context::no_sslv2 |   //Disable the deprecated SSLv2 protocol.
    // //                     ssl::context::single_dh_use); //use a new key for each key exchange (session) in Diffie-Hellman (DH).
    // ssl_ctx.use_private_key_file(server_key_file,ssl::context::pem); //set private key
    // ssl_ctx.use_certificate_chain_file(certificate);
    // ssl_ctx.set_verify_mode(ssl::verify_none);    //no certificate verification

    // std::string client_key_file = "/home/desoky/Fade_Project/keys/testprivkey.pem";
    // ssl::context ctx{ssl::context::tlsv12_client};    //use TLS version 1.2
    // // ctx.set_options(ssl::context::default_workarounds | //set options, Enables workarounds for known bugs in SSL libraries.
    // //                     ssl::context::no_sslv2 |   //Disable the deprecated SSLv2 protocol.
    // //                     ssl::context::single_dh_use); //use a new key for each key exchange (session) in Diffie-Hellman (DH).
    // ctx.use_private_key_file(client_key_file,ssl::context::pem); //set private key
    // ctx.use_certificate_chain_file(certificate);

    // ctx.set_verify_mode(ssl::verify_none);    //no certificate verification

    // wss_server_base server(ioc,ssl_ctx,9091,2);
    // server.start();
    // //client_ioc.run();

    // std::string ip = "127.0.0.1";
    // std::thread client1_thread([&]()  //capture the server object
    //                            {
    //                                std::shared_ptr<wss_client_base> client = std::make_shared<wss_client_base>(client1_ioc,ctx);
    //                                try{
    //                                    if(!client->connect(ip,9091))
    //                                    {
    //                                        std::cout << "Client connect failed" << std::endl;
    //                                        return; //conenction failed
    //                                    }
    //                                }
    //                                catch(std::exception& e)
    //                                {
    //                                    std::cout << "Client connect failed: " + std::string(e.what()) << std::endl;
    //                                }


    //                                std::string mes = "Hello, Websocket, client 1\n";
    //                                std::vector<unsigned char> mes_vec(mes.begin(),mes.end());
    //                                // for(int i=0;i<2;++i)
    //                                // {
    //                                //     client->send_message(mes_vec);
    //                                // }
    //                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //                                client->send_message(mes_vec);

    //                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //                                client->disconnect();
    //                            });
    // client1_thread.detach();

    // //std::this_thread::sleep_for(std::chrono::seconds(1));
    // std::thread client2_thread([&]()  //capture the server object
    //                            {
    //                                std::shared_ptr<wss_client_base> client = std::make_shared<wss_client_base>(client2_ioc,ctx);
    //                                //client->connect(ip,9091);
    //                                try{
    //                                    if(!client->connect(ip,9091))
    //                                    {
    //                                        std::cout << "Client connect failed" << std::endl;
    //                                        return; //conenction failed
    //                                    }
    //                                }
    //                                catch(std::exception& e)
    //                                {
    //                                    std::cout << "Client connect failed: " + std::string(e.what()) << std::endl;
    //                                }
    //                                // if(!client->connect(ip,9090))
    //                                // {
    //                                //     return; //connection failed
    //                                // }
    //                                std::string mes = "Hello, Websocket, client 2\n";
    //                                std::vector<unsigned char> mes_vec(mes.begin(),mes.end());
    //                                // for(int i=0;i<2;++i)
    //                                // {
    //                                //     client->send_message(mes_vec);
    //                                // }
    //                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //                                client->send_message(mes_vec);
    //                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //                                client->disconnect();
    //                                return;
    //                            });
    // client2_thread.detach(); //detach from the session_thread
    // // std::this_thread::sleep_for(std::chrono::seconds(1));
    // std::thread client3_thread([&]()  //capture the server object
    //                            {
    //                                std::shared_ptr<wss_client_base> client = std::make_shared<wss_client_base>(client3_ioc,ctx);
    //                                try{
    //                                    if(!client->connect(ip,9091))
    //                                    {
    //                                        std::cout << "Client connect failed" << std::endl;
    //                                        return; //conenction failed
    //                                    }
    //                                }
    //                                catch(std::exception& e)
    //                                {
    //                                    std::cout << "Client connect failed: " + std::string(e.what()) << std::endl;
    //                                }
    //                                std::string mes = "Hello, Websocket, client 3\n";
    //                                std::vector<unsigned char> mes_vec(mes.begin(),mes.end());
    //                                // for(int i=0;i<2;++i)
    //                                // {
    //                                //     client->send_message(mes_vec);
    //                                // }
    //                                client->send_message(mes_vec);
    //                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //                                client->disconnect();
    //                                return;
    //                            });
    // client3_thread.detach(); //detach from the session_thread



    // std::this_thread::sleep_for(std::chrono::seconds(5));
    // std::cout << "ongoing sessions: " << server.sessions_count() << std::endl;
    // while(server.is_serving());
    // server.stop();
    // std::cout << "SERVER STOPPED" << std::endl;



    //client->disconnect();
    /*===========================================================================*/
    //TestCase1();
    //TestCase2();
    //TestCase3();
    //TestCase4();
    //TestCase5();
    //TestCase6();
    //TestCase7();





    /*===========================================================================*/
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
    //return 0;
    //return a.exec();
}
