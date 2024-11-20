#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

#define PORT_R_UDP 22985  // ServerR listens on port 22985
#define BUFFER_SIZE 1024
#define FILENAME_FILE "filenames.txt" // The filename to check for usernames
using namespace std;

// Function to search for files by username in filenames.txt
vector<string> getFilesForUser(const string &username) {
    vector<string> files;
    ifstream file(FILENAME_FILE);
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string user, filename;
        if (iss >> user >> filename) {
            if (user == username) {
                files.push_back(filename);
            }
        }
    }
    return files;
}

int main() {
    int udpSock;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];
    socklen_t clientLen = sizeof(clientAddr);

    // Create UDP socket
    udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        perror("UDP socket creation failed");
        return -1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_R_UDP);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the UDP socket
    if (bind(udpSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(udpSock);
        return -1;
    }

    cout << "Server R: Listening for requests on UDP port " << PORT_R_UDP << endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);

        // Receive the request from ServerM
        ssize_t len = recvfrom(udpSock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientLen);
        if (len < 0) {
            perror("Failed to receive data");
            continue;
        }

        string request(buffer);
        cout << "Server R received request: " << request << endl;

        // Check if the request is a lookup and contains a username
        if (request.find("lookup") != string::npos) {
            stringstream ss(request);
            string command, username;
            ss >> command >> username;

            // Get the files associated with the username
            vector<string> files = getFilesForUser(username);
            
            // Prepare response
            string response;
            if (files.empty()) {
                response = "Username not found!";
            } else {
                response = "Files for " + username + ":\n";
                for (const string &file : files) {
                    response += file + "\n";
                }
            }

            // Send the response back to ServerM
            ssize_t sent = sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
            if (sent < 0) {
                perror("Failed to send response");
            } else {
                cout << "Server R sent response: " << response << endl;
            }
        }
    }

    close(udpSock);
    return 0;
}
