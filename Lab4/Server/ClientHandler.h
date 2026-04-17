#pragma once
#include <WinSock2.h>
#include <mutex>
#include <vector>
#include <memory>
#include <atomic>
#include "ApplicationProtocol.h"
#include "TaskHandler.h"

class ClientSession {
private:
    SOCKET clientSocket;

    std::mutex sessionMutex;

    uint8_t currentStatus;

    uint32_t threadsNum;
    uint32_t matrixSize;
    std::vector<std::vector<int>> matrix;

    std::unique_ptr<TaskHandler> taskHandler;

    bool receiveExact(char* buffer, int length);
    bool sendExact(const char* buffer, int length);
    void sendMessage(uint8_t commandId, const char* message, uint32_t messageLength);

    void processConfig(const char* message, uint32_t messageLength);
    void processStart(const char* message, uint32_t messageLength);
    void processStatus();
    void processResult();

public:
    explicit ClientSession(SOCKET socket);
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    void handle();
};
