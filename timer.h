#pragma once

#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>
#include <string>

struct timer {
    timer() = delete;
    timer(const char* msg, bool q = true);

    ~timer();

    void reset();
    double elapsed();

    std::chrono::high_resolution_clock::time_point tStart;
    std::string message;
    bool quiet;
};
