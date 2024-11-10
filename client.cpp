#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

#define SERVER_ADDRESS "127.0.0.1"  // Server IP address (localhost in this case)

using namespace std;

// Function to apply the encryption scheme to the password
string encryptPassword(const string &password) {
    string encrypted = password;
    for (char &ch : encrypted) {
        if (isdigit(ch)) {
            ch = (ch - '0' + 3) % 10 + '0';  // Rotate digit by 3, cyclically
        } else if (isalpha(ch)) {
            if (isupper(ch)) {
                ch = (ch - 'A' + 3) % 26 + 'A';  // Rotate uppercase letter by 3
            } else {
                ch = (ch - 'a' + 3) % 26 + 'a';  // Rotate lowercase letter by 3
            }
        }
    }
    return encrypted;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Usage: ./client <username> <password>" << endl;
        return -1;
    }

    string username = argv[1];
    string password = argv[2];

    // Apply the encryption scheme to the password
    string encryptedPassword = encryptPassword(password);
    cout << "Encrypted password: " << encryptedPassword << endl;

    cout << "The client is up and running." << endl;

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024];
    struct sockaddr_in local_addr;
    socklen_t addrlen = sizeof(local_addr);

    // Array of server ports for serverM, serverA, serverR, and serverD
    vector<int> serverPorts = {21985, 21986, 21987, 21988}; // Replace with actual server ports

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr) <= 0) {
        cerr << "Invalid address or address not supported" << endl;
        close(sockfd);
        return -1;
    }

    // Get the locally-bound address information (optional)
    int getsock_check = getsockname(sockfd, (struct sockaddr *)&local_addr, &addrlen);
    if (getsock_check == -1) {
        perror("getsockname failed");
        close(sockfd);
        return -1;
    }
    cout << "Client is using local port: " << ntohs(local_addr.sin_port) << endl;

    // Loop through each server port and send credentials
    for (int port : serverPorts) {
        server_addr.sin_port = htons(port); // Set server port dynamically

        // Prepare credentials with encrypted password
        string credentials = username + " " + encryptedPassword;
        cout << "Sending credentials to server on port " << port << ": " << credentials << endl;

        int sent_bytes = sendto(sockfd, credentials.c_str(), credentials.size(), MSG_CONFIRM,
                                (const struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            perror("Send failed");
            continue; // Try the next server if sending fails
        }

        cout << "Message sent successfully to server on port " << port << "." << endl;

        // Wait for a response from the server (optional, depends on your protocol)
        socklen_t server_len = sizeof(server_addr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        // Null-terminate and display the response from the server
        buffer[n] = '\0';
        cout << "Server response from port " << port << ": " << buffer << endl;
    }

    // Close the socket
    close(sockfd);

    return 0;
}
