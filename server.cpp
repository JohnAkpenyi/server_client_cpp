#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sched.h>  // For sched_getcpu()

#define PORT 12345
#define BUFFER_SIZE 16

int main() {
    // Print the core on which this program is running.
    int core = sched_getcpu();
    std::cout << "Server starting on core " << core << ".\n";

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return 1;
    }
    
    // Allow address reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return 1;
    }
    
    // Bind the socket to all network interfaces on PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return 1;
    }
    
    std::cout << "Server waiting for connection on port " << PORT << "...\n";
    std::cout << "Server looking for connection from client...\n";
    
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        return 1;
    }
    
    std::cout << "Connection found with client.\n";
    
    int cycleCount = 0;
    while (true) {
        // Wait to receive a "ping" message from the client.
        int valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            std::cerr << "Client disconnected or error occurred.\n";
            break;
        }
        buffer[valread] = '\0';
        // (For this protocol, we expect "ping".)

        // Simulate intensive work (busy loop).
        volatile long dummy = 0;
        for (int i = 0; i < 1000000; ++i) {
            dummy += i;
        }
        
        cycleCount++;
        if (cycleCount % 100 == 0) {
            std::cout << "Server: Completed " << cycleCount << " cycles.\n";
        }
        
        // Reply with "pong".
        const char *msg = "pong";
        send(client_socket, msg, strlen(msg), 0);
    }
    
    close(client_socket);
    close(server_fd);
    return 0;
}
