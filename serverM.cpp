#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h> // for inet_pton

#define LOCALHOST "127.0.0.1"  // Define localhost as a global constant
#define UDP_PORT 24985         // UDP port for serverM
#define TCP_PORT 25985         // TCP port for serverM


using namespace std;

// Function to forward a message to another server
int forwardToServer(int server_port, const char *message) {
    int sock;
    struct sockaddr_in serverAddr;
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error creating socket for redirection!" << endl;
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_port);

    // Convert IP address to binary form using LOCALHOST
    if (inet_pton(AF_INET, LOCALHOST, &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        close(sock);
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection to server failed!" << endl;
        close(sock);
        return -1;
    }

    // Send message to the forwarded server
    send(sock, message, strlen(message), 0);

    // Receive the response
    char buffer[1024] = {0};
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Received from forwarded server: " << buffer << endl;
    }

    // Close the socket after communication
    close(sock);
    return 0;
}

int main() {
    // --- UDP Setup ---
    int udpSocket;
    struct sockaddr_in udpAddress;
    int udpOpt = 1;
    socklen_t addrLen = sizeof(udpAddress);

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        cerr << "Failed to create UDP socket!" << endl;
        return -1;
    }
    cout << "UDP socket created on port " << UDP_PORT << endl;

    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &udpOpt, sizeof(udpOpt)) < 0) {
        cerr << "Failed to set UDP socket options!" << endl;
        close(udpSocket);
        return -1;
    }

    udpAddress.sin_family = AF_INET;
    udpAddress.sin_addr.s_addr = INADDR_ANY;
    udpAddress.sin_port = htons(UDP_PORT);

    if (bind(udpSocket, (struct sockaddr *)&udpAddress, sizeof(udpAddress)) < 0) {
        cerr << "Failed to bind UDP socket!" << endl;
        close(udpSocket);
        return -1;
    }
    cout << "Server M is up and running using UDP on port " << UDP_PORT << endl;

    // --- TCP Setup ---
    int tcpSocket, newTcpSocket;
    struct sockaddr_in tcpAddress;
    int tcpOpt = 1;

    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        cerr << "Failed to create TCP socket!" << endl;
        close(udpSocket);
        return -1;
    }
    cout << "TCP socket created on port " << TCP_PORT << endl;

    if (setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &tcpOpt, sizeof(tcpOpt)) < 0) {
        cerr << "Failed to set TCP socket options!" << endl;
        close(tcpSocket);
        close(udpSocket);
        return -1;
    }

    tcpAddress.sin_family = AF_INET;
    tcpAddress.sin_addr.s_addr = INADDR_ANY;
    tcpAddress.sin_port = htons(TCP_PORT);

    if (bind(tcpSocket, (struct sockaddr *)&tcpAddress, sizeof(tcpAddress)) < 0) {
        cerr << "Failed to bind TCP socket!" << endl;
        close(tcpSocket);
        close(udpSocket);
        return -1;
    }
    cout << "TCP server bound to port " << TCP_PORT << endl;

    if (listen(tcpSocket, 3) < 0) {
        cerr << "Failed to listen on TCP socket!" << endl;
        close(tcpSocket);
        close(udpSocket);
        return -1;
    }
    cout << "TCP server is listening on port " << TCP_PORT << endl;

    // Main loop to handle incoming connections and datagrams
    char buffer[1024];
    while (true) {
        // --- Handle UDP Requests ---
        struct sockaddr_in clientAddress;
        int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&clientAddress, &addrLen);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            cout << "Received UDP message: " << buffer << endl;

            // Forward the message to serverA (port 21985)
            // Existing behavior for serverA
            forwardToServer(21985, buffer);  // Forwarding to serverA

            // Forward the message to serverR (port 22985)
            forwardToServer(22985, buffer);  // Forwarding to serverR
        }

        // --- Handle TCP Connections ---
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(tcpSocket, &readfds);
        struct timeval tv = {0, 100000}; // 100ms timeout
        int activity = select(tcpSocket + 1, &readfds, NULL, NULL, &tv);

        if (activity > 0 && FD_ISSET(tcpSocket, &readfds)) {
            newTcpSocket = accept(tcpSocket, (struct sockaddr *)&tcpAddress, &addrLen);
            if (newTcpSocket >= 0) {
                cout << "Accepted TCP connection" << endl;

                // Read from TCP client
                int bytesRead = read(newTcpSocket, buffer, sizeof(buffer) - 1);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    cout << "Received TCP message: " << buffer << endl;

                    // Forward the message to serverA (port 21985)
                    // Existing behavior for serverA
                    forwardToServer(21985, buffer);  // Forwarding to serverA

                    // Forward the message to serverR (port 22985)
                    forwardToServer(22985, buffer);  // Forwarding to serverR
                }
                close(newTcpSocket); // Close the connection after responding
            }
        }
    }

    // Close both sockets when done (won't actually reach here in this infinite loop)
    close(udpSocket);
    close(tcpSocket);

    return 0;
}
