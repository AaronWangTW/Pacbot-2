#include "ServerProcess.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketInitResult.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <cstring>

ServerProcess::ServerProcess() {
    // Initialize the websocket system
    ix::initNetSystem();

    address_buffer = new char[100];
    sprintf(address_buffer, "ws://%s:%d/", "localhost", 1234);

    printf("Connecting to %s\n", address_buffer);

    // Create the websocket and configure parameters
    socket = std::make_unique<ix::WebSocket>();
    socket->setUrl(address_buffer);
    socket->setPingInterval(45);

    socket->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Message)
            {
                std::string str = msg->str;
                if (str.size() == sizeof(GameState))
                {
                    memcpy(&gs, str.c_str(), sizeof(GameState));
                }
                else
                {
                    std::cerr << "Received data size does not match GameState size!" << std::endl;
                }
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
        printf("Error: %s\n", result.errorStr.c_str());
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
        socket->sendBinary(&data);
    }

    // Stop the websocket
    socket->stop();
}