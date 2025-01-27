# websockets-server-client
A websockets and websockets secure client-server module. implemented as following

First, **websockets_client.cpp and websockets_client.h**. 
  * includes base classes and final class of client for both secure and insecure communications.

Second, **websockets_server.cpp and websockets_server.h**
  * includes base classes and final class of server for both secure and insecure communications.
  * includes base classes and final class of sessions handled by server.
  Note: interfacing with sessions is by using server class only.

 Third, **ssl_conf.h**
  * includes essential function for configuring SSL verification and handshaking.

## Features
  * Both types of communications, secure and insecure websockets.
  * websocket secure is secured by "Diffie-Hellman" key exchanging and certificate verification.
  * Asynchronus send/receive operations. utilizing Boost.Asio and Boost.Beast libraries and exploiting multithreading with thread-safe operations.
  * Test suite by "gtest" to perform extensive unit testing for the module -> **in progress**.


## External Dependencies
  * Boost.Asio library
  * Boost.Beast library
  * gtest library
  * openssl for key and certificates creation.

**NOTE:** ssl_conf.h function needs to be modified as needs. 

---

**Author:**
	Ahmed Desoky,
	
**Emails:**
	ahmed0201150@gmail.com
