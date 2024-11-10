#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define UDP_PORT 24985 // Port for UDP server
#define TCP_PORT 25985 // Port for TCP server

using namespace std;

int main() {
    // --- UDP Setup ---
    int udpSocket;
    struct sockaddr_in udpAddress;
    int udpOpt = 1;
    socklen_t addrLen = sizeof(udpAddress);

    // Create UDP socket
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        cerr << "Failed to create UDP socket!" << endl;
        return -1;
    }
    cout << "UDP socket created on port " << UDP_PORT << endl;

    // Set socket options for UDP
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &udpOpt, sizeof(udpOpt)) < 0) {
        cerr << "Failed to set UDP socket options!" << endl;
        close(udpSocket);
        return -1;
    }

    // Define the UDP address and bind it
    udpAddress.sin_family = AF_INET;
    udpAddress.sin_addr.s_addr = INADDR_ANY;
    udpAddress.sin_port = htons(UDP_PORT);

    if (bind(udpSocket, (struct sockaddr *)&udpAddress, sizeof(udpAddress)) < 0) {
        cerr << "Failed to bind UDP socket!" << endl;
        close(udpSocket);
        return -1;
    }
    cout << "Server M is up and running using UDP on port  " << UDP_PORT << endl;

    // --- TCP Setup ---
    int tcpSocket, newTcpSocket;
    struct sockaddr_in tcpAddress;
    int tcpOpt = 1;

    // Create TCP socket
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        cerr << "Failed to create TCP socket!" << endl;
        close(udpSocket); // Close UDP socket on failure
        return -1;
    }
    cout << "TCP socket created on port " << TCP_PORT << endl;

    // Set socket options for TCP
    if (setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &tcpOpt, sizeof(tcpOpt)) < 0) {
        cerr << "Failed to set TCP socket options!" << endl;
        close(tcpSocket);
        close(udpSocket); // Close UDP socket on failure
        return -1;
    }

    // Define the TCP address and bind it
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

    // Listen on TCP socket for incoming connections
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

            // Send a response (optional)
            const char *udpResponse = "Hello from UDP server!";
            sendto(udpSocket, udpResponse, strlen(udpResponse), 0, (struct sockaddr *)&clientAddress, addrLen);
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

                    // Respond to TCP client
                    const char *tcpResponse = "Hello from TCP server!";
                    send(newTcpSocket, tcpResponse, strlen(tcpResponse), 0);
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
