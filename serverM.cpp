#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_M_TCP 25985
#define PORT_A_UDP 21985
#define PORT_R_UDP 22985  // Server R's UDP port
#define BUFFER_SIZE 1024
using namespace std;

int main() {
    int serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientLen = sizeof(clientAddr);

    // Create TCP socket for communication with the client
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_M_TCP);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind and listen
    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSock);
        return -1;
    }
    listen(serverSock, 5);

    std::cout << "Server M: Main Server is up and running using TCP on port " << PORT_M_TCP << std::endl;

    while (true) {
        // Accept client connection
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSock < 0) {
            perror("Connection failed");
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        read(clientSock, buffer, BUFFER_SIZE);

        // Extract username and mask the password
        string credentials(buffer);
        size_t spacePos = credentials.find(" ");
        string username = credentials.substr(0, spacePos);
        string password = credentials.substr(spacePos + 1);

        // Print formatted message when receiving a lookup request
        std::cout << "The main server has received a lookup request from <" << username << "> to lookup <" << username << "'s repository> using TCP over port " << PORT_M_TCP << std::endl;

        // Check if the command is "lookup" and forward it to ServerR
        if (credentials.substr(0, 6) == "lookup") {
            // Forward request to ServerR via UDP
            int udpSockR;
            struct sockaddr_in serverRAddr;
            udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpSockR < 0) {
                perror("UDP socket creation failed for Server R");
                close(clientSock);
                continue;
            }

            serverRAddr.sin_family = AF_INET;
            serverRAddr.sin_port = htons(PORT_R_UDP);
            serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server R's address

            sendto(udpSockR, buffer, strlen(buffer), 0, (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));

            // Print message when sending the request to ServerR
            std::cout << "The main server has sent the lookup request to serverR." << std::endl;

            // Receive response from ServerR
            memset(buffer, 0, BUFFER_SIZE);
            socklen_t serverRLen = sizeof(serverRAddr);
            recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

            // Print message when response is received from ServerR
            std::cout << "Response received from R: " << buffer << std::endl;

            // Send response back to the client
            write(clientSock, buffer, strlen(buffer));

            close(udpSockR);
        } else {
            // Forward request to ServerA via UDP (default case)
            int udpSockA;
            struct sockaddr_in serverAAddr;
            udpSockA = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpSockA < 0) {
                perror("UDP socket creation failed for Server A");
                close(clientSock);
                continue;
            }

            serverAAddr.sin_family = AF_INET;
            serverAAddr.sin_port = htons(PORT_A_UDP);
            serverAAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server A's address

            sendto(udpSockA, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAAddr, sizeof(serverAAddr));

            // Receive response from ServerA
            memset(buffer, 0, BUFFER_SIZE);
            socklen_t serverALen = sizeof(serverAAddr);
            recvfrom(udpSockA, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAAddr, &serverALen);

            std::cout << "Server M received response from Server A: " << buffer << std::endl;

            // Send response back to the client
            write(clientSock, buffer, strlen(buffer));

            close(udpSockA);
        }

        close(clientSock);
    }

    close(serverSock);
    return 0;
}
