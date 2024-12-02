USC EE450 (Computer Networks) Final Project

(a) Name: Karla Vasquez
(b) Student ID: 3166059985

(c) What have I done in this assignment?
I have implemented all expected files, (client.cpp, serverM.cpp, serverA.cpp, serverR.cpp, serverD.cpp), and the extra credit log command. 

General overview of what I implemented: users can access the client program as a member or as a guest. It displays options for lookup, push, remove, deploy, and log, and sends these requests to the main server, Server M. Server M will acknowledge if requests were forwarded successfully (lookup, remove, and deploy are forwarded to Server R and Server D). Server A is sent the user's login credentials to determine if they’re a member or a guest. Server M will return the list of file names associated with a given user. 


(d) My github450 final projects contains the following files:

client.cpp - this does not use any static ports; it uses two dynamic TCP ports to communicate with the main server, serverM. It’s in charge of taking in the command line inputs <username> password or guest guest to determine whether the user is logging in as a member or guest. client then provides a menu of options showing how to further run commands on the terminal. All commands are forwarded to serverM, who will be in charge of forwarding to its respective server. All commands have a different function. At the end, the client returns confirmation messages on screen verifying when the client successfully sent a message to serverM, and when serverM has returned the response. 

serverM.cpp - this uses a UDP port over port# 24985, and a TCP port over port#25985 when forwarding back to the client. It is the main server that handles sending and receiving between client and the three backend servers.

serverA.cpp - this is the authentication server, which includes an appropriate encryption function to take in the password. serverA only outputs messages for member users, and is briefly used at the start just to confirm whether the member user successfully logged in or not.  

serverR.cpp - this is the repository server. When serverM forwards either a “lookup”, “remove”, “push”, or “deploy” request, serverR must either check filenames.txt for the user’s files and send back all the names associated with the user to serverM, remove any specified file from filenames.txt with the user’s name, add a file with the user’s name, and put together all files with the user’s name to “deploy” meaning it’s sent to serverD. 

serverD.cpp - this is the deployment server. When serverR sends the signal to serverD, serverD gathers all the filenames sent from serverR and produces the deployed.txt file. Prints out appropriate messages on screen confirming the procedure. 

Text files:
members.txt - list of member names with the encrypted version of their password
original.txt - list of member names with the unencrypted version of their password
deployed.txt - produced when program is executed using “deploy” command
user_log.txt - produced when “log” command is executed
Makefile - compiles all files 

(e) The format of all messages exchanged: 

Request command sent to serverR by serverM:
lookup <username>
remove <filename>
deploy 

Request command sent to serverD by serverM:
deploy

Request command sent to serverR by serverM:
log 

Request command sent to serverA by serverM:
./client <username> password

If using UDP, it typically followed this code structure, courtesy of GeeksforGeeks : 
// Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Configure server’s address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_A_UDP);
    serverAddr.sin_addr.s_addr = inet_addr(HOST_NAME); // Hardcode localhost 121.0.0.1


    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

In order to run on terminal: 
make all 
Open up six terminals, and run the following commands in this order:
./serverM
./serverA
./serverR
./serverD
./client <username> password
./client guest guest 

One of the client terminals runs as a member; the second client terminal runs as a guest. 
For member users, you retrieve the username and password from the originals.txt file, where you may select any member when running the client program as a member. It is important you retrieve the password from here as other txt file (members.txt) will contain member information in it’s encrypted form and will not work as the authentication server (./serverA) is comparing the passwords up against the members.txt file. If you run the client program as a guest, you will only have access to the “lookup” command in the provided menu. 


(g) Idiosyncrasies 
 
No on screen messages for the following events: 
Lookup Command for serverM:
Event: Upon receiving a lookup request from guest
Event: After forwarding the overwrite confirmation request to the client
Event: After forwarding the overwrite confirmation response to server R

Push Command for serverM:
Event: Upon receiving overwrite confirmation response from client
Event: After forwarding the overwrite confirmation request to the client
Event: After forwarding the overwrite confirmation response to server R

Lookup Command for Client (Guest):
Event: After receiving the response from the main server (the username exists):
Event: After receiving the response from the main server (the username does not exist):
Event: After receiving the response from the main server (the repo is empty): 

Known issues that may arise:
Remove Command: 

Only removes files already in the filenames.txt for that user before you push new ones. If you add a file, like “EE450ProjectFile_77,” it won’t be removed right away until the next couple of remove commands mysteriously. 
Deploy Command:
It’s buggy, it’ll deploy after a couple of push commands or at the very start if there are actually files to be deployed. 


(h) I reused code:
For the UDP/TCP socket creation and binding from GeekForGeeks: 
https://www.geeksforgeeks.org/socket-programming-in-cpp/

For encryption algorithm: Stack Overflow

A lot of googling~

For Makefile: Replaced with my filenames from templates: https://www.cs.swarthmore.edu/~newhall/unixhelp/howto_makefiles.html