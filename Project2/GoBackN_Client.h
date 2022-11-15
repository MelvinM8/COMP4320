#ifndef RDTSERVER_GBN_CLIENT_H
#define RDTSERVER_GBN_CLIENT_H

#include "Go_Back_N.h"
#include "../../../transport_packet/data_packet.h"
#include "../../../transport_packet/ack_packet.h"

class gbn_client : public go_back_n {
    private:
        int expected_packet_count;
        vector<data_packet*> *received_packets;

    public:
        // Constructor
        gbn_client(port_handler *p);

        void init(int expected_packet_count, vector<data_packet*> *received_packets);
        void implement();
};

#endif;