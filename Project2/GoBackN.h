#ifndef RDTSERVER_GO_BACK_N_H
#define RDTSERVER_GO_BACK_N_H

#include "../RDT_Startegy.h"

class go_back_n : public rdt_strategy {
    public:
        // Constructor
        go_back_n(port_handler *p);

        // Destructor
        virtual void implement() = 0;
};

#endif // RDTSERVER_GO_BACK_N_H