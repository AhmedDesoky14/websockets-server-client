/************************************************************************************************************************
 * 	Module: TLS/SSL Options Configurations
 * 	File Name: ssl_conf.h
 *  Authors: Ahmed Desoky
 *	Date: 19/1/2025
 *	*********************************************************************************************************************
 *	Description: TLS/SSL options function and configurations file
 *
 *
 ***********************************************************************************************************************/
#pragma once
/************************************************************************************************************************
 *                     							   INCLUDES
 ***********************************************************************************************************************/
#include <mutex>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
/************************************************************************************************************************
 *                     							   NAMESPACES
 ***********************************************************************************************************************/
namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
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
static bool Set_SSL_CTX(ssl::context& ssl_ctx,const std::string& key_file, const std::string& certificate_file) //TLS context to contain SSL configurations
{
    try
    {
        ssl_ctx.set_options(ssl::context::default_workarounds | //set options, Enables workarounds for known bugs in SSL libraries.
                            net::ssl::context::no_sslv2 |   //Disable the deprecated SSLv2 protocol.
                            net::ssl::context::no_sslv3 |   //Disable the deprecated SSLv3 protocol.
                            net::ssl::context::single_dh_use); //use a new key for each key exchange (session) in Diffie-Hellman (DH).
        ssl_ctx.use_private_key_file(key_file,ssl::context::pem); //set private key
        ssl_ctx.use_certificate_file(certificate_file,ssl::context::pem);   //set certificate

        //verification mode
        ssl_ctx.set_verify_mode(ssl::verify_none);    //no certificate verification
    }
    catch(...)
    {
        return false;   //failed to set SSL configurations
    }
    return true;    //Successfull operation
}
