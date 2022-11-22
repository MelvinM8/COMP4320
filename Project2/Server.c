/**
 * @file Server.c
 * @author Melvin Moreno, Garrison Moore, Will Thompson
 * @brief Server side implementation of UDP File Transfer with Go-Back-N pipeline
 * @version 0.1
 * @date 2022-11-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// Includes

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>



// Define max buffer size
#define MAX 512
// Define Socket Address
#define SA struct sockaddr
// Define port
#define PORT 10028

// Go-Back-N File Transfer from Server to Client.
void GBNFileTransfer(int sockfd, struct sockaddr_in clientAddress, int windowSize, float errorProbability) {
    // Init Variables
    char messageBuffer[MAX]; // Should be 64
    char fileName[MAX]; // Local file name
    int length; // Length of UDP message
    int n;
    clock_t start, end; // Clock for timing
    double timeTaken; // Time taken for transfer

    // Loop for communication with client
    while (1) {
        // Clear buffers
        bzero(messageBuffer, MAX);

        length = sizeof(clientAddress);

        // Receive file name from client
        n = recvfrom(sockfd, messageBuffer, MAX, 0, (struct sockaddr *)&clientAddress, &length);
        messageBuffer[n] = '\0';

        // Print buffer which contains the client contents
        printf("CLIENT: Requesting %s \n", messageBuffer);

        // Terminate connection if client sends 'exit'
        if ((strncmp(messageBuffer, "exit", 4)) == 0) {
            // send exit message back to client
            sendto(sockfd, "exit", MAX, 0, (const struct sockaddr *)&clientAddress, sizeof(clientAddress));
            printf("SERVER: Exiting connection...\n");
            break;
        }

        // Atempt to open file
        memcpy(fileName, messageBuffer, sizeof(messageBuffer)); // Keep a local copy of the file name.
        FILE* serverFile = fopen(fileName, "r"); // Open file in read mode, store as serverFile

        // If the file exists, send it to the client
        if (serverFile != NULL) {
            // Inform client that file exists
            sendto(sockfd, "OK", sizeof("OK"), 0, (const struct sockaddr*) &clientAddress, sizeof(clientAddress));

            // Clear buffers
            bzero(messageBuffer, sizeof(messageBuffer));
            n = recvfrom(sockfd, messageBuffer, sizeof(messageBuffer), 0, (struct sockaddr*) &clientAddress, &length);
            messageBuffer[n] = '\0';

            // If read message is OK send file
            if ((strncmp(messageBuffer, "OK", 2)) == 0) {
                // Init variables
                int transferFlag = 1; // Flag used to break out of while loop
                int packetSize; // Size of packet to send
                int lastACK = -1; // Int to keep track of last ACK received. Set to -1 for now ACKs
                int lastSegment; // Int to keep track of last segment sent
                int j = 0; // Value used to "slide" window for Go-Back-N
                clock_t timeoutStart, timeoutEnd; // Clocks for timeout
                double timeout; // Timeout value
                unsigned int crc32 = 0; // CRC32 checksum
                

                // Start timer
                start = clock();

                // Clear buffers
                bzero(messageBuffer, sizeof(messageBuffer));
                printf("SERVER: Sending file %s to client...\n", fileName);

                // Continuously send packets until the end of the file is reached
                while (transferFlag == 1) {
                    // Send up to N Unacked packets
                    int i;
                    for(i = 0; i < windowSize; i++) { // Should be 32
                        // Send current segment's number
                        bzero(messageBuffer, sizeof(messageBuffer));
                        sprintf(messageBuffer, "%d", i + j);
                        sendto(sockfd, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr*) &clientAddress, sizeof(clientAddress));

                        // Update last segment sent
                        lastSegment = atoi(messageBuffer);
                        printf("SERVER: Sending segment %s \n", messageBuffer);

                        // Set file read head to the bit location at the last ACK'd packet.
                        fseek(serverFile, (sizeof(char) * (MAX - 8) * (i + j)), SEEK_SET);

                        // Clear message buffer and read in pakcet from server file
                        bzero(messageBuffer, MAX);
                        packetSize = fread(messageBuffer, sizeof(char), MAX - 8, serverFile);

                        // Calculate the checksum by adding the bytes of the packet body
                        int k;
                        for (k = 0; k < packetSize; k++) {
                            crc32 += messageBuffer[k];
                        }

                        // Compare the checksum to the checksum in the packet
                        if (crc32 == atoi(messageBuffer + packetSize)) {
                            // Send packet to client
                            sendto(sockfd, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr*) &clientAddress, sizeof(clientAddress));
                            printf("SERVER: Sending packet %s \n", messageBuffer);
                        } else {
                            // Send packet to client
                            sendto(sockfd, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr*) &clientAddress, sizeof(clientAddress));
                            printf("SERVER: Sending packet %s \n", messageBuffer);
                        }

                        // Gremlin
                        float p = fabs(((float)rand()) / RAND_MAX);
                        if (errorProbability < p) {
                            // Randomly mess some things up
                            int i;
                            for (i = 0; i < 5; i++) {
                                crc32 += (rand() % 8 + 0) - 100;
                            }
                        }

                        // Append checksum to end of packet
                        sprintf(&messageBuffer[packetSize], "%x", crc32);
                        packetSize += 8;

                        // Send file
                        if (sendto(sockfd, messageBuffer, packetSize, 0, (const struct sockaddr*) &clientAddress, sizeof(clientAddress)) < 0) {
                            printf("SERVER: ERROR! Failed to send file to client.\n");
                            printf("SERVER: Closing connection...\n");
                            exit(0);
                        }

                        // Receive ACK from client
                        bzero(messageBuffer, sizeof(messageBuffer));
                        n = recvfrom(sockfd, messageBuffer, sizeof(messageBuffer), 0, (struct sockaddr*) &clientAddress, &length);
                        messageBuffer[n] = '\0';

                        // Update last ACK received if client does not have bit errors
                        if (lastACK == atoi(messageBuffer) - 1) {
                            lastACK = atoi(messageBuffer);
                        }
                        printf("SERVER: Received ACK %d \n", lastACK);
                    }
                    // Break out of loop if last segment sent is the last segment in the file
                    if (packetSize == 0 || packetSize < MAX - 8) {
                        if (lastACK == lastSegment) {
                            transferFlag = 0;
                            break;
                        }
                    }
                }

                // Slide window
                j = lastACK + 1;
            }
            // End timer
            end = clock();
            timeTaken = ((double) (end - start)) / CLOCKS_PER_SEC;

            // Display success message
            printf("SERVER: File successfully sent to client.\n");
            printf("Time taken: %f seconds\n", timeTaken);
        }
    else {
        // If file not in server, print error. Send NULL back to client.
        printf("SERVER: ERROR! File %s not found.\n", fileName);
        sendto(sockfd, "NULL", sizeof("NULL"), 0, (struct sockaddr*) &clientAddress, length);
    }
}

// Close the socket
close(sockfd);
}

// Main function
int main(int argc, char* argv[]) {
    // Init variables
    char* port; // Port number
    int portVal; // Port number as int
    int serverSocket; // Socket for server
    int connection; // Connection status
    struct sockaddr_in serverAddress; // Server address
    struct sockaddr_in clientAddress; // Client address
    int windowSize; // Window size
    float errorProbability; // Error probability

    // Random seed
    srand(time(NULL));

    if (argv[1] == NULL) {
        port = "8080"; // TODO: Change port number
    }
    else if (argv[2] == NULL) {
        windowSize = 32; // TODO: Change to 32
    }
    else if (argv[3] == NULL) {
        errorProbability = 0.0;
    }
    else {
        port = argv[1];
        windowSize = atoi(argv[2]);
        errorProbability = atof(argv[3]);
    }

    // Ensure probability is between 0 and 1
    if (errorProbability > 1) {
        errorProbability = 1;
    }
    else if (errorProbability < 0) {
        errorProbability = 0;
    }

    // Convert port string to int value.
    portVal = atoi(port);

    // Create the server's UDP socket, and check for successful creation
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) {
        printf("SERVER: ERROR! Failed to create socket.\n");
        exit(0);
    }
    else {
        printf("SERVER: Socket created successfully.\n");
    }

    // Allocate space for server address
    bzero(&serverAddress, sizeof(serverAddress));

    // Assigne the IP address and port number to the server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(portVal);

    // Bind the socket to the server address, and check for successful binding
    if ((bind(serverSocket, (SA*)&serverAddress, sizeof(serverAddress))) != 0) {
        printf("SERVER: ERROR! Failed to bind socket.\n");
        exit(0);
    }
    else {
        printf("SERVER: Socket successfully bound.\n");
    }

    // Initiate file transfer over UDP
    GBNFileTransfer(serverSocket, clientAddress, windowSize, errorProbability);

    // Close server socket
    close(serverSocket);
    return 0;
    }