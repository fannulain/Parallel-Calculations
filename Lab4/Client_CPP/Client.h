#ifndef CLIENT_H
#define CLIENT_H

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <stdexcept>

#pragma comment(lib, "Ws2_32.lib")

class Client
{
private:
    std::string serverAddress;
    uint16_t port;
    SOCKET connectionSocket;

    bool receiveExact(char* buffer, int length);
    bool sendExact(const char* buffer, int length);
    void sendMessage(uint8_t commandId, const char* message, uint32_t messageLength);

public:
    Client(const std::string& serverAddress, uint16_t port);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void connectToServer();
    void disconnect();
    void gracefulDisconnect();

    void processConfig(uint32_t threadsNum, uint32_t matrixSize, const std::vector<std::vector<int>>& matrix);
    void processStart();
    uint8_t processStatus();
    std::vector<std::vector<int>> processResult(uint32_t matrixSize);
};

#endif //CLIENT_H
