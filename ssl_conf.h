/************************************************************************************************************************
 * 	Module: TLS/SSL Options and Configurations
 * 	File Name: ssl_conf.h
 *  Authors: Ahmed Desoky
 *	Date: 19/1/2025
 *	*********************************************************************************************************************
 *	Description: This file includes 2 essential overloaded functions to apply SSL options required for WebSocket Secure
 *               communications. such as, using keys (asymmetric keys) for Diffie-Hellman key exchanging,
 *               using digital certificates for peers verification and make Diffie-Hellman key exchanging every
 *               new session created.
 *               Default options:
 *                          - No SSL versions.
 *                          - Asymmetric private key for DH.
 *                          - Digital Certificates verification (Optional). 2 function overloaded for this option
 *               If you want to add your options and configurations please go to this webpage
 *               -> https://beta.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ssl__context.html
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
#define DEFAULT_CERTIFICATE "../../credentials/default-cert.pem"
/************************************************************************************************************************
 *                     							   NAMESPACES
 ***********************************************************************************************************************/
namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
/************************************************************************************************************************
* Function Name: Set_SSL_CTX (1)
* Class name: NONE
* Access: Public
* Specifiers: static
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: Yes
* Parameters (in): ssl_context reference
*                  user asymmetric private key path as const string reference
*                  user digital certificate path as const string reference
*                  Certificate Authority digital certificate path as const string reference
* Parameters (out): NONE
* Return value: NONE
* Description: This function sets ssl_context options according to the given input.
*              It disables using SSL old versions, enables Diffie-Hellman key exchanging
*              and enables digital certificates peer verification.
************************************************************************************************************************/
static void Set_SSL_CTX(ssl::context& ssl_ctx,const std::string& key, const std::string& certificate, const std::string& CA)
{
    try
    {
        ssl_ctx.set_options(ssl::context::default_workarounds | //set options, Enables workarounds for known bugs in SSL libraries.
                            net::ssl::context::no_sslv2 |   //Disable the deprecated SSLv2 protocol.
                            net::ssl::context::no_sslv3 |   //Disable the deprecated SSLv3 protocol.
                            net::ssl::context::single_dh_use); //use a new key for each key exchange (session) in Diffie-Helman (DH).
        ssl_ctx.use_private_key_file(key,ssl::context::pem); //set private key
        ssl_ctx.use_certificate_file(certificate,ssl::context::pem);   //set certificate
        ssl_ctx.load_verify_file(CA); //set the verification certificate (CA certificate)
        ssl_ctx.set_verify_mode(ssl::verify_peer);    //certificate verification enabled
    }
    catch(...)
    {throw;}   //failed to set SSL configurations, rethrow exception
}
/************************************************************************************************************************
* Function Name: Set_SSL_CTX (2)
* Class name: NONE
* Access: Public
* Specifiers: static
* Running Thread: Caller thread
* Sync/Async: Synchronous
* Reentrancy: Reentrant
* Expected  Exception: Yes
* Parameters (in): ssl_context reference
*                  user asymmetric private key path as const string reference
* Parameters (out): NONE
* Return value: NONE
* Description: This function sets ssl_context options according to the given input.
*              It disables using SSL old versions, enables Diffie-Hellman key exchanging
*              and disables digital certificates peer verification.
************************************************************************************************************************/
static void Set_SSL_CTX(ssl::context& ssl_ctx,const std::string& key_file) //TLS context to contain SSL configurations
{
    try
    {
        ssl_ctx.set_options(ssl::context::default_workarounds | //set options, Enables workarounds for known bugs in SSL libraries.
                            net::ssl::context::no_sslv2 |   //Disable the deprecated SSLv2 protocol.
                            net::ssl::context::no_sslv3 |   //Disable the deprecated SSLv3 protocol.
                            //net::ssl::context::no_tlsv1_3   |//disable TLSv1.3 to allow anonymous DH
                            net::ssl::context::single_dh_use); //use a new key for each key exchange (session) in Diffie-Helman (DH).
        ssl_ctx.use_private_key_file(key_file,ssl::context::pem); //set private key
        ssl_ctx.use_certificate_file(DEFAULT_CERTIFICATE,ssl::context::pem);   //set certificate default
        ssl_ctx.set_verify_mode(ssl::verify_none);    //no certificate verification, only DH key exchange
    }
    catch(...)
    {throw;}   //failed to set SSL configurations, rethrow exception
}
