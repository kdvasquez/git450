#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"  // Server IP address
#define SERVERA_PORT 21985          // Fixed UDP port for serverA

using namespace std;

// Function to authenticate the username and password
bool authenticate(const string& username, const string& password) {
    ifstream file("members.txt");
    string line;
    
    // Search for the matching credentials in the "members.txt"
    while (getline(file, line)) {
        if (line == username + " " + password) {
            return true; // Credentials match
        }
    }
    return false; // No match found
}

int main() {
    int serverSocket;
    struct sockaddr_in address;
    char buffer[1024] = {0};
    int addrlen = sizeof(address);

    // Step 1: Create UDP socket (AF_INET for IPv4, SOCK_DGRAM for UDP)
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        cout << "Socket creation failed!" << endl;
        return -1;
    }

    // Step 2: Define the address structure for binding
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVERA_PORT);  // Fixed port assignment for serverA

    // Step 3: Bind the socket to the fixed port
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Binding failed!" << endl;
        close(serverSocket);
        return -1;
    }

    cout << "Server A is up and running using UDP on fixed port " << SERVERA_PORT << endl;

    // Step 4: Wait to receive data (no listen/accept in UDP)
    while (true) {
        cout << "Waiting for data from a client..." << endl;
        
        // Receive data from the client
        int bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0,
                                     (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (bytesReceived < 0) {
            cerr << "Failed to receive data!" << endl;
            continue;
        }

        buffer[bytesReceived] = '\0';  // Null-terminate the received data
        cout << "Received message: " << buffer << endl;

        // Extract the username and encrypted password
        string credentials(buffer);
        size_t spacePos = credentials.find(" ");
        string username = credentials.substr(0, spacePos);
        string password = credentials.substr(spacePos + 1);

        cout << "ServerA received username " << username << " and password *****" << endl;

        // Authenticate the received credentials
        bool isAuthenticated = authenticate(username, password);
        if (isAuthenticated) {
            cout << "Member " << username << " has been authenticated" << endl;
            const char* response = "AUTH_SUCCESS";
            sendto(serverSocket, response, strlen(response), 0, (struct sockaddr*)&address, addrlen);
        } else {
            cout << "The username " << username << " or password ***** is incorrect" << endl;
            const char* response = "AUTH_FAILURE";
            sendto(serverSocket, response, strlen(response), 0, (struct sockaddr*)&address, addrlen);
        }
    }

    // Step 5: Close the socket after the loop (this will run infinitely)
    close(serverSocket);

    return 0;
}
