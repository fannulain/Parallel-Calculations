#include "Client.h"
#include "ApplicationProtocol.h"
#include <iostream>
#include <thread>
#include <chrono>

Client::Client(const std::string& serverAddress, uint16_t port)
    : serverAddress(serverAddress), port(port), connectionSocket(INVALID_SOCKET)
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
    }
}

Client::~Client()
{
    disconnect();
    WSACleanup();
}

void Client::disconnect() {
    if (connectionSocket != INVALID_SOCKET) {
        closesocket(connectionSocket);
        connectionSocket = INVALID_SOCKET;
    }
}

void Client::gracefulDisconnect() {
    if (connectionSocket != INVALID_SOCKET) {
        try {
            sendMessage(CMD_END_SESSION, nullptr, 0);
        } catch (...) {
        }
        disconnect();
    }
}

void Client::connectToServer()
{
    connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectionSocket == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address / Address not supported");
    }

    if (connect(connectionSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to connect to server: " + std::to_string(WSAGetLastError()));
    }
}

bool Client::receiveExact(char* buffer, int length) {
    int totalRead = 0;
    while (totalRead < length) {
        int n = recv(connectionSocket, buffer + totalRead, length - totalRead, 0);
        if (n <= 0) return false;
        totalRead += n;
    }
    return true;
}

bool Client::sendExact(const char* buffer, int length) {
    int totalSent = 0;
    while (totalSent < length) {
        int n = send(connectionSocket, buffer + totalSent, length - totalSent, 0);
        if (n <= 0) return false;
        totalSent += n;
    }
    return true;
}

void Client::sendMessage(uint8_t commandId, const char* message, uint32_t messageLength) {
    MessageHeader header;
    header.commandId = commandId;
    header.messageLength = htonl(messageLength);

    if (!sendExact((const char*)&header, sizeof(MessageHeader))) {
        throw std::runtime_error("Failed to send message header");
    }
    
    if (messageLength > 0 && message != nullptr) {
        if (!sendExact(message, messageLength)) {
            throw std::runtime_error("Failed to send message payload");
        }
    }
}

void Client::processConfig(uint32_t threadsNum, uint32_t matrixSize, const std::vector<std::vector<int>>& matrix) {
    if (connectionSocket == INVALID_SOCKET) throw std::runtime_error("Not connected to server");

    ConfigMessage config;
    config.threadsNum = htonl(threadsNum);
    config.matrixSize = htonl(matrixSize);

    uint32_t messageSize = sizeof(ConfigMessage) + matrixSize * matrixSize * sizeof(int);
    std::vector<char> message(messageSize);
    
    std::memcpy(message.data(), &config, sizeof(ConfigMessage));

    int* dataPtr = (int*)(message.data() + sizeof(ConfigMessage));
    for (uint32_t i = 0; i < matrixSize; ++i) {
        if (matrix[i].size() != matrixSize) throw std::runtime_error("Matrix inner size mismatch");
        for (uint32_t j = 0; j < matrixSize; ++j) {
            dataPtr[i * matrixSize + j] = htonl(matrix[i][j]);
        }
    }

    sendMessage(CMD_SEND_CONFIG, message.data(), messageSize);
}

void Client::processStart() {
    if (connectionSocket == INVALID_SOCKET) throw std::runtime_error("Not connected to server");
    sendMessage(CMD_START_PROCESSING, nullptr, 0);
}

uint8_t Client::processStatus() {
    if (connectionSocket == INVALID_SOCKET) throw std::runtime_error("Not connected to server");
    sendMessage(CMD_CHECK_STATUS, nullptr, 0);

    MessageHeader header;
    if (!receiveExact((char*)&header, sizeof(MessageHeader))) 
        throw std::runtime_error("Connection closed by server while waiting for status header");
    
    uint32_t length = ntohl(header.messageLength);
    if (header.commandId != CMD_CHECK_STATUS || length != 1) {
        throw std::runtime_error("Invalid response from server for checkStatus");
    }

    uint8_t status;
    if (!receiveExact((char*)&status, 1)) 
        throw std::runtime_error("Connection closed while reading status byte");
    
    return status;
}

std::vector<std::vector<int>> Client::processResult(uint32_t matrixSize) {
    if (connectionSocket == INVALID_SOCKET) throw std::runtime_error("Not connected to server");
    sendMessage(CMD_GET_RESULT, nullptr, 0);

    MessageHeader header;
    if (!receiveExact((char*)&header, sizeof(MessageHeader))) 
        throw std::runtime_error("Connection closed by server");
    
    uint32_t length = ntohl(header.messageLength);
    if (header.commandId != CMD_GET_RESULT) {
        throw std::runtime_error("Expected GET_RESULT command but got something else");
    }

    if (length == 0) {
        return {}; 
    }

    uint32_t expectedBytes = matrixSize * matrixSize * sizeof(int);
    if (length != expectedBytes) {
        throw std::runtime_error("Result matrix size mismatch");
    }

    std::vector<int> flatData(matrixSize * matrixSize);
    if (!receiveExact((char*)flatData.data(), expectedBytes)) {
        throw std::runtime_error("Connection closed while reading result data");
    }

    std::vector<std::vector<int>> result(matrixSize, std::vector<int>(matrixSize));
    for (uint32_t i = 0; i < matrixSize; ++i) {
        for (uint32_t j = 0; j < matrixSize; ++j) {
            result[i][j] = ntohl(flatData[i * matrixSize + j]);
        }
    }
    return result;
}