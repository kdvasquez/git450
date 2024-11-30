#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#define PORT_M_TCP 25985 // Main server TCP port
#define BUFFER_SIZE 1024
#define SERVER_M "127.0.0.1" // Main server IP (localhost)
using namespace std;

// Function to encrypt the password
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
    
    // Configure serverM address
    serverAddrM.sin_family = AF_INET;
    serverAddrM.sin_port = htons(PORT_M_TCP);
    serverAddrM.sin_addr.s_addr = inet_addr(SERVER_M);
    
    // Connect to the Main Server (serverM)
    if (connect(sock, (struct sockaddr *)&serverAddrM, sizeof(serverAddrM)) < 0) {
        perror("Connection failed to Main Server");
        close(sock);
        return -1;
    }
    
    // Send authentication request
    string authRequest = string(username) + " " + encryptedPassword;
    write(sock, authRequest.c_str(), authRequest.size());
    
    // Receive authentication response
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    
    if (string(buffer) == "AUTH_SUCCESS") {
        cout << "You have been granted access." << endl;
        
        // Menu and command input loop
        while (true) {
            cout << "\nAvailable commands:\n"
                 << "1. lookup <username>\n"
                 << "2. push <filename>\n"
                 << "3. remove <filename>\n"
                 << "4. deploy\n"
                 << "5. log\n"
                 << "Enter your command (or 'exit' to quit): ";
            
            // Instead of getline, use direct input
            char commandBuffer[BUFFER_SIZE];
            cin.getline(commandBuffer, BUFFER_SIZE);
            string command(commandBuffer);
            
            // Trim any trailing whitespace
            command.erase(command.find_last_not_of(" \n\r\t") + 1);
            
            if (command == "exit") {
                cout << "Exiting the client. Goodbye!" << endl;
                break;
            }
            else if (command.substr(0, 6) == "lookup"){
                cout << username << " sent a lookup request to the main server using TCP over port 25985. " << endl;
            } 
            else if (command.substr(0, 6) == "remove"){
                cout << username << " sent a remove request to the main server using TCP over port 25985. " << endl;

            }
            // Send the command directly, with explicit length
            cout << "DEBUG: Sending command (length " << command.length() << "): [" << command << "]" << endl;
        
            ssize_t bytesSent = write(sock, command.c_str(), command.length());
            cout << "DEBUG: Bytes sent: " << bytesSent << endl;
            
            // Receive the server's response
            memset(buffer, 0, BUFFER_SIZE);
            read(sock, buffer, BUFFER_SIZE);
            
            // Display server response
            cout << "Server Response: " << buffer << endl;
        }
    } else {
        cout << "The credentials are incorrect. Please try again. " << endl;
    }
    
    close(sock);
    return 0;
}