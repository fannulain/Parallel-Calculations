#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "Client.h"
#include <vector>

class EventHandler 
{
private:
    Client& client;
    uint32_t currentMatrixSize;
    bool isConnected;
    
    std::vector<std::vector<int>> generateRandomMatrix(uint32_t size);
    void printMenu();

    void handleConnect();
    void handleConfig();
    void handleStart();
    void handleStatus();
    void handleResult();
    
public:
    explicit EventHandler(Client& client);
    void run();
};

#endif //EVENT_HANDLER_H