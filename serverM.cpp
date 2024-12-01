#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
using namespace std;

#define PORT_M_UDP 24985
#define PORT_M_TCP 25985
#define PORT_A_UDP 21985
#define PORT_R_UDP 22985
#define BUFFER_SIZE 1024

int main() {
    int udpSockM, tcpSock, clientSock;
    struct sockaddr_in serverUDPAddr, clientAddr, serverAAddr, serverRAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientLen = sizeof(clientAddr);
    string currentAuthenticatedUsername;

    // Create serverM's UDP socket
    udpSockM = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSockM < 0) {
        perror("UDP socket creation failed");
        return -1;
    }

    // Create serverM's TCP socket for client communication
    tcpSock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSock < 0) {
        perror("TCP socket creation failed");
        close(udpSockM);
        return -1;
    }

    //This supposedly prevents "addres already in use error" but may not need it
    //int optval = 1;
    //setsockopt(udpSockM, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    //setsockopt(tcpSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Set up M's UDP socket address
    serverUDPAddr.sin_family = AF_INET;
    serverUDPAddr.sin_port = htons(PORT_M_UDP);
    serverUDPAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind UDP socket
    if (bind(udpSockM, (struct sockaddr *)&serverUDPAddr, sizeof(serverUDPAddr)) < 0) {
        perror("UDP Bind failed");
        close(udpSockM);
        close(tcpSock);
        return -1;
    }

    // Set up TCP socket address for client communication
    struct sockaddr_in serverTCPAddr;
    serverTCPAddr.sin_family = AF_INET;
    serverTCPAddr.sin_port = htons(PORT_M_TCP);
    serverTCPAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind TCP socket
    if (bind(tcpSock, (struct sockaddr *)&serverTCPAddr, sizeof(serverTCPAddr)) < 0) {
        perror("TCP Bind failed");
        close(udpSockM);
        close(tcpSock);
        return -1;
    }

    // Listen on TCP socket
    listen(tcpSock, 5);
    
    cout << "Server M is up and running using UDP on port " << PORT_M_UDP << endl;

    while (true) {
        // Accept TCP connection
        clientSock = accept(tcpSock, (struct sockaddr *)&clientAddr, &clientLen);
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

            cout << "Server M has received username " << username << " and password ****." << endl;

            // Create serverA's UDP socket 
            int udpSockA = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpSockA < 0) {
                perror("UDP socket creation failed for Server A");
                close(clientSock);
                continue;
            }

            serverAAddr.sin_family = AF_INET;
            serverAAddr.sin_port = htons(PORT_A_UDP);
            serverAAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

            cout << "Server M has sent authentication request to Server A " << endl;
            sendto(udpSockA, buffer, strlen(buffer), 0, (struct sockaddr *)&serverAAddr, sizeof(serverAAddr));

            memset(buffer, 0, BUFFER_SIZE);
            socklen_t serverALen = sizeof(serverAAddr);
            recvfrom(udpSockA, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverAAddr, &serverALen);

            cout << "The main server has received the response from Server A using UDP over " << PORT_M_UDP << endl; //":" << buffer << endl;
            write(clientSock, buffer, strlen(buffer));
            cout << "The main server has sent the response from Server A to client using TCP over port " << PORT_M_TCP << "." << endl;
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

            //cout << "DEBUG: Received command (length " << command.length() << "): [" << command << "]" << endl;

            if (command.substr(0, 6) == "lookup") {
                if(currentAuthenticatedUsername == "guest"){
                    cout << "The main server has received a lookup request from Guest to lookup a repository using TCP over port 25985" << endl;
                }
                cout << "The main server has received a lookup request from " << currentAuthenticatedUsername << "to lookup " << currentAuthenticatedUsername << "'s repository using TCP over port 25985. " << endl;
                int udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
                if (udpSockR < 0) {
                    perror("UDP socket creation failed for Server R");
                    continue;
                }

                serverRAddr.sin_family = AF_INET;
                serverRAddr.sin_port = htons(PORT_R_UDP);
                serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
                // Forward lookup request to serverR
                sendto(udpSockR, command.c_str(), command.length(), 0, (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));
                cout << "The main server has sent the lookup request to server R" << endl;

                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverRLen = sizeof(serverRAddr);
                recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

                cout << "The main server has received the response from server R using UDP over 24985" << endl; //<< buffer << endl;
                write(clientSock, buffer, strlen(buffer));
                cout << "The main server has sent the response to client. " << endl;
                close(udpSockR);
            } 
            else if (command.substr(0, 4) == "push") {
                cout << "The main server has received a push request from" << currentAuthenticatedUsername << " using TCP over port 25985." << endl;
                //cout << "DEBUG: Full push command received: [" << command << "]" << endl;
                //cout << "DEBUG: Current authenticated username: [" << currentAuthenticatedUsername << "]" << endl;
                
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
                cout << "The main server has sent the push request to ServerR" << endl; // [" << fullPushCommand << "]" << endl;

                sendto(udpSockR, fullPushCommand.c_str(), fullPushCommand.length(), 0, 
                       (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));

                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverRLen = sizeof(serverRAddr);
                recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

                cout << "The main server has received the response from server R using UDP over port 24985 " << endl; // << buffer << endl;
                write(clientSock, buffer, strlen(buffer));
                cout << "The main server has sent the response to the client. " << endl;
                close(udpSockR);
            }
            else if (command.substr(0, 6) == "remove") {
                cout << "The main server has received a remove request from member " << currentAuthenticatedUsername << " TCP over port 25985." << endl;
                //cout << "DEBUG: Full remove command received: [" << command << "]" << endl;
                //cout << "DEBUG: Current authenticated username: [" << currentAuthenticatedUsername << "]" << endl;
                
                // Check if filename is specified
                if (command.length() <= 7) {  // "remove " is 7 characters
                    string errorMsg = "Filename is not specified.\nPlease enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>.";
                    write(clientSock, errorMsg.c_str(), errorMsg.length());
                    continue;
                }
                
                // Extract the filename from the remove command
                string filename = command.substr(7);
                //cout << "DEBUG: Extracted filename: [" << filename << "]" << endl;
                
                int udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
                if (udpSockR < 0) {
                    perror("UDP socket creation failed for Server R");
                    continue;
                }

                serverRAddr.sin_family = AF_INET;
                serverRAddr.sin_port = htons(PORT_R_UDP);
                serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

                // Modify the command to include the username
                string fullRemoveCommand = "remove " + currentAuthenticatedUsername + " " + filename;
                //cout << "DEBUG: Full remove command sent to ServerR: [" << fullRemoveCommand << "]" << endl;

                sendto(udpSockR, fullRemoveCommand.c_str(), fullRemoveCommand.length(), 0, 
                       (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));

                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverRLen = sizeof(serverRAddr);
                recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

                //cout << "Response from serverR: " << buffer << endl;
                cout << "The main server has received confirmation of the remove request done by the server R. " << endl;
                write(clientSock, buffer, strlen(buffer));
                close(udpSockR);
            }
            //Added for deploy command
            else if (command.substr(0,6) == "deploy") {
                cout << "The main server has received a deploy request from member " << currentAuthenticatedUsername << " TCP over port 25985 " << endl;

                // Create UDP Socket for serverR
                int udpSockR = socket(AF_INET, SOCK_DGRAM, 0);
                if (udpSockR < 0) {
                    perror("UDP socket creation failed for Server R");
                    continue;
                }

                serverRAddr.sin_family = AF_INET;
                serverRAddr.sin_port = htons(PORT_R_UDP);
                serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

                // Create UDP socket for serverD
                int udpSockD = socket(AF_INET, SOCK_DGRAM, 0);
                if (udpSockD < 0) {
                    perror("UDP socket creation failed for Server D");
                    close(udpSockR);
                    continue;
                }

                struct sockaddr_in serverDAddr;
                serverDAddr.sin_family = AF_INET;
                serverDAddr.sin_port = htons(23985);  // ServerD's port
                serverDAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

                // First, send a lookup request to ServerR to get files for current user
                string deployRequest = "deploy " + currentAuthenticatedUsername;
                sendto(udpSockR, deployRequest.c_str(), deployRequest.length(), 0, 
                    (struct sockaddr *)&serverRAddr, sizeof(serverRAddr));
                cout << "The main server has sent the lookup request to server R" << endl;

                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverRLen = sizeof(serverRAddr);
                recvfrom(udpSockR, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverRAddr, &serverRLen);

                string lookupResponse(buffer);
                cout << "The main server has received the lookup response from Server R" << endl; // << lookupResponse << endl;

                // Check if any files exist for the user
                if (lookupResponse.find("Username not found!") != string::npos) {
                    string errorMsg = "No files found for deployment.";
                    write(clientSock, errorMsg.c_str(), errorMsg.length());
                    close(udpSockR);
                    close(udpSockD);
                    continue;
                }

                // Prepare a list of files to send to ServerD
                vector<string> files;
                istringstream iss(lookupResponse);
                string line;
                getline(iss, line);  // Skip the first line "Files for username:"
                while (getline(iss, line)) {
                    if (!line.empty()) {
                        files.push_back(line);
                    }
                }

            // Send each file to ServerD for deployment
            for (const string& filename : files) {
                string deployMsg = currentAuthenticatedUsername + " " + filename;
                sendto(udpSockD, deployMsg.c_str(), deployMsg.length(), 0, 
                    (struct sockaddr *)&serverDAddr, sizeof(serverDAddr));
                cout << "The main server has sent the deploy request to server D. " << endl;
                // Receive confirmation from ServerD
                memset(buffer, 0, BUFFER_SIZE);
                socklen_t serverDLen = sizeof(serverDAddr);
                recvfrom(udpSockD, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serverDAddr, &serverDLen);
        
                cout << "The user " << currentAuthenticatedUsername << "'s repository has been deployed at server D." << endl; // << filename << ": " << buffer << endl;
            }

            // Prepare and send final response to client
            string successMsg = "Deployment completed. " + to_string(files.size()) + " files deployed.";
            write(clientSock, successMsg.c_str(), successMsg.length());

            // Close sockets
            close(udpSockR);
            close(udpSockD);
        }
            else {
                cout << "DEBUG: Unrecognized command: [" << command << "]" << endl;
                string errorMsg = "Invalid command.\nPlease enter the command: <lookup <username>>, <push <filename>>, <remove <filename>>, <deploy>, <log>.";
                write(clientSock, errorMsg.c_str(), errorMsg.length());
            }
        }

        // Close the client socket after processing all commands
        close(clientSock);
    }

    close(udpSockM);
    close(tcpSock);
    return 0;
}