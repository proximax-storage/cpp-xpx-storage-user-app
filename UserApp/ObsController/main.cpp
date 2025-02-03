#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <json/json.h> // Include a JSON library like JsonCpp
#include <filesystem>

#include <QString>
#include <QCryptographicHash>

#include "ObsController.h"
#include "Logs.h"

class EventHandler: public obs_ws_conn::ObsEventHandler
{
public:
    virtual void onCreateOutputFolderFailed( std::string outputPath, std::string error ) override
    {
        _LOG("@ CreateOutputFolderFailed: " << error );
    }
    virtual void onConnectionFailed() override
    {
        _LOG("@ Connection Failed:" );
    }
    virtual void onAuthenticationFailed() override
    {
        _LOG("@ Authentication Failed:" );
    }
    virtual void onClosed() override
    {
        _LOG("@ Websocket closed:" );
    }
    virtual void onSiriusProfileAbsent() override
    {
        _LOG("@ Sirius Profile Absent:" );
    }
    virtual void onSetProfileError() override
    {
        _LOG("@ Cannot Set Sirius Profile:" );
        exit(0);
    }
    virtual void onRecordingStarted( bool successfully ) override
    {
        _LOG("@ Recording started: " << successfully);
    }
    virtual void onRecordingStopped( bool successfully ) override
    {
        _LOG("@ Recording stopped: " << successfully);
    }
};

EventHandler handler;

#define OUTPUT_FOLDER "/Users/alex2/000/000-recording"

int main()
{
    if ( std::filesystem::exists( OUTPUT_FOLDER ) )
    {
        std::filesystem::remove_all( OUTPUT_FOLDER );
    }
    
    obs_ws_conn::ObsController obsConn( handler );

    obsConn.connect( "localhost", "4455", "JV4xSox4nbi5xlSG", OUTPUT_FOLDER );
    obsConn.disconnect();
    
    obsConn.connect( "localhost", "4455", "JV4xSox4nbi5xlSG", OUTPUT_FOLDER );
    
    bool isPaused;
    if ( obsConn.isRecording( isPaused ) || isPaused )
    {
        std::cout << "obsConn.isRecording( isPaused ) || isPaused";
        exit(0);
    }

    obsConn.startRecord();
    sleep(20);
    obsConn.stopRecord();
    sleep(1);

}

