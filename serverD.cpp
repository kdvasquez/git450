#include <iostream>
#include <fstream>
#include <sys/socket.h>  
#include <arpa/inet.h>
#include <netinet/in.h>  
#include <unistd.h>     
#include <cstring>       
#include <sstream>       

using namespace std;

#define PORT_D_UDP 23985 // Set the UDP port number
#define HOST_NAME "127.0.0.1"
#define DEPLOYED_FILE "deployed.txt"

int main() {
    int serverSocket;
    struct sockaddr_in address;
    char buffer[1024] = {0}; // Buffer for receiving data
    int addrlen = sizeof(address);

    // Socket/Binding Creation - Courtesy of GeeksforGeeks
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        cout << "Socket creation failed!" << endl;
        return -1;
    }
    // Define the address structure for binding
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT_D_UDP);
    address.sin_addr.s_addr = inet_addr(HOST_NAME);

    // Bind the socket to the specified port
    if (::bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Binding failed!" << endl;
        close(serverSocket);
        return -1;
    }
    cout << "Server D is up and running using UDP on port " << PORT_D_UDP << endl;

    // Wait to receive data 
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer before receiving
        
        // Receive data from a client
        int bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0,
                                     (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (bytesReceived < 0) {
            cerr << "Failed to receive data!" << endl;
            continue; // Continue listening instead of closing the socket
        }
        buffer[bytesReceived] = '\0';  // Null-terminate the received data
        
        // Parse the received message
        cout << "Server D has received a deploy request from the main server." << endl;
        string message(buffer);
        size_t spacePos = message.find(" ");
        if (spacePos == string::npos) {
            const char* errorResponse = "Invalid deploy message";
            sendto(serverSocket, errorResponse, strlen(errorResponse), 0,
                   (struct sockaddr*)&address, addrlen);
            continue;
        }

        string username = message.substr(0, spacePos);
        string filename = message.substr(spacePos + 1);

        // Open the deployed.txt file in append mode
        ofstream deployedFile(DEPLOYED_FILE, ios::app);
        if (!deployedFile) {
            const char* errorResponse = "Failed to open deployed file";
            sendto(serverSocket, errorResponse, strlen(errorResponse), 0,
                   (struct sockaddr*)&address, addrlen);
            continue;
        }

        // Write the username and filename to deployed.txt
        deployedFile << username << " " << filename << endl;
        deployedFile.close();

        // Send confirmation back to ServerM
        const char* successResponse = "FILE_DEPLOYED";
        sendto(serverSocket, successResponse, strlen(successResponse), 0,
               (struct sockaddr*)&address, addrlen);

        // Print deployment details
        cout << "Server D has deployed the user " << username << "'s repository." << endl; //Deployed file: " << filename << " for user: " << username << endl;
    }

    // Close the socket after the loop
    close(serverSocket);
    return 0;
}
