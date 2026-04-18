#include "ClientHandler.h"
#include <iostream>
#include <thread>
#include <stdexcept>

ClientSession::ClientSession(SOCKET socket) 
    : clientSocket(socket), currentStatus(STATUS_DONE), threadsNum(0), matrixSize(0)
{}

ClientSession::~ClientSession() 
{
    disconnect();
}

void ClientSession::disconnect() 
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (clientSocket != INVALID_SOCKET) 
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
}

bool ClientSession::receiveExact(char* buffer, int length) 
{
    int totalRead = 0;
    while (totalRead < length) 
    {
        int n = recv(clientSocket, buffer + totalRead, length - totalRead, 0);
        if (n <= 0) return false;
        totalRead += n;
    }
    return true;
}

bool ClientSession::sendExact(const char* buffer, int length) 
{
    int totalSent = 0;
    while (totalSent < length) 
    {
        int n = send(clientSocket, buffer + totalSent, length - totalSent, 0);
        if (n <= 0) return false;
        totalSent += n;
    }
    return true;
}

void ClientSession::sendMessage(uint8_t commandId, const char* message, uint32_t messageLength) 
{
    MessageHeader header;
    header.commandId = commandId;
    header.messageLength = htonl(messageLength);

    if (!sendExact((const char*)&header, sizeof(MessageHeader))) return;
    if (messageLength > 0 && message != nullptr) 
    {
        sendExact(message, messageLength);
    }
}

void ClientSession::processConfig(const char* message, uint32_t messageLength) 
{
    if (messageLength < sizeof(ConfigMessage)) return;

    const ConfigMessage* config = (const ConfigMessage*)message;
    
    std::lock_guard<std::mutex> lock(sessionMutex);
    threadsNum = ntohl(config->threadsNum);
    matrixSize = ntohl(config->matrixSize);
    
    matrix.assign(matrixSize, std::vector<int>(matrixSize, 0));

    uint32_t expectedSize = sizeof(ConfigMessage) + matrixSize * matrixSize * sizeof(int);
    if (messageLength >= expectedSize) 
    {
        const int* data = (const int*)(message + sizeof(ConfigMessage));
        for (uint32_t i = 0; i < matrixSize; i++) 
        {
            for (uint32_t j = 0; j < matrixSize; j++) 
            {
                matrix[i][j] = ntohl(data[i * matrixSize + j]);
            }
        }
    }

    currentStatus = STATUS_DONE;
    std::cout << "[Client " << (int)clientSocket << "] Received configuration: threads=" << threadsNum << ", matrixSize=" << matrixSize << "\n";
}

void ClientSession::processStart(const char* message, uint32_t messageLength) 
{
    std::unique_lock<std::mutex> lock(sessionMutex);
    
    if (matrixSize == 0) return;

    currentStatus = STATUS_PROCESSING;
    std::cout << "[Client " << (int)clientSocket << "] Started processing task...\n";
    
    taskHandler = std::make_unique<TaskHandler>(threadsNum, matrixSize, matrix);
    
    auto thisClient = shared_from_this();
    std::thread([this, thisClient]() 
    {
        try 
        {
            taskHandler->calculate();
            std::lock_guard<std::mutex> lock2(sessionMutex);
            currentStatus = STATUS_DONE;
            std::cout << "[Client " << (int)clientSocket << "] Task processing finished.\n";
        } 
        catch (...) 
        {
            std::lock_guard<std::mutex> lock2(sessionMutex);
            currentStatus = STATUS_ERROR;
            std::cout << "[Client " << (int)clientSocket << "] Task processing failed (ERROR).\n";
        }
    }).detach();
}

void ClientSession::processStatus() 
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    std::cout << "[Client " << (int)clientSocket << "] Requested status. Current status code: " << (int)currentStatus << "\n";
    sendMessage(CMD_CHECK_STATUS, (const char*)&currentStatus, sizeof(currentStatus));
}

void ClientSession::processResult() 
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    std::cout << "[Client " << (int)clientSocket << "] Requested result.\n";
    
    if (currentStatus != STATUS_DONE) 
    {
        sendMessage(CMD_GET_RESULT, nullptr, 0);
        return;
    }

    std::vector<int> vectorizedMatrix;
    vectorizedMatrix.reserve(matrixSize * matrixSize);
    for (uint32_t i = 0; i < matrixSize; i++) 
    {
        for (uint32_t j = 0; j < matrixSize; j++) 
        {
            vectorizedMatrix.push_back(htonl(matrix[i][j]));
        }
    }

    sendMessage(CMD_GET_RESULT, (const char*)vectorizedMatrix.data(), vectorizedMatrix.size() * sizeof(int));
}

void ClientSession::handle() 
{
    while (true) 
    {
        MessageHeader header;
        if (!receiveExact((char*)&header, sizeof(MessageHeader))) 
        {
            break;
        }

        uint32_t messageLength = ntohl(header.messageLength);
        std::vector<char> message;
        
        if (messageLength > 0) 
        {
            message.resize(messageLength);
            if (!receiveExact(message.data(), messageLength)) 
            {
                break;
            }
        }

        if (header.commandId == CMD_END_SESSION) 
        {
            std::cout << "[Client " << (int)clientSocket << "] Ended session.\n";
            return;
        }

        switch (header.commandId) 
        {
            case CMD_SEND_CONFIG:
                processConfig(message.data(), messageLength);
                break;
            case CMD_START_PROCESSING:
                processStart(message.data(), messageLength);
                break;
            case CMD_CHECK_STATUS:
                processStatus();
                break;
            case CMD_GET_RESULT:
                processResult();
                break;
            default:
                break;
        }
    }

    std::cout << "[Client " << (int)clientSocket << "] Disconnected unexpectedly.\n";
}
