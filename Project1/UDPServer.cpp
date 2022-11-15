/**
 * @file UDPServer.cpp
 * @author Melvin Moreno (mem0282@auburn.edu.com)
 * @brief FTP Server over UDP
 * @version 0.1 COMP4320 Project 1
 * @date 2022-10-21
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <numeric>
#include <vector>
#include <cstdlib>
#define PORT 10005
#define PACKET_SIZE 512
using namespace std;

int socketFD;
struct sockaddr_in clientAddress, serverAddress;

// Bind the server to localhost and the port number defined.
int bindServer() {
    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    if (bind(socketFD, (const struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// Calculate the checksum
int calculateCheckSum(char buffer[]) {
    int sum = 0;
    for (int i = 7; i < PACKET_SIZE; i++) {
        sum += buffer[i];
    }
    return sum;
}

// Compare the checksum to the checksum in the message header
bool compareCheckSum(char buffer[]) {
    int sum = calculateCheckSum(buffer);
    int headerCheckSum = 0;
    for (int i = 0; i < 7; i++) {
        headerCheckSum+ + buffer[i];
    }
    return sum == headerCheckSum;
}

// Write the contents of the buffer to the file
int writeToFile(char buffer[], ofstream &file) {
    for (int i = 7; i < PACKET_SIZE; i++) {
        file << buffer[i];
    }
    return 0;
}

// Send a response to the client
int sendReponse(string response, socklen_t socketLength) {
    char buffer[PACKET_SIZE];
    strcpy(buffer, response.c_str());
    if (sendto(socketFD, buffer, PACKET_SIZE, 0, (struct sockaddr *) &clientAddress, socketLength) < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    return 0;
}


// Receive a message from the client
int receiveMessage()
{
    socklen_t socketLength = sizeof(clientAddress);
    char buffer[PACKET_SIZE];
    int n;
    if ((n = recvfrom(socketFD, buffer, PACKET_SIZE, 0, (struct sockaddr *) &clientAddress, &socketLength)) < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    cout << "received : " << buffer << endl;
    
    // If the checksum is correct, write the contents of the buffer to the file
    if (compareCheckSum(buffer)) {
        ofstream file;
        file.open("TestFile.txt", ios::app);
        writeToFile(buffer, file);
        file.close();
        sendReponse("ACK", socketLength);
    } else {
        sendReponse("NACK", socketLength);
    }
    return 0;
}


// Main function
int main(int argc, char *argv[]) {

    cout << "Starting server..." << endl;

    // Create the socket
    socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFD < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    
    cout << "Successfully created the socket..." << endl;

    // Bind the server to the port
    bindServer();

    cout << "Successfully bounded the server to the port..." << endl;

    // Receive the message
    ofstream file;
    receiveMessage();

    cout << "Successfully received the message..." << endl;

    // Close the socket
    close(socketFD);

    cout << "Successfully closed the socket..." << endl;

    return 0;
}
