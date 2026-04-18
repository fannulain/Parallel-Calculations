#include "EventHandler.h"
#include <iostream>
#include <random>
#include <algorithm>
#include "ApplicationProtocol.h"

EventHandler::EventHandler(Client& client) : client(client), currentMatrixSize(0), isConnected(false) {}

std::vector<std::vector<int>> EventHandler::generateRandomMatrix(uint32_t size) 
{
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size));
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> dis(-100, 100); 

    for (uint32_t i = 0; i < size; ++i) 
    {
        for (uint32_t j = 0; j < size; ++j) 
        {
            matrix[i][j] = dis(gen);
        }
    }
    return matrix;
}

void EventHandler::printMenu() 
{
    std::cout << "\n=============================================\n";
    std::cout << "=========== Client Control Panel ============\n";
    std::cout << "=============================================\n";
    std::cout << "1. Connect to server\n";
    std::cout << "2. Generate matrix and send configuration\n";
    std::cout << "3. Start processing\n";
    std::cout << "4. Check server status\n";
    std::cout << "5. Get final result\n";
    std::cout << "0. Exit\n";
    std::cout << "Your choice: ";
}

void EventHandler::run() 
{
    bool running = true;
    while (running) {
        printMenu();
        int choice;
        
        if (!(std::cin >> choice)) 
        {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        try 
        {
            switch (choice) 
            {
                case 1: handleConnect(); break;
                case 2: handleConfig(); break;
                case 3: handleStart(); break;
                case 4: handleStatus(); break;
                case 5: handleResult(); break;
                case 0: running = false; break;
                default: std::cout << "[ERROR] Invalid choice. Please try again.\n"; break;
            }
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "[ERROR] " << e.what() << "\n";
            isConnected = false; 
            client.disconnect();
        }
    }

    if (isConnected) 
    {
        client.safeDisconnect();
    } 
    else 
    {
        client.disconnect();
    }
}

void EventHandler::handleConnect() 
{
    if (isConnected) 
    {
        std::cout << "Already connected to the server!\n";
        return;
    }
    
    std::cout << "Connecting...\n";
    client.connectToServer();
    isConnected = true;
    std::cout << "Successfully connected!\n";
}

void EventHandler::handleConfig() 
{
    if (!isConnected) 
    {
        std::cout << "Please connect to the server first (Option 1)!\n";
        return;
    }

    uint32_t size, threads;
    std::cout << "Enter square matrix size: ";
    std::cin >> size;
    std::cout << "Enter number of worker threads: ";
    std::cin >> threads;

    std::cout << "Generating " << size << "x" << size << " matrix with random numbers...\n";
    auto matrix = generateRandomMatrix(size);
    
    std::cout << "Sending network configuration package...\n";
    client.processConfig(threads, size, matrix);
    currentMatrixSize = size;
    
    std::cout << "\n[OK] Configuration and matrix successfully sent to the server!\n";
}

void EventHandler::handleStart() 
{
    if (!isConnected) 
    {
        std::cout << "[ERROR] Not connected to the server.\n";
        return;
    }
    
    client.processStart();
    std::cout << "[OK] Start processing command sent!\n";
}

void EventHandler::handleStatus() 
{
    if (!isConnected) 
    {
        std::cout << "[ERROR] Not connected.\n";
        return;
    }
    
    uint8_t status = client.processStatus();
    
    std::cout << "\nSERVER STATUS:";
    if (status == STATUS_PROCESSING) 
    {
        std::cout << "STATUS_PROCESSING\n";
    } 
    else if (status == STATUS_DONE) 
    {
        std::cout << "STATUS_DONE\n";
    } 
    else if (status == STATUS_ERROR) 
    {
        std::cout << "STATUS_ERROR\n";
    } 
    else 
    {
        std::cout << "Unknown status: " << (int)status << "\n";
    }
    std::cout << "\n";
}

void EventHandler::handleResult() 
{
    if (!isConnected) 
    {
        std::cout << "[ERROR] Not connected.\n";
        return;
    }
    if (currentMatrixSize == 0) 
    {
        std::cout << "[ERROR] No configuration/matrix size saved. Please configure first.\n";
        return;
    }

    std::cout << "Fetching result from server...\n";
    auto res = client.processResult(currentMatrixSize);
    
    if (res.empty()) 
    {
        std::cout << "Server replied with an empty package. Status might not be STATUS_DONE yet.\n";
    } 
    else 
    {
        std::cout << "\n[SUCCESS] Result matrix " << res.size() << "x" << res.size() << " successfully received!\n";

        int limit = 5;
        std::cout << "Preview of the received matrix (fragment " << limit << "x" << limit << "):\n";
        
        for (int i = 0; i < limit; ++i) 
        {
            for (int j = 0; j < limit; ++j) 
            {
                std::cout << res[i][j] << "\t";
            }
            std::cout << "\n";
        }
        
        if (res.size() > (size_t)limit) 
        {
            std::cout << "(... other elements hidden ...)\n";
        }
    }
}
