#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <sched.h>  // For sched_getcpu()

#define PORT 12345
#define BUFFER_SIZE 16

int main() {
    // Print the core on which this program is running.
    int core = sched_getcpu();
    std::cout << "Client starting on core " << core << ".\n";

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Try connecting until the server is available.
    while (true) {
        std::cout << "Client looking for connection to server...\n";
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << "Socket creation error\n";
            return 1;
        }
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        
        // Use the loopback address (works offline)
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address / Address not supported\n";
            return 1;
        }
        
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection not found, retrying in 1 second...\n";
            close(sock);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        std::cout << "Connection found with server.\n";
        break; // Connected successfully.
    }
    
    int cycleCount = 0;
    while (true) {
        // Send a "ping" to the server.
        const char *msg = "ping";
        send(sock, msg, strlen(msg), 0);
        
        // Simulate intensive work (busy loop).
        volatile long dummy = 0;
        for (int i = 0; i < 1000000; ++i) {
            dummy += i;
        }
        
        // Wait for the "pong" reply from the server.
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            std::cerr << "Server disconnected or error occurred.\n";
            break;
        }
        buffer[valread] = '\0';
        // (We expect "pong" here.)
        
        cycleCount++;
        if (cycleCount % 100 == 0) {
            std::cout << "Client: Completed " << cycleCount << " cycles.\n";
        }
    }
    
    close(sock);
    return 0;
}
