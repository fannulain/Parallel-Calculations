#include <iostream>
#include <winsock2.h>
#include "Server.h"

#pragma comment(lib, "ws2_32.lib")

int main() 
{
    int clientPort = 8080;

    try 
    {
        Server server(clientPort);
        server.start();
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
