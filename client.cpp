#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_M_TCP 25985
#define PORT_A_UDP 21985  // serverA's UDP port
#define PORT_R_UDP 22985  // serverR's UDP port
#define BUFFER_SIZE 1024
#define SERVER_M "127.0.0.1" // Main server IP (localhost)
#define SERVER_A "127.0.0.1" // serverA IP (localhost)
#define SERVER_R "127.0.0.1" // serverR IP (localhost)

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
    struct sockaddr_in serverAddrM, serverAddrA, serverAddrR;
    char buffer[BUFFER_SIZE];

    // Create TCP socket for connecting to serverM
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure serverM address (Main Server)
    serverAddrM.sin_family = AF_INET;
    serverAddrM.sin_port = htons(PORT_M_TCP);
    serverAddrM.sin_addr.s_addr = inet_addr(SERVER_M);  // localhost

    // Connect to the Main Server (serverM)
    if (connect(sock, (struct sockaddr *)&serverAddrM, sizeof(serverAddrM)) < 0) {
        perror("Connection failed to Main Server");
        close(sock);
        return -1;
    }

    // Send encrypted authentication request
    string authRequest = string(username) + " " + encryptedPassword;
    write(sock, authRequest.c_str(), authRequest.size());

    // Receive response from serverM
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

        // Check if the command is "lookup <username>"
        if (command.substr(0, 6) == "lookup") {
            // Set up UDP socket for serverR
            int sockR = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockR < 0) {
                perror("Socket creation failed for serverR");
                return -1;
            }

            // Configure serverR address (serverR for "lookup" request)
            serverAddrR.sin_family = AF_INET;
            serverAddrR.sin_port = htons(PORT_R_UDP);
            serverAddrR.sin_addr.s_addr = inet_addr(SERVER_R);  // localhost

            // Send "lookup <username>" command to serverR
            sendto(sockR, command.c_str(), command.size(), 0, (struct sockaddr *)&serverAddrR, sizeof(serverAddrR));

            // Receive response from serverR
            memset(buffer, 0, BUFFER_SIZE);
            recvfrom(sockR, buffer, BUFFER_SIZE, 0, nullptr, nullptr);

            cout << "Server R Response: " << buffer << endl;

            close(sockR);
        } else {
            // Otherwise, send command to serverM
            write(sock, command.c_str(), command.size());

            // Receive server response
            memset(buffer, 0, BUFFER_SIZE);
            read(sock, buffer, BUFFER_SIZE);
            cout << "Server Response: " << buffer << endl;
        }
    } else {
        // Authentication failed
        cout << "Authentication failed. Please check your username or password." << endl;
    }

    close(sock);
    return 0;
}

