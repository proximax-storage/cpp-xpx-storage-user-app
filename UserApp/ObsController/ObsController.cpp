/*
*** Copyright 2025 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ObsController.h"
#include "Logs.h"

#include <filesystem>
#include <QString>
#include <QCryptographicHash>

#define SIRIUS_PROFILE_NAME "Sirius_stream"

void dbg_print_response(const boost::beast::flat_buffer& buffer)
{
    std::string response = boost::beast::buffers_to_string(buffer.cdata());
    std::cout << "<--\ndbg_print_response: " << response << std::endl;
}

namespace obs_ws_conn {


void ObsController::connect( std::string hostAddr, std::string port, std::string password, std::string outputPath )
{
    std::error_code ec;
    if ( ! std::filesystem::exists( outputPath, ec ) )
    {
        std::filesystem::create_directories( outputPath, ec );
    }
    if ( ec )
    {
        m_eventHandler.onCreateOutputFolderFailed( outputPath, ec.message() );
    }

    
    m_isConnected = false;
    m_isStopped = false;
    
    connect( hostAddr, port );
    
    if ( m_isStopped || ! m_isConnected )
    {
        return;
    }
    
    try
    {
        // Read authentication message
        readResponse("0");
        
        std::string challenge           = m_serverResponse["d"]["authentication"]["challenge"].asString();
        std::string salt                = m_serverResponse["d"]["authentication"]["salt"].asString();
//        std::string obsWebSocketVersion = m_serverResponse["d"]["obsWebSocketVersion"].asString();
//        std::string rpcVersion          = m_serverResponse["d"]["rpcVersion"].asString();
        
        // Authenticate with OBS WebSocket
        {
            auto authenticationStr = createAuthenticationString( password, salt, challenge );
            Json::Value auth_message;
            auth_message["op"] = 1; // Identify operation
            auth_message["d"]["rpcVersion"] = 1;
            auth_message["d"]["authentication"] = authenticationStr;
            sendMessage(auth_message);

            readResponse("2");
        }
        
        //deleteSiriusProfile();
        
        setSiriusProfile( outputPath );
        
//        bool isPaused;
//        isRecording( isPaused );
//        
//        startRecord();
//        sleep(20);
//        stopRecord();
//        sleep(1);
        
    }
    catch ( const std::runtime_error& err )
    {
        _LOG( "Error: " << err.exception::what() );
        disconnect();
    }
}

void ObsController::updateSiriusProfile( std::string outputPath )
{
//    if ( getProfileParameter( "Stream1", "RecQuality" ) != "Stream" )
//    {
//        setProfileParameter( "Stream1", "RecQuality", "Stream" );
//    }
    
    setProfileParameter( "Output", "FilenameFormatting", "obs-stream" );
    setProfileParameter( "AdvOut", "RecFilePath", outputPath );
    setProfileParameter( "AdvOut", "RecFormat2", "hls" );
    setProfileParameter( "AdvOut", "RecSplitFileType", "Time" );
    setProfileParameter( "AdvOut", "FFFormat", "" );
    setProfileParameter( "AdvOut", "FFFormatMimeType", "" );
    setProfileParameter( "AdvOut", "FFVEncoderId", "0" );
    setProfileParameter( "AdvOut", "FFVEncoder", "" );
    setProfileParameter( "AdvOut", "FFVEncoderId", "0" );
    setProfileParameter( "AdvOut", "FFAEncoder", "" );
    setProfileParameter( "Output", "Mode", "Advanced" );
}

void ObsController::setSiriusProfile( std::string outputPath )
{
    {
        Json::Value message;
        message["op"] = 6; // Request operation
        message["d"]["requestType"] = "GetProfileList";
        message["d"]["requestId"] = "1";
        sendMessage(message);
    }
    readResponse("7");
    
    bool siriusProfileExists = false;
    auto profiles = m_serverResponse["d"]["responseData"]["profiles"];
    auto initialProfileName = m_serverResponse["d"]["responseData"]["currentProfileName"].asString();
    _LOG( "Init profile: " << initialProfileName )
    
    if ( initialProfileName == SIRIUS_PROFILE_NAME )
    {
        updateSiriusProfile( outputPath );
        return;
    }
    
    for( auto name : profiles )
    {
        if ( name.asString() == SIRIUS_PROFILE_NAME )
        {
            siriusProfileExists = true;
        }
    }
    
    if ( ! siriusProfileExists )
    {
        createSiriusProfile();
        
        //current profile changed
        readResponse("5");
    }
    else
    {
        {
            Json::Value message;
            message["op"] = 6; // Request operation
            message["d"]["requestType"] = "SetCurrentProfile";
            message["d"]["requestData"]["profileName"] = SIRIUS_PROFILE_NAME;
            message["d"]["requestId"] = "1";
            sendMessage(message);
        }

        //current profile changing
        readResponse("5");
        //current profile changed
        readResponse("5");
        readResponse("7");
    }

    updateSiriusProfile( outputPath );
    
    setCurrentProfile( initialProfileName );
    setCurrentProfile( SIRIUS_PROFILE_NAME );
    
    {
        Json::Value message;
        message["op"] = 6; // Request operation
        message["d"]["requestType"] = "GetProfileList";
        message["d"]["requestId"] = "2";
        sendMessage(message);
    }
    readResponse("7");
    
    auto currentProfile = m_serverResponse["d"]["responseData"]["currentProfileName"].asString();
    if ( currentProfile != SIRIUS_PROFILE_NAME )
    {
        _LOG("Current Profile: " << currentProfile )
        m_eventHandler.onSetProfileError();
    }
}

void ObsController::setCurrentProfile( std::string profileName )
{
    {
        Json::Value message;
        message["op"] = 6; // Request operation
        message["d"]["requestType"] = "SetCurrentProfile";
        message["d"]["requestData"]["profileName"] = profileName;
        message["d"]["requestId"] = "1";
        sendMessage(message);
    }
    if ( "5" == readResponse() )
    {
        readResponse("5");
        readResponse("7");
    }
}


void ObsController::createSiriusProfile()
{
    Json::Value message;
    message["op"] = 6; // Request operation
    message["d"]["requestType"] = "CreateProfile";
    message["d"]["requestData"]["profileName"] = SIRIUS_PROFILE_NAME;
    message["d"]["requestId"] = "1";

    sendMessage(message);

    readResponse("7");
    readResponse("5");
}

void ObsController::deleteSiriusProfile()
{
//    Json::Value message;
//    message["op"] = 6; // Request operation
//    message["d"]["requestType"] = "RemoveProfile";
//    message["d"]["requestData"]["profileName"] = SIRIUS_PROFILE_NAME;
//    message["d"]["requestId"] = "1";
//
//    sendMessage(message);
//
//    readResponse("7");
//    readResponse("5");
}

void ObsController::connect( std::string hostAddr, std::string port )
{
    m_isConnected = false;
    try
    {
        _LOG( "Started" );
        // Resolve OBS WebSocket address
        boost::asio::ip::tcp::resolver resolver{ m_ioc };
        auto const results = resolver.resolve( hostAddr, port );

        // Connect to the WebSocket server
        boost::asio::connect( m_wSocket.next_layer(), results.begin(), results.end() );
        m_wSocket.handshake( hostAddr, "/" );
        m_isConnected = true;
        _LOG( "Connected to OBS WebSocket" );
    }
    catch( const boost::system::system_error& err )
    {
        m_eventHandler.onConnectionFailed();
        
        _LOG("connect error:" << err.what() )
//        if ( err.code().value() == boost::system::errc::connection_refused )
//        {
//            _LOG( "Connection refused" )
//        }
    }
    catch( ... )
    {
        m_eventHandler.onConnectionFailed();

        _LOG("Unknown connect error:" )
    }
}

void ObsController::disconnect()
{
    if ( m_isConnected )
    {
        websocket::close_reason reason{ websocket::close_code::normal };
        reason.reason = "todo";
        boost::beast::error_code ec;
        m_wSocket.close( reason, ec );
    }
    m_eventHandler.onClosed();
}

void ObsController::sendMessage( const Json::Value& message )
{
    //std::cout << "-->\nsendMessage: " << message << std::endl;
    std::string str = Json::FastWriter().write(message);
    std::cout << "-->\nsendMessage: " << str << std::endl;
    m_wSocket.write( boost::asio::buffer( str ) );
}


void ObsController::readResponse( std::string op )
{
    // Get auth message
    beast::flat_buffer buffer;
    m_wSocket.read( buffer );
    dbg_print_response(buffer);
    std::string str = beast::buffers_to_string( buffer.cdata() );
    
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( str, m_serverResponse );     //parse process
    assert( m_serverResponse["op"].asString()==op );
    if ( ! parsingSuccessful )
    {
        std::cout  << "Failed to parse: "
               << reader.getFormattedErrorMessages();
        return;
    }
}

std::string ObsController::readResponse()
{
    // Get auth message
    beast::flat_buffer buffer;
    m_wSocket.read( buffer );
    dbg_print_response(buffer);
    std::string str = beast::buffers_to_string( buffer.cdata() );
    
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( str, m_serverResponse );     //parse process
    if ( ! parsingSuccessful )
    {
        std::cout  << "Failed to parse: "
               << reader.getFormattedErrorMessages();
        return "";
    }
    return m_serverResponse["op"].asString();
}

std::string ObsController::createAuthenticationString( std::string password, std::string salt, std::string challenge )
{
    auto challengeHash = QCryptographicHash(QCryptographicHash::Algorithm::Sha256);
    challengeHash.addData(QByteArray::fromStdString(password));
    challengeHash.addData(QByteArray::fromStdString(salt));
    auto secret = challengeHash.result().toBase64().toStdString();

    QString secretAndChallenge = "";
    secretAndChallenge += QString::fromStdString(secret);
    secretAndChallenge += QString::fromStdString(challenge);

    // Generate a SHA256 hash of secretAndChallenge
    auto hash = QCryptographicHash::hash(secretAndChallenge.toUtf8(), QCryptographicHash::Algorithm::Sha256);

    // Encode the SHA256 hash to Base64
    std::string authenticationString = hash.toBase64().toStdString();

    return authenticationString;
}

void ObsController::setProfileParameter( std::string category, std::string name, std::string value )
{
    {
        Json::Value message;
        message["op"] = 6; // Request operation
        message["d"]["requestType"] = "SetProfileParameter";
        message["d"]["requestData"]["parameterCategory"] = category;
        message["d"]["requestData"]["parameterName"] = name;
        message["d"]["requestData"]["parameterValue"] = value;
        message["d"]["requestId"] = "1";
        
        sendMessage(message);
    }
    readResponse("7");
    
    Json::Value message;
    message["op"] = 6; // Request operation
    message["d"]["requestType"] = "GetProfileParameter";
    message["d"]["requestData"]["parameterCategory"] = category;
    message["d"]["requestData"]["parameterName"] = name;
    message["d"]["requestId"] = "1";

    sendMessage(message);
    
    readResponse("7");
    if ( m_serverResponse["d"]["responseData"]["parameterValue"] !=value )
    {
        m_eventHandler.onSetProfileError();
    }
    
    _LOG("todo")

}


std::string ObsController::getProfileParameter( std::string category, std::string name )
{
    Json::Value message;
    message["op"] = 6; // Request operation
    message["d"]["requestType"] = "GetProfileParameter";
    message["d"]["requestData"]["parameterCategory"] = category;
    message["d"]["requestData"]["parameterName"] = name;
    message["d"]["requestId"] = "1";

    sendMessage(message);

    readResponse("7");
    
    return m_serverResponse["d"]["responseData"]["parameterValue"].asString();
}

bool ObsController::isRecording( bool& isPaused )
{
    Json::Value message;
    message["op"] = 6; // Request operation
    message["d"]["requestType"] = "GetRecordStatus";
    message["d"]["requestId"] = "1";

    sendMessage(message);

    readResponse("7");
    
    isPaused = m_serverResponse["d"]["responseData"]["outputPaused"].asBool();
    
    return m_serverResponse["d"]["responseData"]["outputActive"].asBool();
}


void ObsController::startRecord()
{
    Json::Value message;
    message["op"] = 6; // Request operation
    message["d"]["requestType"] = "StartRecord";
    message["d"]["requestId"] = "1";

    sendMessage(message);

    readResponse("7");
    if ( m_serverResponse["d"]["requestStatus"]["result"].asBool() )
    {
        // OBS_WEBSOCKET_OUTPUT_STARTING
        readResponse("5");
        // OBS_WEBSOCKET_OUTPUT_STARTED
        readResponse("5");
    }
    else
    {
        
    }

}

void ObsController::stopRecord()
{
    Json::Value message;
    message["op"] = 6; // Request operation
    message["d"]["requestType"] = "StopRecord";
    message["d"]["requestId"] = "1";

    sendMessage(message);

    readResponse("7");
    if ( m_serverResponse["d"]["requestStatus"]["result"].asBool() )
    {
        // OBS_WEBSOCKET_OUTPUT_STOPPING
        readResponse("5");
        // OBS_WEBSOCKET_OUTPUT_STOPPED
        readResponse("5");
    }
    else
    {
        
    }
}

}


