#pragma once

#include <memory>
#include <ixwebsocket/IXWebSocket.h>
#include "DecisionModule.hpp"

/**
 * @brief A process that sends the location data to the server
 * 
 */
class ServerProcess{
private:
    GameState gs;
    std::unique_ptr<ix::WebSocket> socket;
    DecisionModule dm;
    char* address_buffer;

public:
    ServerProcess();
    ~ServerProcess();
    void run();
};