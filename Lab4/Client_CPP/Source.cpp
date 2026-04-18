#include <iostream>
#include "Client.h"
#include "EventHandler.h"

int main() {
    try {
        Client client("127.0.0.1", 666); 
        EventHandler handler(client);
        handler.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
