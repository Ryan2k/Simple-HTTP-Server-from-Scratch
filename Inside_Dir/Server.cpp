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
#include <sstream> // neede for string stream


using namespace std;

char* portNumber;

struct threadData {
    int sd;
};

/**
 * Takes in the descriptor of the socket between the communication thread and the client
 * Runs through each line of the request header and formats it to a way the handlRequest() can work with
 * Pretty much just concattenates each character in a line to a string and once we get to new line, it breaks and returns
 */
string formatHeader(int comSocket) {
    string result = "";
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

    // cout << "Formatted Header: " << result << endl;
    return result;
}

/**
 * This function takes in the name of the file we parsed out of the http request and tries to find it.
 * The return variable is a custom object containing 2 variables
 * 1. A status code - Http status code (ex: "200 Ok" for success or "404 Not found" if unable to find)
 * 2. The file if we find it or a custom 404 not found file otherwise
 * There are 5 Possible Outcomes
 *   a) If the system is able to find the file, opens it, sends it back to the client, and sends a 200 status OK
 *   b) If not, sends a 404 File not found with a custom file (not found page)
 *   c) If the name of the file requested is "SecretFile.html" - return a 401 Unauthorized
 *   d) If the request is for a file that is above the directory where the server is running - return a 403 forbidden
 *   e) Finally, if the server cannot understand the request - return a 400 bad request
 *      - This will happen if we get a request type other than get or if the message format is incorrect
 */
void createResponse(string fileName, string& status, string& data, bool unauthorized, bool isBadRequest) {
    //cout << "Entered createResponse() and looking for file: " << fileName << endl;
    // Step 1 - Initialize the variables that will go in the structure
    FILE* file;

    // Step 2 - Try to open the given file name in read mode
    file = fopen(fileName.c_str(), "r");

    // Step 3 - Formulate a response based on the outcome of the open attempt

    // if bad request, just do the 400 status and filename is already set to correct one from function who called this
    if (isBadRequest) {
        status = "HTTP/1.1 400 Unauthroized\r\n";
    }

    // if unauthorized, simply return 401 and the file created for it
    if (unauthorized && !isBadRequest) {
        status = "HTTP/1.1 401 Unauthroized\r\n";
    }
    else if (!isBadRequest) {
        // Outcome 1 - The file did not exist so formulate a "404 not found"
        if (file == NULL) {
            string outsideGuess = "/Simple-Http-Server-from-Scratch/" + fileName;

            file = fopen(outsideGuess.c_str(), "r");

            if (file != NULL) {
                status = "HTTP/1.1 403 Forbidden\r\n";
                file = fopen("forbidden.html", "r");
                cout << "forbidden" << endl;
            }
            else {
                // a) set the status code
                //status = "HTTP/1.1 404 Not Found\r\n";

                //cout << "File Not Found" << endl;

                // b) open the file I created and put in the directory to display for "not found" errors
                file = fopen("filenotfound.html", "r");
            }
        }
        else { // Outcome 2 - File was found, so just set the status to success
            status = "HTTP/1.1 200 OK\r\n";
        }
    }
    // cout << "opened file and now starting to read " << endl;

    // Step 4 - Write the contents of the file to the string, data. This will represent the payload

    while (!feof (file)) {
        char curr = fgetc(file);

        if (curr < 0) {
            continue;
        }

        data += curr;
    }

    cout << "Finished Reading File" << endl;
    cout << data << endl;

    // Step 5 - Finally, close the file

    fclose(file);
}

// also how to handle that forbidden case (not sure how to check if the file is above the dir)
void* handleRequest(void* data) {
    cout << "Entered handleRequest" << endl;

    string fileName = ""; // eventually will extract the name of the file from the data buffer if possible

    // Step 1 - Extract the communication socket from the standardized void* thread function input
    int communicationSocket = ((struct threadData*)data)->sd; // socket of the current thread and the client program

    // Step 2 - Loop through every line of the request until we get to the line with the request type and file name
    // The correct format has both of these on the same line
    // Once there, sotre the value of the file name in a variable
    string currLine = " "; // keeps track of the current line of the request we are in

    int lineCounter = 0;

    bool isGetRequest = false; // loops through the lines and if we never find a line that starts with "GET", never true so 400 error
    
    while (1) {
        // cout << "Entering Line: " << lineCounter << endl;
        lineCounter++;
        currLine = formatHeader(communicationSocket);

        // cout << "Line " << lineCounter << ": " << currLine << endl;

        // if the current line doesnt have any content, we have reached the end of the buffer
        if (currLine == "") {
            cout << "never got to get" << endl;
            break;
        }

        if (currLine.substr(0, 3) == "GET") {
            istringstream input(currLine); // allows us to access the string in a stream format like cin
            string reqType;
            input >> reqType >> fileName;
            fileName = fileName.substr(1, fileName.length()); // the file name will have a '/' before it so need to remove that
            isGetRequest = true;
            break;

            // for debugging:
            // cout << "fileName: " << fileName << endl;
            // cout << "reqType: " << reqType << endl;
        }
    }

    //cout << "Read in file past get" << endl;

    bool isBadRequest = false;

    // Setp 3 - If it isnt a get request, should return a 400 bad request along with the html for it
    if (!isGetRequest) {
        fileName = "badrequest.html";
        isBadRequest = true;
    }

    // Step 3 - Process the data based on the rules of the assignment (rules are commented above createResponse() method)

    // Two of the cases commented above dont require a page to be sent back, just a status code, so that will be checked and done here first

    // Case 1 - The request type was not a GET request (Todo: should probably do this before the loop because that specifies GET)

    // Case 2 - The file name was "SecretFile.html, simply return a 401 unathorized" 
    bool unauthorized = false;

    if (!isBadRequest) {
        if (fileName == "SecretFile.html") {
            unauthorized = true;
            fileName = "unauthorized.html"; // this is the html file that should open if the file is unauthorized
        }
    }

    // If not one of those two cases, just create the response
    string status = ""; // value of the status code as a string
    string payload = ""; // value of the files contents as a string

    createResponse(fileName, status, payload, unauthorized, isBadRequest);

    string contentLength = to_string(payload.size()); // needed as a reponse header

    string httpResponse = status + "Content_Length: " + contentLength + "\r\n" + "Content-Type: text/html\r\n\r\n" + payload;

    cout << " " << endl;
    cout << "Full Http Resonse: " << endl;
    cout << " " << endl;
    cout << httpResponse << endl;

    // Step 4 - Send the newly formatted http response
    // the second argument is a void* for a buffer so need to get the pointer to the first character bytes memory address of the response
    send(communicationSocket, &httpResponse[0], httpResponse.size(), 0);

    // For debugging
    // cout << "Data sent " << endl;

    // Step 5 - Finally, close the socket between the communication process and the client
    close(communicationSocket);

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
 * 
 * 4. istringstream
 *  a) Allows us to read from the string as if it wer like cin
 *  b) It basically allows a sequence of contiguous characters and access to individual parts like a indice
 *  c) But in the format of a string object
 * 
 * 
 */