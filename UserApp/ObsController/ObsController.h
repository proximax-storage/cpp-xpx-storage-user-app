/*
*** Copyright 2025 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <iostream>
#include <memory>

#include <json/json.h> // JsonCpp

namespace obs_ws_conn {


namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;


class ObsEventHandler
{
public:
    ~ObsEventHandler() = default;

    virtual void onCreateOutputFolderFailed( std::string outputPath, std::string error ) = 0;

    virtual void onConnectionFailed() = 0;
    virtual void onAuthenticationFailed() = 0;
    virtual void onSiriusProfileAbsent() = 0;
    virtual void onSetProfileError() = 0;
    virtual void onClosed() = 0;
    
    virtual void onRecordingStarted( bool successfully ) = 0;
    virtual void onRecordingStopped( bool successfully ) = 0;
};

class ObsController
{
    boost::asio::io_context         m_ioc;
    websocket::stream<tcp::socket>  m_wSocket;

    ObsEventHandler&                m_eventHandler;

    bool m_isConnected = false;
    bool m_isStopped = false;
    
    Json::Value m_serverResponse;

public:
    ObsController( ObsEventHandler& eventHandler )
    :   m_wSocket(m_ioc),
        m_eventHandler(eventHandler)
    {}
    
    ~ObsController() {}

    void connect( std::string hostAddr, std::string port, std::string password, std::string outputPath );
    void disconnect();

    bool isRecording( bool& isPaused );
    void startRecord();
    void stopRecord();

    void stop() { m_isStopped = true; }

private:
    void connect( std::string hostAddr, std::string port );
    
    void sendMessage( const Json::Value& );
    void readResponse( std::string op );
    std::string readResponse();
    std::string createAuthenticationString( std::string password, std::string salt, std::string challenge );
    
    void updateSiriusProfile( std::string outputPath );
    void setSiriusProfile( std::string outputPath );
    void setCurrentProfile( std::string profileName );
    void createSiriusProfile();
    void deleteSiriusProfile();

    void setProfileParameter( std::string category, std::string name, std::string value );
    std::string getProfileParameter( std::string category, std::string name );
};

}
