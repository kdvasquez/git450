#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_M_TCP 25985
#define BUFFER_SIZE 1024
#define SERVER_M "127.0.0.1"
using namespace std;

// Encryption function
string encryptPassword(const string &password) {
    string encrypted;
    for (char c : password) {
        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            encrypted += (c - base + 3) % 26 + base;
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
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Create TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_M_TCP);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the Main Server
    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }

    // Send encrypted authentication request
    string authRequest = string(username) + " " + encryptedPassword;
    write(sock, authRequest.c_str(), authRequest.size());

    // Receive response
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);

    if (string(buffer) == "AUTH_SUCCESS") {
        // Authentication success
        cout << "You have been granted member access." << endl;

        // Display the menu and ask for command
        cout << "Please enter the command:" << endl;
        cout << "<lookup <username>>" << endl;
        cout << "<push <filename>>" << endl;
        cout << "<remove <filename>>" << endl;
        cout << "<deploy>" << endl;
        cout << "<log>" << endl;

        // Read user input for the command
        string command;
        cout << "Enter your command: ";
        getline(cin, command);

        // Send command to the server
        write(sock, command.c_str(), command.size());

        // Receive server response
        memset(buffer, 0, BUFFER_SIZE);
        read(sock, buffer, BUFFER_SIZE);
        cout << "Server Response: " << buffer << endl;
    } else {
        // Authentication failed
        cout << "Authentication failed. Please check your username or password." << endl;
    }

    close(sock);
    return 0;
}
