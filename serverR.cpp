#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

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

// Function to check if a file exists for a username
bool fileExists(const string &username, const string &filename) {
    vector<string> userFiles = getFilesForUser(username);
    return find(userFiles.begin(), userFiles.end(), filename) != userFiles.end();
}

// New function to remove a file for a specific user
bool removeFileForUser(const string &username, const string &filename) {
    // Read the entire file
    ifstream inputFile(FILENAME_FILE);
    vector<string> lines;
    string line;
    bool fileRemoved = false;

    while (getline(inputFile, line)) {
        istringstream iss(line);
        string user, existingFilename;
        if (iss >> user >> existingFilename) {
            // Keep all lines except the one matching username and filename
            if (!(user == username && existingFilename == filename)) {
                lines.push_back(line);
            } else {
                fileRemoved = true;
            }
        }
    }
    inputFile.close();

    // Rewrite the file without the removed entry
    ofstream outputFile(FILENAME_FILE);
    for (const string &l : lines) {
        outputFile << l << endl;
    }
    outputFile.close();

    return fileRemoved;
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

    // Add socket reuse option
    int optval = 1;
    setsockopt(udpSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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

        // Existing lookup handling remains the same...
        if (request.find("lookup") != string::npos) {
            // [Previous lookup code remains unchanged]
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
        // Existing push handling remains the same...
        else if (request.substr(0, 4) == "push") {
            // [Previous push code remains unchanged]
            // Parse push command
            istringstream iss(request);
            string pushCmd, username, filename;
            iss >> pushCmd >> username >> filename;

            // Check if file already exists for this user
            bool exists = fileExists(username, filename);

            if (exists) {
                // Send FILE_EXISTS back to ServerM
                string response = "FILE_EXISTS";
                sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);

                // Wait for overwrite decision from ServerM
                memset(buffer, 0, BUFFER_SIZE);
                recvfrom(udpSock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientLen);
                string overwriteDecision(buffer);

                if (overwriteDecision == "OVERWRITE_YES") {
                    // If overwrite is allowed
                    string response = "File push successful.";
                    sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
                } else {
                    string response = "File push canceled.";
                    sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
                }
            } else {
                // Add file to user's repository (add entry to filenames.txt)
                ofstream file(FILENAME_FILE, ios::app);
                file << username << " " << filename << endl;
                file.close();

                string response = "FILE_ADDED";
                sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
            }

        }
        // New remove handling
        else if (request.substr(0, 6) == "remove") {
            // Parse remove command
            istringstream iss(request);
            string removeCmd, username, filename;
            iss >> removeCmd >> username >> filename;

            // Check if file exists for this user
            bool exists = fileExists(username, filename);

            if (exists) {
                // Attempt to remove the file
                bool removed = removeFileForUser(username, filename);
                
                if (removed) {
                    string response = "FILE_REMOVED";
                    sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
                } else {
                    string response = "REMOVE_FAILED";
                    sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
                }
            } else {
                // File not found for this user
                string response = "FILE_NOT_FOUND";
                sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
            }
        }
    }

    close(udpSock);
    return 0;
}