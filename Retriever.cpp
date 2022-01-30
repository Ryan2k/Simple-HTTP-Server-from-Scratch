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
#include <fstream>

using namespace std;

void displayFileToScreen(FILE* httpResponse) {
    char fname[25]; // name of file we are displaying (response.dat)
    char line[80]; // to print each line of the file
    ifstream fin; // file stream object
    fin.open("response.dat", ios::in); //open the file in input mode

    if(!fin) { //file does not exist
        cout << "file does not exist" << endl;
        exit (EXIT_FAILURE);
    }

    cout << "Contents of response.dat" << endl;

    // read data from file upto end of file
    while (fin.eof() == 0) {
        fin.getline(line, sizeof(line));
        cout << line << endl;
    }

    fin.close(); // close the file
}

struct addrinfo* getAddressGuesses (char* port, char* hostName) {
    struct addrinfo hints; // gives hints to the getaddrinfo() function in order to retreive the correct addressinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // says that the address is of the internet family (ipv4 or ipv6)
    hints.ai_socktype = SOCK_STREAM; // says that it is a stream and not a datagram
    hints.ai_protocol = IPPROTO_TCP; // says that it is TCP (making it TCP because we are retreiving a file and need all data)

    struct addrinfo* resultHead; // stores the address of the first addrinfo guess in the results linked list

    cout << "Hostname: " << hostName << endl; //debugging
    cout << "Port Number: " << port << endl; //debugging

    char* hardCodedHost = "csslab11.uwb.edu"; // todp: figure out why first char of hostname is getting deleted

    int status = getaddrinfo(hardCodedHost, port, &hints, &resultHead); // creates the linked list and returns 0 if successful

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
    cout << response << endl;
}

/*
 * This function handles taking in the input from the command line and parsing.
 * Then, it uses the parsed information to retreived a socket, and that to make a request.
 * Command line argument input format example: GET /index.html HTTP/1.1 Host: domainname.com
 */
int main (int argc, char** argv) {
    char* serverPortNumber = "8080"; // hard coded (http request generally use 80)
    char* serverHostName = argv[5]; // domainname.com from example above

    cout << "Server host name in main: " << serverHostName << endl;

    // if the input is not in the format of 5 space seperated strings, the request format is incorrect
    if (argc != 6) { // 6 actuall arguments as argv[0] is filename
        cout << "Incorrect request format. A GET request must have 5 space seperated arguments" << endl;
        cout << "For example: GET /index.html HTTP/1.1 Host: domainname.com" << endl;
        exit (EXIT_FAILURE);
    }

    // turn the arguments into a single string to send to the server
    char* httpRequest = argv[1];

    for (int i = 2; i <= 5; i++) {
        cout << "argv at " << i << ": " << argv[i] << endl;
        strcat(httpRequest, " ");
        strcat(httpRequest, argv[i]);
        cout << "Http Request: " << httpRequest << endl;
    }

    cout << "Http Request: " << httpRequest << endl; // for debugging


    // grab the linked list of address guesses we can obtain with the given port number and hostname/ ip address
    struct addrinfo* addressGuessHead = getAddressGuesses(serverPortNumber, argv[5]);

    // from those guesses, see if we can successfully make a connection
    // if so, grab the file descriptor of the socket we just opened
    int clientSocket = getSocketDescriptor(addressGuessHead);

    handleRequest(clientSocket, (void*)httpRequest);

    return 0;
}