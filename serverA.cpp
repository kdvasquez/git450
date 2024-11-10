
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 21985 // Set the UDP port number for serverA

using namespace std;

// Function to authenticate the username and password
bool authenticate(const string& username, const string& password) {
    ifstream file("members.txt");
    string line;
    
    // Search for the matching credentials in the "member.txt"
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
    char buffer[1024] = {0}; // Buffer for receiving data
    int addrlen = sizeof(address);

    // Step 1: Create UDP socket (AF_INET for IPv4, SOCK_DGRAM for UDP)
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        cout << "Socket creation failed!" << endl;
        return -1;
    }
    cout << "UDP socket created on port " << PORT << endl;

    // Step 2: Define the address structure for binding
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Step 3: Bind the socket to the specified port
    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Binding failed!" << endl;
        close(serverSocket);
        return -1;
    }
    cout << "Server A is up and running using UDP on port " << PORT << endl;

    // Step 4: Wait to receive data (no listen/accept in UDP)
    while (true) {
        cout << "Waiting for data from a client..." << endl;
        
        // Receive data from serverM (authentication request)
        int bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0,
                                     (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (bytesReceived < 0) {
            cerr << "Failed to receive data!" << endl;
            close(serverSocket);
            return -1;
        }

        buffer[bytesReceived] = '\0';  // Null-terminate the received data
        cout << "Received message: " << buffer << endl;

        // Split the received message into username and password
        string credentials(buffer);
        size_t spacePos = credentials.find(" ");
        string username = credentials.substr(0, spacePos);
        string password = credentials.substr(spacePos + 1);

        // Authenticate the received credentials
        bool isAuthenticated = authenticate(username, password);
        const char* response = isAuthenticated ? "AUTH_SUCCESS" : "AUTH_FAILURE";

        // Send the authentication result back to serverM
        sendto(serverSocket, response, strlen(response), 0,
               (struct sockaddr*)&address, addrlen);
        cout << "Authentication result sent to serverM: " << response << endl;
    }

    // Step 5: Close the socket after the loop (though this will run infinitely)
    close(serverSocket);

    return 0;
}
