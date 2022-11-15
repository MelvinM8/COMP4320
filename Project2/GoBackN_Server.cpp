#include "GoBackN_Server.h"


// Constructor
gbn_server::gbn_server(port_handler *p) : go_back_n(p) {
    pthread_mutex_init(&lock, NULL);
}

void gbn_server::init(float plp, int seed, vector<data_packet*> &data_packets) {
    this->data_packets = data_packets;
    ack_packet ak(data_packets.size());
    p_handler->send(ak.to_string());

    // Setting the random generator for packet loss
    randomGenerator = random_generator(plp, seed, data_packets.size());
}

void gbn_server::implement() {
    int number_of_packets = data_packets.size();
    int base, next_seq_num;, duplicate_ack;
    base = next_seq_num = duplicate_ack = 0;

    while (base < number_of_packets) {
        int window_size = cg.get_curr_window_size();
        window_size_analysis.emplace_back(window_size);

        // Loop the size of the window, or the remaining of not-sent packets
        int remaining = min(number_of_packets-base, window_size);
        for (; next_seq_num < (base + remaining); next_seq_num++) {
            // Send packet
            send_packet(next_seq_num);
        }
        string bufffer;
        if (p_handler->receive(buffer, window_size) == 0) {
            // Timeout, resend all again.
            cg.update_window_size(TIMEOUT);
            next_seq_num = base;
            continue;
        }
        else {
            ack_packet *packet = packet_parse::create_ackpacket(buffer);
            uint32_t seq_num = packet->get_seqnum();
            ack_packet check_packet(seq_num);
            if (checksum_calculator::validate(packet->get_checksum(), check_packet.get_checksum())) {
                if (seq_num == base) {
                    // Got the expected packet
                    base++;
                    cg.update_window_size(ACK);
                    successful_sent++;
                    duplicate_ack = 0;
                }
                else {
                    // Got a duplicate ACK
                    duplicate_ack++;
                    if (duplicate_ack == 3) {
                        cg.update_window_size(DupACK);
                        duplicate_ack = 0;
                    }
                }
            }
        }
    }
}

void gbn_server::send_packet(int seq_num) {
    // Get packet by seq num
    data_packet *curr_packet = data_packets[seq_num];

    // Send packet to client
    if(randomGenerator.can_send(seq_num)) {
        // Will send if it is defined as lost.
        int r = p_handler->send(curr_packet->to_string());
        cout << "Packet with sequence number = " << seq_num << "will be sent." << endl;
    }
    else {
        cout << "Packet with sequence number = " << seq_num << "will be lost." << endl;
    }
    total_sent++;
}