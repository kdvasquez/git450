#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <algorithm>

#define PORT_M_TCP 25985
#define PORT_A_UDP 21985
#define PORT_R_UDP 22985
#define BUFFER_SIZE 1024

using namespace std;

int main() {
    int serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr, serverAAddr, serverRAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientLen = sizeof(clientAddr);
    string currentAuthenticatedUsername;

    // Create TCP socket for serverM
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Allow socket reuse to prevent "Address already in use" error
    int optval = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_M_TCP);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(serverSock);
        return -1;
    }

    listen(serverSock, 5);
    cout << "Server M is up and running on TCP port " << PORT_M_TCP << endl;

    while (true) {
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSock < 0) {
            perror("Connection failed");
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t authBytes = read(clientSock, buffer, BUFFER_SIZE);
        if (authBytes <= 0) {
            close(clientSock);
            continue;
        }

        string authRequest(buffer);
        size_t spacePos = authRequest.find(" ");

        if (spacePos != string::npos) {
            string username = authRequest.substr(0, spacePos);
            string password = authRequest.substr(spacePos + 1);

            // Store the authenticated username
            currentAuthenticatedUsername = username;

            cout << "Received login request from <" << username << "> on port " << PORT_M_TCP << endl;

            // Create UDP socket for serverA
            int udpSockA = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpSockA < 0) {
                perror("UDP socket creation failed for Server A");
                close(clientSock);
                continue;
            }

            serverAAddr.sin_family = AF_INET;
            serverAAddr.sin_port = htons(PORT_A_UDP);
            serverAAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

            sendto(udpSockA, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAAddr, sizeof(serverAAddr));

            memset(buffer, 0, BUFFER_SIZE);
            socklen_t serverALen = sizeof(serverAAddr);
            recvfrom(udpSockA, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAAddr, &serverALen);

            cout << "ServerM received authentication response: " << buffer << endl;
            write(clientSock, buffer, strlen(buffer));
            close(udpSockA);

            if (string(buffer) != "AUTH_SUCCESS") {
                close(clientSock);
                continue;
            }
        }

        // Multiple command handling loop
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t cmdBytes = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
            if (cmdBytes <= 0) {
                // Client disconnected or error occurred
                cout << "Client disconnected or error occurred" << endl;
                break;
            }

            // Null-terminate the buffer to ensure it's a proper string
            buffer[cmdBytes] = '\0';

            string command(buffer);
            // Trim whitespace
            command.erase(0, command.find_first_not_of(" \n\r\t"));
            command.erase(command.find_last_not_of(" \n\r\t") + 1);

            cout << "DEBUG: Received command (length " << command.length() << "): [" << command << "]" << endl;

            if (command.substr(0, 6) == "lookup") {
                int udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
                if (udpSockR < 0) {
                    perror("UDP socket creation failed for Server R");
                    continue;
                }

                serverRAddr.sin_family = AF_INET;
                serverRAddr.sin_port = htons(PORT_R_UDP);
                serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

                sendto(udpSockR, command.c_str(), command.length(), 0, (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));
                cout << "Forwarded lookup request to serverR" << endl;

                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverRLen = sizeof(serverRAddr);
                recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

                cout << "Response from serverR: " << buffer << endl;
                write(clientSock, buffer, strlen(buffer));
                close(udpSockR);
            } 
            else if (command.substr(0, 4) == "push") {
                cout << "DEBUG: Full push command received: [" << command << "]" << endl;
                cout << "DEBUG: Current authenticated username: [" << currentAuthenticatedUsername << "]" << endl;
                
                // Extract the filename from the push command
                string filename = command.substr(5);
                cout << "DEBUG: Extracted filename: [" << filename << "]" << endl;
                
                int udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
                if (udpSockR < 0) {
                    perror("UDP socket creation failed for Server R");
                    continue;
                }

                serverRAddr.sin_family = AF_INET;
                serverRAddr.sin_port = htons(PORT_R_UDP);
                serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

                // Modify the command to include the username
                string fullPushCommand = "push " + currentAuthenticatedUsername + " " + filename;
                cout << "DEBUG: Full push command sent to ServerR: [" << fullPushCommand << "]" << endl;

                sendto(udpSockR, fullPushCommand.c_str(), fullPushCommand.length(), 0, 
                       (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));

                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverRLen = sizeof(serverRAddr);
                recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

                cout << "Response from serverR: " << buffer << endl;
                write(clientSock, buffer, strlen(buffer));
                close(udpSockR);
            }
            else {
                cout << "DEBUG: Unrecognized command: [" << command << "]" << endl;
            }
        }

        // Close the client socket after processing all commands
        close(clientSock);
    }

    close(serverSock);
    return 0;
}