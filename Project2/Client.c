/**
 * @file Client.c
 * @author Melvin Moreno, Garrison Moore, Will Thompson
 * @brief This is a client side implementation of UDP File Transfer with Go-Back-N pipeline
 * @version 0.1
 * @date 2022-11-15
 * 
 * Compile Instructions:
 * gcc servevr.c -o client
 * 
 * Run Instructions:
 * ./client <server IP> 10028
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

// Define max buffer size
#define MAX 512
// Define Socket Address
#define SA struct sockaddr
// Define Port
#define PORT 10028

// Go-Back-N File Transfer from Server to Client.
void GBNFileTransfer(int sockfd, struct sockaddr_in serverAddress) {
    // Init Variables
    char currWindow[MAX]; // Window size = 32
    char messageBuffer[MAX]; // Should be 64
    char fileName[MAX]; // Local file name
    int length; // Length of UDP message
    int n;
    clock_t start, end; // Clock for timing
    double timeTaken; // Time taken for transfer

    // Client will receive each 512-byte packet in a loop and write them
    // to a local file sequentially
    while (1) {
        // Clear buffers
        bzero(messageBuffer, sizeof(messageBuffer));

        // Get user input for file name, send that file name to server to send.
        printf("CLIENT: Enter the file name to request from server, or 'exit' to close connection: ");
        scanf("%s", messageBuffer);
        memcpy(fileName, messageBuffer, sizeof(messageBuffer)); // Keep a local copy of the file name.

        // Send file name to server
        sendto(sockfd, messageBuffer, MAX, 0, (const struct sockaddr *)&serverAddress, sizeof(serverAddress));

        // If user enters 'exit', close connection
        if ((strncmp(messageBuffer, "exit", 4)) == 0) {
            // Clear buffers
            bzero(messageBuffer, sizeof(messageBuffer));
            recvfrom(sockfd, messageBuffer, sizeof(messageBuffer), 0, (struct sockaddr *)&serverAddress, &length);

            // Close connection
            if ((strncmp(messageBuffer, "exit", 4)) == 0) {
                printf("SERVER: Connection closed.\n");
                printf("CLIENT: Exiting...\n");
                break;
            }
            else {
                printf("CLIENT: ERROR! Server did not acknowledge exit request.\n");
                printf("Force closing connection...\n");
                exit(0);
            }
        }
        // Clear messageBuffer, wait for OK message to confirm file existence
        bzero(messageBuffer, sizeof(messageBuffer));
        n = recvfrom(sockfd, messageBuffer, MAX, 0, (struct sockaddr *)&serverAddress, &length);
        messageBuffer[n] = '\0';

        // If server finds file, start receiving it
        if ((strncmp(messageBuffer, "OK", 2)) == 0) {
            // Print OK message
            printf("SERVEFR: %s\n", messageBuffer);

            // Send OK message to client to begin transfer
            bzero(messageBuffer, sizeof(messageBuffer));
            sendto(sockfd, "OK", sizeof("OK"), 0, (const struct sockaddr *)&serverAddress, sizeof(serverAddress));

            // Receive file from server
            printf("CLIENT: Receiving %s from Server and saving to local file...\n", fileName);

            // Write file
            FILE *clientFile = fopen(fileName, "w"); // Open file for writing

            // If the file is NULL, something went wrong.
            // Otherwise, download the file.
            if (clientFile == NULL) {
                printf("CLIENT: ERROR! File %s could not be opened.\n");
            }
            else {
                // Clear Buffer
                bzero(messageBuffer, MAX);

                // Client variables
                int transferFlag = 1; // Flag to indicate if transfer is complete
                int packetSize = 0; // Size of packet received
                int lastACK = -1; // Int of last ACK received. Set to -1 to indicate no ACKs received yet.
                int lastSegment; // Int of last segment received
                char localCRC32[8]; // Local CRC32 hash
                char expectedCRC32[8]; // Expected CRC32 hash
                unsigned int crc32 = 0; // CRC32 hash

                // Start timer
                start = clock(); // Start clock

                // Transfer File from server to client using Go-Back-N
                while (transferFlag == 1) {
                    // Clear file buffer
                    bzero(currWindow, sizeof(currWindow));

                    // Receive the segement number from server
                    bzero(messageBuffer, sizeof(messageBuffer));
                    n = recvfrom(sockfd, messageBuffer, MAX, 0, (struct sockaddr *)&serverAddress, &length);
                    messageBuffer[n] = '\0';
                    printf("CLIENT: Receiving segment %s from server...\n", messageBuffer);

                    // Set last segment
                    lastSegment = atoi(messageBuffer);

                    // Receive packet size from server
                    packetSize = recvfrom(sockfd, messageBuffer, MAX, 0, (struct sockaddr *)&serverAddress, &length);

                    // Extract appened CRC32 hash from packet
                    memcpy(expectedCRC32, messageBuffer + packetSize - 8, 8);
                    expectedCRC32[8] = '\0';

                    // Calculate CRC32 hash of packet
                    crc32 = crc32c(0, messageBuffer, packetSize - 8);

                    // Convert CRC32 hash to string
                    sprintf(localCRC32, "%x", crc32);

                    // If no gaps in acknowledgement, write to file
                    if (lastACK == lastSegment - 1) {
                        // Check for bit errors using checksum
                        if(strcmp(localCRC32, expectedCRC32) == 0) {
                            lastACK = lastSegment; // Updated ACK number

                            // Write to file
                            fwrite(currWindow, sizeof(char), packetSize - 8, clientFile);
                        }
                        else {
                            printf("CLIENT: ERROR! Bit error detected - Expected CRC32 Value of %s, but received %s.\n", expectedCRC32, localCRC32);
                            printf("Ignoring packet...\n");
                        }
                    }
                    // Send ACK to server for last segment received
                    bzero(messageBuffer, sizeof(messageBuffer));
                    sprintf(messageBuffer, "%d", lastACK);
                    sendto(sockfd, messageBuffer, sizeof(messageBuffer), 0, (const struct sockaddr *)&serverAddress, sizeof(serverAddress));
                    printf("CLIENT: Sending ACK %s to server...\n", messageBuffer);

                    // Break out of loop if last segment received and ACK'd
                    if (packetSize == 0 || packetSize < MAX) {
                        if (lastACK == lastSegment) {
                            transferFlag = 0;
                            break;
                        }
                    }
                }
            }
            // End timer
            end = clock(); // End clock
            timeTaken = ((double)(end - start)) / CLOCKS_PER_SEC; // Calculate time taken

            // Display success message and close file
            printf("CLIENT: File transfer complete.\n");
            printf("Time Taken: %f seconds.\n", timeTaken);
            
            // Close file
            fclose(clientFile);
        }
    }
    close(sockfd);
}

// Main Function
int main(int argc, char* argv[]) {
    // Init Variables
    char* ipAddress; // Desination IP Address
    char* port; // Destination Port
    int portVal; // Used convert port to int
    int clientSocket; // Client socket
    struct sockaddr_in serverAddress; // Server address

    // Set defaults based on how many arguments are passed
    if (argv[1] == NULL) {
        ipAddress = "127.0.0.1"; // TODO: Change to local IP address
        port = "10028"; // Default port: 10028
    }
    else if(argv[2] == NULL) {
        ipAddress = argv[1];
        port = "10028"; // Default port: 10028
    }
    else {
        ipAddress = argv[1];
        port = argv[2];
    }

    // COonvert port string to int value
    portVal = atoi(port);

    // Create socket
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket < 0) {
        printf("CLIENT: ERROR! Socket creation failed.\n");
        exit(0);
    }
    else {
        printf("CLIENT: Socket created successfully.\n");
    }

    // Set server address
    bzero(&serverAddress, sizeof(serverAddress));

    // Assign the IP and Port Number for destination address 
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress);
    serverAddress.sin_port = htons(portVal);

    // Initiate file transfer over UDP
    GBNFileTransfer(clientSocket, serverAddress);

    // Close client socket
    close(clientSocket);
    return 0;
}