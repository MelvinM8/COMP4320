#include "rdtStrategy.h"

// Constructor
rdt_strategy::rdt_strategy(port_handler *p) : p(p) {
    p_handler = p;
}

void rdt_strategy::print_window_size_analysis() {
    stringstream ss;
    int sz = min(1000, (int)window_size_analysis.size());
    for (int i = 0; i < sz; i++) {
        ss << window_size_analysis[i] << " ";
    }
    string s = ss.str();
    io_handler::writeData("window_size_analysis.txt", (char*)s.data(), s.size());

    cout << float(successful_snet * 1.0 / total_sent) << endl;
}