#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <sched.h>  // For sched_getcpu()
#include <iomanip>  // For formatting time
#include <ctime>    // For time functions

#define PORT 12345
#define BUFFER_SIZE 16

// Function to get current timestamp
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    
    std::ostringstream oss;
    oss << "[" << std::setfill('0') << std::setw(2) << now_tm.tm_hour << ":"
        << std::setfill('0') << std::setw(2) << now_tm.tm_min << ":"
        << std::setfill('0') << std::setw(2) << now_tm.tm_sec << "]";
    
    return oss.str();
}

int main() {
    // Print the core on which this program is running.
    int core = sched_getcpu();
    std::cout << getCurrentTimestamp() << " Client starting on core " << core << ".\n";
    
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    
    // Try connecting until the server is available.
    while (true) {
        std::cout << getCurrentTimestamp() << " Client looking for connection to server...\n";
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << getCurrentTimestamp() << " Socket creation error: " << strerror(errno) << std::endl;
            return 1;
        }
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        
        // Use the loopback address (works offline)
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
            std::cerr << getCurrentTimestamp() << " Invalid address / Address not supported: " << strerror(errno) << std::endl;
            return 1;
        }
        
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << getCurrentTimestamp() << " Connection not found, retrying in 1 second...\n";
            close(sock);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        std::cout << getCurrentTimestamp() << " Connection found with server.\n";
        break; // Connected successfully.
    }
    
    // Send initial ping to confirm connection
    const char *initial_msg = "ping";
    send(sock, initial_msg, strlen(initial_msg), 0);
    std::cout << getCurrentTimestamp() << " Sent initial ping to server.\n";
    
    // Wait for acknowledgment
    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        std::cerr << getCurrentTimestamp() << " Server disconnected or error occurred.\n";
        close(sock);
        return 1;
    }
    buffer[valread] = '\0';
    std::cout << getCurrentTimestamp() << " Received acknowledgment: " << buffer << "\n";
    
    // Now run the intensive computation
    std::cout << getCurrentTimestamp() << " Starting intensive computation...\n";
    
    int cycleCount = 0;
    volatile long dummy = 0;
    for (int i = 0; i < 1000000; ++i) {
        dummy += i;
        
        // Report progress periodically
        if (i > 0 && i % 100000 == 0) {
            cycleCount++;
            std::cout << getCurrentTimestamp() << " Client: Completed " << cycleCount << "0% of intensive work.\n";
        }
    }
    
    // Wait for final message from server after its intensive work
    std::cout << getCurrentTimestamp() << " Waiting for server to complete...\n";
    valread = recv(sock, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        std::cerr << getCurrentTimestamp() << " Server disconnected or error occurred.\n";
        close(sock);
        return 1;
    }
    buffer[valread] = '\0';
    std::cout << getCurrentTimestamp() << " Received final message from server: " << buffer << "\n";
    
    std::cout << getCurrentTimestamp() << " Client completed.\n";
    
    close(sock);
    return 0;
}
