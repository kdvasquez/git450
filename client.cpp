#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>


#define BUFFER_SIZE 1024
#define SERVER_M "127.0.0.1" // Main server IP (localhost)
using namespace std;

// Function to encrypt the password (found using Caesar Cipher algorithm)
string encryptPassword(const string &password) {
    string encrypted;
    for (char c : password) {
        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            encrypted += (c - base + 3) % 26 + base; // Caesar cipher (shift by 3)
        } else if (isdigit(c)) {
            encrypted += (c - '0' + 3) % 10 + '0';
        } else {
            encrypted += c;
        }
    }
    return encrypted;
}

int main(int argc, char *argv[]) {
    // Check if username and password are provided
    if (argc != 3) {
        cerr << "Usage: ./client <username> <password>" << endl;
        return -1;
    }
    
    const char *username = argv[1];
    string password = argv[2];
    cout << "The client is up and running." << endl;

    // Encrypt the password
    string encryptedPassword = encryptPassword(password);
    
    int sock;
    struct sockaddr_in serverAddrM;
    char buffer[BUFFER_SIZE];
    
    // Create TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Configure serverM's address
    serverAddrM.sin_family = AF_INET;
    serverAddrM.sin_port = htons(25985); // PortM's TCP
    serverAddrM.sin_addr.s_addr = inet_addr(SERVER_M);
    
    // Connect to the serverM
    if (connect(sock, (struct sockaddr *)&serverAddrM, sizeof(serverAddrM)) < 0) {
        perror("Connection failed to Main Server");
        close(sock);
        return -1;
    }

    // GUEST user has limited access
    if (string(username) == "guest" && password == "guest") {
        cout << "You have been granted guest access. " << endl;
        
        // Guest command input loop
        while (true) {
            cout << "\nPlease enter the command:\n"
                 << "1. lookup <username>" << endl;
            
            char commandBuffer[BUFFER_SIZE];
            cin.getline(commandBuffer, BUFFER_SIZE);
            string command(commandBuffer);
            
            // Trim any trailing whitespace
            command.erase(command.find_last_not_of(" \n\r\t") + 1);
            
            if (command == "exit") {
                cout << "Exiting the client. Goodbye!" << endl;
                break;
            }
            if (command.substr(0, 6) != "lookup") {
                cout << "Guests can only use the lookup command" << endl;
                continue;
            }
            // Check if username is specified
            size_t usernamePos = command.find(" ");
            if (usernamePos == string::npos || usernamePos + 1 >= command.length()) {
                cout << "Error: Username is required. Please specify a username to lookup." << endl;
                cout << "---Start a new request---" << endl;
                continue;
            }
            
            string lookupUsername = command.substr(usernamePos + 1);
            cout << "Guest sent a lookup request to the main server." << endl;
            // Send the lookup command
            write(sock, command.c_str(), command.length());
            
            // Receive the serverM's response
            memset(buffer, 0, BUFFER_SIZE);
            read(sock, buffer, BUFFER_SIZE);
            string response(buffer); // this buffer returns the user's files
            
            cout << "The client received the response from the main server using TCP over port 25985." << endl;
            if(strlen(buffer) == 0){
                cout << "Empty repository." << endl;
                cout << "---Start a new request---" << endl;
            }
            // Handle different response scenarios NOT WORKING RIGHT NOW???
            if (response.find("does not exist") != string::npos) {
                cout << lookupUsername << " does not exist. Please try again." << endl;
            } else if (response == "Empty repository.") {
                cout << "Empty repository." << endl;
            } else {
                cout << response << endl;
            }
            
            cout << "---Start a new request---" << endl;
        }
        
        close(sock);
        return 0;
    }
    
    // MEMBER sends authentication request
    string authRequest = string(username) + " " + encryptedPassword;
    write(sock, authRequest.c_str(), authRequest.size());
    
    // Receive authentication response
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    
    if (string(buffer) == "AUTH_SUCCESS") {
        cout << "You have been granted member access." << endl;
        
        // Member command input loop 
        while (true) {
            cout << "\nPlease enter the command:\n"
                 << "1. lookup <username>\n"
                 << "2. push <filename>\n"
                 << "3. remove <filename>\n"
                 << "4. deploy\n"
                 << "5. log\n"
                 << "Enter your command (or 'exit' to quit): ";
            
            char commandBuffer[BUFFER_SIZE];
            cin.getline(commandBuffer, BUFFER_SIZE);
            string command(commandBuffer);
            
            // Trim any trailing whitespace
            command.erase(command.find_last_not_of(" \n\r\t") + 1);
            
            if (command == "exit") {
                cout << "Exiting the client. Goodbye!" << endl;
                break;
            }
            
            // Command on-screen messages
            if (command.substr(0, 6) == "lookup") {
                cout << username << " sent a lookup request to the main server." << endl;
                if(command.substr(6).empty()){
                    cout << "Username is not specified. Will lookup " << username << endl;
                    //command = username + " " + "lookup";
                }
            }
            else if (command.substr(0, 4) == "push") {
                // push command logic???
            }
            else if (command.substr(0, 6) == "remove") {
                cout << username << " sent a remove request to the main server." << endl; //using TCP over port 25985. " << endl;
            }
            else if (command.substr(0, 6) == "deploy") {
                cout << username << " sent a lookup request to the main server. " << endl;
            }
            
            // Send command
            write(sock, command.c_str(), command.length());
            
            // Receive response
            memset(buffer, 0, BUFFER_SIZE);
            read(sock, buffer, BUFFER_SIZE);
            
            // Display response
            cout << "The client received the response from the main server using TCP over port 25985: " << buffer << endl;
            //cout << "The " << command << " request was successful. " << endl;
            cout << "---Start a new request---" << endl;
        }
    } else {
        cout << "The credentials are incorrect. Please try again. " << endl;
    }
    
    close(sock);
    return 0;
}