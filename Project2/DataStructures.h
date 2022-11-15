#ifndef RDTSERVER_DATA_STRUCTURES_H
#define RDTSERVER_DATA_STRUCTURES_H

#include <constants.h>
#include <bits/stdc++.h>

using namespace std;

struct packet_in {
    packet_in() : status(NOT_SEND), start_time(clock()) {}
    PACKET_STATUS status;
    clock_t start_time;
};

#endif //RDTSERVER_DATA_STRUCTURES_H