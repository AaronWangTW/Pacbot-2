#include "ServerProcess.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketInitResult.h>
#include <fstream>
#include <memory>
#include <format>
#include <stdio.h>
#include <cstring>

ServerProcess::ServerProcess() {
    // Initialize the websocket system
    ix::initNetSystem();

    // Create the address
    std::string address = std::format("ws://{}:{}/", "localhost", "1234");

    printf("Connecting to %s\n", address);

    // Create the websocket and configure parameters
    socket = std::make_unique<ix::WebSocket>();
    socket->setUrl(address);
    socket->setPingInterval(45);

    socket->setOnMessageCallback([](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                char buffer[strlen(msg->str)];
                strcpy(buffer,msg->str);
                gs = *(GameState*)buffer;
            }
            else if (msg->type == ix::WebSocketMessageType::Open)
            {
                std::cout << "Connection established" << std::endl;
                std::cout << "> " << std::flush;
            }
            else if (msg->type == ix::WebSocketMessageType::Error)
            {
                // Maybe SSL is not configured properly
                std::cout << "Connection error: " << msg->errorInfo.reason << std::endl;
                std::cout << "> " << std::flush;
            }
        }
    );

    // Attempt to connect to the server
    ix::WebSocketInitResult result = socket->connect(10);
    if (not result.success) {
        spdlog::error("Failed to connect to server");
        spdlog::trace(result.errorStr);
    }
}

ServerProcess::~ServerProcess() {
    // Shutdown the websocket system
    ix::uninitNetSystem();
}

void ServerProcess::run() {
    GameAgent ga;
    
    // Buffer for sending to the server
    char data;

    // Run the process while connected to the server
    while (socket->getReadyState() == ix::ReadyState::Open) {
        ga.update(gs);
        dm.overrideAgent(ga);
        Directions direc = dm.decide();

        switch (direc)
        {
        case Directions::UP:
            data = 'n';
            break;
        case Directions::DOWN:
            data = 's';
            break; 
        case Directions::LEFT:
            data = 'w';
            break; 
        case Directions::RIGHT:
            data = 'e';
            break; 
        default:
            data = 'x';
            break;
        }

        // Send the position data to the server
        socket->sendBinary(data);
    }

    // Stop the websocket
    socket->stop();
}