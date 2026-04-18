#include <iostream>
#include "Server.h"

int main() 
{
    try 
    {
        Server server(666);
        server.start();
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << "\n";
        return -1;
    }
    return 0;
}