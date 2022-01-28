/*
 * This program waits for a connection and an HTTP GET request. Once recieving the request, checks to see if the file
 * that the client is requesting exists on the machine, and if so, opens the file requested and sends it over to the client.
 * Along with the file, it should send over a status code (200 OK if successful). If unsuccessful, it should return a 
 * 404 Not Found code along with a custom "File Not Found" page. Each of these accepted connections are going to be handled
 * by a seperate thread. Once a request is handled, it should keep listening for others.
 */

#include <sys/types.h>    // socket, bind 
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa 
#include <netinet/in.h>   // htonl, htons, inet
#include <arpa/inet.h>    // inet_ntoa 
#include <netdb.h>        // gethostbyname 
#include <unistd.h>       // read, write, close 
#include <strings.h>      // bzero 
#include <netinet/tcp.h>  // SO_REUSEADDR 
#include <sys/uio.h>      // writev

#include <iostream>
#include <stdio.h>
#include <cstring> // needed for memset

using namespace std;

/*
 * Takes in the number for the port that this process will be running on and tries to create an address info.
 * From there checks to see if we can make a successful connection, and if so, returns the descriptor of the
 * socket we are creating to listen for connections
 */
int getSocketDescriptor (char* portNumber) {
    // Step 1 - Create hints to get the address information
    struct addrinfo hints; // hints to feed the getaddressinfo function
    memset(&hints, 0, sizeof(struct addrinfo)); // sets all the bytes of hints to 0. Why do we do this?
    hints.ai_family = AF_UNSPEC; // part of the internet family (ipv4 or ipv6)
    hints.ai_socktype = SOCK_STREAM; // hint that the socket will be a stream
    hints.ai_flags = AI_PASSIVE; // says that the address is suitable for bind calls

    // Step 2 - Get the address information linked list (guesses that hopefully one will be correct)
    struct addrinfo* guessHead; // where the head of the linked list is stored
    int canTranslate = getaddrinfo(NULL, portNumber, &hints, &guessHead); // creates the linked list and returns 0 if successful
    
    // Step 3 - Try to open a socket with each guess until we find one or reach the end of the list
    struct addrinfo* curr = guessHead; // current record
    int serverSocket; // the descriptor of the socket if successful (our return value)

    while (curr != NULL) {
        serverSocket = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);

        if (serverSocket == -1) {
            curr = curr->ai_next;
            continue; // couldnt create a socket so move to the next
        }

        // Step 4 - If we were successfully able to create a socket, we must configure it
        // Make sure that the ports can be reused
        int enable = 1; // the value of so_reuseaddr to 1 means that it is enabled (we can reuse it)
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

        // Step 5 - Bind the socket we created to the port we are running this process on
        int successfullyBinded = bind(serverSocket, curr->ai_addr, curr->ai_addrlen);

        // if we could bind, we can use this socket so return the descriptor and keep the socket open
        if (successfullyBinded == 0) {
            freeaddrinfo(guessHead);
            return serverSocket;
        }

        // if we couldnt bind, close the socket and try the next guess
        close(serverSocket);
    }

    freeaddrinfo(guessHead);

    // if we have reached the end of the linked list without a successfull socket, kill the program
    cout << "Could not create a successfull socket!" << endl;
    exit (EXIT_FAILURE);
}

/*
 * This function takes in a socket descriptor for the new connection we create at the bottom of the function below
 * and is ran by a new thread. Its job is to handle the http request sent by the client. Does this by
 * 1. Reading in the buffer information (the get request itself)
 * 2. Checking the system to see if the requested file exits
 * 3. Formulate a response depending on the request
 *    a) If the system is able to find the file, opens it, sends it back to the client, and sends a 200 status OK
 *    b) If not, sends a 404 File not found with a custom file (not found page)
 *    c) If the name of the file requested is "SecretFile.html" - return a 401 Unauthorized
 *    d) If the request is for a file that is above the directory where the server is running - return a 403 forbidden
 *    e) Finally, if the server cannot understand the request - return a 400 bad request
 *       - This will happen if we get a request type other than get or if the message format is incorrect
 */
void* handleRequest(void* sd) {
    // Step 1 - Create buffer to read in the clients write call (read in the get request)
    int bufferSize = 1024; // max size of the get request is technically double this I believe but we shouldnt need that much
    char dataBuffer[bufferSize]; // where we are reading the write() call from the client containing the request (char array = string)

    int socketDescriptor = (int)sd;

    // Step 2 - Read in the request to the buffer
    read(socketDescriptor, dataBuffer, sizeof(dataBuffer)); // description of read at the bottom of this file

    cout << "Successfully Read in Content from the Clients Reqeust, Here are the contents: " << endl;
    cout << dataBuffer << endl;
}

int main (int argc, char** argv) {
    // Step 1 - Grab the port number the user would like our process to be running on
    char* portNumber = argv[1];

    // Step 2 - Create a socket for this process to listen for requests on
    int serverSocket = getSocketDescriptor(portNumber);

    // Step 3 - Listen on the socket we just created
    // System call to tell the OS we are ready to start listening on the specified port
    int listening = listen(serverSocket, 5); // 5 is the backlog - the number of open threads it can have

    // returns -1 if the listen failed
    if (listening == -1) {
        cout << "Succesfully created a socket but cannot listen on the given port" << endl;
        exit (EXIT_FAILURE);
    }

    // Once we are successfully listening, print to the console what port we are listening on and that we are
    cout << "Listening for incoming connections on port " << portNumber << endl;

    // Step 4 - Create an infinte loop that is listening for connections
    // If successfully able to accept the connection, create another socket to handle the connection (get request) for each client
    while (1) {
        struct sockaddr clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        int newConnectionDescriptor = accept(serverSocket, &clientAddress, &clientAddressLength);
    
        if (newConnectionDescriptor == -1) {
            cout << "Failed to accept connection request from client" << endl;
            // cout << "Failed to accept connection request form client address " << clientAddress << endl;
        }

        // if we were able to successfully accept a connection, w should now create a new thread to hanld it
        pthread_t newClientThread;
        pthread_create(&newClientThread, NULL, handleRequest, (void*)newConnectionDescriptor);
    }
}

/*
 * Notes
 * 1. Read System Call
 *  a) First argument is a descriptor for the file it is going to start reading from
 *  b) The second is the buffer it is going to read the content into
 *  c) The third is the maximum number of bytes it will attempt to read
 *  d) It starts at the byte that is supplied by the second argument
 *  e) If the file offset is at or past the end of the file, no bytes are read and read() returns 0
 *  f) On success, the number of bytes read is returned, and the file position is advanced by this number
 * 
 * 2. File Offset
 *  a) The file offset is the character location within that file
 *  b) Usually starts at 0
 *  c) So if the file offset is 100, we are reading the 101st byte in the file
 */