#include <winsock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

#define SERVER_PORT 5005            // The port to listen on (must match server's port)
#define SERVER_IP "192.168.10.75"         // Bind to all available network interfaces

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    int addr_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return -1;
    }   

    // Create socket (UDP)
    sock = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return -1;
    }

    // Set up the server address structure (bind to any available IP address)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);  // The port to listen on
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // 0.0.0.0 listens on all available network interfaces

    // Bind the socket to the specified IP address and port
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::cout << "Listening for messages on " << SERVER_IP << ":" << SERVER_PORT << "...\n";

    // Receive message from the server (listener)
    while (true) {
        int n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n == SOCKET_ERROR) {
            std::cerr << "Failed to receive response!" << std::endl;
            closesocket(sock);
            WSACleanup();
            return -1;
        }
        buffer[n] = '\0';  // Null-terminate the received string
        std::cout << "Received message from server: " << buffer << std::endl;
    }

    // Close socket and clean up Winsock
    closesocket(sock);
    WSACleanup();

    return 0;
}
