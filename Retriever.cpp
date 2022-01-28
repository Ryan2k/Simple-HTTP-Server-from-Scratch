/*
 * This program will take in as input a server address and file name and send a GET request to the server to
 * retrieve the file with the given name. If the request is successful and the response sends back the file,
 * the file will be saved to this system and displayed to the screen.
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

struct addrinfo* getAddressGuesses (char* port, char* hostName) {
    struct addrinfo hints; // gives hints to the getaddrinfo() function in order to retreive the correct addressinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // says that the address is of the internet family (ipv4 or ipv6)
    hints.ai_socktype = SOCK_STREAM; // says that it is a stream and not a datagram
    hints.ai_protocol = IPPROTO_TCP; // says that it is TCP (making it TCP because we are retreiving a file and need all data)

    struct addrinfo* resultHead; // stores the address of the first addrinfo guess in the results linked list

    // cout << "Hostname: " << hostName << endl; //debugging
    // cout << "Port Number: " << port << endl; //debugging

    int status = getaddrinfo(hostName, port, &hints, &resultHead); // creates the linked list and returns 0 if successful

    if (status != 0) {
        cout << "could not formulate any guesses based on host name and port number" << endl;
        exit (EXIT_FAILURE);
    }

    return resultHead;
}

int getSocketDescriptor (struct addrinfo* head) {
    struct addrinfo* curr; // to keep track of the current guess we are at

    int sd; // return variable mapped to the socket descriptor of our connection (-1 if unsuccessful)

    // loop through list until we reach end or find successful connection
    for (curr = head; curr != NULL; curr = curr->ai_next) {
        sd = socket(curr->ai_family, curr-> ai_socktype, curr->ai_protocol);

        // if it couldnt successfully create a socket, returns -1 instead of a descriptor so move to next
        if (sd == -1) {
            continue;
        }

        // otherwise, if it created a socket, test the connection to see if it actually works
        if (connect(sd, curr->ai_addr, curr->ai_addrlen) != -1) {
            return sd; // if something other than -1 was returned, it successfully connected so return the descriptor and keep the socket open
        }

        // if it couldnt successfully connect, close the socket and try the next guess
        close(sd);
    }

    // if we reached the end of the loop, it means no successful connection could be made with the guesses so just exit the program
    cout << "No successful connection could be made with the guesses obtained from the port and hostname" << endl;
    exit(EXIT_FAILURE);
}

/*
 * This function will write the name of the file we want to the server and if it successfully recieves the message
 * and is able to find the file, it will write the entire file back to the socket. On success, we will save the
 * file to our current system and display its contents to the screen (done in another method). The two variables we pass in are
 * 1. The descriptor of the open socket connection we have between our current process and the process running on the server
 * 2. The name of the file we are requesting from the server
 */
void handleRequest(int clientSocket, void* requestMessage) {
    write(clientSocket, requestMessage, 1024); // if you use sizeof(request message), it just returns 8 (size of pointer) so can only send 8 chars

    char response[4096]; // buffer that reads in the reponse

    read(clientSocket, response, 4096);

    cout << "succesfully read in a response" << endl;

    // todo: convert this back into the HttpResponse object like the server program
}

int main (int argc, char** argv) {
    char* serverPortNumber = argv[1];
    char* serverHostName = argv[2];
    char* requestingFileName = argv[3];

    // grab the linked list of address guesses we can obtain with the given port number and hostname/ ip address
    struct addrinfo* addressGuessHead = getAddressGuesses(serverPortNumber, serverHostName);

    // from those guesses, see if we can successfully make a connection
    // if so, grab the file descriptor of the socket we just opened
    int clientSocket = getSocketDescriptor(addressGuessHead);

    handleRequest(clientSocket, (void*)requestingFileName);

    return 0;
}