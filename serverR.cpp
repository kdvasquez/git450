#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define UDP_PORT 22985

void handleLookup(const std::string& username, int sockfd, struct sockaddr_in& clientAddr) {
    std::ifstream file("filenames.txt"); // Ensure this matches your actual file name
    std::string line, response;
    bool found = false;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string fileUsername, filename;

        if (iss >> fileUsername >> filename) {
            if (fileUsername == username) {
                response += filename + "\n"; // Collect all matching filenames
                found = true;
            }
        }
    }

    if (!found) {
        response = "Username not found!";
    }

    // Send the response back to the client
    sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientAddrLen = sizeof(clientAddr);

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(UDP_PORT);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "ServerR is running and listening on port " << UDP_PORT << std::endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);

        // Receive a command from the client
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0'; // Null-terminate the received string
        std::cout << "Received command: " << buffer << std::endl;

        // Parse the command
        std::string command(buffer);
        if (command.substr(0, 6) == "lookup") {
            std::string username = command.substr(7); // Extract username
            handleLookup(username, sockfd, clientAddr);
        } else {
            std::string response = "Invalid command!";
            sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr*)&clientAddr, clientAddrLen);
        }
    }

    close(sockfd);
    return 0;
}
