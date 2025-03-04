#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sched.h>  // For sched_getcpu()
#include <chrono>   // For getting current time
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
    std::cout << getCurrentTimestamp() << " Server starting on core " << core << ".\n";
    
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << getCurrentTimestamp() << " socket failed: " << strerror(errno) << std::endl;
        return 1;
    }
    
    // Allow address reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << getCurrentTimestamp() << " setsockopt: " << strerror(errno) << std::endl;
        return 1;
    }
    
    // Bind the socket to all network interfaces on PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << getCurrentTimestamp() << " bind failed: " << strerror(errno) << std::endl;
        return 1;
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << getCurrentTimestamp() << " listen: " << strerror(errno) << std::endl;
        return 1;
    }
    
    std::cout << getCurrentTimestamp() << " Server waiting for connection on port " << PORT << "...\n";
    std::cout << getCurrentTimestamp() << " Server looking for connection from client...\n";
    
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        std::cerr << getCurrentTimestamp() << " accept: " << strerror(errno) << std::endl;
        return 1;
    }
    
    std::cout << getCurrentTimestamp() << " Connection found with client.\n";
    
    // Receive a single message to confirm connection
    int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        std::cerr << getCurrentTimestamp() << " Client disconnected or error occurred.\n";
        close(client_socket);
        close(server_fd);
        return 1;
    }
    buffer[valread] = '\0';
    
    // Send acknowledgment
    const char *ack_msg = "connected";
    send(client_socket, ack_msg, strlen(ack_msg), 0);
    
    std::cout << getCurrentTimestamp() << " Starting intensive computation...\n";
    
    // Now run the intensive loop outside the connection handling loop
    int cycleCount = 0;
    volatile long dummy = 0;
    for (int i = 0; i < 1000000; ++i) {
        dummy += i;
        
        // Report progress periodically
        if (i > 0 && i % 100000 == 0) {
            cycleCount++;
            std::cout << getCurrentTimestamp() << " Server: Completed " << cycleCount << "0% of intensive work.\n";
        }
    }
    
    // Send final message
    const char *final_msg = "pong";
    send(client_socket, final_msg, strlen(final_msg), 0);
    
    std::cout << getCurrentTimestamp() << " Server completed.\n";
    
    close(client_socket);
    close(server_fd);
    return 0;
}
