#include <iostream>
#include <fstream>
#include <cstring>
#include <arpa/inet.h> // for the local host
#include <unistd.h>
using namespace std;

#define PORT_A_UDP 21985
#define BUFFER_SIZE 1024
#define HOST_NAME "127.0.0.1"

// Encryption function (must match client encryption)
string encryptPassword(const std::string &password) {
    string encrypted;
    for (char c : password) {
        if (std::isalpha(c)) {
            char base = std::islower(c) ? 'a' : 'A';
            encrypted += (c - base + 3) % 26 + base;
        } else if (std::isdigit(c)) {
            encrypted += (c - '0' + 3) % 10 + '0';
        } else {
            encrypted += c;
        }
    }
    return encrypted;
}

// Function to authenticate username and encrypted password
bool authenticate(const std::string &username, const std::string &encryptedPassword) {
    ifstream file("members.txt");
    if (!file.is_open()) {
        cerr << "Error opening members.txt file!" << endl;
        return false;
    }

    string line;
    while (getline(file, line)) {
        // Each line contains "username encryptedPassword"
        size_t spacePos = line.find(" ");
        string storedUsername = line.substr(0, spacePos);
        string storedEncryptedPassword = line.substr(spacePos + 1);

        if (storedUsername == username && storedEncryptedPassword == encryptedPassword) {
            return true; // Credentials match!
        }
    }

    return false; // Credentials don't match!
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientLen = sizeof(clientAddr);

    // Socket/Binding Creation - Courtesy of GeeksforGeeks
    // Create A's UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure serverA's address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_A_UDP);
    serverAddr.sin_addr.s_addr = inet_addr(HOST_NAME); // Hardcode localhost 121.0.0.1


    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    cout << "Server A is up and running using UDP on port " << PORT_A_UDP << std::endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);

        // Receive data from serverM
        int bytesReceived = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                     (struct sockaddr *)&clientAddr, &clientLen);
        if (bytesReceived < 0) {
            cerr << "Failed to receive data!" << endl;
            continue;
        }

        // Parse username and encrypted password
        string credentials(buffer);
        size_t spacePos = credentials.find(" ");
        if (spacePos == string::npos) {
            cerr << "Invalid message format received by ServerA!" << endl;
            continue;
        }

        string username = credentials.substr(0, spacePos);
        string encryptedPassword = credentials.substr(spacePos + 1);

        cout << "ServerA received username " << username << " and password ******" << endl;
        // Authenticate user
        bool isAuthenticated = authenticate(username, encryptedPassword);

        // Prepare response to send back to serverM
        const char *response = isAuthenticated ? "AUTH_SUCCESS" : "AUTH_FAILURE";

        if (isAuthenticated) {
            cout << "Member " << username << " has been authenticated" << endl;
        } else {
            cout << "The username " << username << " or password ****** is incorrect" << endl;
        }


        // Send response back to serverM
        sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&clientAddr, clientLen);
    }

    close(sockfd);
    return 0;
}