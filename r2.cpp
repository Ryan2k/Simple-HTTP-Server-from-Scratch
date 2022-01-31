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
#include <fstream>

using namespace std;

// Global Variables - values given in main()
char* portNumber;
char* hostName;
char* fileName;

/**
 * Takes in the port number and the host name of the server and creates a socket between our
 * current process and the process listening on the specified port and hostname
 */
int createSocket() {
    // For Debugging
    cout << "port number: " << portNumber << ", host name: " << hostName << "!" << endl;

    int clientSocket; // return value of the descriptor for the socket we are going to create

    // Step 1 - Use the getaddrinfo() function supplied by socket.h to get a linked list of guesses
    // for where the hosts address is. To do that, need to supply it hints
    struct addrinfo hints; // gives hints to the getaddrinfo() function in order to retreive the correct addressinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // says that the address is of the internet family (ipv4 or ipv6)
    hints.ai_socktype = SOCK_STREAM; // says that it is a stream and not a datagram
    hints.ai_protocol = IPPROTO_TCP; // says that it is TCP (making it TCP because we are retreiving a file and need all data)

    struct addrinfo* resultHead; // stores the address of the first addrinfo guess in the results linked list

    int status = getaddrinfo(hostName, portNumber, &hints, &resultHead); // creates the linked list and returns 0 if successful

    if (status != 0) {
        cout << "could not formulate any guesses based on host name and port number" << endl;
        exit (EXIT_FAILURE);
    }

    // Step 2 - Loop through the linked list and try to successfully connect to one of the guesses
    struct addrinfo* curr; // value to keep track of each record

    for (curr = resultHead; curr != NULL; curr = curr->ai_next) {
        clientSocket = socket(curr->ai_family, curr-> ai_socktype, curr->ai_protocol);

        // if it couldnt successfully create a socket, returns -1 instead of a descriptor so move to next
        if (clientSocket == -1) {
            continue;
        }

        // otherwise, if it created a socket, test the connection to see if it actually works
        if (connect(clientSocket, curr->ai_addr, curr->ai_addrlen) != -1) {
            return clientSocket; // if something other than -1 was returned, it successfully connected so return the descriptor and keep the socket open
        }

        // if it couldnt successfully connect, close the socket and try the next guess
        close(clientSocket);
    }

    // if we reached the end of the loop, it means no successful connection could be made with the guesses so just exit the program
    cout << "No successful connection could be made with the guesses obtained from the port and hostname" << endl;
    exit(EXIT_FAILURE);
}

int main (int argc, char** argv) {
    // Step 1 - Verify validity of the input
    if (argc != 6) {
        cout << "Arguments were not in the form of a GET Request" << endl;
        cout << "Must pass in 5 space seperated strings in the format: <Request> <Filename> <Http Version> <Host:> <Host Name>" << endl;
        cout << "GET /samplefile.txt HTTP/1.1 Host: csslab11.uwb.edu" << endl;
        exit (EXIT_FAILURE);
    }

    // Step 2 - Hard Code the port number (80 is typically used for Http Requests but for our custom program, using 8080)
    // This is the port number that the server program I created is running on and listening on for incoming TCP connection requests
    // and parse out the hostname from the input (should always be argv[5] as argv[0] is the filename and hostname is the 5th string)
    char* portNumber = "8080";
    char* hostName = argv[5];

    // Step 3 - Create a socket between the current program (the client) and the program running on the server
    int clientSocket = createSocket();
}