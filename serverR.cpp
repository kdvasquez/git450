#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <ctime>

#define PORT_R_UDP 22985  
#define BUFFER_SIZE 1024
#define HOST_NAME "127.0.0.1"
#define FILENAME_FILE "filenames.txt" 
#define LOG_FILE "user_log.txt"

using namespace std;

// Function to keep track of log
void addLogEntry(const string &username, const string &action) {
    ofstream logFile(LOG_FILE, ios::app);
    time_t now = time(0);
    char* dt = ctime(&now);
    string timestamp(dt);
    // Remove newline from timestamp
    timestamp.erase(timestamp.length() - 1);
    
    logFile << username << " " << action << " " << timestamp << endl;
    logFile.close();
}

// Function to retrieve log entries for a user
vector<string> getUserLogEntries(const string &username) {
    vector<string> logEntries;
    ifstream logFile(LOG_FILE);
    string line;
    
    while (getline(logFile, line)) {
        istringstream iss(line);
        string logUsername, action, timestamp;
        
        // Extract username, action, and full timestamp
        if (iss >> logUsername >> action) {
            // Get the rest of the line as timestamp
            getline(iss, timestamp);
            
            if (logUsername == username) {
                // Reconstruct log entry
                logEntries.push_back(action + " " + timestamp);
            }
        }
    }
    
    return logEntries;
} 
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

bool fileExists(const string &username, const string &filename) {
    vector<string> userFiles = getFilesForUser(username);
    return find(userFiles.begin(), userFiles.end(), filename) != userFiles.end();
}

bool removeFileForUser(const string &username, const string &filename) {
    // Reads entire file
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

    // Socket/Binding Creation - Courtesy of GeeksforGeeks
    // Create serverR's UDP socket
    udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        perror("UDP socket creation failed");
        return -1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_R_UDP);
    serverAddr.sin_addr.s_addr = inet_addr(HOST_NAME); // Hardcode localhost 121.0.0.1

    // Bind the UDP socket
    if (bind(udpSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(udpSock);
        return -1;
    }

    cout << "Server R is up and running using UDP on port " << PORT_R_UDP << endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        // Receive the request from ServerM
        ssize_t len = recvfrom(udpSock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientLen);
        if (len < 0) {
            perror("Failed to receive data");
            continue;
        }

        string request(buffer);

        // Existing commands (lookup, push, remove, deploy, log)
        if (request.find("lookup") != string::npos) {
            cout << "Server R has received a lookup request from main server. " << endl;
            stringstream ss(request);
            string command, username;
            ss >> command >> username;

            // Log the lookup action
            addLogEntry(username, "LOOKUP");

            // Get the files associated with the username
            vector<string> files = getFilesForUser(username);
            
            // Prepare response
            string response;
            if (files.empty()) {
                response = username + " does not exist. Please try again."; // Username not found!";
            } else {
                for (const string &file : files) { // Preparing the list of files to show in client as response
                    response += file + "\n";
                }
            }

            // Send the response back to ServerM
            ssize_t sent = sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
            if (sent < 0) {
                perror("Failed to send response");
            } else {
                cout << "Server R has finished sending the response to the main server. " << endl;
            }
        }
        else if (request.substr(0, 4) == "push") {
            cout << "Server R has received a push request from the main server " << endl;
            // Parse push command
            istringstream iss(request);
            string pushCmd, username, filename;
            iss >> pushCmd >> username >> filename;

            // Log the push action
            addLogEntry(username, "PUSH " + filename);

            // Check if file already exists for this user
            bool exists = fileExists(username, filename);

            if (exists) {
                // Send FILE_EXISTS back to ServerM
                cout << filename << " exists in " << username << "'s repository; requesting overwrite confirmation. " << endl;
                string response = "FILE_EXISTS";
                sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);

                // Wait for overwrite decision from ServerM
                memset(buffer, 0, BUFFER_SIZE);
                recvfrom(udpSock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clientLen);
                string overwriteDecision(buffer);
                cout << "This is what is in buffer " << buffer << endl;

                if (overwriteDecision == "OVERWRITE_YES") {
                    // If overwrite is allowed
                    cout << "User requested overwrite; overwrite successful. " << endl;
                    string response = "File push successful.";
                    sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
                    
                    // Log the overwrite action
                    addLogEntry(username, "OVERWRITE " + filename);
                } else {
                    cout << "Overwrite denied" << endl;
                    string response = "File push unsuccesful";
                    sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
                }
            } 
            else {
                // Add file to user's repository (add entry to filenames.txt)
                ofstream file(FILENAME_FILE, ios::app);
                file << username << " " << filename << endl;
                file.close();

                cout << filename << " pushed successfully." << endl;
                string response = "FILE_ADDED";
                sendto(udpSock, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, clientLen);
            }
        }
        else if (request.substr(0, 6) == "remove") {
            cout << "Server R has received a remove request from the main server. " << endl;
            // Parse remove command
            istringstream iss(request);
            string removeCmd, username, filename;
            iss >> removeCmd >> username >> filename;

            // Log the remove action
            addLogEntry(username, "REMOVE " + filename);

            // Check if file exists for this user
            bool exists = fileExists(username, filename);

            if (exists) {
                //Remove file
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
        else if (request.substr(0, 6) == "deploy") {
            cout << "Server R has received a deploy request from the main server " << endl;
            // Extract the username from the deploy request
            istringstream iss(request);
            string deployCmd, username;
            iss >> deployCmd >> username;

            // Log the deploy action
            addLogEntry(username, "DEPLOY");

            // Get the files associated with the username
            vector<string> files = getFilesForUser(username);
    
            // Prepare response
            string response;
            if (files.empty()) {
                response = username + " does not exist. Please try again.";
            } else {
                for (const string &file : files) {
                    response += file + "\n";
                }
            }

            // Send the response back to serverM
            ssize_t sent = sendto(udpSock, response.c_str(), response.size(), 0, 
                          (struct sockaddr *)&clientAddr, clientLen);
            if (sent < 0) {
                perror("Failed to send response");
            } else {
                // Log that the response has been sent
                cout << "Server R has finished sending the response to the main server." << endl;
            }
        }
        else if (request.substr(0, 3) == "log") {
            cout << "Server R has received a log request from the main server. " << endl;
            // Extract the username from the log request
            istringstream iss(request);
            string logCmd, username;
            iss >> logCmd >> username;

            // Retrieve log entries for the user
            vector<string> logEntries = getUserLogEntries(username);

            // Prepare response
            string response;
            if (logEntries.empty()) {
                response = "No log entries found for " + username;
            } else {
                response = "Log history for " + username + ":\n";
                for (const string &entry : logEntries) {
                    response += entry + "\n";
                }
            }

            // Send the response back to serverM
            ssize_t sent = sendto(udpSock, response.c_str(), response.size(), 0, 
                          (struct sockaddr *)&clientAddr, clientLen);
            if (sent < 0) {
                perror("Failed to send response");
            } else {
                cout << "Server R has finished sending the log response to the main server." << endl;
            }
        }
    }

    close(udpSock);
    return 0;
}