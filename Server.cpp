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

struct threadData {
    int sd;
};

struct HttpResponse {
    char* statusLine;
    FILE* file;
};

const char* HTTP_RESPONSE_FORMAT_OUT = "(%s, %s)\n";

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

/**
 * Takes in the pointer to the file we read in along with the socket with the connection to the client
 * we are currently serving, and sends the file over to the client over the socket.
 */
void sendFile(FILE* filePointer, int socket) {
    char data[1024] = {0};

    while (fgets(data, 1024, filePointer) != 0) {
        if (send(socket, data, 1024, 0) == -1) {
            cout << "Error Sending Data" << endl;
            exit (EXIT_FAILURE);
        }

        bzero(data, 1024);
    }

    cout << "Successfully sent file!" << endl;
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
void* handleRequest(void* data) {
    cout << "Entered handleRequest" << endl;
    // Step 1 - Create buffer to read in the clients write call (read in the get request)
    int bufferSize = 1024; // max size of the get request is technically double this I believe but we shouldnt need that much
    char dataBuffer[bufferSize]; // where we are reading the write() call from the client containing the request (char array = string)

    int sd = ((struct threadData*)data)->sd;

    cout << "new socket descriptor: " << sd << endl;

    // Step 2 - Read in the request to the buffer
    read(sd, dataBuffer, bufferSize); // description of read at the bottom of this file

    cout << "Successfully Read in Content from the Clients Reqeust, Here are the contents: " << endl;
    cout << dataBuffer << endl; // todo: for now I am just sending the file name but this might change

    // Step 3 - Extract Individual Values from Http Request String (the value inside databuffer)
    char* currString;
    currString = strtok(dataBuffer, " ");

    cout << "Request Type: " << currString << endl; // for debugging purposes

    // if the first string is not GET, return a 400 bad request as we only are able to support GET with this program
    if (strcmp(currString, "GET") != 0) {

    }

    currString = strtok(NULL, " /"); // Grabs the next space seperated string from the buffer (also with a slash as the format is /index.html)
    char* fileName = currString;

    cout << "fileName: " << fileName << endl;



    // Step 4 - Try to open the file
    FILE* file; // where we store the results of opening the file
    file = fopen(fileName, "r"); // fileName contains the file name and "r" indicates it is for reading (file pointer points to first location)

    // send the file
    sendFile(file, sd);

    /* Step 5 - Store the file and the status code in a variable to send over depending on the 3 outcomes listed above
    struct HttpResponse* response = new HttpResponse;

    // If the file does not hold null, it means we found it in our directory todo: not sure if this is right?
    // So put the file in the structure and give a 200 OK status message
    if (file == NULL) {
        response->statusLine = "200 OK";
        response->file = file;
    }
    else { 
        response->statusLine = "404 Not Found";
        response->file = file; // this value will just be null
    }

    // Step 6 - Store this response structure we just created to a file
    FILE* responseFile;
    fopen("response.dat", "w+"); // creates a new file called response.dat

    if (file == NULL) {
        cout << "Couldnt open a file response.dat" << endl;
        exit (EXIT_FAILURE);
    }

    write(sd, (void*)file, 4096); // just for testing

    cout << "created response.dat" << endl;

    fwrite(&response, sizeof(struct HttpResponse), 1, responseFile);

    cout << "successfully wrote to response file" << endl;

    //fprintf(responseFile, HTTP_RESPONSE_FORMAT_OUT, response->statusLine, response->file); // write the structure to the file with the specified format
    //fseek(responseFile, 0, SEEK_SET); // sets the pointer to the file to the begining

    // todo: not sure how to handle third case or if these two cases above are even correct

    // Step 7 - Send this structure back to the client
    write(sd, &responseFile, 4096);

    cout << "Made it to post write" << endl;
    */
    close(sd);
    return NULL;
}

int main (int argc, char** argv) {
    // Step 1 - Hard code a port number for the process that is listening for connections
    char* portNumber = "8080"; // http requests generally port 80

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
    cout << "Listening for incoming connections on port " << portNumber << " via descriptor: " << serverSocket << endl;

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

        cout << "Accepted a connection which is being handled through new descriptor: " << newConnectionDescriptor << endl;

        // if we were able to successfully accept a connection, w should now create a new thread to hanld it
        pthread_t newClientThread;
        struct threadData* data = new threadData;
        data->sd = newConnectionDescriptor;
        pthread_create(&newClientThread, NULL, handleRequest, (void*)data);
    }

    return 0;
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
 * 
 * 3. fopen
 *  a) Takes in a name of a file and a mode
 *  b) 6 different modes - read, write, etc.
 *  c) Returns a file pointer if successfull or null if unable to find the file
 */