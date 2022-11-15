#ifndef RDT_SERVER_GBN_CLIENT_H
#define RDT_SERVER_GBN_CLIENT_H

#include "Go_Back_N.h"
#include "../../../utilities/random_generator.h"
#include "../../../congestion_control/congestion_controller.h"
#include "../utilities/data_structures.h"

class gbn_server : public go_back_n {
    private:
        // attributes
        vector<data_packet* data_packets;
        pthread_t send_id, recv_id;
        pthread_mutex_t lock;
        random_generator randomGenerator;
        congestion_controller cg;
        int global_base;
        clock_t glob_timer;

        // Utility functions
        void send_packet(int index);

    public:
        // Constructor
        gbn_server(port_handler *p);

        void init(float plp, int seed, vector<data_packet*> &data_packets);
        void implement();
};

#endif //RDTSERVER_GBN_SERVER_H