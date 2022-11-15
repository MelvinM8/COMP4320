/**
 * @file UDPClient.cpp
 * @author Melvin Moreno (mem0282@auburn.edu.com)
 * @brief FTP Client over UDP
 * @version 0.1 COMP4320 Project 1
 * @date 2022-10-19
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
#define TESTFILE "/home/u3/mem0282/COMP4320/Client/src/TestFile.txt"
using std::cout;
using std::cin;
using std::endl;

std::stringstream buffer;
int damageProbability;
int lossProbability;
int socketFD;
struct sockaddr_in serverAddress;
struct hostent *server;
char packetBuffer[512];


// Connect to the client
int connectToServer() {
    memset(&serverAddress, 0, sizeof(serverAddress));

    // Bind the socket to the port
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    return 0;
}

// Calculate the checksum
int calculateCheckSum(char packet[]) {
    int sum = 0;
    for (int i = 0; i < 512; i++) {
        sum += packet[i];
    }
    return sum;
}

// Calculate the checksum by summing the bytes.
void setCheckSum(char packet[]) {
    int sum = calculateCheckSum(packet);
    
    // Set the checksum
    char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	packet[2] = digits[sum / 10000 % 10];
	packet[3] = digits[sum / 1000 % 10];
	packet[4] = digits[sum / 100 % 10];
	packet[5] = digits[sum / 10 % 10];
	packet[6] = digits[sum % 10];
    cout << "Checksum: " << packet[2] << packet[3] << packet[4] << packet[5] << packet[6] << endl;
}

// Allow the probabilities of damaged and lost packets to be input as arguments when the program is run.
int setProbabilities() {
    cout << "Enter the probability of damaged packets: ";
    cin >> damageProbability;
    cout << "Enter the probability of lost packets: ";
    cin >> lossProbability;
    cout << endl;
    cout << "Successfully set Gremlin probabilities! They are shown below: " << endl;
    cout << "Damage Probability: " << damageProbability << "%" << endl;
    cout << "Loss Probability: " << lossProbability << "%" << endl;
    return 0;
}

// Damage the packet
void damagePacket(char packet[], int amount) {
    int random = rand() % 100;
    if (random < damageProbability) {
        int randomIndex = rand() % 512;
        packet[randomIndex] = 'X' + rand() % 26;
    }
    cout << "GREMLIN: Packet damaged." << amount << " times" << endl;
}

// Implement Gremlin to damage, lose, or successfully send the packet.
void gremlin(char packet[]) {
    int random = rand() % 100;
    // Randomly damage the packet
    if (random < damageProbability) {
        damagePacket(packet, 1);
    }
    // Randomly lose the packet and set to null.
    if (random < lossProbability) {
        cout << "GREMLIN: Packet lost." << endl;
        packet = NULL;
        return;
    }
    // Send the packet
    else {
        cout << "GREMLIN: Packet successfully sent." << endl;
    }
}

// Create the packets
void createPacket(char packet[], int sequenceNumber, int packetSize) {
    // Set the sequence number
    char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    packet[0] = digits[sequenceNumber / 10000 % 10];
    packet[1] = digits[sequenceNumber / 1000 % 10];
    packet[2] = digits[sequenceNumber / 100 % 10];
    packet[3] = digits[sequenceNumber / 10 % 10];
    packet[4] = digits[sequenceNumber % 10];
    cout << "Sequence Number: " << packet[0] << packet[1] << packet[2] << packet[3] << packet[4] << endl;
    // Set the packet size
    packet[5] = digits[packetSize / 10000 % 10];
    packet[6] = digits[packetSize / 1000 % 10];
    packet[7] = digits[packetSize / 100 % 10];
    packet[8] = digits[packetSize / 10 % 10];
    packet[9] = digits[packetSize % 10];
    cout << "Packet Size: " << packet[5] << packet[6] << packet[7] << packet[8] << packet[9] << endl;
    // Set the data
    for (int i = 10; i < 512; i++) {
        packet[i] = packetBuffer[i];
    }
    // Set the checksum
    setCheckSum(packet);
}

// Send the packet
void sendPacket(char packet[]) {

    sendto(socketFD, packet, 512, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    cout << "Packet sent." << endl;
    int n;
    socklen_t length;
    n = recvfrom(socketFD, packet, 512, 0, (struct sockaddr *) &serverAddress, &length);
    packetBuffer[n] = '\0';
    cout << "Server: " << packetBuffer << endl;
    close(socketFD);
}

// Read the test file into buffer
bool readFile() {
    std::ifstream file(TESTFILE);
    if (!file.is_open()) {
        cout << "Error: File not found." << endl;
        return false;
    }
    buffer << file.rdbuf();
    return true;
}

// Send Request
bool sendRequest() {
    char request[] = "GET TestFile";
    sendto(socketFD, request, 512, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    cout << endl;
    cout << "Sending request... " << request << endl;
    cout << endl;
    return true;
}

//main function
int main(int argc, char *argv[]) {
    {
	srand(time(0));
	socketFD = connectToServer();
	if (socketFD != 0)
	{
		return -1;
	}
    
    cout << "Successfully connected to server..." << endl << endl;

	setProbabilities();

	if (!sendRequest())
	{
		return -1;
	}

	createPacket(packetBuffer, 0, 512);
    cout << endl;
    cout << "Successfully created the packet..." << endl;

    //Damage or loss the packet.
    gremlin(packetBuffer);
    
    //Send the packet.
    sendPacket(packetBuffer);
	return 0;
}
}