#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <string>

#define PORT_M_TCP 25985
#define PORT_A_UDP 21985
#define PORT_R_UDP 22985  // Server R's UDP port
#define BUFFER_SIZE 1024
using namespace std;

int main() {
    int serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    struct sockaddr_in serverAAddr, serverRAddr;
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

        // First, handle authentication
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t authBytes = read(clientSock, buffer, BUFFER_SIZE);
        if (authBytes <= 0) {
            close(clientSock);
            continue;
        }

        string authRequest(buffer);
        size_t spacePos = authRequest.find(" ");

        // Authentication request
        if (spacePos != string::npos) {
            string username = authRequest.substr(0, spacePos);
            string password = authRequest.substr(spacePos + 1);

            std::cout << "The main server has received a login request from <" 
                      << username << "> using TCP over port " << PORT_M_TCP << std::endl;

            // Create UDP socket for authentication
            int udpSockA = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpSockA < 0) {
                perror("UDP socket creation failed for Server A");
                close(clientSock);
                continue;
            }

            serverAAddr.sin_family = AF_INET;
            serverAAddr.sin_port = htons(PORT_A_UDP);
            serverAAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

            // Forward authentication request to ServerA
            sendto(udpSockA, buffer, strlen(buffer), 0, 
                   (struct sockaddr *)&serverAAddr, sizeof(serverAAddr));

            // Receive response from ServerA
            memset(buffer, 0, BUFFER_SIZE);
            socklen_t serverALen = sizeof(serverAAddr);
            recvfrom(udpSockA, buffer, BUFFER_SIZE, 0, 
                     (struct sockaddr *)&serverAAddr, &serverALen);

            std::cout << "Server M received response from Server A: " << buffer << std::endl;

            // Send authentication result back to client
            write(clientSock, buffer, strlen(buffer));

            close(udpSockA);

            // If authentication failed, close connection
            if (string(buffer) != "AUTH_SUCCESS") {
                close(clientSock);
                continue;
            }
        }

        // Now handle subsequent commands
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t cmdBytes = read(clientSock, buffer, BUFFER_SIZE);
        if (cmdBytes <= 0) {
            close(clientSock);
            continue;
        }

        string command(buffer);
        std::cout << "ServerM received command: [" << command << "]" << std::endl;

        // Check if the command is "lookup"
        if (command.substr(0, 6) == "lookup") {
            // Forward request to ServerR via UDP
            int udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpSockR < 0) {
                perror("UDP socket creation failed for Server R");
                close(clientSock);
                continue;
            }

            serverRAddr.sin_family = AF_INET;
            serverRAddr.sin_port = htons(PORT_R_UDP);
            serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

            // Extract username for lookup
            istringstream iss(command);
            string lookupCmd, username;
            iss >> lookupCmd >> username;

            std::cout << "The main server has received a lookup request from <" 
                      << username << "> to lookup <" << username << "'s repository> using TCP over port " 
                      << PORT_M_TCP << std::endl;

            // Send lookup request to ServerR
            sendto(udpSockR, command.c_str(), command.length(), 0, 
                   (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));

            std::cout << "The main server has sent the lookup request to serverR." << std::endl;

            // Receive response from ServerR
            memset(buffer, 0, BUFFER_SIZE);
            socklen_t serverRLen = sizeof(serverRAddr);
            recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, 
                     (struct sockaddr *)&serverRAddr, &serverRLen);

            std::cout << "Response received from R: " << buffer << std::endl;

            // Send response back to the client
            write(clientSock, buffer, strlen(buffer));

            close(udpSockR);
        }

        close(clientSock);
    }

    close(serverSock);
    return 0;
}