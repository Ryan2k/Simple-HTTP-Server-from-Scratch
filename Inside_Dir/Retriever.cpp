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
    //cout << "Entered createSocket()" << endl;
    //cout << "port number: " << portNumber << endl;
    //cout << "host name: " << hostName << "!" << endl;

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
            cout << "created socket: " << clientSocket << endl;
            return clientSocket; // if something other than -1 was returned, it successfully connected so return the descriptor and keep the socket open
        }

        // if it couldnt successfully connect, close the socket and try the next guess
        close(clientSocket);
    }

    // if we reached the end of the loop, it means no successful connection could be made with the guesses so just exit the program
    cout << "No successful connection could be made with the guesses obtained from the port and hostname" << endl;
    exit(EXIT_FAILURE);
}

void sendRequest(int clientSocket, char** argv) {
    // Step 1 - Format the request in the propper way
    // Http Requests are formatted in the way described in main, however, it is in two seperate lines as shown below
    string requestString = string(argv[1]) + " " + string(argv[2]) + " " + string(argv[3]) + "\r\n" + string(argv[4]) + " " + string(argv[5]) + "\r\n\r\n";
    cout << "Request Format as String: " << endl;
    cout << requestString << endl;

    // convert it back to a character array to be able to use the write sys call
    // other alternative was to use str.cpy to save a tiny amount of memory but it wasnt working with the slashes
    int rLen = requestString.length();
    char httpRequest[rLen + 1];
    strcpy(httpRequest, requestString.c_str());

    //cout << "Http Request: " << endl;
    //cout << httpRequest << endl;

    // Step 2 - Write the request to the socket
    // Instead of a file descriptor, can simply use the socket descriptor
    // 3rd argument is the max amount of bytes to write but this should never come close to 1024
    int numBytesWritten = write(clientSocket, httpRequest, 1024);

    if (numBytesWritten < 0) { // -1 means that the write failed so dont continue program
        cout << "Failed writing request to socket" << endl;
        exit (EXIT_FAILURE);
    }

    cout << "Request Sent w/ number of bytes = " << numBytesWritten << endl;

}

void readResponse(int clientSocket, string& file, string& status) {
    //cout << "Entered readResponse() " << endl; // for debugging
    char prev = 0;

    int lineNumber = 1;
    
    while (1) {
        int bytesRead;
        string currLine = "";
        while (1) {
            char curr = 0;

            bytesRead = recv(clientSocket, &curr, 1, 0);

            if (curr == '\n' || curr == '\r' || bytesRead == 0) {
                if (prev == '\n' || prev == '\r' || bytesRead == 0) {
                    break;
                }
            }
            else {
                currLine += curr;
            }

            prev = curr;
        }

        if (bytesRead == 0) {
            break;
        }

        //cout << "line " << lineNumber << ": " << currLine << " Bytes Read = " << bytesRead << endl;

        if (lineNumber == 1) {
            status += currLine;
        }
        else if (lineNumber > 4){
            file += currLine;
            file += "\r\n";
        }

        lineNumber++;
    }

    lineNumber--;

    //cout << "Finished Reading " << lineNumber << " lines" << endl;
}

int main (int argc, char** argv) {
    // Step 1 - Verify validity of the input
    //cout << "argc: " << argc << endl;
    if (argc != 6) {
        cout << "Arguments were not in the form of a GET Request" << endl;
        cout << "Must pass in 5 space seperated strings in the format: <Request> <Filename> <Http Version> <Host:> <Host Name>" << endl;
        cout << "GET /samplefile.txt HTTP/1.1 Host: csslab11.uwb.edu" << endl;
        exit (EXIT_FAILURE);
    }

    // Step 2 - Hard Code the port number (80 is typically used for Http Requests but for our custom program, using 8080)
    // This is the port number that the server program I created is running on and listening on for incoming TCP connection requests
    // and parse out the hostname from the input (should always be argv[5] as argv[0] is the filename and hostname is the 5th string)
    portNumber = "8080";
    hostName = argv[5];

    // Step 3 - Create a socket between the current program (the client) and the program running on the server
    int clientSocket = createSocket();

    // Step 4 - Send the request (Description over the sendRequest Method)
    sendRequest(clientSocket, argv);

    // Step 5 - Read in the response from the server
    string status = "";
    string file = "";
    readResponse(clientSocket, file, status);

    // for debugging
    //cout << "status: " << endl;
    //cout << status << endl;
    //cout << "file contents: " << endl;
    //cout << file << endl;

    // Step 6 - Save to file if it is 200 OK status
    string fileName = (string)argv[2];
    fileName = fileName.substr(1);

    cout << "File name: " << fileName << endl;

    string statusNum = status.substr(9, 3);

    cout << "Status code: " << statusNum << endl;
    if (statusNum == "200") {
        ofstream out(fileName.c_str(), ios_base::out); // todo: change to fileName.c_str()
        out << file;
        out.close();
        cout << "Finished writing" << endl; // for debugging
    }

    // Step 7 - Whether or not it was a 404 or 200, print the file to the screen
    cout << file << endl;
}

/**
 * Notes
 * 
 * 1. ofstream
 *  a) Connects with the file system through the operating system and brings back a file handle so I can write a file
 *  b) The out object becomes a file handle that I can interact with and add content into
 *  c) Acts the same cout would with stdin (file descriptor 1)
 *  d) For this we want to close it so that when the program is over we are done writing
 *  e) Stores the result in the same directory if we dont specify a complete file path
 */