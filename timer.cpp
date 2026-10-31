#include "timer.h"

timer::timer(const char* msg, bool q) : message(msg), quiet(q) {
    tStart = std::chrono::high_resolution_clock::now();
}

double timer::elapsed() {
    return std::chrono::duration_cast<std::chrono::duration<double>>(
               std::chrono::high_resolution_clock::now() - tStart)
        .count();
}

void timer::reset() {
    tStart = std::chrono::high_resolution_clock::now();
}

timer::~timer() {
    if (!message.empty()) {
        if (!quiet)
            std::cout << message << ' ' << elapsed() << " s\n";
    }
}
