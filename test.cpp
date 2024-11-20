#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>

#define LOCALHOST "127.0.0.1"
#define SERVER_R_PORT 22985

using namespace std;

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char buffer[1024];

    // Setup UDP socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);  // Use SOCK_DGRAM for UDP
    if (serverSocket < 0) {
        cerr << "Error creating socket!" << endl;
        return -1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_R_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed!" << endl;
        close(serverSocket);
        return -1;
    }

    cout << "Server R is up and running using UDP on port " << SERVER_R_PORT << endl;

    while (true) {
        // Receive the message from the client
        int bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer) - 1, 0,
                                 (struct sockaddr *)&clientAddr, &addrLen);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            cout << "Server R has received a lookup request from the main server: " << buffer << endl;

            // Simulate a response (e.g., returning a document list or success message)
            const char *response = "Document 1, Document 2, Document 3";  // Example response
            sendto(serverSocket, response, strlen(response), 0, 
                   (struct sockaddr *)&clientAddr, addrLen);

            cout << "Server R has finished sending the response to the main server." << endl;
        }
    }

    close(serverSocket);  // Close the socket when the server shuts down
    return 0;
}
