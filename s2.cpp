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
#include <netinet/tcp.h>  // SO_REUSEADDR git pu
#include <sys/uio.h>      // writev

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <cstring> // needed for memset
#include <fstream>
#include <stdlib.h>
#include <arpa/inet.h>


using namespace std;

char* portNumber;

struct threadData {
    int sd;
};

/**
 * Takes in the descriptor of the socket between the communication thread and the client
 * Runs through each line of the request header and formats it to a way the handlRequest() can work with
 */
string formatHeader(int comSocket) {
    cout << "Entered formatHeader()" << endl;
    string result;
    char prev = 0;

    while (1) {
        char curr = 0;

        recv(comSocket, &curr, 1, 0);

        if (curr == '\n' || curr == '\r') {
            if (prev == '\n' || prev == '\r'){
                break;
            }
        }
        else {
            result += curr;
        }

        prev = curr;
    }

    return result;
}

void* handleRequest(void* data) {
    cout << "Entered handleRequest" << endl;
    int communicationSocket = ((struct threadData*)data)->sd; // socket of the current thread and the client program

    string header = formatHeader(communicationSocket);
    cout << header << endl;

    return NULL;
}

/*
 * Takes in the number for the port that this process will be running on and tries to create an address info.
 * From there checks to see if we can make a successful connection, and if so, returns the descriptor of the
 * socket we are creating to listen for connections
 */
int getSocketDescriptor () {
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

int main (int argc, char** argv) {
    if (argc > 1) {
        cout << "There should not be any arguments supplied with this program!" << endl;
        exit (EXIT_FAILURE);
    }

    // Step 1 - Hardcode the port number
    portNumber = "8080"; // port number the main thread of this program (listener) is running on

    // Step 2 - Create a socket for this program to listen on
    int listeningSocket = getSocketDescriptor();

    // Step 3 - Listen on the socket we just created
    // System call to tell the OS we are ready to start listening on the specified port
    int listening = listen(listeningSocket, 5); // 5 is the backlog - the number of open threads it can have

    // returns -1 if the listen failed
    if (listening == -1) {
        cout << "Succesfully created a socket but cannot listen on the given port" << endl;
        exit (EXIT_FAILURE);
    }

    // Once we are successfully listening, print to the console what port we are listening on and that we are
    cout << "Listening for incoming connections on port " << portNumber << " via descriptor: " << listeningSocket << endl;

    // Step 4 - Create an infinte loop that is listening for connections
    // If successfully able to accept the connection, create another socket to handle the connection (get request) for each client
    while (1) {
        struct sockaddr clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        int newConnectionDescriptor = accept(listeningSocket, &clientAddress, &clientAddressLength);
    
        if (newConnectionDescriptor == -1) {
            cout << "Failed to accept connection request from client" << endl;
            // cout << "Failed to accept connection request form client address " << clientAddress << endl;
        }

        cout << "Accepted a connection which is being handled through new descriptor: " << newConnectionDescriptor << endl;

        // if we were able to successfully accept a connection, w should now create a new thread to hanld it
        pthread_t newClientThread;
        struct threadData* data = new threadData;
        data->sd = newConnectionDescriptor;
        pthread_create(&newClientThread, NULL, handleRequest, (void*)data);
    }

    return 0;

}