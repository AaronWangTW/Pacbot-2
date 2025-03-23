#include <iostream>
#include "ServerProcess.hpp"

int main() { 
    std::cout << "Starting Simulation" << std::endl;
    ServerProcess sp;
    sp.run();
}
