#include "GoBackN_Client.h"

// Contructor
gbn_client:: gbn_client(port_handler *p) : go_back_n(p) {}

// init

void gbn_client::init(int expected_packet_count, vector<data_packet*> *received_packets) {
    this->expected_packet_count = expected_packet_count;
    this->received_packets = received_packets;
}

// Implement strategy

void gbn_client::implement() {
    int received_packet_count = 0;

    while (received_packet_count < expected_packet_count) {

        string data_string;
        // 1. Blocking receive
        int r = p_handler->receive(data_string);

        // 2 Parse packet
        data_packet *curr_packet = packet_parse::create_datapacket(data_string);
        data_packet comp_packe(curr_packet->get_seqnum(), curr_packet->get_data());

        if (checksum_calculator::validate(curr_packet->get_checksum(), comp_packet.get_checksum())) {
            // Packet is valid

            // 3. Send ACK
            ack_packet ack(curr_packet->get_seqnum());
            string ack_string = ack.to_string();

            p_handler->send(ack_string);

            // Got the packet we expected
            if (received_packet_count == curr_packet->get_seqnum()) {

                // 4. Add packet to the list
                received_packets->push_back(curr_packet);

                // Update variables
                received_packet_count++;
            }
        }
    }
}
